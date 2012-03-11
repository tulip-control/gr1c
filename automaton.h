/* automaton.h -- Routines for working with a strategy, as a finite automaton.
 *
 * Note that the length of the state vector in each node is not stored
 * anywhere in this data structure.  It is assumed to be positive and
 * constant for a particular automaton (strategy).
 *
 *
 * SCL; Mar 2012.
 */


#ifndef _AUTOMATON_H_
#define _AUTOMATON_H_

#include <stdio.h>

#include "common.h"
#include "ptree.h"


typedef struct anode_t
{
	bool *state;
	int mode;
	struct anode_t **trans;  /* Array of transitions */
	int trans_len;
	
	struct anode_t *next;
} anode_t;


anode_t *insert_anode( anode_t *head, int mode, bool *state, int state_len );
/* Insert node at the front of the given node list.  If given head is
   NULL, a new list will be created.

   Return new head on success, NULL on error. */

anode_t *pop_anode( anode_t *head );

anode_t *build_anode_trans( anode_t *head, int mode, bool *state, int state_len,
							int next_mode, bool **next_states, int next_len );
/* Build the transition array for the node with given state and mode.

   next_states is an array of state vectors, with length next_len,
   used to build transitions for this node. All of these states have
   mode next_mode.

   If the base node already has a transition array, then it is not
   replaced until the new array has been successfully built. (That is,
   if error, the original node should be unaffected.)

   Return given head on success, NULL if one of the needed nodes is
   not found. */

anode_t *append_anode_trans( anode_t *head,
							 int mode, bool *state, int state_len,
							 int next_mode, bool *next_state );
/* Append transition to array for the node with given state and mode. */

anode_t *find_anode( anode_t *head, int mode, bool *state, int state_len );
/* Return pointer to node with given state and mode, or NULL if not found. */

int find_anode_index( anode_t *head, int mode, bool *state, int state_len );
/* Return the position of the node with given state and mode, or -1 if
   not found.  0-based indexing. */

anode_t *delete_anode( anode_t *head, anode_t *target );
/* Return (possibly new) head pointer.  Transition arrays are not
   altered by this function. */

void replace_anode_trans( anode_t *head, anode_t *old, anode_t *new );
/* Replace all occurrences of "old" with "new" in transition arrays.
   If "new" is NULL, then the transitions into "old" are deleted and
   all dependent transition array lengths are decremented. */

anode_t *aut_prune_deadends( anode_t *head );
/* Return (possibly new) head pointer. */

int tulip_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list, FILE *fp );
/* Dump tulipcon XML file describing the automaton (strategy).
   Variable names are obtained from evar_list and svar_list, in which
   the combined order is assumed to match that of the state vector in
   each automaton node.

   If fp = NULL, then write to stdout.  Return nonzero if error. */

void list_aut_dump( anode_t *head, int state_len, FILE *fp );
/* Dump list of nodes; mostly useful for debugging.
   If fp = NULL, then write to stdout. */

int aut_size( anode_t *head );
void delete_aut( anode_t *head );


#endif
