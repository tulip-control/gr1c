/* autman.c -- entry point for a finite-state machine (automaton) manipulator
 *
 * Try invoking it with "-h"...
 *
 *
 * SCL; 2014-2015
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "logging.h"
#include "automaton.h"
#include "ptree.h"
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

/* Runtime modes */
#define AUTMAN_SYNTAX 1
#define AUTMAN_VARTYPES 2
#define AUTMAN_VERMODEL 3
#define AUTMAN_CONVERT 4

/* Verification model targets */
#define VERMODEL_TARGET_SPIN 1


int main( int argc, char **argv )
{
    FILE *fp;
    int i, j;
    int in_filename_index = -1;
    FILE *in_fp = NULL;
    anode_t *head;
    int version;
    int state_len = -1;
    byte format_option = OUTPUT_FORMAT_JSON;
    byte verification_model = 0;  /* For command-line flag "-P". */

    unsigned char verbose = 0;
    bool logging_flag = False;
    int run_option = AUTMAN_SYNTAX;
    int spc_file_index = -1;
    int output_file_index = -1;  /* For command-line flag "-o". */
    FILE *spc_fp;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][2] != '\0'
                && !(argv[i][1] == 'v' && argv[i][2] == 'v')) {
                fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                return 1;
            }

            if (argv[i][1] == 'h') {
                printf( "Usage: %s [-hVvlsP] [-t TYPE] [-L N] [-i FILE] [-o FILE] [FILE]\n\n"
                        "If no input file is given, or if FILE is -, read from stdin.  If no action\n"
                        "is requested, then assume -s.\n\n"
                        "  -h          this help message\n"
                        "  -V          print version and exit\n"
                        "  -v          be verbose; use -vv to be more verbose\n"
                        "  -l          enable logging\n"
                        "  -s          check syntax and get version;\n"
                        "              print format version number, or -1 if error.\n",
                        argv[0] );
/*                        "  -ss         extends -s to also check the number of and values\n"
                        "              assigned to variables, given specification.\n" */
                printf( "  -t TYPE     convert to format: txt, dot, aut, json, tulip\n"
                        "              some of these require a reference specification.\n"
                        "  -P          create Spin Promela model of strategy\n"
                        "              if used with -o, then the LTL formula is printed to stdout.\n"
                        "  -L N        declare that state vector size is N\n"
                        "  -i FILE     process strategy with respect to specification FILE\n"
                        "  -o FILE     output to FILE, rather than stdout (default)\n" );
                return 0;
            } else if (argv[i][1] == 'V') {
                printf( "gr1c-autman (automaton file manipulator, distributed with"
                        " gr1c v" GR1C_VERSION ")\n\n" GR1C_COPYRIGHT "\n" );
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
                if (argv[i][2] == 's')
                    run_option = AUTMAN_VARTYPES;
                else
                    run_option = AUTMAN_SYNTAX;
            } else if (argv[i][1] == 't') {
                run_option = AUTMAN_CONVERT;
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
            } else if (argv[i][1] == 'i') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                spc_file_index = i+1;
                i++;
            } else if (argv[i][1] == 'L') {
                if (i == argc-1) {
                    fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
                    return 1;
                }
                state_len = strtol( argv[i+1], NULL, 10 );
                i++;
            } else if (argv[i][1] == 'P') {
                run_option = AUTMAN_VERMODEL;
                verification_model = VERMODEL_TARGET_SPIN;
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
        } else {
            in_filename_index = i;
        }
    }

    if (run_option == AUTMAN_VERMODEL && spc_file_index < 0) {
        fprintf( stderr,
                 "-P flag requires a reference specification to be given"
                 " (-i switch).\n" );
        return 1;
    }

    if (run_option == AUTMAN_CONVERT && spc_file_index < 0
        && (format_option == OUTPUT_FORMAT_DOT
            || format_option == OUTPUT_FORMAT_JSON
            || format_option == OUTPUT_FORMAT_TULIP)) {
        fprintf( stderr,
                 "Conversion of output to selected format requires a"
                 " reference\nspecification to be given (-i switch).\n" );
        return 1;
    }

    if (spc_file_index < 0 && state_len < 1) {
        if (state_len < 0)
            fprintf( stderr,
                     "State vector length must be declared (-L switch)"
                     " when no reference\nspecification is given.\n" );
        else
            fprintf( stderr,
                     "State vector length must be at least 1.  Try \"-h\".\n" );
        return 1;
    }

