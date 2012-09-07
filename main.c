/* main.c -- main entry point for execution.
 *
 * Command-line arguments are processed by hand.  Eventually switch to
 * getopt, once sophistication of usage demands.
 *
 *
 * SCL; 2012.
 */


#include <math.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "logging.h"
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


extern int compute_minmax( DdManager *manager, unsigned char verbose );


int main( int argc, char **argv )
{
	FILE *fp;
	FILE *stdin_backup = NULL;
	byte run_option = GR1C_MODE_SYNTHESIS;
	bool help_flag = False;
	bool ptdump_flag = False;
	bool logging_flag = False;
	byte format_option = OUTPUT_FORMAT_TULIP;
	unsigned char verbose = 0;
	int input_index = -1;
	int edges_input_index = -1;  /* If patching, command-line flag "-e". */
	int aut_input_index = -1;  /* For command-line flag "-a". */
	int output_file_index = -1;  /* For command-line flag "-o". */
	FILE *strategy_fp;
	char dumpfilename[64];

	int i, var_index, maxval, val;
	ptree_t *tmppt;  /* General purpose temporary ptree pointer */
	ptree_t *prevpt, *expt, *var_separator;
	ptree_t *nonbool_var_list = NULL;
	int maxbitval;

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

	if (edges_input_index >= 0 && aut_input_index < 0) {
		fprintf( stderr, "\"-e\" flag can only be used with \"-a\"\n" );
		return 1;
	}

	if (help_flag) {
		printf( "Usage: %s [-hVvlspri] [-t TYPE] [-aeo FILE] [FILE]\n\n"
				"  -h        this help message\n"
				"  -V        print version and exit\n"
				"  -v        be verbose\n"
				"  -l        enable logging\n"
				"  -t TYPE   strategy output format; default is \"tulip\";\n"
				"            supported formats: txt, tulip, dot, aut\n"
				"  -s        only check specification syntax (return -1 on error)\n"
				"  -p        dump parse trees to DOT files, and echo formulas to screen\n"
				"  -r        only check realizability; do not synthesize strategy\n"
				"            (return 0 if realizable, -1 if not)\n"
				"  -i        interactive mode\n"
				"  -a FILE   automaton file in \"gr1c\" format;\n"
				"            if FILE is -, then read from stdin\n"
				"  -e FILE   patch, given game edge set change file; requires -a flag\n"
				"  -o FILE   output strategy to FILE, rather than stdout (default)\n", argv[0] );
		return 1;
	}

	if (input_index < 0 && (run_option == GR1C_MODE_INTERACTIVE || (run_option == GR1C_MODE_PATCH && !strncmp( argv[aut_input_index], "-", 1 )))) {
		printf( "Reading spec from stdin in interactive mode, or in some cases while patching,\nis not yet implemented.\n" );
		return 1;
	}

	if (logging_flag) {
		openlogfile( NULL );  /* Use default filename prefix */
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
	if (verbose)
		logprint( "Parsing input..." );
	if (yyparse())
		return -1;
	if (verbose)
		logprint( "Done." );
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

	/* Merge component safety (transition) formulas if not in patching
	   (or incremental) mode, or if DOT dumps of the parse trees were
	   requested. */
	if ((run_option != GR1C_MODE_PATCH) || ptdump_flag) {
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
	}


	/* Compute domains of every variable, and encode the result in the
	   "value" field.  The domain is of the form {0,1,...,n}, where n
	   is a positive integer; for Boolean variables the domain is {0,1}. */
	if (evar_list == NULL) {
		var_separator = NULL;
		evar_list = svar_list;  /* that this is the deterministic case
								   is indicated by var_separator = NULL. */
	} else {
		var_separator = get_list_item( evar_list, -1 );
		if (var_separator == NULL) {
			fprintf( stderr, "Error: get_list_item failed on environment variables list.\n" );
			return NULL;
		}
		var_separator->left = svar_list;
	}
	tmppt = evar_list;
	while (tmppt) {
		maxval = rmax_tree_value( env_init, tmppt->name );
		val = rmax_tree_value( sys_init, tmppt->name );
		if (val != -9999 && maxval != -9999) {
			maxval = (val > maxval) ? val : maxval;
		} else if (val != -9999) {
			maxval = val;
		}

		val = rmax_tree_value( env_trans, tmppt->name );
		if (val != -9999 && maxval != -9999) {
			maxval = (val > maxval) ? val : maxval;
		} else if (val != -9999) {
			maxval = val;
		}

		val = rmax_tree_value( sys_trans, tmppt->name );
		if (val != -9999 && maxval != -9999) {
			maxval = (val > maxval) ? val : maxval;
		} else if (val != -9999) {
			maxval = val;
		}

		for (i = 0; i < num_egoals; i++) {
			val = rmax_tree_value( *(env_goals+i), tmppt->name );
			if (val != -9999 && maxval != -9999) {
				maxval = (val > maxval) ? val : maxval;
			} else if (val != -9999) {
				maxval = val;
			}
		}

		for (i = 0; i < num_sgoals; i++) {
			val = rmax_tree_value( *(sys_goals+i), tmppt->name );
			if (val != -9999 && maxval != -9999) {
				maxval = (val > maxval) ? val : maxval;
			} else if (val != -9999) {
				maxval = val;
			}
		}

		if (maxval == -9999) {  /* ...must be Boolean */
			tmppt->value = 0;
		} else {
			tmppt->value = maxval;
		}

		tmppt = tmppt->left;
	}
	if (var_separator == NULL) {
		evar_list = NULL;
	} else {
		var_separator->left = NULL;
	}


	/* Handle "don't care" bits */
	tmppt = evar_list;
	while (tmppt) {
		maxbitval = (int)(pow( 2, ceil(log2( tmppt->value+1 )) ));
		if (maxbitval-1 > tmppt->value) {
			prevpt = env_trans;
			env_trans = init_ptree( PT_AND, NULL, 0 );
			env_trans->right = prevpt;
			env_trans->left = unreach_expanded_bool( tmppt->name, tmppt->value+1, maxbitval-1 );
		}
		tmppt = tmppt->left;
	}

	tmppt = svar_list;
	while (tmppt) {
		maxbitval = (int)(pow( 2, ceil(log2( tmppt->value+1 )) ));
		if (maxbitval-1 > tmppt->value) {
			prevpt = sys_trans;
			sys_trans = init_ptree( PT_AND, NULL, 0 );
			sys_trans->right = prevpt;
			sys_trans->left = unreach_expanded_bool( tmppt->name, tmppt->value+1, maxbitval-1 );
		}
		tmppt = tmppt->left;
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
		printf( "Environment variables (indices; domains): " );
		if (evar_list == NULL) {
			printf( "(none)" );
		} else {
			tmppt = evar_list;
			while (tmppt) {
				if (tmppt->left == NULL) {
					printf( "%s (%d; {0..%d})", tmppt->name, var_index, tmppt->value );
				} else {
					printf( "%s (%d; {0..%d}), ", tmppt->name, var_index, tmppt->value );
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
				if (tmppt->left == NULL) {
					printf( "%s (%d; {0..%d})", tmppt->name, var_index, tmppt->value );
				} else {
					printf( "%s (%d; {0..%d}), ", tmppt->name, var_index, tmppt->value );
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


	/* Make all variables Boolean. */
	if (evar_list == NULL) {
		var_separator = NULL;
		evar_list = svar_list;  /* that this is the deterministic case
								   is indicated by var_separator = NULL. */
	} else {
		var_separator = get_list_item( evar_list, -1 );
		if (var_separator == NULL) {
			fprintf( stderr, "Error: get_list_item failed on environment variables list.\n" );
			return NULL;
		}
		var_separator->left = svar_list;
	}
	tmppt = evar_list;
	while (tmppt) {
		if (tmppt->value > 0) {
			if (nonbool_var_list == NULL) {
				nonbool_var_list = init_ptree( PT_VARIABLE, tmppt->name, tmppt->value );
			} else {
				append_list_item( nonbool_var_list, PT_VARIABLE, tmppt->name, tmppt->value );
			}

			sys_init = expand_to_bool( sys_init, tmppt->name, tmppt->value );
			env_init = expand_to_bool( env_init, tmppt->name, tmppt->value );
			sys_trans = expand_to_bool( sys_trans, tmppt->name, tmppt->value );
			env_trans = expand_to_bool( env_trans, tmppt->name, tmppt->value );
			if (sys_init == NULL || env_init == NULL || sys_trans == NULL || env_trans == NULL) {
				fprintf( stderr, "Failed to convert non-Boolean variable to its Boolean equivalent." );
				return -1;
			}
			for (i = 0; i < num_egoals; i++) {
				*(env_goals+i) = expand_to_bool( *(env_goals+i), tmppt->name, tmppt->value );
				if (*(env_goals+i) == NULL) {
					fprintf( stderr, "Failed to convert non-Boolean variable to its Boolean equivalent." );
					return -1;
				}
			}
			for (i = 0; i < num_sgoals; i++) {
				*(sys_goals+i) = expand_to_bool( *(sys_goals+i), tmppt->name, tmppt->value );
				if (*(sys_goals+i) == NULL) {
					fprintf( stderr, "Failed to convert non-Boolean variable to its Boolean equivalent." );
					return -1;
				}
			}
		}
		tmppt = tmppt->left;
	}
	if (var_separator == NULL) {
		evar_list = NULL;
	} else {
		var_separator->left = NULL;
	}

	/* Finally, expand the variable list. */
	if (evar_list != NULL) {
		tmppt = evar_list;
		if (tmppt->value > 0) {  /* Handle special case of head node */
			expt = var_to_bool( tmppt->name, tmppt->value );
			evar_list = expt;
			prevpt = get_list_item( expt, -1 );
			prevpt->left = tmppt->left;
		}
		tmppt = tmppt->left;
		while (tmppt) {
			if (tmppt->value > 0) {
				expt = var_to_bool( tmppt->name, tmppt->value );
				prevpt->left = expt;
				prevpt = get_list_item( expt, -1 );
				prevpt->left = tmppt->left;
			} else {
				prevpt = tmppt;
			}
			tmppt = tmppt->left;
		}
	}

	if (svar_list != NULL) {
		tmppt = svar_list;
		if (tmppt->value > 0) {  /* Handle special case of head node */
			expt = var_to_bool( tmppt->name, tmppt->value );
			svar_list = expt;
			prevpt = get_list_item( expt, -1 );
			prevpt->left = tmppt->left;
		}
		tmppt = tmppt->left;
		while (tmppt) {
			if (tmppt->value > 0) {
				expt = var_to_bool( tmppt->name, tmppt->value );
				prevpt->left = expt;
				prevpt = get_list_item( expt, -1 );
				prevpt->left = tmppt->left;
			} else {
				prevpt = tmppt;
			}
			tmppt = tmppt->left;
		}
	}


	if (verbose) {
		/* Dump the spec to show results of conversion (if any). */
		logprint_startline();
		logprint_raw( "ENV:" );
		tmppt = evar_list;
		while (tmppt) {
			logprint_raw( " %s", tmppt->name );
			tmppt = tmppt->left;
		}
		logprint_raw( ";" );
		logprint_endline();

		logprint_startline();
		logprint_raw( "SYS:" );
		tmppt = svar_list;
		while (tmppt) {
			logprint_raw( " %s", tmppt->name );
			tmppt = tmppt->left;
		}
		logprint_raw( ";" );
		logprint_endline();

		logprint_startline();
		logprint_raw( "ENV INIT:  " );
		print_formula( env_init, getlogstream() );
		logprint_raw( ";" );
		logprint_endline();

		logprint_startline();
		logprint_raw( "SYS INIT:  " );
		print_formula( sys_init, getlogstream() );
		logprint_raw( ";" );
		logprint_endline();

		logprint_startline();
		logprint_raw( "ENV TRANS:  [] " );
		print_formula( env_trans, getlogstream() );
		logprint_raw( ";" );
		logprint_endline();

		logprint_startline();
		logprint_raw( "SYS TRANS:  [] " );
		print_formula( sys_trans, getlogstream() );
		logprint_raw( ";" );
		logprint_endline();

		logprint_startline();
		logprint_raw( "ENV GOALS:  " );
		if (num_egoals == 0) {
			logprint_raw( "(none)" );
		} else {
			logprint_raw( "[]<> " );
			print_formula( *env_goals, getlogstream() );
			for (i = 1; i < num_egoals; i++) {
				logprint_raw( " & []<> " );
				print_formula( *(env_goals+i), getlogstream() );
			}
		}
		logprint_raw( ";" );
		logprint_endline();

		logprint_startline();
		logprint_raw( "SYS GOALS:  " );
		if (num_sgoals == 0) {
			logprint_raw( "(none)" );
		} else {
			logprint_raw( "[]<> " );
			print_formula( *sys_goals, getlogstream() );
			for (i = 1; i < num_sgoals; i++) {
				logprint_raw( " & []<> " );
				print_formula( *(sys_goals+i), getlogstream() );
			}
		}
		logprint_raw( ";" );
		logprint_endline();
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
		if (!strncmp( argv[aut_input_index], "-", 1 )) {
				strategy_fp = stdin;
		} else {
			strategy_fp = fopen( argv[aut_input_index], "r" );
			if (strategy_fp == NULL) {
				perror( "gr1c, fopen" );
				return -1;
			}
		}

		if (verbose)
			logprint( "Patching given strategy..." );
		strategy = patch_localfixpoint( manager, strategy_fp, fp, verbose );
		fclose( fp );
		if (strategy_fp != stdin)
			fclose( strategy_fp );
		if (verbose)
			logprint( "Done." );
		if (strategy == NULL) {
			fprintf( stderr, "Failed to patch strategy.\n" );
			return -1;
		}
		
		T = NULL;  /* To avoid seg faults by the generic clean-up code. */
	} else {

		T = check_realizable( manager, EXIST_SYS_INIT, verbose );
		if (T != NULL && (run_option == GR1C_MODE_REALIZABLE)) {
			printf( "Realizable.\n" );
		} else if (run_option == GR1C_MODE_REALIZABLE) {
			printf( "Not realizable.\n" );
		}
		if (verbose) {
			if (T != NULL) {
				logprint( "Realizable." );
			} else {
				logprint( "Not realizable." );
			}
		}

		if (T != NULL) { /* Print measure data */
			compute_minmax( manager, verbose );
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

	if (strategy != NULL) {  /* De-expand nonboolean variables */
		tmppt = nonbool_var_list;
		while (tmppt) {
			aut_compact_nonbool( strategy, evar_list, svar_list, tmppt->name );
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

		if (format_option == OUTPUT_FORMAT_TEXT) {
			list_aut_dump( strategy, num_env+num_sys, fp );
		} else if (format_option == OUTPUT_FORMAT_DOT) {
			dot_aut_dump( strategy, evar_list, svar_list,
						  DOT_AUT_ATTRIB, fp );
		} else if (format_option == OUTPUT_FORMAT_AUT) {
			aut_aut_dump( strategy, num_env+num_sys, fp );
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
	if (run_option == GR1C_MODE_INTERACTIVE || run_option == GR1C_MODE_PATCH
		|| T != NULL) {
		return 0;
	} else {
		return -1;
	}

	return 0;
}
