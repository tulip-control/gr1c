/** \file solve.h
 * \brief Compute realizability and a strategy for a GR(1) game.
 *
 *
 * SCL; 2012, 2014-2015
 */


#ifndef SOLVE_H
#define SOLVE_H

#include "common.h"
#include "ptree.h"
#include "automaton.h"

/* Flags concerning initial conditions. (Consult comments for check_realizable.) */
#define UNDEFINED_INIT 0
#define ALL_ENV_EXIST_SYS_INIT 1
#define ALL_INIT 2
#define ONE_SIDE_INIT 3

#define LOGPRINT_INIT_FLAGS(X) \
    if ((X) == ALL_ENV_EXIST_SYS_INIT) { \
        logprint_raw( "ALL_ENV_EXIST_SYS_INIT" ); \
    } else if ((X) == ALL_INIT) { \
        logprint_raw( "ALL_INIT" ); \
    } else if ((X) == ONE_SIDE_INIT) {    \
        logprint_raw( "ONE_SIDE_INIT" ); \
    } else if ((X) == UNDEFINED_INIT) {    \
        logprint_raw( "UNDEFINED_INIT (not interpreted)" ); \
    } else { \
        logprint_raw( "(unrecognized)" ); \
    }


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

   init_flags can be one of

     - ALL_ENV_EXIST_SYS_INIT : realizable if for each possible
                       environment initialization, there exists a
                       system initialization such that the
                       corresponding state is in the winning set.
                       Empty ...INIT:; clauses are equivalent to
                       ...INIT:True;

     - ALL_INIT :      realizable if every state satisfying both
                       environment and system initial conditions is in
                       the winning set.  Empty ...INIT:; clauses are
                       equivalent to ...INIT:True;

     - ONE_SIDE_INIT : At most one of ENVINIT or SYSINIT can be
                       nonempty.  If ENVINIT is nonempty, and let
                       ENVINIT: f;, then f is an arbitrary state
                       formula in terms of environment and system
                       variables and the system strategy must allow
                       for any initial state that satisfies f.  If
                       SYSINIT is nonempty, and let SYSINIT: f;, then
                       f is an arbitrary state formula in terms of
                       environment and system variables and the system
                       strategy must find a strategy for some state
                       that satisfies f.  If both of the ...INIT:;
                       clauses are empty, then it is equivalent to
                       having ENVINIT:True;

   Let ENVINIT: f; and SYSINIT: g;, where it may be that f or g is the
   empty string.  If init_flags is ALL_ENV_EXIST_SYS_INIT, then f must
   only be in terms of ENV variables, and g only in terms of SYS
   variables.  For other init_flags settings, f and g are not over
   constrained variable sets.  However, a common convention for
   init_flags of ALL_INIT is to only have ENV variables in f, etc.,
   like for ALL_ENV_EXIST_SYS_INIT.
*/
DdNode *check_realizable( DdManager *manager, unsigned char init_flags,
                          unsigned char verbose );

/** Synthesize a strategy.  The specification is assumed to be
   realizable when this function is invoked.  Return pointer to
   automaton representing the strategy, or NULL if error. Also read
   documentation for check_realizable(). */
anode_t *synthesize( DdManager *manager, unsigned char init_flags,
                     unsigned char verbose );

/** Compute the set of states that are winning for the system, under
   the specification defined by the global parse trees (generated from
   gr1c input in main()). Basically creates BDDs from parse trees and
   then calls compute_winning_set_BDD(). */
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
   winning states, e.g., as returned by compute_winning_set().
   num_sublevels is an int array of length equal to the number of
   system goals.  Space for it is allocated by compute_sublevel_sets()
   and expected to be freed by the caller (or elsewhere).

   The argument X_ijr should be a reference to a pointer. Upon
   successful termination it contains (pointers to) the X fixed point
   sets computed for each Y_ij sublevel set. For each Y_ij sublevel
   set, the number of X sets is equal to the number of environment
   goals. */
DdNode ***compute_sublevel_sets( DdManager *manager,
                                 DdNode *W,
                                 DdNode *etrans, DdNode *strans,
                                 DdNode **egoals, int num_env_goals,
                                 DdNode **sgoals, int num_sys_goals,
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
