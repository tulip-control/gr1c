/* rg_main.c -- main entry point for execution of gr1c-rg.
 *
 *
 * SCL; 2012-2015
 */


#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "common.h"
#include "logging.h"
#include "ptree.h"
#include "automaton.h"
#include "solve.h"
#include "patching.h"
#include "gr1c_util.h"
extern int yyparse( void );


/**************************
 **** Global variables ****/

extern specification_t spc;

/**************************/


/* Output formats */
#define OUTPUT_FORMAT_TEXT 0
#define OUTPUT_FORMAT_TULIP 1
#define OUTPUT_FORMAT_DOT 2
#define OUTPUT_FORMAT_AUT 3
#define OUTPUT_FORMAT_JSON 5

/* Runtime modes */
#define RG_MODE_SYNTAX 0
/* #define RG_MODE_REALIZABLE 1 */
#define RG_MODE_SYNTHESIS 2


#define PRINT_VERSION() \
    printf( "gr1c-rg (part of gr1c) " GR1C_VERSION "\n\n" GR1C_COPYRIGHT "\n" )


extern anode_t *synthesize_reachgame_BDD( DdManager *manager,
                                          int num_env, int num_sys,
                                          DdNode *Entry, DdNode *Exit,
                                          DdNode *etrans, DdNode *strans,
                                          DdNode **egoals, DdNode *N_BDD,
                                          unsigned char verbose );


