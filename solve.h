/* solve.h -- Compute feasibility and a strategy for a GR(1) game.
 *
 *
 * SCL; Feb 2012.
 */


#ifndef _SOLVE_H_
#define _SOLVE_H_

#include "cudd.h"

#include "ptree.h"


DdNode *check_feasible( DdManager *manager );
/* If feasible, then returns (a pointer to) the characteristic
   function of the winning set.  Otherwise (if problem is not
   feasible), returns NULL.  Given manager must already be
   initialized.  To account for all variables and their primed
   ("next") forms, a reasonable initialization can be achieved with

     manager = Cudd_Init( 2*(tree_size( evar_list )+tree_size( svar_list )),
                          0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );

   If an error occurs, then a message is printed to stderr and NULL is
   returned.  Depending on when and what provoked the error, there may
   be memory leaks. */


#endif
