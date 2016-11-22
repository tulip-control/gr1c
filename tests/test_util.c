/* Unit tests for small utility routines.
 *
 * SCL; 2016
 */

#include <stdlib.h>

#include "common.h"
#include "tests_common.h"
#include "gr1c_util.h"


int main( int argc, char **argv )
{
    vartype bv[] = {0,1,0,1};

    if (bitvec_to_int( bv, 4 ) != 0xA) {
        ERRPRINT( "bitvec_to_int( [0,1,0,1], 4) != 0xA" );
        abort();
    }

    return 0;
}
