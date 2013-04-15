/* main.c -- main entry point for execution.
 *
 * Command-line arguments are processed by hand.  Eventually switch to
 * getopt, once sophistication of usage demands.
 *
 *
 * SCL; 2012, 2013.
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

ptree_t *nonbool_var_list = NULL;

extern ptree_t *evar_list;
extern ptree_t *svar_list;
extern ptree_t *env_init;
extern ptree_t *sys_init;
ptree_t *env_trans = NULL;  /* Built from component parse trees in env_trans_array. */
ptree_t *sys_trans = NULL;
extern ptree_t **env_goals;
extern ptree_t **sys_goals;
extern int num_egoals;
extern int num_sgoals;

extern ptree_t **env_trans_array;
extern ptree_t **sys_trans_array;
extern int et_array_len;
extern int st_array_len;

/**************************/


/* Output formats */
#define OUTPUT_FORMAT_TEXT 0
#define OUTPUT_FORMAT_TULIP 1
#define OUTPUT_FORMAT_DOT 2
#define OUTPUT_FORMAT_AUT 3
#define OUTPUT_FORMAT_TULIP0 4

/* Runtime modes */
#define GR1C_MODE_SYNTAX 0
#define GR1C_MODE_REALIZABLE 1
#define GR1C_MODE_SYNTHESIS 2
#define GR1C_MODE_INTERACTIVE 3


