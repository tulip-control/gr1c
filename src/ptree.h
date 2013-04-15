/** \file ptree.h 
 * \brief Routines for working with a GR(1) formula parse tree.
 *
 * To avoid defining another basic data structure, we may effectively
 * obtain a linked list by making a binary tree with no right children
 * (or more generally, a tree in which each node has at most one
 * child).  Functions intended specifically to support this use case
 * include "list" in their names.
 *
 * For functions concerning tree nodes, any arguments that are not
 * applicable to the given type are ignored.
 *
 *
 * SCL; 2012, 2013.
 */


#ifndef PTREE_H
#define PTREE_H

#include <stdio.h>

#include "common.h"


/**
 * \defgroup PTreeNodeTypes parse tree node types
 *
 * @{
 */
/* variables or constant */
#define PT_EMPTY 0
#define PT_VARIABLE 1
#define PT_NEXT_VARIABLE 2
#define PT_CONSTANT 3

/* unary */
#define PT_NEG 4

/* binary */
#define PT_AND 5
#define PT_OR 6
#define PT_IMPLIES 7
#define PT_EQUALS 8
#define PT_EQUIV 9
#define PT_LT 10  /* less than, i.e., "<" */
#define PT_GT 11  /* greater than, i.e., ">" */
#define PT_LE 12  /* less than or equal to, i.e., "<=" */
#define PT_GE 13  /* greater than or equal to, i.e., ">=" */
#define PT_NOTEQ 14  /* not equals */
/**@}*/

/** \brief Parse tree nodes. */
typedef struct ptree_t
{
	int type;  /**<\brief See table of \ref PTreeNodeTypes. */
	char *name;  /**<\brief Name of the variable, if applicable. */
	int value;  /**<\brief Value of a constant.  If of type boolean, then 0
				   means "false", and 1 "true". */
	
	struct ptree_t *left;
	struct ptree_t *right;
} ptree_t;


/** Create root node with given type.  Return NULL on error. */
ptree_t *init_ptree( int type, char *name, int value );

/** Return number of nodes in tree. */
int tree_size( ptree_t *head );

/** If f is NULL, then use stdout. */
void print_node( ptree_t *node, FILE *fp );

/** Traverse the tree in-order, calling *node_fn at each node and
   passing it arg. */
void inorder_trav( ptree_t *head,
				   void (* node_fn)(ptree_t *, void *), void *arg );

/** If f is NULL, then use stdout. */
void print_formula( ptree_t *head, FILE *fp );

/** Create a new tree in which all of the given trees are included by
   the specified binary operator.  len is the length of heads.  Return
   pointer to the new tree root, or NULL on error.  N.B., the given
   trees are *pointed to* by the new tree, i.e, their roots become
   nodes in the new tree, and therefore you generally should *not* try
   to free the originals (but can discard the old heads pointers). */
ptree_t *merge_ptrees( ptree_t **heads, int len, int type );

/** Generate BDD corresponding to given parse tree.  var_list is the
   linked list of variable names to refer to; ordering in var_list
   determines index in the BDD.  Non-Boolean variables and equality
   are not supported.  fn should be NULL, unless you wish to
   initialize with a non-constant-True function.

   Any primed variables (type of PT_NEXT_VARIABLE) will be given an
   index corresponding to unprimed variables but offset by the total
   number of variables (length of list var_list). */
DdNode *ptree_BDD( ptree_t *head, ptree_t *var_list, DdManager *manager );

/** Generate Graphviz DOT file depicting the parse tree.  Return 0 on
   success, -1 on error. */
int tree_dot_dump( ptree_t *head, char *filename );

/** Delete tree with given root node. */
void delete_tree( ptree_t *head );

/** Return the minimum value among PT_CONSTANT nodes in given tree.

   N.B., -9999 is used as a special indicator value to allow
   min_tree_value() to work on the tree recursively.

   If head is NULL or there are not any PT_CONSTANT nodes, then the
   output is undefined. */
int min_tree_value( ptree_t *head );

/** Return the maximum value among PT_CONSTANT nodes in given tree.

   Also see documentation for min_tree_value(). */
int max_tree_value( ptree_t *head );

/** Same as min_tree_value() but restrict attention to specified variable. */
int rmin_tree_value( ptree_t *head, char *name );

/** Same as max_tree_value() but restrict attention to specified variable. */
int rmax_tree_value( ptree_t *head, char *name );

/** Verify that every variable (resp., next variable) in given parse
   tree is contained in var_list (resp., nextvar_list).  Return NULL
   if successfully verified; else, return a pointer to a string of a
   violating variable, which the caller is expected to free. */
char *check_vars( ptree_t *head, ptree_t *var_list, ptree_t *nextvar_list );

/** name is a variable with domain {0,...,maxval}, where we assume
   that maxval is at least 2.  Return a list of variables in order of
   increasing bit index, e.g., invoking with a variable named "foo"
   and maxval=2 causes a list to be returned of the form foo0,foo1.

   Return NULL on error. */
ptree_t *var_to_bool( char *name, int maxval );

/** Expand all occurrences of name (a variable) in formula described
   by the tree head, replacing by Boolean variables as would be found
   by var_to_bool().  Changes are made in-place.

   Return the (possibly new) head pointer, or NULL if error. */
ptree_t *expand_to_bool( ptree_t *head, char *name, int maxval );

/** Create tree describing unreachable values of the primed form of a
   nonboolean-expanded-to-boolean variable.  E.g., this can be used to
   handle "don't care" values that appear as a side-effect of
   conversion to a bitvector. */
ptree_t *unreach_expanded_bool( char *name, int min, int max );

/** Push variable or constant into top of tree.  (Behavior is
   like reverse Polish notation.)  Return the new head. */
ptree_t *pusht_terminal( ptree_t *head, int type, char *name, int value );

/** Push unary or binary operator into top of tree.  (Behavior is like reverse
   Polish notation.)  Return the new head. */
ptree_t *pusht_operator( ptree_t *head, int type );

/** Return pointer to new item (which is of course accessible via given
   root node).  If argument head is NULL, then behave exactly as
   init_ptree() (and thus, a new tree root node is returned). */
ptree_t *append_list_item( ptree_t *head, int type, char *name, int value );

/** 0-based indexing.  An index of -1 refers to the last item in the
   list.  Return pointer to parent of removed node if non-root, or to
   item with original index of 1 if root.

   If head is NULL (i.e., tree is empty), then print a warning and
   return NULL.  If index is outside of possible range, then print
   warning and return NULL. */
ptree_t *remove_list_item( ptree_t *head, int index );

/** 0-based indexing.  An index of -1 refers to the last item in the
   list.  Return pointer to the node, or NULL on error. */
ptree_t *get_list_item( ptree_t *head, int index );

/** Return index (0-base) to first match in list, or -1 if not found. */
int find_list_item( ptree_t *head, int type, char *name, int value );


#endif
