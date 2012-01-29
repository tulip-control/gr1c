/* main.c -- main entry point for execution.
 *
 * Command-line arguments are processed by hand.  Eventually switch to
 * getopt, once sophistication of usage demands.
 *
 *
 * SCL; Jan 2012.
 */


#include <stdio.h>

#include "ptree.h"
extern int yyparse( void );


typedef unsigned char byte;

typedef unsigned char bool;
#define True 1
#define False 0


ptree_t *evar_list = NULL;
ptree_t *svar_list = NULL;


int main( int argc, char **argv )
{
	FILE *fd;
	bool help_flag = False;
	bool ptdump_flag = False;
	int i;
	int input_index = -1;

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
				"  -p    dump parse trees to DOT files, and echo textual trees to screen\n", argv[0] );
		return 1;
	}

	/* If filename for specification given at command-line, then use
	   it.  Else, read from stdin. */
	if (input_index > 0) {
		fd = fopen( argv[input_index], "r" );
		if (fd == NULL) {
			perror( "gr1c, fopen" );
			return -1;
		}
		stdin = fd;
	}

	/* Initialization (preparing for results of parsing spec). */
	evar_list = NULL;
	svar_list = NULL;

	/* Parse the specification. */
	if (yyparse())
		return -1;

	if (ptdump_flag) {
		printf( "Number of environment variables: %d\n", tree_size( evar_list ) );
		inorder_print( evar_list );
		printf( "\nNumber of system variables: %d\n", tree_size( svar_list ) );
		inorder_print( svar_list );
	}

	/* Clean-up */
	delete_tree( evar_list );
	
	return 0;
}
