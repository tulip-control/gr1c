/* grpatch.c -- main entry point for execution of patching routines.
 *
 *
 * SCL; 2012-2015
 */


#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "logging.h"
#include "ptree.h"
#include "solve.h"
#include "patching.h"
#include "automaton.h"
#include "solve_metric.h"
#include "gr1c_util.h"
extern int yyparse( void );
extern void yyrestart( FILE *new_file );


/**************************
 **** Global variables ****/

extern specification_t spc;
extern ptree_t *gen_tree_ptr;

/**************************/


/* Output formats */
#define OUTPUT_FORMAT_TEXT 0
#define OUTPUT_FORMAT_TULIP 1
#define OUTPUT_FORMAT_DOT 2
#define OUTPUT_FORMAT_AUT 3
#define OUTPUT_FORMAT_JSON 5

/* Runtime modes */
#define GR1C_MODE_UNSET 0
#define GR1C_MODE_PATCH 4


#define PRINT_VERSION() \
    printf( "gr1c-patch (part of gr1c v" GR1C_VERSION ")\n\n" GR1C_COPYRIGHT "\n" )


int main( int argc, char **argv )
{
    FILE *fp;
    byte run_option = GR1C_MODE_UNSET;
    bool help_flag = False;
    bool ptdump_flag = False;
    bool logging_flag = False;
    byte format_option = OUTPUT_FORMAT_JSON;
    unsigned char verbose = 0;
    bool reading_options = True;  /* For disabling option parsing using "--" */
    int input_index = -1;
    int edges_input_index = -1;  /* If patching, command-line flag "-e". */
    int aut_input_index = -1;  /* For command-line flag "-a". */
    int output_file_index = -1;  /* For command-line flag "-o". */
    FILE *strategy_fp;
    char dumpfilename[64];

    FILE *clf_file = NULL;
    int clformula_index = -1;  /* For command-line flag "-f". */
    ptree_t *clformula = NULL;

    /* System goal to be removed; -1 indicates unset.  Order matches
       that of the given specification file, which should also be
       consistent with goal mode labeling of the given automaton. */
    int remove_goal_mode = -1;  /* For command-line flag "-r". */

    int i, j, var_index;
    ptree_t *tmppt;  /* General purpose temporary ptree pointer */
    char *tmpstr = NULL;

    DdManager *manager;
    DdNode *T = NULL;
    anode_t *strategy = NULL;
    int num_env, num_sys;
    int original_num_env, original_num_sys;

    char *metric_vars = NULL;
    int *offw = NULL, num_metric_vars;

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
            } else if (argv[i][1] == 'p') {
                ptdump_flag = True;
            } else if (argv[i][1] == 'm') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                metric_vars = strdup( argv[i+1] );
                i++;
            } else if (argv[i][1] == 't') {
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
            } else if (argv[i][1] == 'e') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                run_option = GR1C_MODE_PATCH;
                edges_input_index = i+1;
                i++;
            } else if (argv[i][1] == 'a') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                aut_input_index = i+1;
                i++;
            } else if (argv[i][1] == 'o') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                output_file_index = i+1;
                i++;
            } else if (argv[i][1] == 'f') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                run_option = GR1C_MODE_PATCH;
                clformula_index = i+1;
                i++;
            } else if (argv[i][1] == 'r') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                run_option = GR1C_MODE_PATCH;
                remove_goal_mode = strtol( argv[i+1], NULL, 10 );
                if (remove_goal_mode < 0) {
                    fprintf( stderr,
                             "System goal indices must be nonnegative.\n" );
                    return 1;
                }
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

    if (edges_input_index >= 0 && clformula_index >= 0) {
        fprintf( stderr,
                 "\"-e\" and \"-a\" flags cannot be used simultaneously.\n" );
        return 1;
    } else if (edges_input_index >= 0 && aut_input_index < 0) {
        fprintf( stderr, "\"-e\" flag can only be used with \"-a\"\n" );
        return 1;
    } else if (clformula_index >= 0 && aut_input_index < 0) {
        fprintf( stderr, "\"-f\" flag can only be used with \"-a\"\n" );
        return 1;
    } else if (remove_goal_mode >= 0 && aut_input_index < 0) {
        fprintf( stderr, "\"-r\" flag can only be used with \"-a\"\n" );
        return 1;
    }

    if (help_flag) {
        /* Split among printf() calls to conform with ISO C90 string length */
        printf( "Usage: %s [-hVvlp] [-m VARS] [-t TYPE] [-aeo FILE] [-f FORM] [-r N] [[--] FILE]\n\n"
                "  -h          this help message\n"
                "  -V          print version and exit\n"
                "  -v          be verbose; use -vv to be more verbose\n"
                "  -l          enable logging\n"
                "  -t TYPE     strategy output format; default is \"json\";\n"
                "              supported formats: txt, dot, aut, json, tulip\n"
                "  -p          dump parse trees to DOT files, and echo formulas to screen\n", argv[0] );
        printf( "  -m VARS     VARS is a space-separated list of metric variables\n"
                "  -a FILE     automaton input file, in gr1c \"aut\" format;\n"
                "              if FILE is -, then read from stdin\n"
                "  -e FILE     patch, given game edge set change file; requires -a flag\n"
                "  -o FILE     output strategy to FILE, rather than stdout (default)\n" );
        printf( "  -f FORM     FORM is a Boolean (state) formula, currently only\n"
                "              used for appending a system goal; requires -a flag.\n"
                "  -r N        remove system goal N (in order, according to given file);\n"
                "              requires -a flag.\n" );
        return 0;
    }

    if (input_index < 0 && (run_option == GR1C_MODE_PATCH
                            && !strncmp( argv[aut_input_index], "-", 1 ))) {
        printf( "Reading spec from stdin in some cases while patching is"
                " not yet implemented.\n" );
        return 1;
    }
    if (run_option == GR1C_MODE_UNSET) {
        fprintf( stderr,
                 "%s requires a patching request. Try \"-h\".\n",
                 argv[0] );
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

    if (metric_vars != NULL && strlen(metric_vars) == 0) {
        free( metric_vars );
        metric_vars = NULL;
        if (verbose > 1)
            logprint( "Empty metric variable list given at command-line." );
    }

    if (clformula_index >= 0) {
        if (verbose > 1) {
            logprint( "Parsing command-line formula \"%s\"...",
                      argv[clformula_index] );
        }

        clf_file = tmpfile();
        if (clf_file == NULL) {
            perror( "gr1c-patch, tmpfile" );
            return -1;
        }
        fprintf( clf_file, "%s\n", argv[clformula_index] );
        if (fseek( clf_file, 0, SEEK_SET )) {
            perror( "gr1c-patch, fseek" );
            return -1;
        }
        yyrestart( clf_file );
        SPC_INIT( spc );
        yyparse();
        fclose( clf_file );

        clformula = gen_tree_ptr;
        gen_tree_ptr = NULL;

        if (ptdump_flag)
            tree_dot_dump( clformula, "clformula_ptree.dot" );
        if (verbose > 1) {
            logprint_startline();
            logprint_raw( "Command-line formula, printed from ptree: " );
            print_formula( clformula, getlogstream(), FORMULA_SYNTAX_GR1C );
            logprint_endline();
        }
    }

    /* If filename for specification given at command-line, then use
       it.  Else, read from stdin. */
    if (input_index > 0) {
        fp = fopen( argv[input_index], "r" );
        if (fp == NULL) {
            perror( "gr1c-patch, fopen" );
            return -1;
        }
        yyrestart( fp );
    } else {
        yyrestart( stdin );
    }

    /* Parse the specification. */
    spc.evar_list = NULL;
    spc.svar_list = NULL;
    gen_tree_ptr = NULL;
    if (verbose)
        logprint( "Parsing input..." );
    SPC_INIT( spc );
    if (yyparse())
        return 2;
    if (verbose)
        logprint( "Done." );

    if (check_gr1c_form( spc.evar_list, spc.svar_list, spc.env_init, spc.sys_init,
                         spc.env_trans_array, spc.et_array_len,
                         spc.sys_trans_array, spc.st_array_len,
                         spc.env_goals, spc.num_egoals, spc.sys_goals, spc.num_sgoals,
                         UNDEFINED_INIT ) < 0)
        return 2;

    /* Close input file, if opened. */
    if (input_index > 0)
        fclose( fp );

    /* Check clformula for clandestine variables */
    tmppt = NULL;
    if (spc.svar_list == NULL) {
        spc.svar_list = spc.evar_list;
    } else {
        tmppt = get_list_item( spc.svar_list, -1 );
        tmppt->left = spc.evar_list;
    }
    if ((tmpstr = check_vars( clformula, spc.svar_list, NULL )) != NULL) {
        fprintf( stderr, "Unrecognized variable in -f formula: %s\n", tmpstr );
        return 1;
    }
    if (tmppt != NULL) {
        tmppt->left = NULL;
        tmppt = NULL;
    } else {
        spc.svar_list = NULL;
    }


    /* Omission implies empty. */
    if (spc.et_array_len == 0) {
        spc.et_array_len = 1;
        spc.env_trans_array = malloc( sizeof(ptree_t *) );
        if (spc.env_trans_array == NULL) {
            perror( "gr1c-patch, malloc" );
            return -1;
        }
        *spc.env_trans_array = init_ptree( PT_CONSTANT, NULL, 1 );
    }
    if (spc.st_array_len == 0) {
        spc.st_array_len = 1;
        spc.sys_trans_array = malloc( sizeof(ptree_t *) );
        if (spc.sys_trans_array == NULL) {
            perror( "gr1c-patch, malloc" );
            return -1;
        }
        *spc.sys_trans_array = init_ptree( PT_CONSTANT, NULL, 1 );
    }
    if (spc.num_sgoals == 0) {
        spc.num_sgoals = 1;
        spc.sys_goals = malloc( sizeof(ptree_t *) );
        if (spc.sys_goals == NULL) {
            perror( "gr1c-patch, malloc" );
            return -1;
        }
        *spc.sys_goals = init_ptree( PT_CONSTANT, NULL, 1 );
    }

    /* Number of variables, before expansion of those that are nonboolean */
    original_num_env = tree_size( spc.evar_list );
    original_num_sys = tree_size( spc.svar_list );


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
                            ALL_ENV_EXIST_SYS_INIT, verbose ) < 0)
        return -1;
    spc.nonbool_var_list = expand_nonbool_variables( &spc.evar_list, &spc.svar_list,
                                                     verbose );

    tmppt = spc.nonbool_var_list;
    while (tmppt) {
        if (clformula != NULL) {
            if (verbose > 1)
                logprint( "Expanding nonbool variable %s in command-line"
                          " formula...", tmppt->name );
            clformula = expand_to_bool( clformula, tmppt->name, tmppt->value );
            if (verbose > 1)
                logprint( "Done." );
        }
        tmppt = tmppt->left;
    }

    if (verbose > 1)
        /* Dump the spec to show results of conversion (if any). */
        print_GR1_spec( spc.evar_list, spc.svar_list, spc.env_init, spc.sys_init,
                        spc.env_trans_array, spc.et_array_len,
                        spc.sys_trans_array, spc.st_array_len,
                        spc.env_goals, spc.num_egoals, spc.sys_goals, spc.num_sgoals, NULL );


    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

    /* Compute bitwise offsets for metric variables, if requested. */
    if (metric_vars != NULL)
        offw = get_offsets( metric_vars, &num_metric_vars );

    manager = Cudd_Init( 2*(num_env+num_sys),
                         0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_SetMaxCacheHard( manager, (unsigned int)-1 );
    Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );

    if (!strncmp( argv[aut_input_index], "-", 1 )) {
        strategy_fp = stdin;
    } else {
        strategy_fp = fopen( argv[aut_input_index], "r" );
        if (strategy_fp == NULL) {
            perror( "gr1c-patch, fopen" );
            return -1;
        }
    }

    if (edges_input_index >= 0) {  /* patch_localfixpoint() */

        if (offw != NULL)
            free( offw );
        offw = get_offsets_list( spc.evar_list, spc.svar_list, spc.nonbool_var_list );

        fp = fopen( argv[edges_input_index], "r" );
        if (fp == NULL) {
            perror( "gr1c-patch, fopen" );
            return -1;
        }

        if (verbose)
            logprint( "Patching given strategy..." );
        strategy = patch_localfixpoint( manager, strategy_fp, fp,
                                        original_num_env, original_num_sys,
                                        spc.nonbool_var_list, offw, verbose );
        if (verbose)
            logprint( "Done." );

        fclose( fp );
    } else if (clformula_index >= 0) {  /* add_metric_sysgoal() */

        if (spc.et_array_len > 1) {
            spc.env_trans = merge_ptrees( spc.env_trans_array, spc.et_array_len, PT_AND );
        } else if (spc.et_array_len == 1) {
            spc.env_trans = *spc.env_trans_array;
        } else {
            fprintf( stderr,
                     "Syntax error: GR(1) specification is missing environment"
                     " transition rules.\n" );
            return 2;
        }
        if (spc.st_array_len > 1) {
            spc.sys_trans = merge_ptrees( spc.sys_trans_array, spc.st_array_len, PT_AND );
        } else if (spc.st_array_len == 1) {
            spc.sys_trans = *spc.sys_trans_array;
        } else {
            fprintf( stderr,
                     "Syntax error: GR(1) specification is missing system"
                     " transition rules.\n" );
            return 2;
        }

        if (verbose)
            logprint( "Patching given strategy..." );
        strategy = add_metric_sysgoal( manager, strategy_fp,
                                       original_num_env, original_num_sys,
                                       offw, num_metric_vars, clformula,
                                       verbose );
        if (verbose)
            logprint( "Done." );

    } else if (remove_goal_mode >= 0) {  /* rm_sysgoal() */

        if (remove_goal_mode > spc.num_sgoals-1) {
            fprintf( stderr,
                     "Requested system goal index %d exceeds maximum %d.\n",
                     remove_goal_mode,
                     spc.num_sgoals-1 );
            return 1;
        }

        if (spc.et_array_len > 1) {
            spc.env_trans = merge_ptrees( spc.env_trans_array, spc.et_array_len, PT_AND );
        } else if (spc.et_array_len == 1) {
            spc.env_trans = *spc.env_trans_array;
        } else {
            fprintf( stderr,
                     "Syntax error: GR(1) specification is missing environment"
                     " transition rules.\n" );
            return 2;
        }
        if (spc.st_array_len > 1) {
            spc.sys_trans = merge_ptrees( spc.sys_trans_array, spc.st_array_len, PT_AND );
        } else if (spc.st_array_len == 1) {
            spc.sys_trans = *spc.sys_trans_array;
        } else {
            fprintf( stderr,
                     "Syntax error: GR(1) specification is missing system"
                     " transition rules.\n" );
            return 2;
        }

        if (verbose)
            logprint( "Patching given strategy..." );
        strategy = rm_sysgoal( manager, strategy_fp,
                               original_num_env, original_num_sys,
                               remove_goal_mode, verbose );
        if (verbose)
            logprint( "Done." );

    } else {
        fprintf( stderr, "Unrecognized patching request.  Try \"-h\".\n" );
        return 1;
    }
    if (strategy_fp != stdin)
        fclose( strategy_fp );
    if (strategy == NULL) {
        fprintf( stderr, "Failed to patch strategy.\n" );
        return -1;
    }

    T = NULL;  /* To avoid seg faults by the generic clean-up code. */

    if (strategy != NULL) {  /* De-expand nonboolean variables */
        tmppt = spc.nonbool_var_list;
        while (tmppt) {
            aut_compact_nonbool( strategy, spc.evar_list, spc.svar_list,
                                 tmppt->name, tmppt->value );
            tmppt = tmppt->left;
        }

        num_env = tree_size( spc.evar_list );
        num_sys = tree_size( spc.svar_list );
    }

    if (strategy != NULL) {
        /* Open output file if specified; else point to stdout. */
        if (output_file_index >= 0) {
            fp = fopen( argv[output_file_index], "w" );
            if (fp == NULL) {
                perror( "gr1c-patch, fopen" );
                return -1;
            }
        } else {
            fp = stdout;
        }

        if (verbose)
            logprint( "Dumping automaton of size %d...", aut_size( strategy ) );

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
    if (metric_vars != NULL)
        free( metric_vars );
    if (offw != NULL)
        free( offw );
    delete_tree( clformula );
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

    return 0;
}
