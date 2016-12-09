/* Unit tests for functions that implement aspects of bitblasting.
 *
 * SCL; 2016
 */

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>

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
    if (tree_size( head ) != 2) {
        ERRPRINT1( "var_to_bool() returned list of %d variables, "
                   "rather than list of length 2.",
                   tree_size( head ) );
        abort();
    }
    if (strncmp( "x0", (get_list_item( head, 0 ))->name, 2 )
        || strncmp( "x1", (get_list_item( head, 1 ))->name, 2 )) {
        ERRPRINT( "unexpected bit-index variable name from var_to_bool()" );
        abort();
    }

    delete_tree( head );
    head = NULL;

    return 0;
}
