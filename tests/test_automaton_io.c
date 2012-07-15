/* Unit tests for input and output routines of automaton (strategy) objects.
 *
 * SCL; July 2012.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "tests/common.h"
#include "automaton.h"


/* Trivial reference; state length of 2. */
#define REF_GR1CAUT_TRIVIAL \
"0 1 0 0 2 1 2\n" \
"1 0 0 0 1 1 2\n" \
"2 1 1 1 1 1 0\n"

/* ...after modification */
#define REF_GR1CAUT_TRIVIAL_MOD \
"0 0 1 0 -1 1\n" \
"1 1 0 0 2 2 0\n" \
"2 0 0 0 1 2 3\n" \
"3 1 1 1 1 2 0\n"



#define STRING_MAXLEN 1024
int main( int argc, char **argv )
{
	char *result;
	FILE *fp;
	char filename[STRING_MAXLEN];
	char instr[STRING_MAXLEN];
	anode_t *head, *node, *out_node;
	bool state[2], next_state[2];

	strcpy( filename, "temp_automaton_io_dumpXXXXXX" );
	result = mktemp( filename );
	if (result == NULL) {
		perror( "test_automaton_io, mktemp" );
		abort();
	}
	fp = fopen( filename, "w+" );
	if (fp == NULL) {
		perror( "test_automaton_io, fopen" );
		abort();
	}
	fprintf( fp, REF_GR1CAUT_TRIVIAL );
	if (fseek( fp, 0, SEEK_SET )) {
		perror( "test_automaton_io, fseek" );
		abort();
	}

	/* Load in "gr1c automaton" format */
	head = aut_aut_load( 2, fp );
	if (head == NULL) {
		ERRPRINT( "Failed to read 3-node automaton in \"gr1c automaton\" format." );
		abort();
	}

	/* Check number of nodes, labels, and outgoing edges */
	if (aut_size( head ) != 3) {
		ERRPRINT1( "size 3 automaton detected as having size %d.", aut_size( head ) );
		abort();
	}
	
	state[0] = 1;
	state[1] = 0;
	node = find_anode( head, 0, state, 2 );
	if (node == NULL) {
		ERRPRINT2( "could not find expected node with state [%d %d].", state[0], state[1] );
		abort();
	}
	if (node->rgrad != 2) {
		ERRPRINT1( "node should have reachability gradient value of 2 but actually has %d", node->rgrad );
		abort();
	}
	
	state[0] = 0;
	state[1] = 0;
	out_node = find_anode( head, 0, state, 2 );
	if (out_node == NULL) {
		ERRPRINT2( "could not find expected node with state [%d %d].", state[0], state[1] );
		abort();
	}
	if (out_node->rgrad != 1) {
		ERRPRINT1( "node should have reachability gradient value of 2 but actually has %d", out_node->rgrad );
		abort();
	}
	
	if (node->trans_len != 2) {
		ERRPRINT1( "node should have 2 outgoing transitions but actually has %d.", node->trans_len );
		abort();
	}
	if (*(node->trans) != out_node && *(node->trans+1) != out_node) {
		ERRPRINT( "an edge is missing." );
		abort();
	}

	/* Modify the automaton and dump the result */
	*(node->trans+1) = node;
	state[0] = 0;
	state[1] = 1;
	head = insert_anode( head, 0, -1, state, 2 );
	if (head == NULL) {
		ERRPRINT( "failed to insert new node into automaton." );
		abort();
	}
	next_state[0] = 1;
	next_state[1] = 0;
	head = append_anode_trans( head, 0, state, 2,
							   0, next_state );
	if (head == NULL) {
		ERRPRINT( "failed to append transition to new node." );
		abort();
	}
	fclose( fp );
	fp = fopen( filename, "w+" );
	if (fp == NULL) {
		perror( "test_automaton_io, fopen" );
		abort();
	}
	aut_aut_dump( head, 2, fp );
	if (fseek( fp, 0, SEEK_SET )) {
		perror( "test_automaton_io, fseek" );
		abort();
	}

	/* NB, assumed width may cause problems if we start using Unicode. */
	if (fread( instr, sizeof(char), strlen(REF_GR1CAUT_TRIVIAL_MOD), fp ) < strlen(REF_GR1CAUT_TRIVIAL_MOD)) {
		ERRPRINT( "output of aut_aut_dump is too short." );
		abort();
	}
	if (!strncmp( instr, REF_GR1CAUT_TRIVIAL_MOD, strlen(REF_GR1CAUT_TRIVIAL_MOD) )) {
		ERRPRINT( "output of aut_aut_dump does not match expectation." );
		abort();
	}

	fclose( fp );
	if (remove( filename )) {
		perror( "test_automaton_io, remove" );
		abort();
	}

	return 0;
}
