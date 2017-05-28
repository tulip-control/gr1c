/* main.c -- main entry point for execution.
 *
 *
 * SCL; 2012-2015
 */


#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "common.h"
#include "logging.h"
#include "ptree.h"
#include "solve.h"
#include "automaton.h"
#include "gr1c_util.h"
extern int yyparse( void );
extern void yyrestart( FILE *new_file );


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

/* Verification model targets */
#define VERMODEL_TARGET_SPIN 1

/* Runtime modes */
#define GR1C_MODE_SYNTAX 0
#define GR1C_MODE_REALIZABLE 1
#define GR1C_MODE_SYNTHESIS 2
#define GR1C_MODE_INTERACTIVE 3


#define PRINT_VERSION() \
    printf( "gr1c " GR1C_VERSION "\n\n" GR1C_COPYRIGHT "\n" )


int main( int argc, char **argv )
{
    FILE *fp;
    byte run_option = GR1C_MODE_SYNTHESIS;
    bool help_flag = False;
    bool ptdump_flag = False;
    bool logging_flag = False;
    unsigned char init_flags = ALL_ENV_EXIST_SYS_INIT;
    byte format_option = OUTPUT_FORMAT_JSON;
    unsigned char verbose = 0;
    bool reading_options = True;  /* For disabling option parsing using "--" */
    int input_index = -1;
    int output_file_index = -1;  /* For command-line flag "-o". */
    char dumpfilename[64];
    char **command_argv = NULL;

    byte verification_model = 0;  /* For command-line flag "-P". */
    ptree_t *original_env_init = NULL;
    ptree_t *original_sys_init = NULL;
    ptree_t **original_env_goals = NULL;
    ptree_t **original_sys_goals = NULL;
    int original_num_egoals = 0;
    int original_num_sgoals = 0;
    ptree_t **original_env_trans_array = NULL;
    ptree_t **original_sys_trans_array = NULL;
    int original_et_array_len = 0;
    int original_st_array_len = 0;

    int i, j, var_index;
    ptree_t *tmppt;  /* General purpose temporary ptree pointer */

    DdManager *manager;
    DdNode *T = NULL;
    anode_t *strategy = NULL;
    int num_env, num_sys;

    /* Try to handle sub-commands first */
    if (argc >= 2) {
        if (!strncmp( argv[1], "rg", strlen( "rg" ) )
            && argv[1][strlen("rg")] == '\0') {

            /* Pass arguments after rg */
            command_argv = malloc( sizeof(char *)*argc );
            command_argv[0] = strdup( "gr1c rg" );
            command_argv[argc-1] = NULL;
            for (i = 1; i < argc-1; i++)
                command_argv[i] = argv[i+1];

            if (execvp( "gr1c-rg", command_argv ) < 0) {
                perror( __FILE__ ",  execvp" );
                return -1;
            }

        } else if (!strncmp( argv[1], "patch", strlen( "patch" ) )
                   && argv[1][strlen("patch")] == '\0') {

            command_argv = malloc( sizeof(char *)*argc );
            command_argv[0] = strdup( "gr1c patch" );
            command_argv[argc-1] = NULL;
            for (i = 1; i < argc-1; i++)
                command_argv[i] = argv[i+1];

            if (execvp( "gr1c-patch", command_argv ) < 0) {
                perror( __FILE__ ",  execvp" );
                return -1;
            }

        } else if (!strncmp( argv[1], "autman", strlen( "autman" ) )
                   && argv[1][strlen("autman")] == '\0') {

            command_argv = malloc( sizeof(char *)*argc );
            command_argv[0] = strdup( "gr1c autman" );
            command_argv[argc-1] = NULL;
            for (i = 1; i < argc-1; i++)
                command_argv[i] = argv[i+1];

            if (execvp( "gr1c-autman", command_argv ) < 0) {
                perror( __FILE__ ",  execvp" );
                return -1;
            }

        } else if (!strncmp( argv[1], "help", strlen( "help" ) )
                   && argv[1][strlen("help")] == '\0') {
            reading_options = False;
            help_flag = True;
        }
    }

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
                run_option = GR1C_MODE_SYNTAX;
            } else if (argv[i][1] == 'p') {
                ptdump_flag = True;
            } else if (argv[i][1] == 'r') {
                run_option = GR1C_MODE_REALIZABLE;
            } else if (argv[i][1] == 'i') {
                run_option = GR1C_MODE_INTERACTIVE;
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
            } else if (argv[i][1] == 'n') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                for (j = 0; j < strlen( argv[i+1] ); j++)
                    argv[i+1][j] = tolower( argv[i+1][j] );
                if (!strncmp( argv[i+1], "all_env_exist_sys_init",
                              strlen( "all_env_exist_sys_init" ) )) {
                    init_flags = ALL_ENV_EXIST_SYS_INIT;
                } else if (!strncmp( argv[i+1], "all_init",
                                     strlen( "all_init" ) )) {
                    init_flags = ALL_INIT;
                } else if (!strncmp( argv[i+1], "one_side_init",
                                     strlen( "one_side_init" ) )) {
                    init_flags = ONE_SIDE_INIT;
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
            } else if (argv[i][1] == 'P') {
                verification_model = VERMODEL_TARGET_SPIN;
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
            if (i < argc-1) {
                fprintf( stderr,
                         "Unexpected arguments after filename. Try \"-h\".\n" );
                return 1;
            }
            input_index = i;
        }
    }

    if (help_flag) {
        /* Split among printf() calls to conform with ISO C90 string length */
        printf( "Usage: %s [-hVvlspriP] [-n INIT] [-t TYPE] [-o FILE] [[--] FILE]\n\n"
                "  -h          this help message\n"
                "  -V          print version and exit\n"
                "  -v          be verbose; use -vv to be more verbose\n"
                "  -l          enable logging\n"
                "  -t TYPE     strategy output format; default is \"json\";\n"
                "              supported formats: txt, dot, aut, json, tulip\n", argv[0] );
        printf( "  -n INIT     initial condition interpretation; (not case sensitive)\n"
                "              one of\n"
                "                  ALL_ENV_EXIST_SYS_INIT (default)\n"
                "                  ALL_INIT\n"
                "                  ONE_SIDE_INIT\n"
                "  -s          only check specification syntax (return 2 on error)\n" );
        printf( "  -p          dump parse trees to DOT files, and echo formulas to screen\n"
                "  -r          only check realizability; do not synthesize strategy\n"
                "              (return 0 if realizable, 3 if not)\n"
                "  -i          interactive mode\n"
                "  -o FILE     output strategy to FILE, rather than stdout (default)\n"
                "  -P          create Spin Promela model of strategy;\n"
                "              output to stdout, so requires -o flag to also be used\n" );
        printf( "\nFor other commands, use: %s COMMAND [...]\n\n"
                "  rg          solve reachability game\n"
                "  autman      manipulate finite-memory strategies\n"
                "  patch       patch or modify a given strategy (incremental synthesis)\n"
                "  help        this help message (equivalent to -h)\n\n"
                "When applicable, any arguments after COMMAND are passed on to the\n"
                "appropriate program. Use -h to get the corresponding help message.\n", argv[0] );
        return 0;
    }

    if (input_index < 0 && (run_option == GR1C_MODE_INTERACTIVE)) {
        printf( "Reading spec from stdin in interactive mode is not yet"
                " implemented.\n" );
        return 1;
    }
    if (verification_model > 0 && output_file_index < 0) {
        printf( "-P flag can only be used with -o flag because the"
                " verification model is\noutput to stdout.\n" );
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
            perror( __FILE__ ",  fopen" );
            return -1;
        }
        yyrestart( fp );
    } else {
        yyrestart( stdin );
    }

    /* Parse the specification. */
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
                         init_flags ) < 0)
        return 2;

    if (run_option == GR1C_MODE_SYNTAX)
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
    if (spc.num_sgoals == 0) {
        spc.num_sgoals = 1;
        spc.sys_goals = malloc( sizeof(ptree_t *) );
        if (spc.sys_goals == NULL) {
            perror( __FILE__ ",  malloc" );
            return -1;
        }
        *spc.sys_goals = init_ptree( PT_CONSTANT, NULL, 1 );
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

        print_GR1_spec( spc.evar_list, spc.svar_list, spc.env_init, spc.sys_init,
                        spc.env_trans_array, spc.et_array_len,
                        spc.sys_trans_array, spc.st_array_len,
                        spc.env_goals, spc.num_egoals, spc.sys_goals, spc.num_sgoals, stdout );
    }

    /* If verification model will be created, then save copy of
       pristine ptrees, before nonbool expansion. */
    if (verification_model > 0) {
        original_num_egoals = spc.num_egoals;
        original_num_sgoals = spc.num_sgoals;
        original_et_array_len = spc.et_array_len;
        original_st_array_len = spc.st_array_len;
        original_env_goals = malloc( original_num_egoals*sizeof(ptree_t *) );
        original_sys_goals = malloc( original_num_sgoals*sizeof(ptree_t *) );
        original_env_trans_array = malloc( original_et_array_len*sizeof(ptree_t *) );
        original_sys_trans_array = malloc( original_st_array_len*sizeof(ptree_t *) );
        if (original_env_goals == NULL || original_sys_goals == NULL
            || original_env_trans_array == NULL || original_sys_trans_array == NULL) {
            perror( __FILE__ ",  malloc" );
            return -1;
        }
        original_env_init = copy_ptree( spc.env_init );
        original_sys_init = copy_ptree( spc.sys_init );
        for (i = 0; i < original_num_egoals; i++)
            *(original_env_goals+i) = copy_ptree( *(spc.env_goals+i) );
        for (i = 0; i < original_num_sgoals; i++)
            *(original_sys_goals+i) = copy_ptree( *(spc.sys_goals+i) );
        for (i = 0; i < original_et_array_len; i++)
            *(original_env_trans_array+i) = copy_ptree( *(spc.env_trans_array+i) );
        for (i = 0; i < original_st_array_len; i++)
            *(original_sys_trans_array+i) = copy_ptree( *(spc.sys_trans_array+i) );
    }

    if (expand_nonbool_GR1( spc.evar_list, spc.svar_list, &spc.env_init, &spc.sys_init,
                            &spc.env_trans_array, &spc.et_array_len,
                            &spc.sys_trans_array, &spc.st_array_len,
                            &spc.env_goals, spc.num_egoals, &spc.sys_goals, spc.num_sgoals,
                            init_flags, verbose ) < 0) {
        if (verification_model > 0) {
            for (j = 0; j < original_num_egoals; j++)
                free( *(original_env_goals+j) );
            free( original_env_goals );
            for (j = 0; j < original_num_sgoals; j++)
                free( *(original_sys_goals+j) );
            free( original_sys_goals );
            for (j = 0; j < original_et_array_len; j++)
                free( *(original_env_trans_array+j) );
            free( original_env_trans_array );
            for (j = 0; j < original_st_array_len; j++)
                free( *(original_sys_trans_array+j) );
            free( original_sys_trans_array );
        }
        return -1;
    }
    spc.nonbool_var_list = expand_nonbool_variables( &spc.evar_list, &spc.svar_list,
                                                     verbose );

    /* Merge component safety (transition) formulas */
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

    if (run_option == GR1C_MODE_INTERACTIVE) {

        /* NOT IMPLEMENTED YET FOR NONBOOL VARIABLES */
        if (spc.nonbool_var_list != NULL) {
            fprintf( stderr,
                     "gr1c interaction does not yet support specifications"
                     " with nonboolean domains.\n" );
            return -1;
        }

        i = levelset_interactive( manager, init_flags, stdin, stdout, verbose );
        if (i == 0) {
            printf( "Not realizable.\n" );
            return 3;
        } else if (i < 0) {
            printf( "Failure during interaction.\n" );
            return -1;
        }

        T = NULL;  /* To avoid seg faults by the generic clean-up code. */
    } else {

        T = check_realizable( manager, init_flags, verbose );
        if (run_option == GR1C_MODE_REALIZABLE) {
            if ((verbose == 0) || (getlogstream() != stdout)) {
                if (T != NULL) {
                    printf( "Realizable.\n" );
                } else {
                    printf( "Not realizable.\n" );
                }
            }
        }

        if (verbose) {
            if (T != NULL) {
                logprint( "Realizable." );
            } else {
                logprint( "Not realizable." );
            }
        }

        if (run_option == GR1C_MODE_SYNTHESIS && T != NULL) {

            if (verbose)
                logprint( "Synthesizing a strategy..." );
            strategy = synthesize( manager, init_flags, verbose );
            if (verbose)
                logprint( "Done." );
            if (strategy == NULL) {
                fprintf( stderr, "Error while attempting synthesis.\n" );
                return -1;
            }

        }
    }

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
                perror( __FILE__ ",  fopen" );
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

        if (verification_model > 0) {
            /* Currently, only target supported is Spin Promela */
            spin_aut_dump( strategy, spc.evar_list, spc.svar_list,
                           original_env_init, original_sys_init,
                           original_env_trans_array, original_et_array_len,
                           original_sys_trans_array, original_st_array_len,
                           original_env_goals, original_num_egoals,
                           original_sys_goals, original_num_sgoals,
                           stdout, NULL );
        }
    }

    /* Clean-up */
    delete_tree( spc.evar_list );
    delete_tree( spc.svar_list );
    delete_tree( spc.nonbool_var_list );
    delete_tree( spc.env_init );
    delete_tree( spc.sys_init );
    delete_tree( spc.env_trans );
    free( spc.env_trans_array );
    delete_tree( spc.sys_trans );
    free( spc.sys_trans_array );
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

    /* Return 0 if realizable, 1 if not realizable. */
    if (run_option == GR1C_MODE_INTERACTIVE || T != NULL) {
        return 0;
    } else {
        return 3;
    }

    return 0;
}
