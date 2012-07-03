/* main.c -- main entry point for execution.
 *
 * Command-line arguments are processed by hand.  Eventually switch to
 * getopt, once sophistication of usage demands.
 *
 *
 * SCL; 2012.
 */


#include <string.h>
#include <stdio.h>
#include "util.h"
#include "cudd.h"

#include "common.h"
#include "ptree.h"
#include "solve.h"
#include "patching.h"
#include "automaton.h"
extern int yyparse( void );


/**************************
 **** Global variables ****/

ptree_t *evar_list = NULL;
ptree_t *svar_list = NULL;
ptree_t *env_init = NULL;
ptree_t *sys_init = NULL;
ptree_t *env_trans = NULL;  /* Built from component parse trees in env_trans_array. */
ptree_t *sys_trans = NULL;
ptree_t **env_goals = NULL;
ptree_t **sys_goals = NULL;
int num_egoals = 0;
int num_sgoals = 0;

ptree_t **env_trans_array = NULL;
ptree_t **sys_trans_array = NULL;
int et_array_len = 0;
int st_array_len = 0;

/* General purpose tree pointer, which facilitates cleaner Yacc
   parsing code. */
ptree_t *gen_tree_ptr = NULL;

/**************************/


/* Output formats */
#define OUTPUT_FORMAT_TEXT 0
#define OUTPUT_FORMAT_TULIP 1
#define OUTPUT_FORMAT_DOT 2
#define OUTPUT_FORMAT_AUT 3

/* Runtime modes */
#define GR1C_MODE_SYNTAX 0
#define GR1C_MODE_REALIZABLE 1
#define GR1C_MODE_SYNTHESIS 2
#define GR1C_MODE_INTERACTIVE 3
#define GR1C_MODE_PATCH 4


