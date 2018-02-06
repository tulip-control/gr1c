/* Unit tests for aut_prune_deadends()
 *
 * The function is documented in automaton.h.
 *
 *
 * SCL; 2018.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "automaton.h"
#include "tests_common.h"


int main( int argc, char **argv )
{
    anode_t *head;
    anode_t *node;

    assert( aut_prune_deadends( NULL ) == NULL );

    head = insert_anode( NULL, 0, -1, True, NULL, 0 );
    assert( aut_prune_deadends( head ) == NULL );

    node = insert_anode( NULL, 0, -1, True, NULL, 0 );
    head = insert_anode( node, 0, -1, True, NULL, 0 );

    node->trans_len = 1;
    node->trans = malloc( sizeof(anode_t *) );
    assert( node->trans != NULL );
    *(node->trans) = head;

    head->trans_len = 1;
    head->trans = malloc( sizeof(anode_t *) );
    assert( head->trans != NULL );
    *(head->trans) = node;
    assert( aut_prune_deadends( head ) !=  NULL );

    head->trans_len = 0;
    free( head->trans );
    head->trans = NULL;
    assert( aut_prune_deadends( head ) ==  NULL );

    return 0;
}
