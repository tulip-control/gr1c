/* grjit.c -- run experiments for "just in time" synthesis and related work.
 *
 * Some core functions for working with strategy automata have changed
 * recently, and sim_rhc(), which is invoked by grjit, has not yet
 * been carefully checked following those changes.  As such, grjit
 * should be considered as possibly temporarily defunct.
 *
 *
 * SCL; 2012, 2013.
 */


#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "logging.h"
#include "ptree.h"
#include "solve.h"
#include "automaton.h"
#include "util.h"
#include "solve_metric.h"
#include "solve_support.h"
#include "sim.h"
#include "gr1c_util.h"
extern int yyparse( void );


/**************************
 **** Global variables ****/

extern specification_t spc;

/**************************/


/* See solve_metric.c */
extern DdNode *compute_winning_set_saveBDDs( DdManager *manager,
                                             DdNode **etrans, DdNode **strans,
                                             DdNode ***egoals, DdNode ***sgoals,
                                             unsigned char verbose );


void dump_simtrace( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
                    FILE *fp )
{
    int j, last_nonzero_env, last_nonzero_sys;
    anode_t *node;
    int node_counter = 0;
    ptree_t *var;
    int num_env, num_sys;

    if (fp == NULL)
        fp = stdout;

    num_env = tree_size( evar_list );
    num_sys = tree_size( svar_list );

    node = head;
    while (node->mode != 0)  /* Find starting state */
        node = node->next;
    while (node->trans_len >= 0) {
        fprintf( fp, "%d: ", node_counter );
        last_nonzero_env = num_env-1;
        last_nonzero_sys = num_sys-1;
        if (last_nonzero_env < 0 && last_nonzero_sys < 0) {
            fprintf( fp, "{}" );
        } else {
            for (j = 0; j < num_env; j++) {
                var = get_list_item( evar_list, j );
                if (j == last_nonzero_env) {
                    fprintf( fp, "%s=%d", var->name, *(node->state+j) );
                    fprintf( fp, ", " );
                } else {
                    fprintf( fp, "%s=%d, ", var->name, *(node->state+j) );
                }
            }
            for (j = 0; j < num_sys; j++) {
                var = get_list_item( svar_list, j );
                if (j == last_nonzero_sys) {
                    fprintf( fp, "%s=%d", var->name, *(node->state+num_env+j) );
                } else {
                    fprintf( fp, "%s=%d, ",
                             var->name, *(node->state+num_env+j) );
                }
            }
        }
        fprintf( fp, "\n" );
        if (node->trans_len == 0)
            break;

        node_counter++;
        node = *(node->trans);
    }
}


