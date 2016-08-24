/** \file patching.h
 * \brief Implementation of patching and incremental synthesis algorithms.
 *
 *
 * SCL; 2012-2015
 */


#ifndef PATCHING_H
#define PATCHING_H

#include "common.h"
#include "automaton.h"


/** Implementation of algorithm from [[LPJM13]](md_papers.html#LPJM13)

     S.C. Livingston, P. Prabhakar, A.B. Jose, R.M. Murray.
     Patching task-level robot controllers based on a local
     mu-calculus formula. presented at ICRA in May 2013.
     (The original Caltech CDS technical report is
      http://resolver.caltech.edu/CaltechCDSTR:2012.003)

   Read given strategy in "gr1c automaton" format from stream
   strategy_fp, using edge set changes to the game graph listed in
   stream change_fp.  Read [external_notes](md_formats.html) for format
   details.  If strategy_fp = NULL, then read from stdin.

   Return the head pointer of the patched strategy, or NULL if error.

   As usual, this function will not try to close given streams when
   finished. */
anode_t *patch_localfixpoint( DdManager *manager,
                              FILE *strategy_fp, FILE *change_fp,
                              int original_num_env, int original_num_sys,
                              ptree_t *nonbool_var_list, int *offw,
                              unsigned char verbose );

/** Solve a reachability game symbolically, by blocking an environment
   goal or reaching Exit from Entry.  States are restricted to the set
   N (given as a characteristic function named N_BDD). */
anode_t *synthesize_reachgame( DdManager *manager, int num_env, int num_sys,
                               anode_t **Entry, int Entry_len,
                               anode_t **Exit, int Exit_len,
                               DdNode *etrans, DdNode *strans, DdNode **egoals,
                               DdNode *N_BDD, unsigned char verbose );

/** Implementation of algorithm from [[LM14]](md_papers.html#LM14) for adding sys goals

     S.C. Livingston, R.M. Murray.
     Hot-swapping robot task goals in reactive formal synthesis.
     presented at CDC in Dec 2014.
     Extended version available as Caltech CDS tech report at
     http://resolver.caltech.edu/CaltechCDSTR:2014.001

   new_sysgoal is assumed to have had nonboolean variables expanded.
   If no metric variables are given (i.e., num_metric_vars = 0 or offw
   is NULL), then the new system goal is appended at the end of the
   existing goal visitation sequence.

   Return the head pointer of the augmented strategy, or NULL if error. */
anode_t *add_metric_sysgoal( DdManager *manager, FILE *strategy_fp,
                             int original_num_env, int original_num_sys,
                             int *offw, int num_metric_vars,
                             ptree_t *new_sysgoal, unsigned char verbose );

/** Implementation of algorithm from [[LM14]](md_papers.html#LM14) for removing sys goals

     S.C. Livingston, R.M. Murray.
     Hot-swapping robot task goals in reactive formal synthesis.
     presented at CDC in Dec 2014.
     Extended version available as Caltech CDS tech report at
     http://resolver.caltech.edu/CaltechCDSTR:2014.001

   The given strategy is assumed to have the following form.  System
   goals are sought in order, so that upon reaching a psi_i-state, the
   next system goal sought is psi_{i+1}, where subscript arithmetic is
   modulo n, the number of system goals.  We say that goal i is sought
   from node n if n->mode = i (treating n as having type anode_t*).
   Thus, upon deleting nodes with "goal mode" (part of "reach
   annotation") i, it suffices to label the new substrategy with "goal
   mode" (i+1) mod n.

   delete_i is the index of the system goal (also known as "goal
   mode") that is to be removed.  The "index" is determined by order
   in the SYSGOAL section of the given specification.

   Two special cases are not currently treated: given strategies in
   which there is less than three system goals or in which nodes
   providing initial conditions have goal mode corresponding to
   delete_i or delete_i+1 mod n.

   Return the head pointer of the modified strategy, or NULL if error or if no
   operation performed because the call was vacuous (e.g., no nodes to delete
   for requested mode). */
anode_t *rm_sysgoal( DdManager *manager, FILE *strategy_fp,
                     int original_num_env, int original_num_sys,
                     int delete_i, unsigned char verbose );

#endif
