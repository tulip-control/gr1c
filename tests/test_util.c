/* Unit tests for small utility routines.
 *
 * SCL; 2016
 */

#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "tests_common.h"
#include "gr1c_util.h"


void print_arr( FILE *fp, vartype *arr, int len )
{
    int k;
    assert( len > 0 );
    fprintf( fp, "[" );
    for (k = 0; k < len; k++) {
        fprintf( fp, "%d", arr[k] );
        if (k < len-1)
            fprintf( fp, "," );
    }
    fprintf( fp, "]" );
}


void check_bitvec_to_int( vartype *bv, int len, int expected )
{
    if (bitvec_to_int( bv, len ) != expected) {
        ERRPRINT( "Unexpected return value of bitvec_to_int" );
        fprintf( stderr, "bitvec_to_int( " );
        print_arr( stderr, bv, len );
        fprintf( stderr, ", %d ) != 0x%X\n\n", len, expected );
        abort();
    }
}


void test_bitvec_to_int()
{
    vartype bv_A[] = {0,1,0,1};
    vartype bv_5[] = {1,0,1,0};
    vartype bv_0[] = {0,0,0,0};
    vartype bv_F[] = {1,1,1,1};
    vartype bv_5F[] = {1,1,1,1,1,0,1,0};

    check_bitvec_to_int( bv_A, 4, 0xA );
    check_bitvec_to_int( bv_5, 4, 0x5 );
    check_bitvec_to_int( bv_0, 4, 0x0 );
    check_bitvec_to_int( bv_F, 4, 0xF );
    check_bitvec_to_int( bv_5F, 8, 0x5F );
}


void check_int_to_bitvec( int x, int vec_len )
{
    int k;  /* counter */
    int actual = 0;
    vartype *bv = int_to_bitvec( x, vec_len );
    for (k = 0; k < vec_len; k++) {
        if (*(bv+k) != 0)
            actual += 1 << k;
    }

    if (actual != x) {
        ERRPRINT( "Unexpected return value of int_to_bitvec" );
        fprintf( stderr, "int_to_bitvec( %d, %d ) returned ", x, vec_len );
        print_arr( stderr, bv, vec_len );
        fprintf( stderr, "\n" );
        abort();
    }

    free( bv );
}


void test_int_to_bitvec()
{
    check_int_to_bitvec( 0, 1 );
    check_int_to_bitvec( 0xA, 4 );

    /* Check error conditions */
    if (int_to_bitvec( 2, 0 ) != NULL) {
        ERRPRINT( "Unexpected return value of int_to_bitvec" );
        fprintf( stderr, "int_to_bitvec( 2, 0 ) did not return NULL.\n" );
        abort();
    }
}


int main( int argc, char **argv )
{
    test_bitvec_to_int();
    test_int_to_bitvec();

    return 0;
}
