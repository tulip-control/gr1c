/** \file automaton.h
 * \brief Routines for working with a strategy, as a finite automaton.
 *
 * A node is uniquely determined by its state and goal mode.  Thus
 * other attributes such as reachability index are not required when
 * invoking some functions.  Note that the length of the state vector
 * in each node is not stored anywhere in this data structure.  It is
 * assumed to be positive and constant for a particular automaton
 * (strategy).
 *
 *
 * SCL; Mar 2012.
 */


#ifndef _AUTOMATON_H_
#define _AUTOMATON_H_

#include <stdio.h>

#include "common.h"
#include "ptree.h"


/** \brief Data structure for strategy automaton nodes. */
typedef struct anode_t
{
	bool *state;
	int mode;  /**< Goal mode; indicates which system goal is currently being pursued. */
	int rindex;  /**< Reachability index; least number of steps in
				     which reaching the goal set can be guaranteed.
				     Unset value is indicated by -1. */
	struct anode_t **trans;  /**< Array of transitions */
	int trans_len;
	
	struct anode_t *next;
} anode_t;


/* Flags to set format for output.  Combine non-conflicting flags with or. */
#define DOT_AUT_ALL 0
#define DOT_AUT_BINARY 1


/** Insert node at the front of the given node list.  If given head is
   NULL, a new list will be created.

   Return new head on success, NULL on error. */
anode_t *insert_anode( anode_t *head, int mode, int rindex, bool *state, int state_len );

anode_t *pop_anode( anode_t *head );

/** Build the transition array for the node with given state and mode.

   next_states is an array of state vectors, with length next_len,
   used to build transitions for this node. All of these states have
   mode next_mode.

   If the base node already has a transition array, then it is not
   replaced until the new array has been successfully built. (That is,
   if error, the original node should be unaffected.)

   Return given head on success, NULL if one of the needed nodes is
   not found. */
anode_t *build_anode_trans( anode_t *head, int mode, bool *state, int state_len,
							int next_mode, bool **next_states, int next_len );

/** Append transition to array for the node with given state and mode. */
anode_t *append_anode_trans( anode_t *head,
							 int mode, bool *state, int state_len,
							 int next_mode, bool *next_state );

/** Return pointer to node with given state and mode, or NULL if not found. */
anode_t *find_anode( anode_t *head, int mode, bool *state, int state_len );

/** Return the position of the node with given state and mode, or -1 if
   not found.  0-based indexing. */
int find_anode_index( anode_t *head, int mode, bool *state, int state_len );

/** Return (possibly new) head pointer.  Transition arrays are not
   altered by this function. */
anode_t *delete_anode( anode_t *head, anode_t *target );

/** Replace all occurrences of "old" with "new" in transition arrays.
   If "new" is NULL, then the transitions into "old" are deleted and
   all dependent transition array lengths are decremented. */
void replace_anode_trans( anode_t *head, anode_t *old, anode_t *new );

/** Return (possibly new) head pointer. */
anode_t *aut_prune_deadends( anode_t *head );

/** Dump tulipcon XML file describing the automaton (strategy).
   Variable names are obtained from evar_list and svar_list, in which
   the combined order is assumed to match that of the state vector in
   each automaton node.

   For each node, the goal mode and reachability index are placed in a
   <name> tag in that order.

   If fp = NULL, then write to stdout.  Return nonzero if error. */

int tulip_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list, FILE *fp );

/** Dump DOT file describing the automaton (strategy).  See comments
   for tulip_aut_dump.  In addition to the state (presentation depends
   on given format_flags), each node is labeled with

       i;
       (m, r)

   where i is the node ID, m the goal mode, and r the reachability
   index. */
int dot_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
				  unsigned char format_flags, FILE *fp );

/** Dump list of nodes; mostly useful for debugging.
   If fp = NULL, then write to stdout.  The basic format is

       i : S - m - r - [t0 t1 ...]

   where i is the node ID (used only as a means to uniquely refer to
   nodes), S is the state (as a bitvector) at that node, m is the goal
   mode, r is the reachability index, and [t0 t1 ...] is the list of
   IDs of nodes reachable in one step. */
void list_aut_dump( anode_t *head, int state_len, FILE *fp );

int aut_size( anode_t *head );
void delete_aut( anode_t *head );


#endif
