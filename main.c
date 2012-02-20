/* main.c -- main entry point for execution.
 *
 * Command-line arguments are processed by hand.  Eventually switch to
 * getopt, once sophistication of usage demands.
 *
 *
 * SCL; Jan, Feb 2012.
 */


#include <stdio.h>
#include "util.h"
#include "cudd.h"

#include "ptree.h"
extern int yyparse( void );


typedef unsigned char byte;

typedef unsigned char bool;
#define True 1
#define False 0


/**************************
 **** Global variables ****/

ptree_t *evar_list = NULL;
ptree_t *svar_list = NULL;
ptree_t *env_init = NULL;
ptree_t *sys_init = NULL;

/* General purpose tree pointer, which facilitates cleaner Yacc
   parsing code. */
ptree_t *gen_tree_ptr = NULL;

/**************************/


int main( int argc, char **argv )
{
	FILE *fp;
	bool help_flag = False;
	bool ptdump_flag = False;
	int input_index = -1;

	int i, var_index;
	ptree_t *tmppt;  /* General purpose temporary ptree pointer */

	DdManager *manager;
	DdNode *fn, *tmp;
	ptree_t *var_separator;
	int ddin[3] = {1, 0, 1};

	/* Look for flags in command-line arguments. */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'h') {
				help_flag = True;
			} else if (argv[i][1] == 'p') {
				ptdump_flag = True;
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

	if (argc > 3 || help_flag) {
		printf( "Usage: %s [-hp] [FILE]\n\n"
				"  -h    help message\n"
				"  -p    dump parse trees to DOT files, and echo formulas to screen\n", argv[0] );
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
		stdin = fp;
	}

	/* Parse the specification. */
	evar_list = NULL;
	svar_list = NULL;
	gen_tree_ptr = NULL;
	if (yyparse())
		return -1;

	/* Close input file, if opened. */
	if (input_index > 0)
		fclose( fp );

	if (ptdump_flag) {
		tree_dot_dump( env_init, "env_init_ptree.dot" );
		tree_dot_dump( sys_init, "sys_init_ptree.dot" );

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
	}

	/* Build BDD for sys init, and play with it to learn CUDD. */
	manager = Cudd_Init( tree_size( evar_list )+tree_size( svar_list ),
						 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );

	if (evar_list == NULL) {
		var_separator = NULL;
	} else {
		var_separator = get_list_item( evar_list, -1 );
		if (var_separator == NULL) {
			fprintf( stderr, "Error: get_list_item failed on environment variables list.\n" );
			return -1;
		}
		var_separator->left = svar_list;
	}
	
	if (evar_list == NULL) {  /* Handle deterministic case */
		fn = ptree_BDD( sys_init, svar_list, manager );
	} else {
		fn = ptree_BDD( sys_init, evar_list, manager );
	}
	
	/* Break the link that appended the system variables list to the
	   environment variables list. */
	if (evar_list != NULL)
		var_separator->left = NULL;

	Cudd_PrintDebug( manager, fn, 1, 3 );
	tmp = Cudd_Eval( manager, fn, ddin );
	printf( "Given input (%d, %d, %d),\nOutput: %.2f",
			ddin[0], ddin[1], ddin[2],
			(tmp->type).value );

	/* Clean-up */
	delete_tree( evar_list );
	delete_tree( svar_list );
	delete_tree( sys_init );
	delete_tree( env_init );
	Cudd_Quit(manager);
	
	return 0;
}
