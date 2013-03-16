/** \file gr1c_util.h
 * \brief Small generic routines not otherwise sorted.
 *
 *
 * SCL; 2013.
 */


#ifndef GR1C_UTIL_H
#define GR1C_UTIL_H

#include "common.h"


/** least significant bit first; unsigned. */
int bitvec_to_int( bool *vec, int vec_len );

/** least significant bit first; unsigned.  Up to given length vec_len;
   if vec_len is not sufficiently large, then the result is
   effectively a truncated form of the full bitvector for the given
   integer.  The caller is assumed to free the bitvector. */
bool *int_to_bitvec( int x, int vec_len );


#endif