int main( int argc, char **argv )
{
    FILE *fp;
    FILE *stdin_backup = NULL;
    byte run_option = RG_MODE_SYNTHESIS;
    bool help_flag = False;
    bool ptdump_flag = False;
    bool logging_flag = False;
    unsigned char init_flags = ALL_INIT;
    byte format_option = OUTPUT_FORMAT_JSON;
    unsigned char verbose = 0;
    bool reading_options = True;  /* For disabling option parsing using "--" */
    int input_index = -1;
    int output_file_index = -1;  /* For command-line flag "-o". */
    char dumpfilename[64];

    int i, j, var_index;
    ptree_t *tmppt;  /* General purpose temporary ptree pointer */
    DdNode **vars, **pvars;
    bool env_nogoal_flag = False;
    ptree_t *var_separator;

    DdManager *manager;
    anode_t *strategy = NULL;
    int num_env, num_sys;

    DdNode *Entry, *Exit;
    DdNode *sinit, *einit, *etrans, *strans, **egoals;

    /* Look for flags in command-line arguments. */
    for (i = 1; i < argc; i++) {
        if (reading_options && argv[i][0] == '-' && argv[i][1] != '-') {
            if (argv[i][2] != '\0'
                && !(argv[i][1] == 'v' && argv[i][2] == 'v')) {
                fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                return 1;
            }

            if (argv[i][1] == 'h') {
                help_flag = True;
            } else if (argv[i][1] == 'V') {
                PRINT_VERSION();
                PRINT_LINKED_VERSIONS();
                return 0;
            } else if (argv[i][1] == 'v') {
                verbose++;
                j = 2;
                /* Only support up to "level 2" of verbosity */
                while (argv[i][j] == 'v' && j <= 2) {
                    verbose++;
                    j++;
                }
            } else if (argv[i][1] == 'l') {
                logging_flag = True;
            } else if (argv[i][1] == 's') {
                run_option = RG_MODE_SYNTAX;
            } else if (argv[i][1] == 'p') {
                ptdump_flag = True;
            } /*else if (argv[i][1] == 'r') {
                run_option = RG_MODE_REALIZABLE;
                } */
            else if (argv[i][1] == 't') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                if (!strncmp( argv[i+1], "txt", strlen( "txt" ) )) {
                    format_option = OUTPUT_FORMAT_TEXT;
                } else if (!strncmp( argv[i+1], "tulip", strlen( "tulip" ) )) {
                    format_option = OUTPUT_FORMAT_TULIP;
                } else if (!strncmp( argv[i+1], "dot", strlen( "dot" ) )) {
                    format_option = OUTPUT_FORMAT_DOT;
                } else if (!strncmp( argv[i+1], "aut", strlen( "aut" ) )) {
                    format_option = OUTPUT_FORMAT_AUT;
                } else if (!strncmp( argv[i+1], "json", strlen( "json" ) )) {
                    format_option = OUTPUT_FORMAT_JSON;
                } else {
                    fprintf( stderr,
                             "Unrecognized output format. Try \"-h\".\n" );
                    return 1;
                }
                i++;
            } else if (argv[i][1] == 'n') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                for (j = 0; j < strlen( argv[i+1] ); j++)
                    argv[i+1][j] = tolower( argv[i+1][j] );
                if (!strncmp( argv[i+1], "all_init",
                                     strlen( "all_init" ) )) {
                    init_flags = ALL_INIT;
                } else {
                    fprintf( stderr,
                             "Unrecognized init flags. Try \"-h\".\n" );
                    return 1;
                }
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
        } else if (reading_options && argv[i][0] == '-' && argv[i][1] == '-') {
            if (argv[i][2] == '\0') {
                reading_options = False;
            } else if (!strncmp( argv[i]+2, "help", strlen( "help" ) )) {
                help_flag = True;
            } else if (!strncmp( argv[i]+2, "version", strlen( "version" ) )) {
                PRINT_VERSION();
                return 0;
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
        printf( "Usage: %s [-hVvls] [-t TYPE] [-o FILE] [[--] FILE]\n\n"
                "  -h        this help message\n"
                "  -V        print version and exit\n"
                "  -v        be verbose\n"
                "  -l        enable logging\n"
                "  -t TYPE   strategy output format; default is \"json\";\n"
                "            supported formats: txt, dot, aut, json, tulip\n", argv[0] );
        printf( "  -n INIT   initial condition interpretation; (not case sensitive)\n"
                "              one of\n"
                "                  ALL_INIT (default)\n"
                "  -s        only check specification syntax (return 2 on error)\n"
/*                "  -r        only check realizability; do not synthesize strategy\n"
                "            (return 0 if realizable, 3 if not)\n" */
                "  -o FILE   output strategy to FILE, rather than stdout (default)\n" );
        return 0;
    }

    if (logging_flag) {
        openlogfile( "rg" );
        verbose = 1;
    } else {
        setlogstream( stdout );
        setlogopt( LOGOPT_NOTIME );
    }

    /* If filename for specification given at command-line, then use
       it.  Else, read from stdin. */
    if (input_index > 0) {
        fp = fopen( argv[input_index], "r" );
        if (fp == NULL) {
            perror( __FILE__ ",  fopen" );
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
        return 2;
    if (verbose)
        logprint( "Done." );
    if (stdin_backup != NULL) {
        stdin = stdin_backup;
    }

    if (spc.num_sgoals > 1) {
        fprintf( stderr,
                 "Syntax error: reachability game specification has more"
                 " than 1 system goal.\n" );
        return 2;
    }

    if (check_gr1c_form( spc.evar_list, spc.svar_list, spc.env_init, spc.sys_init,
                         spc.env_trans_array, spc.et_array_len,
                         spc.sys_trans_array, spc.st_array_len,
                         spc.env_goals, spc.num_egoals, spc.sys_goals, spc.num_sgoals,
                         init_flags ) < 0)
        return 2;

    if (run_option == RG_MODE_SYNTAX)
        return 0;

    /* Close input file, if opened. */
    if (input_index > 0)
        fclose( fp );

    /* Omission implies empty. */
    if (spc.et_array_len == 0) {
        spc.et_array_len = 1;
        spc.env_trans_array = malloc( sizeof(ptree_t *) );
        if (spc.env_trans_array == NULL) {
            perror( __FILE__ ",  malloc" );
            return -1;
        }
        *spc.env_trans_array = init_ptree( PT_CONSTANT, NULL, 1 );
    }
    if (spc.st_array_len == 0) {
        spc.st_array_len = 1;
        spc.sys_trans_array = malloc( sizeof(ptree_t *) );
        if (spc.sys_trans_array == NULL) {
            perror( __FILE__ ",  malloc" );
            return -1;
        }
        *spc.sys_trans_array = init_ptree( PT_CONSTANT, NULL, 1 );
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
        if (spc.num_sgoals > 0)
            tree_dot_dump( *spc.sys_goals, "sys_goal_ptree.dot" );

        var_index = 0;
        printf( "Environment variables (indices; domains): " );
        if (spc.evar_list == NULL) {
            printf( "(none)" );
        } else {
            tmppt = spc.evar_list;
            while (tmppt) {
                if (tmppt->value == -1) {  /* Boolean */
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
                if (tmppt->value == -1) {  /* Boolean */
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

        printf( "ENV INIT:  " );
        print_formula( spc.env_init, stdout, FORMULA_SYNTAX_GR1C );
        printf( "\n" );

        printf( "SYS INIT:  " );
        print_formula( spc.sys_init, stdout, FORMULA_SYNTAX_GR1C );
        printf( "\n" );

        printf( "ENV TRANS:  [] " );
        print_formula( spc.env_trans, stdout, FORMULA_SYNTAX_GR1C );
        printf( "\n" );

        printf( "SYS TRANS:  [] " );
        print_formula( spc.sys_trans, stdout, FORMULA_SYNTAX_GR1C );
        printf( "\n" );

        printf( "ENV GOALS:  " );
        if (spc.num_egoals == 0) {
            printf( "(none)" );
        } else {
            printf( "[]<> " );
            print_formula( *spc.env_goals, stdout, FORMULA_SYNTAX_GR1C );
            for (i = 1; i < spc.num_egoals; i++) {
                printf( " & []<> " );
                print_formula( *(spc.env_goals+i), stdout, FORMULA_SYNTAX_GR1C );
            }
        }
        printf( "\n" );

        printf( "SYS GOAL:  " );
        if (spc.num_sgoals == 0) {
            printf( "(none)" );
        } else {
            printf( "<> " );
            print_formula( *spc.sys_goals, stdout, FORMULA_SYNTAX_GR1C );
        }
        printf( "\n" );
    }

    if (expand_nonbool_GR1( spc.evar_list, spc.svar_list, &spc.env_init, &spc.sys_init,
                            &spc.env_trans_array, &spc.et_array_len,
                            &spc.sys_trans_array, &spc.st_array_len,
                            &spc.env_goals, spc.num_egoals, &spc.sys_goals, spc.num_sgoals,
                            init_flags, verbose ) < 0)
        return -1;
    spc.nonbool_var_list = expand_nonbool_variables( &spc.evar_list, &spc.svar_list,
                                                     verbose );

    /* Merge component safety (transition) formulas */
    if (spc.et_array_len > 1) {
        spc.env_trans = merge_ptrees( spc.env_trans_array, spc.et_array_len, PT_AND );
    } else if (spc.et_array_len == 1) {
        spc.env_trans = *spc.env_trans_array;
    } else {  /* No restrictions on transitions. */
        fprintf( stderr,
                 "Syntax error: GR(1) specification is missing environment"
                 " transition rules.\n" );
        return 2;
    }
    if (spc.st_array_len > 1) {
        spc.sys_trans = merge_ptrees( spc.sys_trans_array, spc.st_array_len, PT_AND );
    } else if (spc.st_array_len == 1) {
        spc.sys_trans = *spc.sys_trans_array;
    } else {  /* No restrictions on transitions. */
        fprintf( stderr,
                 "Syntax error: GR(1) specification is missing system"
                 " transition rules.\n" );
        return 2;
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

    if (verbose > 1) {
        logprint_startline();
        logprint_raw( "rg invoked with init_flags: " );
        LOGPRINT_INIT_FLAGS( init_flags );
        logprint_endline();
    }
    if (verbose)
        logprint( "Synthesizing a reachability game strategy..." );

    /* Set environment goal to True (i.e., any state) if none was
       given. This simplifies the implementation below. */
    if (spc.num_egoals == 0) {
        env_nogoal_flag = True;
        spc.num_egoals = 1;
        spc.env_goals = malloc( sizeof(ptree_t *) );
        *spc.env_goals = init_ptree( PT_CONSTANT, NULL, 1 );
    }

    /* Chain together environment and system variable lists for
       working with BDD library. */
    if (spc.evar_list == NULL) {
        var_separator = NULL;
        spc.evar_list = spc.svar_list;  /* that this is the deterministic case
                                           is indicated by var_separator = NULL. */
    } else {
        var_separator = get_list_item( spc.evar_list, -1 );
        if (var_separator == NULL) {
            fprintf( stderr,
                     "Error: get_list_item failed on environment variables"
                     " list.\n" );
            return -1;
        }
        var_separator->left = spc.svar_list;
    }

    /* Generate BDDs for the various parse trees from the problem spec. */
    if (spc.env_init != NULL) {
        einit = ptree_BDD( spc.env_init, spc.evar_list, manager );
    } else {
        einit = Cudd_ReadOne( manager );
        Cudd_Ref( einit );
    }
    if (spc.sys_init != NULL) {
        sinit = ptree_BDD( spc.sys_init, spc.evar_list, manager );
    } else {
        sinit = Cudd_ReadOne( manager );
        Cudd_Ref( sinit );
    }
    if (verbose > 1)
        logprint( "Building environment transition BDD..." );
    etrans = ptree_BDD( spc.env_trans, spc.evar_list, manager );
    if (verbose > 1) {
        logprint( "Done." );
        logprint( "Building system transition BDD..." );
    }
    strans = ptree_BDD( spc.sys_trans, spc.evar_list, manager );
    if (verbose > 1)
        logprint( "Done." );
    if (spc.num_egoals > 0) {
        egoals = malloc( spc.num_egoals*sizeof(DdNode *) );
        for (i = 0; i < spc.num_egoals; i++)
            *(egoals+i) = ptree_BDD( *(spc.env_goals+i), spc.evar_list, manager );
    } else {
        egoals = NULL;
    }

    Entry = Cudd_bddAnd( manager, einit, sinit );
    Cudd_Ref( Entry );
    if (spc.num_sgoals > 0) {
        Exit = ptree_BDD( *spc.sys_goals, spc.evar_list, manager );
    } else {
        Exit = Cudd_Not( Cudd_ReadOne( manager ) );  /* No exit */
        Cudd_Ref( Exit );
    }

    if (var_separator == NULL) {
        spc.evar_list = NULL;
    } else {
        var_separator->left = NULL;
    }

    /* Define a map in the manager to easily swap variables with their
       primed selves. */
    vars = malloc( (num_env+num_sys)*sizeof(DdNode *) );
    pvars = malloc( (num_env+num_sys)*sizeof(DdNode *) );
    for (i = 0; i < num_env+num_sys; i++) {
        *(vars+i) = Cudd_bddIthVar( manager, i );
        *(pvars+i) = Cudd_bddIthVar( manager, i+num_env+num_sys );
    }
    if (!Cudd_SetVarMap( manager, vars, pvars, num_env+num_sys )) {
        fprintf( stderr,
                 "Error: failed to define variable map in CUDD manager.\n" );
        return -1;
    }
    free( vars );
    free( pvars );

    strategy = synthesize_reachgame_BDD( manager, num_env, num_sys,
                                         Entry, Exit, etrans, strans,
                                         egoals, Cudd_ReadOne( manager ),
                                         verbose );

    if (strategy == NULL) {
        fprintf( stderr, "Synthesis failed.\n" );
        return -1;
    } else {

        /* De-expand nonboolean variables */
        tmppt = spc.nonbool_var_list;
        while (tmppt) {
            aut_compact_nonbool( strategy, spc.evar_list, spc.svar_list,
                                 tmppt->name, tmppt->value );
            tmppt = tmppt->left;
        }

        num_env = tree_size( spc.evar_list );
        num_sys = tree_size( spc.svar_list );

        /* Open output file if specified; else point to stdout. */
        if (output_file_index >= 0) {
            fp = fopen( argv[output_file_index], "w" );
            if (fp == NULL) {
                perror( __FILE__ ",  fopen" );
                return -1;
            }
        } else {
            fp = stdout;
        }

        if (format_option == OUTPUT_FORMAT_TEXT) {
            list_aut_dump( strategy, num_env+num_sys, fp );
        } else if (format_option == OUTPUT_FORMAT_DOT) {
            if (spc.nonbool_var_list != NULL) {
                dot_aut_dump( strategy, spc.evar_list, spc.svar_list,
                              DOT_AUT_ATTRIB, fp );
            } else {
                dot_aut_dump( strategy, spc.evar_list, spc.svar_list,
                              DOT_AUT_BINARY | DOT_AUT_ATTRIB, fp );
            }
        } else if (format_option == OUTPUT_FORMAT_AUT) {
            aut_aut_dump( strategy, num_env+num_sys, fp );
        } else if (format_option == OUTPUT_FORMAT_JSON) {
            json_aut_dump( strategy, spc.evar_list, spc.svar_list, fp );
        } else { /* OUTPUT_FORMAT_TULIP */
            tulip_aut_dump( strategy, spc.evar_list, spc.svar_list, fp );
        }

        if (fp != stdout)
            fclose( fp );
    }

    /* Clean-up */
    delete_tree( spc.evar_list );
    delete_tree( spc.svar_list );
    delete_tree( spc.env_init );
    delete_tree( spc.sys_init );
    delete_tree( spc.env_trans );
    delete_tree( spc.sys_trans );
    if (spc.num_sgoals > 0) {
        delete_tree( *spc.sys_goals );
        free( spc.sys_goals );
    }
    Cudd_RecursiveDeref( manager, einit );
    Cudd_RecursiveDeref( manager, sinit );
    Cudd_RecursiveDeref( manager, etrans );
    Cudd_RecursiveDeref( manager, strans );
    Cudd_RecursiveDeref( manager, Entry );
    Cudd_RecursiveDeref( manager, Exit );
    for (i = 0; i < spc.num_egoals; i++)
        Cudd_RecursiveDeref( manager, *(egoals+i) );
    free( egoals );
    if (env_nogoal_flag) {
        spc.num_egoals = 0;
        delete_tree( *spc.env_goals );
        free( spc.env_goals );
    } else {
        for (i = 0; i < spc.num_egoals; i++)
            delete_tree( *(spc.env_goals+i) );
        if (spc.num_egoals > 0)
            free( spc.env_goals );
    }
    if (strategy)
        delete_aut( strategy );
    if (verbose > 1)
        logprint( "Cudd_CheckZeroRef -> %d", Cudd_CheckZeroRef( manager ) );
    Cudd_Quit(manager);
    if (logging_flag)
        closelogfile();

    /* Return 0 if realizable, 3 if not realizable. */
    if (strategy != NULL) {
        return 0;
    } else {
        return 3;
    }

    return 0;
}
