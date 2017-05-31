/* Unit tests for aut_aut_dump()
 *
 * SCL; 2017
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>

#include "common.h"
#include "automaton.h"


int main(void)
{
    anode_t *head = NULL;
    const int state_len = 3;
    vartype state[] = {0, 0, 0};
    int j;

    head = insert_anode( NULL, 0, -1, True, state, state_len );
    aut_aut_dump( head, state_len, NULL );

    return 0;
}