int main( int argc, char **argv )
{
	FILE *fp;
	byte run_option = GR1C_MODE_SYNTHESIS;
	bool help_flag = False;
	bool ptdump_flag = False;
	bool logging_flag = False;
	byte format_option = OUTPUT_FORMAT_TULIP;
	unsigned char verbose = 0;
	int input_index = -1;
	int output_file_index = -1;  /* For command-line flag "-o". */
	char dumpfilename[64];

	int i, j, var_index;
	ptree_t *tmppt;  /* General purpose temporary ptree pointer */

	DdManager *manager;
	DdNode *T = NULL;
	anode_t *strategy = NULL;
	int num_env, num_sys;
	int original_num_env, original_num_sys;

	/* Look for flags in command-line arguments. */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'h') {
				help_flag = True;
			} else if (argv[i][1] == 'V') {
				printf( "gr1c " GR1C_VERSION "\n\n" GR1C_COPYRIGHT "\n" );
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
				} else if (!strncmp( argv[i+1], "tulip0", strlen( "tulip0" ) )) {
					format_option = OUTPUT_FORMAT_TULIP0;
				} else if (!strncmp( argv[i+1], "tulip", strlen( "tulip" ) )) {
					format_option = OUTPUT_FORMAT_TULIP;
				} else if (!strncmp( argv[i+1], "dot", strlen( "dot" ) )) {
					format_option = OUTPUT_FORMAT_DOT;
				} else if (!strncmp( argv[i+1], "aut", strlen( "aut" ) )) {
					format_option = OUTPUT_FORMAT_AUT;
				} else {
					fprintf( stderr, "Unrecognized output format. Try \"-h\".\n" );
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
		} else if (input_index < 0) {
			/* Use first non-flag argument as filename whence to read
			   specification. */
			input_index = i;
		}
	}

	if (help_flag) {
		/* Split among printf() calls to conform with ISO C90 string length */
		printf( "Usage: %s [-hVvlspri] [-t TYPE] [-o FILE] [FILE]\n\n"
				"  -h          this help message\n"
				"  -V          print version and exit\n"
				"  -v          be verbose; use -vv to be more verbose\n"
				"  -l          enable logging\n"
				"  -t TYPE     strategy output format; default is \"tulip\";\n"
				"              supported formats: txt, dot, aut, tulip, tulip0\n"
				"  -s          only check specification syntax (return -1 on error)\n"
				"  -p          dump parse trees to DOT files, and echo formulas to screen\n", argv[0] );
		printf( "  -r          only check realizability; do not synthesize strategy\n"
				"              (return 0 if realizable, -1 if not)\n"
				"  -i          interactive mode\n"
				"  -o FILE     output strategy to FILE, rather than stdout (default)\n" );
		return 1;
	}

	if (input_index < 0 && (run_option == GR1C_MODE_INTERACTIVE)) {
		printf( "Reading spec from stdin in interactive mode is not yet implemented.\n" );
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
			perror( "gr1c, fopen" );
			return -1;
		}
		yyrestart( fp );
	} else {
		yyrestart( stdin );
	}

	/* Parse the specification. */
	if (verbose)
		logprint( "Parsing input..." );
	if (yyparse())
		return -1;
	if (verbose)
		logprint( "Done." );

	if (check_gr1c_form( evar_list, svar_list, env_init, sys_init,
						 env_trans_array, et_array_len,
						 sys_trans_array, st_array_len,
						 env_goals, num_egoals, sys_goals, num_sgoals ) < 0)
		return -1;

	if (run_option == GR1C_MODE_SYNTAX)
		return 0;

	/* Close input file, if opened. */
	if (input_index > 0)
		fclose( fp );

	/* Treat deterministic problem in which ETRANS or EINIT is omitted. */
	if (evar_list == NULL) {
		if (et_array_len == 0) {
			et_array_len = 1;
			env_trans_array = malloc( sizeof(ptree_t *) );
			if (env_trans_array == NULL) {
				perror( "gr1c, malloc" );
				return -1;
			}
			*env_trans_array = init_ptree( PT_CONSTANT, NULL, 1 );
		}
		if (env_init == NULL)
			env_init = init_ptree( PT_CONSTANT, NULL, 1 );
	}

	/* Merge component safety (transition) formulas */
	if (et_array_len > 1) {
		env_trans = merge_ptrees( env_trans_array, et_array_len, PT_AND );
	} else if (et_array_len == 1) {
		env_trans = *env_trans_array;
	} else {
		fprintf( stderr, "Syntax error: GR(1) specification is missing environment transition rules.\n" );
		return -1;
	}
	if (st_array_len > 1) {
		sys_trans = merge_ptrees( sys_trans_array, st_array_len, PT_AND );
	} else if (st_array_len == 1) {
		sys_trans = *sys_trans_array;
	} else {
		fprintf( stderr, "Syntax error: GR(1) specification is missing system transition rules.\n" );
		return -1;
	}

	/* Number of variables, before expansion of those that are nonboolean */
	original_num_env = tree_size( evar_list );
	original_num_sys = tree_size( svar_list );


	if (ptdump_flag) {
		tree_dot_dump( env_init, "env_init_ptree.dot" );
		tree_dot_dump( sys_init, "sys_init_ptree.dot" );
		tree_dot_dump( env_trans, "env_trans_ptree.dot" );
		tree_dot_dump( sys_trans, "sys_trans_ptree.dot" );

		if (num_egoals > 0) {
			for (i = 0; i < num_egoals; i++) {
				snprintf( dumpfilename, sizeof(dumpfilename),
						 "env_goal%05d_ptree.dot", i );
				tree_dot_dump( *(env_goals+i), dumpfilename );
			}
		}
		if (num_sgoals > 0) {
			for (i = 0; i < num_sgoals; i++) {
				snprintf( dumpfilename, sizeof(dumpfilename),
						 "sys_goal%05d_ptree.dot", i );
				tree_dot_dump( *(sys_goals+i), dumpfilename );
			}
		}

		var_index = 0;
		printf( "Environment variables (indices; domains): " );
		if (evar_list == NULL) {
			printf( "(none)" );
		} else {
			tmppt = evar_list;
			while (tmppt) {
				if (tmppt->value == 0) {  /* Boolean */
					if (tmppt->left == NULL) {
						printf( "%s (%d; bool)", tmppt->name, var_index );
					} else {
						printf( "%s (%d; bool), ", tmppt->name, var_index);
					}
				} else {
					if (tmppt->left == NULL) {
						printf( "%s (%d; {0..%d})", tmppt->name, var_index, tmppt->value );
					} else {
						printf( "%s (%d; {0..%d}), ", tmppt->name, var_index, tmppt->value );
					}
				}
				tmppt = tmppt->left;
				var_index++;
			}
		}
		printf( "\n\n" );

		printf( "System variables (indices; domains): " );
		if (svar_list == NULL) {
			printf( "(none)" );
		} else {
			tmppt = svar_list;
			while (tmppt) {
				if (tmppt->value == 0) {  /* Boolean */
					if (tmppt->left == NULL) {
						printf( "%s (%d; bool)", tmppt->name, var_index );
					} else {
						printf( "%s (%d; bool), ", tmppt->name, var_index );
					}
				} else {
					if (tmppt->left == NULL) {
						printf( "%s (%d; {0..%d})", tmppt->name, var_index, tmppt->value );
					} else {
						printf( "%s (%d; {0..%d}), ", tmppt->name, var_index, tmppt->value );
					}
				}
				tmppt = tmppt->left;
				var_index++;
			}
		}
		printf( "\n\n" );

		print_GR1_spec( evar_list, svar_list, env_init, sys_init, env_trans, sys_trans,
						env_goals, num_egoals, sys_goals, num_sgoals, stdout );
	}

	if (expand_nonbool_GR1( evar_list, svar_list, &env_init, &sys_init,
							&env_trans, &sys_trans,
							&env_goals, num_egoals, &sys_goals, num_sgoals,
							verbose ) < 0)
		return -1;
	nonbool_var_list = expand_nonbool_variables( &evar_list, &svar_list, verbose );

	if (verbose > 1)
		/* Dump the spec to show results of conversion (if any). */
		print_GR1_spec( evar_list, svar_list, env_init, sys_init, env_trans, sys_trans,
						env_goals, num_egoals, sys_goals, num_sgoals, NULL );


	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	manager = Cudd_Init( 2*(num_env+num_sys),
						 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
	Cudd_SetMaxCacheHard( manager, (unsigned int)-1 );
	Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );

	if (run_option == GR1C_MODE_INTERACTIVE) {

		i = levelset_interactive( manager, EXIST_SYS_INIT, stdin, stdout, verbose );
		if (i == 0) {
			printf( "Not realizable.\n" );
			return -1;
		} else if (i < 0) {
			printf( "Failure during interaction.\n" );
			return -1;
		}

		T = NULL;  /* To avoid seg faults by the generic clean-up code. */
	} else {

		T = check_realizable( manager, EXIST_SYS_INIT, verbose );
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
			strategy = synthesize( manager, EXIST_SYS_INIT, verbose );
			if (verbose)
				logprint( "Done." );
			if (strategy == NULL) {
				fprintf( stderr, "Error while attempting synthesis.\n" );
				return -1;
			}

		}
	}

	if (strategy != NULL) {  /* De-expand nonboolean variables */
		tmppt = nonbool_var_list;
		while (tmppt) {
			aut_compact_nonbool( strategy, evar_list, svar_list, tmppt->name, tmppt->value );
			tmppt = tmppt->left;
		}

		num_env = tree_size( evar_list );
		num_sys = tree_size( svar_list );
	}

	if (strategy != NULL) {
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

		if (verbose)
			logprint( "Dumping automaton of size %d...", aut_size( strategy ) );

		if (format_option == OUTPUT_FORMAT_TEXT) {
			list_aut_dump( strategy, num_env+num_sys, fp );
		} else if (format_option == OUTPUT_FORMAT_DOT) {
			if (nonbool_var_list != NULL) {
				dot_aut_dump( strategy, evar_list, svar_list,
							  DOT_AUT_ATTRIB, fp );
			} else {
				dot_aut_dump( strategy, evar_list, svar_list,
							  DOT_AUT_BINARY | DOT_AUT_ATTRIB, fp );
			}
		} else if (format_option == OUTPUT_FORMAT_AUT) {
			aut_aut_dump( strategy, num_env+num_sys, fp );
		} else if (format_option == OUTPUT_FORMAT_TULIP0) {
			tulip0_aut_dump( strategy, evar_list, svar_list, fp );
		} else { /* OUTPUT_FORMAT_TULIP */
			tulip_aut_dump( strategy, evar_list, svar_list, fp );
		}

		if (fp != stdout)
			fclose( fp );
	}

	/* Clean-up */
	delete_tree( evar_list );
	delete_tree( svar_list );
	delete_tree( env_init );
	delete_tree( sys_init );
	delete_tree( env_trans );
	delete_tree( sys_trans );
	for (i = 0; i < num_egoals; i++)
		delete_tree( *(env_goals+i) );
	if (num_egoals > 0)
		free( env_goals );
	for (i = 0; i < num_sgoals; i++)
		delete_tree( *(sys_goals+i) );
	if (num_sgoals > 0)
		free( sys_goals );
	if (T != NULL)
		Cudd_RecursiveDeref( manager, T );
	if (strategy)
		delete_aut( strategy );
	if (verbose)
		logprint( "Cudd_CheckZeroRef -> %d", Cudd_CheckZeroRef( manager ) );
	Cudd_Quit(manager);
	if (logging_flag)
		closelogfile();

    /* Return 0 if realizable, -1 if not realizable. */
	if (run_option == GR1C_MODE_INTERACTIVE || T != NULL) {
		return 0;
	} else {
		return -1;
	}

	return 0;
}
