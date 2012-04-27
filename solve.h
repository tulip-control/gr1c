/** \file solve.h
 * \brief Compute realizability and a strategy for a GR(1) game.
 *
 *
 * SCL; Feb-Apr 2012.
 */


#ifndef SOLVE_H
#define SOLVE_H

#include "cudd.h"

#include "common.h"
#include "ptree.h"
#include "automaton.h"

/* Flags concerning initial conditions. (See comments for check_realizable.) */
#define EXIST_SYS_INIT 1
#define ALL_SYS_INIT 2


/** If realizable, then returns (a pointer to) the characteristic
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

       ALL_SYS_INIT   : realizable if every state satisfying both
                        environment and system initial conditions is
                        in the winning set.
*/
DdNode *check_realizable( DdManager *manager, unsigned char init_flags,
						  unsigned char verbose );

/** Synthesize a strategy.  The specification is assumed to be
   realizable when this function is invoked.  Return pointer to
   automaton representing the strategy, or NULL if error. Also see
   documentation for check_realizable. */
anode_t *synthesize( DdManager *manager, unsigned char init_flags,
					 unsigned char verbose );

/** Compute the set of states that are winning for the system, under
   the specification defined by the global parse trees (generated from
   gr1c input in main()). Basically creates BDDs from parse trees and
   then calls compute_winning_set_BDD. */
DdNode *compute_winning_set( DdManager *manager, unsigned char verbose );

/** Compute the set of states that are winning for the system, under
   the specification, while not including initial conditions. The
   transition (safety) formulas are defined by the given environment
   and system BDDs (etrans and strans, respectively), and the
   environment and system goal formulas are defined by egoals and
   sgoals, respectively. */
DdNode *compute_winning_set_BDD( DdManager *manager,
								 DdNode *etrans, DdNode *strans,
								 DdNode **egoals, DdNode **sgoals,
								 unsigned char verbose );

/** W is assumed to be (the characteristic function of) the set of
   winning states, e.g., as returned by compute_winning_set.
   num_sublevels is an int array of length equal to the number of
   system goals.  Space for it is allocated by compute_sublevel_sets
   and expected to be freed by the caller (or elsewhere).

   The argument X_ijr should be a reference to a pointer. Upon
   successful termination it contains (pointers to) the X fixed point
   sets computed for each Y_ij sublevel set. For each Y_ij sublevel
   set, the number of X sets is equal to the number of environment
   goals. */
DdNode ***compute_sublevel_sets( DdManager *manager,
								 DdNode *W,
								 DdNode *etrans, DdNode *strans,
								 DdNode **egoals, DdNode **sgoals,
								 int **num_sublevels,
								 DdNode *****X_ijr,
								 unsigned char verbose );

/** Read commands from input stream infp and write results to outfp.
   Return 1 on successful completion, 0 if specification unrealizable,
   and -1 if error. */
int levelset_interactive( DdManager *manager, unsigned char init_flags,
						  FILE *infp, FILE *outfp,
						  unsigned char verbose );


#endif
