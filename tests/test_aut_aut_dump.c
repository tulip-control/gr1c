/* Unit tests for aut_aut_dump()
 *
 * SCL; 2017
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "tests_common.h"
#include "automaton.h"


#define REF_GR1CAUT_SINGLE_LOOP \
"1\n" \
"0 0 0 0 1 0 -1 0\n"

#define REF_GR1CAUT_SINGLE_LOOP_V0 \
"0\n" \
"0 0 0 0 0 -1 0\n"

#define REF_GR1CAUT_SINGLE_LOOP_AND_SUCC \
"1\n" \
"0 1 0 0 1 0 -1 1\n" \
"1 0 0 0 1 0 -1 1\n"

#define REF_GR1CAUT_SINGLE_LOOP_AND_2SUCC \
"1\n" \
"0 0 1 0 1 0 -1 0 2\n" \
"1 1 0 0 1 0 -1 2\n" \
"2 0 0 0 1 0 -1 2\n"


#define STRING_MAXLEN 2048
int main(void)
{
    int fd;
    FILE *fp;
    char filename[STRING_MAXLEN];
    char instr[STRING_MAXLEN];
    anode_t *head = NULL;
    anode_t *this_node, *that_node;
    const int state_len = 3;
    vartype state[] = {0, 0, 0};

    head = insert_anode( NULL, 0, -1, True, state, state_len );
    head->trans = malloc( sizeof(anode_t *) );
    assert( head->trans );
    head->trans_len = 1;
    *(head->trans) = head;

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

    assert( !aut_aut_dumpver( head, state_len, fp, 1 ) );

    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ", fseek" );
        abort();
    }
    /* NB, assumed width may cause problems if we start using Unicode. */
    if (fread( instr, sizeof(char), strlen(REF_GR1CAUT_SINGLE_LOOP)+1, fp )
        < strlen(REF_GR1CAUT_SINGLE_LOOP)) {
        ERRPRINT( "output of aut_aut_dump is too short." );
        abort();
    }
    if (strncmp( instr, REF_GR1CAUT_SINGLE_LOOP,
                 strlen(REF_GR1CAUT_SINGLE_LOOP) )) {
        ERRPRINT( "output of aut_aut_dump does not match expectation" );
        ERRPRINT1( "%s", instr  );
        ERRPRINT1( "%s", REF_GR1CAUT_SINGLE_LOOP );
        abort();
    }

    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ", fseek" );
        abort();
    }
    assert( !aut_aut_dumpver( head, state_len, fp, 0 ) );

    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ", fseek" );
        abort();
    }
    /* NB, assumed width may cause problems if we start using Unicode. */
    if (fread( instr, sizeof(char), strlen(REF_GR1CAUT_SINGLE_LOOP_V0)+1, fp )
        < strlen(REF_GR1CAUT_SINGLE_LOOP_V0)) {
        ERRPRINT( "output of aut_aut_dump is too short." );
        abort();
    }
    if (strncmp( instr, REF_GR1CAUT_SINGLE_LOOP_V0,
                 strlen(REF_GR1CAUT_SINGLE_LOOP_V0) )) {
        ERRPRINT( "output of aut_aut_dump does not match expectation" );
        ERRPRINT1( "%s", instr  );
        ERRPRINT1( "%s", REF_GR1CAUT_SINGLE_LOOP_V0 );
        abort();
    }

    state[0] = 1;
    head = insert_anode( head, 0, -1, True, state, state_len );
    assert( head != NULL );

    this_node = find_anode( head, 0, state, state_len );
    assert( this_node != NULL );

    state[0] = 0;
    that_node = find_anode( head, 0, state, state_len );
    assert( that_node != NULL );
    assert( this_node != that_node );

    this_node->trans = malloc( sizeof(anode_t *) );
    assert( this_node->trans );
    this_node->trans_len = 1;
    *(this_node->trans) = that_node;

    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ", fseek" );
        abort();
    }

    assert( !aut_aut_dumpver( head, state_len, fp, 1 ) );

    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ", fseek" );
        abort();
    }

    /* NB, assumed width may cause problems if we start using Unicode. */
    if (fread( instr, sizeof(char), strlen(REF_GR1CAUT_SINGLE_LOOP_AND_SUCC)+1, fp )
        < strlen(REF_GR1CAUT_SINGLE_LOOP_AND_SUCC)) {
        ERRPRINT( "output of aut_aut_dump is too short." );
        ERRPRINT1( "%s", instr  );
        ERRPRINT1( "%s", REF_GR1CAUT_SINGLE_LOOP_AND_SUCC );
        abort();
    }
    if (strncmp( instr, REF_GR1CAUT_SINGLE_LOOP_AND_SUCC,
                 strlen(REF_GR1CAUT_SINGLE_LOOP_AND_SUCC) )) {
        ERRPRINT( "output of aut_aut_dump does not match expectation" );
        ERRPRINT1( "%s", instr  );
        ERRPRINT1( "%s", REF_GR1CAUT_SINGLE_LOOP_AND_SUCC );
        abort();
    }

    state[0] = 0;
    state[1] = 1;
    state[2] = 0;
    head = insert_anode( head, 0, -1, True, state, state_len );
    assert( head != NULL );

    this_node = find_anode( head, 0, state, state_len );
    assert( this_node != NULL );

    state[1] = 0;
    that_node = find_anode( head, 0, state, state_len );
    assert( that_node != NULL );

    this_node->trans = malloc( 2*sizeof(anode_t *) );
    assert( this_node->trans );
    this_node->trans_len = 2;
    *(this_node->trans) = this_node;
    *(this_node->trans+1) = that_node;

    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ", fseek" );
        abort();
    }
    if (ftruncate( fd, 0 )) {
        perror( __FILE__ ", ftruncate" );
        abort();
    }

    assert( !aut_aut_dumpver( head, state_len, fp, 1 ) );

    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ", fseek" );
        abort();
    }

    /* NB, assumed width may cause problems if we start using Unicode. */
    if (fread( instr, sizeof(char), strlen(REF_GR1CAUT_SINGLE_LOOP_AND_2SUCC), fp )
        < strlen(REF_GR1CAUT_SINGLE_LOOP_AND_2SUCC)) {
        ERRPRINT( "output of aut_aut_dump is too short." );
        ERRPRINT1( "%s", instr  );
        ERRPRINT1( "%s", REF_GR1CAUT_SINGLE_LOOP_AND_2SUCC );
        abort();
    }
    if (strncmp( instr, REF_GR1CAUT_SINGLE_LOOP_AND_2SUCC,
                 strlen(REF_GR1CAUT_SINGLE_LOOP_AND_2SUCC) )) {
        ERRPRINT( "output of aut_aut_dump does not match expectation" );
        ERRPRINT1( "%s", instr  );
        ERRPRINT1( "%s", REF_GR1CAUT_SINGLE_LOOP_AND_2SUCC );
        abort();
    }

    fclose( fp );
    if (remove( filename )) {
        perror( __FILE__ ", remove" );
        abort();
    }

    return 0;
}
