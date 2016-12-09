/* Unit tests for functions that implement aspects of bitblasting.
 *
 * SCL; 2016
 */

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>

#include "ptree.h"
#include "tests_common.h"


int main( int argc, char **argv )
{
    ptree_t *head = NULL;

    head = var_to_bool( "x", 2 );
    if (head == NULL) {
        ERRPRINT( "var_to_bool() unexpectedly returned NULL (error)." );
        abort();
    }

    delete_tree( head );
    head = NULL;

    return 0;
}
