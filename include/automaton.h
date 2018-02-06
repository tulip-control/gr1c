/** \file automaton.h
 * \brief Routines for working with a strategy, as a finite automaton.
 *
 * Note that the length of the state vector in each node is not stored
 * anywhere in this data structure.  It is assumed to be positive and
 * constant for a particular automaton (strategy).
 *
 *
 * SCL; 2012-2015
 */


#ifndef AUTOMATON_H
#define AUTOMATON_H

#include <stdio.h>

#include "common.h"
#include "ptree.h"


/** \brief Strategy automaton nodes. */
typedef struct anode_t
{
    bool initial;  /**<\brief If True, then this node corresponds to a state
                      that satisfies an initial condition.  Note that it may be
                      False even when this node could be used for
                      initialization.  Possible interpretations of initial
                      conditions are described in the documentation for
                      check_realizable(). */
    vartype *state;
    int mode;  /**<\brief Goal mode; indicates which system goal is
                  currently being pursued. */
    int rgrad;  /**<\brief reach annotation value; unset value is
                   indicated by -1. */
    struct anode_t **trans;  /**<\brief Array of transitions */
    int trans_len;

    struct anode_t *next;
} anode_t;


/**
 * \defgroup DotDumpFlags format flags for dot_aut_dump
 *
 * \brief Flags to set format for output.  Combine non-conflicting
 *   flags with or.
 *
 * @{
 */
#define DOT_AUT_ALL 0  /**<\brief Show all variables with values. */
#define DOT_AUT_BINARY 1  /**<\brief Assume variables have Boolean
                             domains, and only label nodes with those
                             that are True. */
#define DOT_AUT_EDGEINPUT 2  /**<\brief Show environment variables on edges. */
#define DOT_AUT_ATTRIB 4  /**<\brief Show node attributes.

                             In addition to the state (presentation
                             depends on format_flags given to
                             dot_aut_dump()), each node is labeled with

                                 i;
                                 (m, r)

                             where i is the node ID, m the goal mode,
                             and r the reach annotation value. */
/**@}*/


/** Insert node at the front of the given node list.  If given head is
   NULL, a new list will be created.

   Return new head on success, NULL on error. */
anode_t *insert_anode( anode_t *head, int mode, int rgrad,
                       bool initial, vartype *state, int state_len );

/** Delete topmost (head) node from list.  Return pointer to new head. */
anode_t *pop_anode( anode_t *head );

/** Append transition to array for the first node with given state and mode.

   This function does not check for duplicate outgoing edges.

   \return head on success, NULL on error. */
anode_t *append_anode_trans( anode_t *head,
                             int mode, vartype *state, int state_len,
                             int next_mode, vartype *next_state );

/** Return pointer to the first node with given state and mode,
   or NULL if not found. */
anode_t *find_anode( anode_t *head, int mode, vartype *state, int state_len );

/** Return the position of the first node with given state and mode,
   or -1 if not found.  0-based indexing. */
int find_anode_index( anode_t *head, int mode, vartype *state, int state_len );

/** Return the position of the given node, or -1 if not found.
   0-based indexing. */
int anode_index( anode_t *head, anode_t *node );

/** Delete target node from strategy automaton.

   Note that any references to \p target in transition arrays of other nodes are
   not modified by this function. If you want to change or delete any such
   pointers to \p target, use replace_anode_trans().

   \param head pointer to the strategy automaton to be modified.

   \param target node to be deleted. The associated state vector and transition
   array are freed. If \p target is not found, then return value is NULL.

   \return (possibly new) head pointer. If \p head or \p target is NULL
   or if \p target is not found, then return NULL.
   If head=target and there is only one node, then return NULL (because the only
   node is deleted, there can be no head pointer). */
anode_t *delete_anode( anode_t *head, anode_t *target );

/** Delete nodes in U that are not reachable in the graph from outside
   U, and then repeat.  Nodes marked as initial are ignored; such
   nodes are supposed to satisfy initial conditions, i.e., from which
   execution is allowed to begin and hence do not need ingoing edges.

   The given array U is altered and freed during execution of
   forward_prune(), so the caller should not attempt to use it
   afterward.

   U may be redundant, i.e., the implementation is tolerant to U
   having multiple pointers to the same node.

   Return (possibly new) head pointer, or NULL on error. */
anode_t *forward_prune( anode_t *head, anode_t **U, int U_len );

/** Replace all occurrences of "old" with "new" in transition arrays.
   If "new" is NULL, then the transitions into "old" are deleted and
   all dependent transition array lengths are decremented. */
void replace_anode_trans( anode_t *head, anode_t *old, anode_t *new );

/** Return (possibly new) head pointer. Return NULL if entire automaton is
   deleted. If head == NULL, then return NULL. */
anode_t *aut_prune_deadends( anode_t *head );

/** Dump tulipcon XML file describing the automaton (strategy).
   Variable names are obtained from evar_list and svar_list, in which
   the combined order is assumed to match that of the state vector in
   each automaton node.

   For each node, the goal mode and reach annotation value are
   placed in a <anno> tag in that order.

   If fp = NULL, then write to stdout.  Return nonzero if error. */
