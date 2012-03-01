/* solve.h -- Compute realizability and a strategy for a GR(1) game.
 *
 *
 * SCL; Feb 2012.
 */


#ifndef _SOLVE_H_
#define _SOLVE_H_

#include "cudd.h"

#include "ptree.h"

/* Flags concerning initial conditions. (See comments for check_realizable.) */
#define EXIST_SYS_INIT 1
#define ALL_SYS_INIT 2


DdNode *check_realizable( DdManager *manager, unsigned char init_flags,
						  unsigned char verbose );
/* If realizable, then returns (a pointer to) the characteristic
   function of the winning set.  Otherwise (if problem is not
   realizable), returns NULL.  Given manager must already be
   initialized.  To account for all variables and their primed
   ("next") forms, a reasonable initialization can be achieved with

     manager = Cudd_Init( 2*(tree_size( evar_list )+tree_size( svar_list )),
                          0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );

   If an error occurs, then a message is printed to stderr and NULL is
   returned.  Depending on when and what provoked the error, there may
   be memory leaks.

   init_flags can be

       EXIST_SYS_INIT : realizable if for each possible environment
                        initialization, there exists a system
                        initialization such that the corresponding
                        state is in the winning set.

       ALL_SYS_INIT    : realizable if every state satisfying both
                         environment and system initial conditions is
                         in the winning set.
*/


#endif
