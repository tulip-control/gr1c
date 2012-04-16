/* SCL; Apr 2012. */

#include <stdlib.h>
#include <stdio.h>
#include "automaton.h"

#define ERRPRINT(msg) fprintf( stderr, "============================================================\nERROR: " msg "\n\n" )
#define ERRPRINT1(msg, arg) fprintf( stderr, "============================================================\nERROR: " msg "\n\n", arg )


int main( int argc, char **argv )
{
	anode_t *head;

	/* Insertion */
	head = insert_anode( NULL, -1, NULL, 0 );
	if (head == NULL) {
		ERRPRINT( "node insertion into empty automaton failed." );
		abort();
	}

	/* Size computation */
	if (aut_size( head ) != 1) {
		ERRPRINT1( "size 1 automaton detected as having size %d", aut_size( head ) );
		abort();
	}

	delete_aut( head );
	head = NULL;

	return 0;
}
