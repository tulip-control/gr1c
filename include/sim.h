/** \file sim.h
 * \brief Simulation support
 *
 *
 * SCL; 2012, 2013.
 */


#ifndef SIM_H
#define SIM_H

#include "common.h"
#include "automaton.h"


/* Environment strategy is random walk (so permissibility depends on
   the specification, and is only with probability 1).

   Some core functions for working with strategy automata have changed
   recently, and sim_rhc() has not yet been carefully checked
   following those changes.  As such, sim_rhc() should be considered
   as possibly temporarily defunct. */
anode_t *sim_rhc( DdManager *manager, DdNode *W,
                  DdNode *etrans, DdNode *strans, DdNode **sgoals,
                  char *metric_vars, int horizon, vartype *init_state,
                  int num_it, unsigned char verbose );


#endif
