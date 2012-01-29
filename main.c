/* SCL; Jan 2012. */


#include <stdio.h>

extern int yyparse( void );


int main( int argc, char **argv )
{
	FILE *fd;

	if (argc > 2) {
		printf( "Usage: %s [FILE]\n", argv[0] );
		return 1;
	} else if (argc == 2) {
		fd = fopen( argv[1], "r" );
		if (fd == NULL) {
			perror( "syncheck, fopen" );
			return -1;
		}
		stdin = fd;
	}

	return yyparse();
}
