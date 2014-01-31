/* autman.c -- main entry point for a small "gr1c automaton" manipulator
 *
 * Try invoking it with "-h"...
 *
 *
 * SCL; 2014.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "logging.h"
#include "automaton.h"
#include "ptree.h"


/* Runtime modes */
#define AUTMAN_SYNTAX 1

int main( int argc, char **argv )
{
	int i, j;
	int in_filename_index = -1;
	FILE *in_fp = NULL;
	anode_t *head;
	int version;
	int state_len = -1;

	unsigned char verbose = 0;
	bool logging_flag = False;
	int run_option = AUTMAN_SYNTAX;

	/* Look for flags in command-line arguments. */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'h') {
				printf( "Usage: %s [-hVvls] [FILE]\n\n"
						"If no input file is given, or if FILE is -, read from stdin.  If no action\n"
						"is requested, then assume -s.\n\n"
						"  -h          this help message\n"
						"  -V          print version and exit\n"
						"  -v          be verbose; use -vv to be more verbose\n"
						"  -l          enable logging\n"
						"  -s          check syntax and get version;\n"
						"              return version number, or -1 if error.\n"
						"  -L N        declare that state vector size is N\n",
/*						"              note that not all gr1c aut versions require this.\n", */
						argv[0] );
				return 1;
			} else if (argv[i][1] == 'V') {
				printf( "autman (automaton file manipulator, distributed with"
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
			} else if (argv[i][1] == 's') {
				run_option = AUTMAN_SYNTAX;
			} else if (argv[i][1] == 'L') {
				if (i == argc-1) {
					fprintf( stderr, "Invalid flag given. Try \"-h\".\n" );
					return 1;
				}
				state_len = strtol( argv[i+1], NULL, 10 );
				i++;
			}
		} else {
			in_filename_index = i;
		}
	}

	if (state_len < 1) {
		fprintf( stderr, "State vector size must be at least 1.  Try \"-h\".\n" );
		return -1;
	}

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

	switch (run_option) {
	case AUTMAN_SYNTAX:
		head = aut_aut_loadver( state_len, in_fp, &version );
		if (head == NULL)
			return -1;
		if (verbose)
			logprint( "Detected format version %d.", version );
		return version;

	default:
		fprintf( stderr, "Unrecognized run option.  Try \"-h\".\n" );
		return -1;
	}

	return 0;
}
