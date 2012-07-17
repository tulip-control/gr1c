/* \file solve_support.h
 * \brief Mostly internal routines for state vector manipulation.
 *
 *
 * SCL; 2012.
 */


#ifndef SOLVE_SUPPORT_H
#define SOLVE_SUPPORT_H

#include "cudd.h"
#include "common.h"


/* Read space-separated values from given string. Allocate space for
   the resulting array and store a pointer to it in argument
   "state". Read at most max_len values, and return the number of
   values read (thus, the length of the array pointed to by state at
   exit). */
int read_state_str( char *input, bool **state, int max_len );


/* Internal routines for setting typical cube arrays. */
void cube_env( int *cube, int num_env, int num_sys );
void cube_sys( int *cube, int num_env, int num_sys );
void cube_prime_env( int *cube, int num_env, int num_sys );
void cube_prime_sys( int *cube, int num_env, int num_sys );


/* CUDD generates cubes with "don't care" values when either 0 or 1 is
   possible. To allow for explicit enumeration, increment_cube will
   increment these values in the given cube using gcube as a
   reference. Overflow can occur (effectively clearing all "don't
   care" bits in cube). Call saturated_cube to test if this will
   happen on the next increment.

   For example,
   PRECONDITION: len = 4, gcube = [0,2,0,2], and cube = [0,0,0,1];
   POSTCONDITION: cube = [0,1,0,0]. */
void increment_cube( bool *cube, int *gcube, int len );

/* Will cube overflow upon next increment? (See documentation for
   increment_cube.) */
bool saturated_cube( bool *cube, int *gcube, int len );

void initialize_cube( bool *cube, int *gcube, int len );

/* Assume that full cube would include primed variables, thus all
   values in cube array at index len onward are set to 2. */
void state2cube( bool *state, int *cube, int len );

/* Construct cofactor of trans BDD from state vector to get possible
   next states (via cube generation). */
DdNode *state2cof( DdManager *manager, int *cube, int cube_len,
				   bool *state, DdNode *trans, int offset, int len );

/* Compute characteristic function for a single state. */
DdNode *state2BDD( DdManager *manager, bool *state, int offset, int len );


bool **get_env_moves( DdManager *manager, int *cube,
					  bool *state, DdNode *etrans,
					  int num_env, int num_sys, int *emoves_len );

/* Compute exists modal operator applied to set C. */
DdNode *compute_existsmodal( DdManager *manager, DdNode *C,
							 DdNode *etrans, DdNode *strans,
							 int num_env, int num_sys, int *cube );


#endif
