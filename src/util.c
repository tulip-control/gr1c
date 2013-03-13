/* util.c -- Small generic routines.
 *
 *
 * SCL; 2012, 2013.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"


/* least significant bit first; unsigned. */
int bitvec_to_int( bool *vec, int vec_len )
{
	int i;
	int result = 0;
	for (i = 0; i < vec_len; i++) {
		if (*(vec+i))
			result += (1 << i);
	}
	return result;
}


/* least significant bit first; unsigned.  Up to given length vec_len;
   if vec_len is not sufficiently large, then the result is
   effectively a truncated form of the full bitvector for the given
   integer.  The caller is assumed to free the bitvector. */
bool *int_to_bitvec( int x, int vec_len )
{
	int i;
	bool *vec;
	if (vec_len < 1)
		return NULL;
	vec = malloc( vec_len*sizeof(bool) );
	if (vec == NULL) {
		perror( "int_to_bitvec, malloc" );
		return NULL;
	}
	for (i = 0; i < vec_len; i++) {
		if (x & (1 << i)) {
			*(vec+i) = 1;
		} else {
			*(vec+i) = 0;
		}
	}
	return vec;
}
