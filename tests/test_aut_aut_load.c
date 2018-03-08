/* Unit tests for aut_aut_load()
 *
 * SCL; 2012-2015
 */

#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "common.h"
#include "tests_common.h"
#include "automaton.h"


/* Trivial reference; state length of 2. */
#define REF_GR1CAUT_TRIVIAL \
"1\n" \
"0 1 0 1 0 2 1 2\n" \
"1 0 0 1 0 1 1 2\n" \
"2 1 1 1 1 1 1 0\n"

/* ...after modification */
#define REF_GR1CAUT_TRIVIAL_MOD \
"1\n" \
"0 0 1 0 0 -1 1\n"  \
"1 1 0 1 0 2 2 1\n" \
"2 0 0 1 0 1 2 3\n" \
"3 1 1 1 1 1 2 1\n"

/* Trivial reference in gr1caut v0 format; state length of 2. */
#define REF_GR1CAUT_TRIVIAL_V0 \
"0 1 0 0 2 1 2\n" \
"1 0 0 0 1 1 2\n" \
"2 1 1 1 1 1 0\n"


#define STRING_MAXLEN 2048
anode_t *aut_aut_loads( char *autstr, int state_len )
{
    int fd;
    FILE *fp;
    char filename[STRING_MAXLEN];
    anode_t *head;

    assert( autstr != NULL );
    assert( state_len > 0 );

    strcpy( filename, "dumpXXXXXX" );
    fd = mkstemp( filename );
    if (fd == -1) {
        perror( __FILE__ ", mkstemp" );
        abort();
    }
    fp = fdopen( fd, "w+" );
    if (fp == NULL) {
        perror( __FILE__ ", fdopen" );
        abort();
    }
    fprintf( fp, autstr );
    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ", fseek" );
        abort();
    }

    /* Load in "gr1c automaton" format */
    head = aut_aut_load( state_len, fp );
    if (head == NULL) {
        ERRPRINT( "Failed to read automaton "
                  "in \"gr1c automaton\" format." );
        abort();
    }

    fclose( fp );
    if (remove( filename )) {
        perror( __FILE__ ", remove" );
        abort();
    }

    return head;
}


int main( int argc, char **argv )
{
    int fd;
    FILE *fp;
    char filename[STRING_MAXLEN];
    char instr[STRING_MAXLEN];
    anode_t *head, *node, *out_node;
    vartype state[2], next_state[2];

    head = aut_aut_loads( REF_GR1CAUT_TRIVIAL, 2 );

    if (aut_size( head ) != 3) {
        ERRPRINT1( "size 3 automaton detected as having size %d.",
                   aut_size( head ) );
        abort();
    }

    state[0] = 1;
    state[1] = 0;
    node = find_anode( head, 0, state, 2 );
    if (node == NULL) {
        ERRPRINT2( "could not find expected node with state [%d %d].",
                   state[0], state[1] );
        abort();
    }
    if (node->rgrad != 2) {
        ERRPRINT1( "node should have reach annotation value of 2 "
                   "but actually has %d",
                   node->rgrad );
        abort();
    }
    if (node->mode != 0) {
        ERRPRINT1( "node should have mode value of 0 "
                   "but actually has %d",
                   node->mode );
        abort();
    }

    state[0] = 0;
    state[1] = 0;
    out_node = find_anode( head, 0, state, 2 );
    if (out_node == NULL) {
        ERRPRINT2( "could not find expected node with state [%d %d].",
                   state[0], state[1] );
        abort();
    }
    if (out_node->rgrad != 1) {
        ERRPRINT1( "node should have reach annotation value of 1 "
                   "but actually has %d",
                   out_node->rgrad );
        abort();
    }
    if (out_node->mode != 0) {
        ERRPRINT1( "node should have mode value of 0 "
                   "but actually has %d",
                   out_node->mode );
        abort();
    }

    if (node->trans_len != 2) {
        ERRPRINT1( "node should have 2 outgoing transitions "
                   "but actually has %d.",
                   node->trans_len );
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
    head = insert_anode( head, 0, -1, False, state, 2 );
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

    strcpy( filename, "dumpXXXXXX" );
    fd = mkstemp( filename );
    if (fd == -1) {
        perror( __FILE__ ", mkstemp" );
        abort();
    }
    fp = fdopen( fd, "w+" );
    if (fp == NULL) {
        perror( __FILE__ ", fdopen" );
        abort();
    }

    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ", fseek" );
        abort();
    }
    if (ftruncate( fd, 0 )) {
        perror( __FILE__ ", ftruncate" );
        abort();
    }
    aut_aut_dumpver( head, 2, fp, 1 );
    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ", fseek" );
        abort();
    }

    /* NB, assumed width may cause problems if we start using Unicode. */
    if (fread( instr, sizeof(char), strlen(REF_GR1CAUT_TRIVIAL_MOD), fp )
        < strlen(REF_GR1CAUT_TRIVIAL_MOD)) {
        ERRPRINT( "output of aut_aut_dump is too short." );
        abort();
    }
    if (strncmp( instr, REF_GR1CAUT_TRIVIAL_MOD,
                 strlen(REF_GR1CAUT_TRIVIAL_MOD) )) {
        ERRPRINT( "output of aut_aut_dump does not match expectation." );
        ERRPRINT1( "%s", instr  );
        ERRPRINT1( "%s", REF_GR1CAUT_TRIVIAL_MOD );
        abort();
    }

    fclose( fp );
    if (remove( filename )) {
        perror( __FILE__ ", remove" );
        abort();
    }

    delete_aut( head );
    head = NULL;

    head = aut_aut_loads( REF_GR1CAUT_TRIVIAL_V0, 2 );
    assert( aut_size( head ) == 3 );
    delete_aut( head );
    head = NULL;

    return 0;
}