int main( int argc, char **argv )
{
    FILE *fp;
    FILE *stdin_backup = NULL;
    bool help_flag = False;
    bool ptdump_flag = False;
    bool logging_flag = False;
    unsigned char verbose = 0;
    int input_index = -1;
    int output_file_index = -1;  /* For command-line flag "-o". */
    char dumpfilename[64];

    int i, j, var_index;
    ptree_t *tmppt;  /* General purpose temporary ptree pointer */
    int horizon = -1;

    DdNode *W, *etrans, *strans, **sgoals, **egoals;

    DdManager *manager;
    DdNode *T = NULL;
    anode_t *strategy = NULL;
    int num_env, num_sys;
    int original_num_env, original_num_sys;

    int max_sim_it;  /* Number of simulation iterations */
    anode_t *play;
    vartype *init_state;
    int *init_state_ints = NULL;
    char *all_vars = NULL, *metric_vars = NULL;
    int *offw, num_vars;
    int init_state_acc;  /* Accumulate components of initial state
                            before expanding into a (bool) bitvector. */

    /* Look for flags in command-line arguments. */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'h') {
                help_flag = True;
            } else if (argv[i][1] == 'V') {
                printf( "grjit (experiment-related program, distributed with"
                        " gr1c v" GR1C_VERSION ")\n\n" GR1C_COPYRIGHT "\n" );
                return 0;
            } else if (argv[i][1] == 'v') {
                verbose = 1;
                j = 2;
                /* Only support up to "level 2" of verbosity */
                while (argv[i][j] == 'v' && j <= 2) {
                    verbose++;
                    j++;
                }
            } else if (argv[i][1] == 'l') {
                logging_flag = True;
            } else if (argv[i][1] == 'p') {
                ptdump_flag = True;
            } else if (argv[i][1] == 'm') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                all_vars = strtok( argv[i+1], "," );
                max_sim_it = strtol( all_vars, NULL, 10 );
                all_vars = strtok( NULL, "," );
                if (all_vars == NULL) {
                    fprintf( stderr, "Invalid use of -m flag.\n" );
                    return 1;
                }
                metric_vars = strdup( all_vars );
                if (max_sim_it >= 0) {
                    all_vars = strtok( NULL, "," );
                    if (all_vars == NULL) {
                        fprintf( stderr, "Invalid use of -m flag.\n" );
                        return 1;
                    }
                    init_state_acc = read_state_str( all_vars,
                                                     &init_state_ints, -1 );
                    all_vars = strtok( NULL, "," );
                    if (all_vars == NULL) {
                        horizon = -1;  /* The horizon was not given. */
                    } else {
                        horizon = strtol( all_vars, NULL, 10 );
                        if (horizon < 1) {
                            fprintf( stderr,
                                     "Invalid use of -m flag.  Horizon must"
                                     " be positive.\n" );
                            return 1;
                        }
                    }
                }
                all_vars = NULL;
                i++;
            } else if (argv[i][1] == 'o') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                output_file_index = i+1;
                i++;
            } else {
                fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                return 1;
            }
        } else if (input_index < 0) {
            /* Use first non-flag argument as filename whence to read
               specification. */
            input_index = i;
        }
    }

    if (help_flag) {
        /* Split among printf() calls to conform with ISO C90 string length */
        printf( "Usage: %s [-hVvlp] [-m ARG1,ARG2,...] [-o FILE] [FILE]\n\n"
                "  -h          this help message\n"
                "  -V          print version and exit\n"
                "  -v          be verbose; use -vv to be more verbose\n"
                "  -l          enable logging\n"
                "  -p          dump parse trees to DOT files, and echo formulas to screen\n"
                "  -o FILE     output results to FILE, rather than stdout (default)\n", argv[0] );
        printf( "  -m ARG1,... run simulation using comma-separated list of arguments:\n"
                "                ARG1 is the max simulation duration; -1 to only compute horizon;\n"
                "                ARG2 is a space-separated list of metric variables;\n"
                "                ARG3 is a space-separated list of initial values;\n"
                "                    ARG3 is ignored and may be omitted if ARG1 equals -1.\n"
                "                ARG4 is the horizon, if provided; otherwise compute it.\n" );
        return 1;
    }

    if (logging_flag) {
        openlogfile( NULL );  /* Use default filename prefix */
        /* Only change verbosity level if user did not specify it */
        if (verbose == 0)
            verbose = 1;
    } else {
        setlogstream( stdout );
        setlogopt( LOGOPT_NOTIME );
    }
    if (verbose > 0)
        logprint( "Running with verbosity level %d.", verbose );

    /* If filename for specification given at command-line, then use
       it.  Else, read from stdin. */
    if (input_index > 0) {
        fp = fopen( argv[input_index], "r" );
        if (fp == NULL) {
            perror( "grjit, fopen" );
            return -1;
        }
        stdin_backup = stdin;
        stdin = fp;
    }

    /* Parse the specification. */
    if (verbose)
        logprint( "Parsing input..." );
    SPC_INIT( spc );
    if (yyparse())
        return -1;
    if (verbose)
        logprint( "Done." );
    if (stdin_backup != NULL) {
        stdin = stdin_backup;
    }

    /* Close input file, if opened. */
    if (input_index > 0)
        fclose( fp );

    /* Treat deterministic problem in which ETRANS or EINIT is omitted. */
    if (spc.evar_list == NULL) {
        if (spc.et_array_len == 0) {
            spc.et_array_len = 1;
            spc.env_trans_array = malloc( sizeof(ptree_t *) );
            if (spc.env_trans_array == NULL) {
                perror( "gr1c, malloc" );
                return -1;
            }
            *spc.env_trans_array = init_ptree( PT_CONSTANT, NULL, 1 );
        }
        if (spc.env_init == NULL)
            spc.env_init = init_ptree( PT_CONSTANT, NULL, 1 );
    }

    /* Number of variables, before expansion of those that are nonboolean */
    original_num_env = tree_size( spc.evar_list );
    original_num_sys = tree_size( spc.svar_list );

    /* Build list of variable names if needed for simulation, storing
       the result as a string in all_vars */
    if (max_sim_it >= 0) {
        /* At this point, init_state_acc should contain the number of
           integers read by read_state_str() during
           command-line argument parsing. */
        if (init_state_acc != original_num_env+original_num_sys) {
            fprintf( stderr,
                     "Number of initial values given does not match number"
                     " of problem variables.\n" );
            return 1;
        }

        num_vars = 0;
        tmppt = spc.evar_list;
        while (tmppt) {
            num_vars += strlen( tmppt->name )+1;
            tmppt = tmppt->left;
        }
        tmppt = spc.svar_list;
        while (tmppt) {
            num_vars += strlen( tmppt->name )+1;
            tmppt = tmppt->left;
        }
        all_vars = malloc( num_vars*sizeof(char) );
        if (all_vars == NULL) {
            perror( "main, malloc" );
            return -1;
        }
        i = 0;
        tmppt = spc.evar_list;
        while (tmppt) {
            strncpy( all_vars+i, tmppt->name, num_vars-i );
            i += strlen( tmppt->name )+1;
            *(all_vars+i-1) = ' ';
            tmppt = tmppt->left;
        }
        tmppt = spc.svar_list;
        while (tmppt) {
            strncpy( all_vars+i, tmppt->name, num_vars-i );
            i += strlen( tmppt->name )+1;
            *(all_vars+i-1) = ' ';
            tmppt = tmppt->left;
        }
        *(all_vars+i-1) = '\0';
        if (verbose)
            logprint( "String of all variables found to be \"%s\"", all_vars );
    }


    if (ptdump_flag) {
        tree_dot_dump( spc.env_init, "env_init_ptree.dot" );
        tree_dot_dump( spc.sys_init, "sys_init_ptree.dot" );

        for (i = 0; i < spc.et_array_len; i++) {
            snprintf( dumpfilename, sizeof(dumpfilename),
                      "env_trans%05d_ptree.dot", i );
            tree_dot_dump( *(spc.env_trans_array+i), dumpfilename );
        }
        for (i = 0; i < spc.st_array_len; i++) {
            snprintf( dumpfilename, sizeof(dumpfilename),
                      "sys_trans%05d_ptree.dot", i );
            tree_dot_dump( *(spc.sys_trans_array+i), dumpfilename );
        }

        if (spc.num_egoals > 0) {
            for (i = 0; i < spc.num_egoals; i++) {
                snprintf( dumpfilename, sizeof(dumpfilename),
                         "env_goal%05d_ptree.dot", i );
                tree_dot_dump( *(spc.env_goals+i), dumpfilename );
            }
        }
        if (spc.num_sgoals > 0) {
            for (i = 0; i < spc.num_sgoals; i++) {
                snprintf( dumpfilename, sizeof(dumpfilename),
                         "sys_goal%05d_ptree.dot", i );
                tree_dot_dump( *(spc.sys_goals+i), dumpfilename );
            }
        }

        var_index = 0;
        printf( "Environment variables (indices; domains): " );
        if (spc.evar_list == NULL) {
            printf( "(none)" );
        } else {
            tmppt = spc.evar_list;
            while (tmppt) {
                if (tmppt->value == 0) {  /* Boolean */
                    if (tmppt->left == NULL) {
                        printf( "%s (%d; bool)", tmppt->name, var_index );
                    } else {
                        printf( "%s (%d; bool), ", tmppt->name, var_index);
                    }
                } else {
                    if (tmppt->left == NULL) {
                        printf( "%s (%d; {0..%d})",
                                tmppt->name, var_index, tmppt->value );
                    } else {
                        printf( "%s (%d; {0..%d}), ",
                                tmppt->name, var_index, tmppt->value );
                    }
                }
                tmppt = tmppt->left;
                var_index++;
            }
        }
        printf( "\n\n" );

        printf( "System variables (indices; domains): " );
        if (spc.svar_list == NULL) {
            printf( "(none)" );
        } else {
            tmppt = spc.svar_list;
            while (tmppt) {
                if (tmppt->value == 0) {  /* Boolean */
                    if (tmppt->left == NULL) {
                        printf( "%s (%d; bool)", tmppt->name, var_index );
                    } else {
                        printf( "%s (%d; bool), ", tmppt->name, var_index );
                    }
                } else {
                    if (tmppt->left == NULL) {
                        printf( "%s (%d; {0..%d})",
                                tmppt->name, var_index, tmppt->value );
                    } else {
                        printf( "%s (%d; {0..%d}), ",
                                tmppt->name, var_index, tmppt->value );
                    }
                }
                tmppt = tmppt->left;
                var_index++;
            }
        }
        printf( "\n\n" );

        print_GR1_spec( spc.evar_list, spc.svar_list, spc.env_init, spc.sys_init,
                        spc.env_trans_array, spc.et_array_len,
                        spc.sys_trans_array, spc.st_array_len,
                        spc.env_goals, spc.num_egoals, spc.sys_goals, spc.num_sgoals, stdout );
    }

    if (expand_nonbool_GR1( spc.evar_list, spc.svar_list, &spc.env_init, &spc.sys_init,
                            &spc.env_trans_array, &spc.et_array_len,
                            &spc.sys_trans_array, &spc.st_array_len,
                            &spc.env_goals, spc.num_egoals, &spc.sys_goals, spc.num_sgoals,
                            verbose ) < 0)
        return -1;
    spc.nonbool_var_list = expand_nonbool_variables( &spc.evar_list, &spc.svar_list,
                                                     verbose );

    if (spc.et_array_len > 1) {
        spc.env_trans = merge_ptrees( spc.env_trans_array, spc.et_array_len, PT_AND );
    } else if (spc.et_array_len == 1) {
        spc.env_trans = *spc.env_trans_array;
    } else {
        fprintf( stderr,
                 "Syntax error: GR(1) specification is missing environment"
                 " transition rules.\n" );
        return -1;
    }
    if (spc.st_array_len > 1) {
        spc.sys_trans = merge_ptrees( spc.sys_trans_array, spc.st_array_len, PT_AND );
    } else if (spc.st_array_len == 1) {
        spc.sys_trans = *spc.sys_trans_array;
    } else {
        fprintf( stderr,
                 "Syntax error: GR(1) specification is missing system"
                 " transition rules.\n" );
        return -1;
    }

    if (verbose > 1)
        /* Dump the spec to show results of conversion (if any). */
        print_GR1_spec( spc.evar_list, spc.svar_list, spc.env_init, spc.sys_init,
                        spc.env_trans_array, spc.et_array_len,
                        spc.sys_trans_array, spc.st_array_len,
                        spc.env_goals, spc.num_egoals, spc.sys_goals, spc.num_sgoals, NULL );


    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

    manager = Cudd_Init( 2*(num_env+num_sys),
                         0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_SetMaxCacheHard( manager, (unsigned int)-1 );
    Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );

    T = check_realizable( manager, EXIST_SYS_INIT, verbose );
    if (verbose) {
        if (T != NULL) {
            logprint( "Realizable." );
        } else {
            logprint( "Not realizable." );
        }
    }

    if (T != NULL) { /* Print measure data and simulate. */
        if (horizon < 0) {
            if (verbose)
                logprint( "Computing horizon with metric variables: %s",
                          metric_vars );
            horizon = compute_horizon( manager, &W, &etrans, &strans, &sgoals,
                                       metric_vars, verbose );
            logprint( "horizon: %d", horizon );
            if (getlogstream() != stdout)
                printf( "horizon: %d\n", horizon );
        } else {
            W = compute_winning_set_saveBDDs( manager, &etrans, &strans,
                                              &egoals, &sgoals, verbose );
            if (verbose)
                logprint( "Using given horizon: %d", horizon );
        }

        if (max_sim_it >= 0 && horizon > -1) {
            /* Compute variable offsets and use it to get the initial
               state as a bitvector */
            offw = get_offsets( all_vars, &num_vars );
            if (num_vars != original_num_env+original_num_sys) {
                fprintf( stderr,
                         "Error while computing bitwise variable offsets.\n" );
                return -1;
            }
            free( all_vars );
            init_state_acc = 0;
            if (verbose) {
                logprint_startline();
                logprint_raw( "initial state for simulation:" );
            }
            for (i = 0; i < original_num_env+original_num_sys; i++) {
                if (verbose)
                    logprint_raw( " %d", *(init_state_ints+i) );
                init_state_acc += *(init_state_ints+i) << *(offw + 2*i);
            }
            if (verbose)
                logprint_endline();
            free( offw );
            init_state = int_to_bitvec( init_state_acc, num_env+num_sys );
            if (init_state == NULL)
                return -1;

            play = sim_rhc( manager, W, etrans, strans, sgoals, metric_vars,
                            horizon, init_state, max_sim_it, verbose );
            if (play == NULL) {
                fprintf( stderr,
                         "Error while attempting receding horizon"
                         " simulation.\n" );
                return -1;
            }
            free( init_state );
            logprint( "play length: %d", aut_size( play ) );
            tmppt = spc.nonbool_var_list;
            while (tmppt) {
                aut_compact_nonbool( play, spc.evar_list, spc.svar_list,
                                     tmppt->name, tmppt->value );
                tmppt = tmppt->left;
            }

            num_env = tree_size( spc.evar_list );
            num_sys = tree_size( spc.svar_list );

            /* Open output file if specified; else point to stdout. */
            if (output_file_index >= 0) {
                fp = fopen( argv[output_file_index], "w" );
                if (fp == NULL) {
                    perror( "grjit, fopen" );
                    return -1;
                }
            } else {
                fp = stdout;
            }

            /* Print simulation trace */
            dump_simtrace( play, spc.evar_list, spc.svar_list, fp );
            if (fp != stdout)
                fclose( fp );
        }


        Cudd_RecursiveDeref( manager, W );
        Cudd_RecursiveDeref( manager, etrans );
        Cudd_RecursiveDeref( manager, strans );
    }

    /* Clean-up */
    free( metric_vars );
    delete_tree( spc.evar_list );
    delete_tree( spc.svar_list );
    delete_tree( spc.env_init );
    delete_tree( spc.sys_init );
    delete_tree( spc.env_trans );
    delete_tree( spc.sys_trans );
    for (i = 0; i < spc.num_egoals; i++)
        delete_tree( *(spc.env_goals+i) );
    if (spc.num_egoals > 0)
        free( spc.env_goals );
    for (i = 0; i < spc.num_sgoals; i++)
        delete_tree( *(spc.sys_goals+i) );
    if (spc.num_sgoals > 0)
        free( spc.sys_goals );
    if (T != NULL)
        Cudd_RecursiveDeref( manager, T );
    if (strategy)
        delete_aut( strategy );
    if (verbose > 1)
        logprint( "Cudd_CheckZeroRef -> %d", Cudd_CheckZeroRef( manager ) );
    Cudd_Quit(manager);
    if (logging_flag)
        closelogfile();

    /* Return 0 if realizable, -1 if not realizable. */
    if (T != NULL) {
        return 0;
    } else {
        return -1;
    }

    return 0;
}
