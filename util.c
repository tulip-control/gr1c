/* util.c -- Small generic routines.
 *
 *
 * SCL; 2012.
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