int tulip_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
                    FILE *fp );

/** Dump DOT file describing the automaton (strategy).  The appearance can be
   configured using \ref DotDumpFlags.  Also read comments for
   tulip_aut_dump().  */
int dot_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
                  unsigned char format_flags, FILE *fp );

/** Dump list of nodes; mostly useful for debugging.
   If fp = NULL, then write to stdout.  The basic format is

       i [(init)] : S - m - r - [t0 t1 ...]

   where i is the node ID (used only as a means to uniquely refer to
   nodes), S is the state at that node as comma-separated values, m is
   the goal mode, r is the reach annotation value, and [t0 t1 ...] is
   the list of IDs of nodes reachable in one step.  The node ID is
   followed by "(init)" if the initial field is marked True.  */
void list_aut_dump( anode_t *head, int state_len, FILE *fp );

/** Dump strategy using the current version of the "gr1c automaton"
   file format.  Read [external_notes](md_formats.html) for details.
   If fp = NULL, then write to stdout. */
void aut_aut_dump( anode_t *head, int state_len, FILE *fp );

/** Dump strategy using the specified version of the "gr1c automaton"
   file format.  This function is wrapped by aut_aut_dump().  Return 0
   on success.  If the given version number is not supported, then
   return -1. */
int aut_aut_dumpver( anode_t *head, int state_len, FILE *fp, int version );

/** Load strategy given in "gr1c automaton" format from file fp.  Read
   [external_notes](md_formats.html) for details.  If fp = NULL, then
   read from stdin.  Return resulting head pointer, or NULL if error.
   If version is not NULL, then the detected format version number is
   placed in *version.

   Note that attempting to load a gr1c automaton file for a version
   that includes fields not present in this build of gr1c results in a
   warning message while all supported fields are used.  If expected
   fields are missing (e.g., no rgrad number is available), then the
   appropriate "unset" indicator is set to each such field; typically
   this is -1, check the definition of anode_t for details. */
anode_t *aut_aut_loadver( int state_len, FILE *fp, int *version );

/** Legacy wrapper for aut_aut_load().  Equivalent to calling
   aut_aut_loadver() with version == NULL */
anode_t *aut_aut_load( int state_len, FILE *fp );

/** Dump strategy using the current version of the gr1c-JSON file
   format.  Consult [external_notes](md_formats.html) for details. */
int json_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
                   FILE *fp );

/** Get number of nodes in given automaton. */
int aut_size( anode_t *head );

/** Delete entire automaton pointed to by given head.

   Essentially, traverses node list and frees them and their member
   data.  At completion, the given head pointer is no longer valid.
   Invoking with NULL pointer causes return with no error. */
void delete_aut( anode_t *head );


/** Compute forward reachable set from given node in automaton,
   restricting attention to nodes with state in N and goal mode of
   mode, and setting the mode field of each reached node to
   magic_mode.  Return zero on success, nonzero on error. */
int forward_modereach( anode_t *node, int mode, vartype **N, int N_len,
                       int magic_mode, int state_len );


/** Convert binary-expanded form of a variable back into nonboolean.
   The domain of the variable is [0,maxval], and to indicate this the
   value field is set to maxval in the resulting (merged) variable
   entry (in evar_list or svar_list).
   Returns the new state vector length, or -1 on error. */
int aut_compact_nonbool( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
                         char *name, int maxval );

/** Inverse operation of aut_compact_nonbool().
    Return zero on success, nonzero on error. */
int aut_expand_bool( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
                     ptree_t *nonbool_var_list );

/** Dump strategy as Spin Promela model.

   Assumptions:
     - no more than 10000 environment goals;
     - no more than 10000 system goals;
     - none of the variables has the following names: checketrans,
       checkstrans, pmlfault, envinit, envtrans, envgoal0, envgoal1, ...
       sysinit, systrans, sysgoal0, sysgoal1, ...

   An LTL formula that can be used to decide whether the strategy realizes the
   GR(1) specification is included in the comments near the top of the generated
   Spin Promela model.

   Example instructions for model checking are provided in doc/verification.md.

   If fp = NULL, then write to stdout.

   If fp != NULL, then formula_fp is the file in which to print the LTL formula,
   or stdout if formula_fp = NULL. Note that this is a copy of the LTL formula
   that is included in the Promela model, which is described above.

   If fp = formula_fp (i.e., the same file pointer), then do not print the LTL
   formula to formula_fp.

   Return nonzero if error. */
int spin_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
                   ptree_t *env_init, ptree_t *sys_init,
                   ptree_t **env_trans_array, int et_array_len,
                   ptree_t **sys_trans_array, int st_array_len,
                   ptree_t **env_goals, int num_env_goals,
                   ptree_t **sys_goals, int num_sys_goals,
                   FILE *fp, FILE *formula_fp );

#endif