int main( int argc, char **argv )
{
	FILE *fp;
	FILE *stdin_backup = NULL;
	byte run_option = GR1C_MODE_SYNTHESIS;
	bool help_flag = False;
	bool ptdump_flag = False;
	byte format_option = OUTPUT_FORMAT_TULIP;
	unsigned char verbose = 0;
	int input_index = -1;
	int edges_input_index = -1;  /* If patching, command-line flag "-e". */
	char dumpfilename[32];

	int i, var_index;
	ptree_t *tmppt;  /* General purpose temporary ptree pointer */

	DdManager *manager;
	DdNode *T = NULL;
	anode_t *strategy = NULL;
	int num_env, num_sys;

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
				} else {
					fprintf( stderr, "Unrecognized output format. Try \"-h\".\n" );
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
		printf( "Usage: %s [-hVvspri] [-t TYPE] [-e FILE] [FILE]\n\n"
				"  -h        this help message\n"
				"  -V        print version and exit\n"
				"  -v        be verbose\n"
				"  -t TYPE   strategy output format; default is \"tulip\";\n"
				"            supported formats: txt, tulip, dot, aut\n"
				"  -s        only check specification syntax (return -1 on error)\n"
				"  -p        dump parse trees to DOT files, and echo formulas to screen\n"
				"  -r        only check realizability; do not synthesize strategy\n"
				"            (return 0 if realizable, -1 if not)\n"
				"  -i        interactive mode\n"
				"  -e FILE   patch, given game edge set change file\n", argv[0] );
		return 1;
	}

	if (input_index < 0 && (run_option == GR1C_MODE_INTERACTIVE
							|| run_option == GR1C_MODE_PATCH)) {
		printf( "Reading spec from stdin in interactive or patching mode is not yet implemented.\n" );
		return 1;
	}

	/* If filename for specification given at command-line, then use
	   it.  Else, read from stdin. */
	if (input_index > 0) {
		fp = fopen( argv[input_index], "r" );
		if (fp == NULL) {
			perror( "gr1c, fopen" );
			return -1;
		}
		stdin_backup = stdin;
		stdin = fp;
	}

	/* Parse the specification. */
	evar_list = NULL;
	svar_list = NULL;
	gen_tree_ptr = NULL;
	if (verbose) {
		printf( "Parsing input..." );
		fflush( stdout );
	}
	if (yyparse())
		return -1;
	if (verbose) {
		printf( "Done.\n" );
		fflush( stdout );
	}
	if (stdin_backup != NULL) {
		stdin = stdin_backup;
	}

	if (run_option == GR1C_MODE_SYNTAX)
		return 0;

	/* Close input file, if opened. */
	if (input_index > 0)
		fclose( fp );

	/* Handle empty initial conditions, i.e., no restrictions. */
	if (env_init == NULL)
		env_init = init_ptree( PT_CONSTANT, NULL, 1 );
	if (sys_init == NULL)
		sys_init = init_ptree( PT_CONSTANT, NULL, 1 );

	/* Merge component safety (transition) formulas. */
	if (et_array_len > 1) {
		env_trans = merge_ptrees( env_trans_array, et_array_len, PT_AND );
	} else if (et_array_len == 1) {
		env_trans = *env_trans_array;
	} else {  /* No restrictions on transitions. */
		env_trans = init_ptree( PT_CONSTANT, NULL, 1 );
	}
	if (st_array_len > 1) {
		sys_trans = merge_ptrees( sys_trans_array, st_array_len, PT_AND );
	} else if (st_array_len == 1) {
		sys_trans = *sys_trans_array;
	} else {  /* No restrictions on transitions. */
		sys_trans = init_ptree( PT_CONSTANT, NULL, 1 );
	}

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
		printf( "Environment variables (indices): " );
		if (evar_list == NULL) {
			printf( "(none)" );
		} else {
			tmppt = evar_list;
			while (tmppt) {
				if (tmppt->left == NULL) {
					printf( "%s (%d)", tmppt->name, var_index );
				} else {
					printf( "%s (%d), ", tmppt->name, var_index );
				}
				tmppt = tmppt->left;
				var_index++;
			}
		}
		printf( "\n\n" );

		printf( "System variables (indices): " );
		if (svar_list == NULL) {
			printf( "(none)" );
		} else {
			tmppt = svar_list;
			while (tmppt) {
				if (tmppt->left == NULL) {
					printf( "%s (%d)", tmppt->name, var_index );
				} else {
					printf( "%s (%d), ", tmppt->name, var_index );
				}
				tmppt = tmppt->left;
				var_index++;
			}
		}
		printf( "\n\n" );

		printf( "ENV INIT:  " );
		print_formula( env_init, stdout );
		printf( "\n" );

		printf( "SYS INIT:  " );
		print_formula( sys_init, stdout );
		printf( "\n" );

		printf( "ENV TRANS:  [] " );
		print_formula( env_trans, stdout );
		printf( "\n" );

		printf( "SYS TRANS:  [] " );
		print_formula( sys_trans, stdout );
		printf( "\n" );

		printf( "ENV GOALS:  " );
		if (num_egoals == 0) {
			printf( "(none)" );
		} else {
			printf( "[]<> " );
			print_formula( *env_goals, stdout );
			for (i = 1; i < num_egoals; i++) {
				printf( " & []<> " );
				print_formula( *(env_goals+i), stdout );
			}
		}
		printf( "\n" );

		printf( "SYS GOALS:  " );
		if (num_sgoals == 0) {
			printf( "(none)" );
		} else {
			printf( "[]<> " );
			print_formula( *sys_goals, stdout );
			for (i = 1; i < num_sgoals; i++) {
				printf( " & []<> " );
				print_formula( *(sys_goals+i), stdout );
			}
		}
		printf( "\n" );
	}

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	manager = Cudd_Init( 2*(num_env+num_sys),
						 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
	Cudd_SetMaxCacheHard( manager, (unsigned int)-1 );
	Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );

	if (run_option == GR1C_MODE_PATCH) {
		fp = fopen( argv[edges_input_index], "r" );
		if (fp == NULL) {
			perror( "gr1c, fopen" );
			return -1;
		}

		if (verbose) {
			printf( "Patching given strategy..." );
			fflush( stdout );
		}
		strategy = patch_localfixpoint( manager, NULL, fp, verbose );
		fclose( fp );
		if (verbose) {
			printf( "Done.\n" );
			fflush( stdout );
		}
		if (strategy == NULL) {
			fprintf( stderr, "Failed to patch strategy.\n" );
			return -1;
		}
		
		T = NULL;  /* To avoid seg faults by the generic clean-up code. */
	} else {

		T = check_realizable( manager, EXIST_SYS_INIT, verbose );
		if (T != NULL && (run_option == GR1C_MODE_REALIZABLE || verbose)) {
			printf( "Realizable.\n" );
		} else if (run_option == GR1C_MODE_REALIZABLE || verbose) {
			printf( "Not realizable.\n" );
		}

		if (run_option == GR1C_MODE_SYNTHESIS && T != NULL) {

			if (verbose) {
				printf( "Synthesizing a strategy..." );
				fflush( stdout );
			}
			strategy = synthesize( manager, EXIST_SYS_INIT, verbose );
			if (verbose) {
				printf( "Done.\n" );
				fflush( stdout );
			}
			if (strategy == NULL) {
				fprintf( stderr, "Error while attempting synthesis.\n" );
				return -1;
			}

		} else if (run_option == GR1C_MODE_INTERACTIVE && T != NULL) {

			Cudd_RecursiveDeref( manager, T );
			T = NULL;

			i = levelset_interactive( manager, EXIST_SYS_INIT, stdin, stdout, verbose );
			if (i == 0) {
				printf( "Not realizable.\n" );
				return -1;
			} else if (i < 0) {
				printf( "Failure during interaction.\n" );
				return -1;
			}

		}
	}

	if (strategy != NULL) {
		/* strategy = aut_prune_deadends( strategy ); */
		if (format_option == OUTPUT_FORMAT_TEXT) {
			list_aut_dump( strategy, num_env+num_sys, stdout );
		} else if (format_option == OUTPUT_FORMAT_DOT) {
			dot_aut_dump( strategy, evar_list, svar_list,
						  DOT_AUT_BINARY | DOT_AUT_ATTRIB, stdout );
		} else if (format_option == OUTPUT_FORMAT_AUT) {
			aut_aut_dump( strategy, num_env+num_sys, stdout );
		} else { /* OUTPUT_FORMAT_TULIP */
			tulip_aut_dump( strategy, evar_list, svar_list, stdout );
		}
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
		printf( "Cudd_CheckZeroRef -> %d\n", Cudd_CheckZeroRef( manager ) );
	Cudd_Quit(manager);

    /* Return 0 if realizable, -1 if not realizable. */
	if (run_option == GR1C_MODE_INTERACTIVE || run_option == GR1C_MODE_PATCH
		|| T != NULL) {
		return 0;
	} else {
		return -1;
	}

	return 0;
}