/*    if (run_option == AUTMAN_VARTYPES && spc_file_index < 0) {
        fprintf( stderr,
                 "-ss flag requires a reference specification to be given"
                 " (-i switch).\n" );
        return 1;
    } */

    if (logging_flag) {
        openlogfile( NULL );
        if (verbose == 0)
            verbose = 1;
    } else {
        setlogstream( stdout );
        setlogopt( LOGOPT_NOTIME );
    }
    if (verbose > 0)
        logprint( "Running with verbosity level %d.", verbose );

    /* Parse the specification file if given. */
    if (spc_file_index >= 0) {
        if (verbose > 1)
            logprint( "Using file \"%s\" for reference specification.",
                      argv[spc_file_index] );

        spc_fp = fopen( argv[spc_file_index], "r" );
        if (spc_fp == NULL) {
            perror( "gr1c-autman, fopen" );
            return -1;
        }
        yyrestart( spc_fp );
        if (verbose)
            logprint( "Parsing reference specification file..." );
        SPC_INIT( spc );
        if (yyparse())
            return 2;
        if (verbose)
            logprint( "Done." );
        fclose( spc_fp );
        spc_fp = NULL;

        state_len = tree_size( spc.evar_list ) + tree_size( spc.svar_list );
        if (verbose)
            logprint( "Detected state vector length of %d.", state_len );
    }

    if (in_filename_index < 0 || !strncmp( argv[in_filename_index], "-", 1 )) {
        if (verbose > 1)
            logprint( "Using stdin for input." );
        in_fp = stdin;
    } else {
        if (verbose > 1)
            logprint( "Using file \"%s\" for input.", argv[in_filename_index] );
        in_fp = fopen( argv[in_filename_index], "r" );
        if (in_fp == NULL) {
            perror( "autman, fopen" );
            return -1;
        }
    }

    if (verbose > 1)
        logprint( "Loading automaton..." );
    head = aut_aut_loadver( state_len, in_fp, &version );
    if (head == NULL) {
        if (verbose)
            fprintf( stderr, "Error: failed to load aut.\n" );
        return 3;
    }
    if (verbose > 1)
        logprint( "Done." );
    if (verbose) {
        logprint( "Detected format version %d.", version );
        logprint( "Given automaton has size %d.", aut_size( head ) );
    }


    /* Open output file if specified; else point to stdout. */
    if (output_file_index >= 0) {
        fp = fopen( argv[output_file_index], "w" );
        if (fp == NULL) {
            perror( "gr1c, fopen" );
            return -1;
        }
    } else {
        fp = stdout;
    }

    switch (run_option) {
    case AUTMAN_SYNTAX:
        printf( "%d\n", version );
        return 0;

    case AUTMAN_VERMODEL:
        /* Currently, only target supported is Spin Promela,
           so the variable verification_model is not checked. */
        spin_aut_dump( head, spc.evar_list, spc.svar_list,
                       spc.env_init, spc.sys_init,
                       spc.env_trans_array, spc.et_array_len,
                       spc.sys_trans_array, spc.st_array_len,
                       spc.env_goals, spc.num_egoals,
                       spc.sys_goals, spc.num_sgoals,
                       fp, stdout );
        break;

    case AUTMAN_CONVERT:
        if (format_option == OUTPUT_FORMAT_TEXT) {
            list_aut_dump( head, state_len, fp );
        } else if (format_option == OUTPUT_FORMAT_DOT) {
            dot_aut_dump( head, spc.evar_list, spc.svar_list,
                          DOT_AUT_ATTRIB, fp );
        } else if (format_option == OUTPUT_FORMAT_AUT) {
            aut_aut_dump( head, state_len, fp );
        } else if (format_option == OUTPUT_FORMAT_JSON) {
            json_aut_dump( head, spc.evar_list, spc.svar_list, fp );
        } else { /* OUTPUT_FORMAT_TULIP */
            tulip_aut_dump( head, spc.evar_list, spc.svar_list, fp );
        }
        break;

    default:
        fprintf( stderr, "Unrecognized run option.  Try \"-h\".\n" );
        return 1;
    }

    if (fp != stdout)
        fclose( fp );

    return 0;
}
