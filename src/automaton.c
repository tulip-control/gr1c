/* automaton.c -- Definitions for core routines on automaton objects.
 *
 *
 * SCL; 2012, 2013.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ptree.h"
#include "gr1c_util.h"
#include "automaton.h"
#include "solve_support.h"


anode_t *insert_anode( anode_t *head, int mode, int rgrad, vartype *state, int state_len )
{
	int i;
	anode_t *new_head = malloc( sizeof(anode_t) );
	if (new_head == NULL) {
		perror( "insert_anode, malloc" );
		return NULL;
	}

	new_head->state = malloc( state_len*sizeof(vartype) );
	if (new_head->state == NULL) {
		perror( "insert_anode, malloc" );
		free( new_head );
		return NULL;
	}
	for (i = 0; i < state_len; i++)
		*(new_head->state + i) = *(state+i);
	new_head->mode = mode;
	new_head->rgrad = rgrad;
	new_head->trans = NULL;
	new_head->trans_len = 0;

	if (find_anode( head, mode, state, state_len )) {
		fprintf( stderr, "WARNING: inserting indistinguishable nodes into automaton.\n" );
	}

	if (head == NULL) {
		new_head->next = NULL;
		return new_head;
	} else {
		new_head->next = head;
		return new_head;
	}
}


anode_t *build_anode_trans( anode_t *head, int mode, vartype *state, int state_len,
							int next_mode, vartype **next_states, int next_len )
{
	int i;
	anode_t **trans;
	anode_t *base = find_anode( head, mode, state, state_len );

	/* Handle special case of no next states. */
	if (next_len == 0) {
		if (base) {
			return head;
		} else {
			return NULL;
		}
	}

	trans = malloc( next_len*sizeof(anode_t *) );
	if (trans == NULL) {
		perror( "build_anode_trans, malloc" );
		return NULL;
	}
	for (i = 0; i < next_len; i++) {
		*(trans+i) = find_anode( head, next_mode, *(next_states+i), state_len );
		if (*(trans+i) == NULL) {
			free( trans );
			return NULL;
		}
	}
	if (base->trans != NULL)
		free( base->trans );
	base->trans = trans;
	base->trans_len = next_len;
	return head;
}


anode_t *append_anode_trans( anode_t *head,
							 int mode, vartype *state, int state_len,
							 int next_mode, vartype *next_state )
{
	anode_t **trans;
	anode_t *base = find_anode( head, mode, state, state_len );

	trans = realloc( base->trans, (base->trans_len+1)*sizeof(anode_t *) );
	if (trans == NULL) {
		perror( "append_anode_trans, realloc" );
		return NULL;
	}

	*(trans + base->trans_len) = find_anode( head, next_mode, next_state,
											 state_len );
	if (*(trans + base->trans_len) == NULL) {
		base->trans = realloc( trans, (base->trans_len)*sizeof(anode_t *) );
		return NULL;
	}
	(base->trans_len)++;
	base->trans = trans;
	return head;
}


anode_t *find_anode( anode_t *head, int mode, vartype *state, int state_len )
{
	int i;
	bool match_flag = False;

	while (head) {
		if (head->mode == mode) {
			match_flag = True;
			for (i = 0; i < state_len; i++) {
				if (*(head->state+i) != *(state+i)) {
					match_flag = False;
					break;
				}
			}
			if (match_flag)
				break;
		}
		head = head->next;
	}
	
	if (match_flag) {
		return head;
	} else {
		return NULL;
	}
}


int find_anode_index( anode_t *head, int mode, vartype *state, int state_len )
{
	int node_counter = 0;
	int i;
	bool match_flag = False;

	while (head) {
		if (head->mode == mode) {
			match_flag = True;
			for (i = 0; i < state_len; i++) {
				if (*(head->state+i) != *(state+i)) {
					match_flag = False;
					break;
				}
			}
			if (match_flag)
				break;
		}
		head = head->next;
		node_counter++;
	}
	
	if (match_flag) {
		return node_counter;
	} else {
		return -1;
	}
}


anode_t *delete_anode( anode_t *head, anode_t *target )
{
	anode_t *node, *next;
	if (head == NULL)
		return NULL;
	next = head->next;
	if (head == target) {
		if (head->state != NULL)
			free( head->state );
		if (head->trans != NULL)
			free( head->trans );
		free( head );
		return next;
	}
	node = head;
	while (next) {
		if (next == target) {
			node->next = next->next;
			if (next->state != NULL)
				free( next->state );
			if (next->trans != NULL)
				free( next->trans );
			free( next );
			return head;
		}
		node = next;
		next = node->next;
	}
	return NULL;
}


void replace_anode_trans( anode_t *head, anode_t *old, anode_t *new )
{
	anode_t **trans;
	int i, j;
	while (head) {
		for (i = 0; i < head->trans_len; i++) {
			if (*(head->trans+i) == old) {
				if (new == NULL) {
					if (head->trans_len == 1) {
						free( head->trans );
						head->trans = NULL;
						head->trans_len = 0;
					} else {
						trans = malloc( (head->trans_len-1)*sizeof(anode_t *) );
						if (trans == NULL) {
							perror( "replace_anode_trans, malloc" );
							return;
						}
						for (j = 0; j < i; j++)
							*(trans+j) = *(head->trans+j);
						for (j = i+1; j < head->trans_len; j++)
							*(trans+j-1) = *(head->trans+j);
						free( head->trans );
						head->trans = trans;
						(head->trans_len)--;
					}
					i--;
				} else {
					*(head->trans+i) = new;
				}
			}
		}
		head = head->next;
	}
}


anode_t *aut_prune_deadends( anode_t *head )
{
	anode_t *node;
	if (head == NULL)
		return NULL;
	do {
		while (head->trans_len == 0) {
			replace_anode_trans( head, head, NULL );
			head = delete_anode( head, head );
		}
		node = head->next;
		while (node) {
			if (node->trans_len == 0) {
				replace_anode_trans( head, node, NULL );
				head = delete_anode( head, node );
				break;
			}
			node = node->next;
		}
	} while (node);
	return head;
}


anode_t *pop_anode( anode_t *head )
{
	anode_t *next;
	next = head->next;
	if (head->state != NULL)
		free( head->state );
	if (head->trans != NULL)
		free( head->trans );
	free( head );
	return next;
}

void delete_aut( anode_t *head )
{
	anode_t *next;

	while (head) {
		next = head->next;
		if (head->state != NULL)
			free( head->state );
		if (head->trans != NULL)
			free( head->trans );
		free( head );
		head = next;
	}
}

int aut_size( anode_t *head )
{
	int len = 0;
	while (head) {
		len++;
		head = head->next;
	}
	return len;
}


int forward_modereach( anode_t *head, anode_t *node, int mode, vartype **N, int N_len, int magic_mode, int state_len )
{
	int i, j;
	for (i = 0; i < node->trans_len; i++) {
		if ((*(node->trans+i))->mode == mode) {
			for (j = 0; j < N_len; j++)
				if (statecmp( (*(node->trans+i))->state, *(N+j), state_len ))
					break;
			if (j < N_len) {
				(*(node->trans+i))->mode = magic_mode;
				if (forward_modereach( head, *(node->trans+i), mode, N, N_len, magic_mode, state_len ))
					return -1;
			}
		}
	}

	return 0;
}


int aut_compact_nonbool( anode_t *head, ptree_t *evar_list, ptree_t *svar_list, char *name, int maxval )
{
	int num_env, num_sys;
	ptree_t *var = evar_list, *var_tail, *var_next;
	int start_index, stop_index, i;
	vartype *new_state;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	start_index = 0;
	while (var) {
		if (strstr( var->name, name ) == var->name)
			break;
		var = var->left;
		start_index++;
	}
	if (var == NULL) {
		var = svar_list;
		while (var) {
			if (strstr( var->name, name ) == var->name)
				break;
			var = var->left;
			start_index++;
		}
		if (var == NULL)
			return -1;  /* Could not find match. */
	}

	var_tail = var;
	stop_index = start_index;
	while (var_tail->left) {
		if (strstr( var_tail->left->name, name ) != var_tail->left->name )
			break;
		var_tail = var_tail->left;
		stop_index++;
	}
	free( var->name );
	var->name = strdup( name );
	if (var->name == NULL) {
		perror( "aut_compact_nonbool, strdup" );
		return -1;
	}
	var->value = maxval;
	if (var != var_tail) {  /* More than one bit? */
		var_next = var->left;
		var->left = var_tail->left;
		var_tail->left = NULL;
		delete_tree( var_next );
	}

	while (head) {
		new_state = malloc( (num_env+num_sys - (stop_index-start_index))*sizeof(vartype) );
		if (new_state == NULL) {
			perror( "aut_compact_nonbool, malloc" );
			return -1;
		}

		for (i = 0; i < start_index; i++)
			*(new_state+i) = *(head->state+i);
		*(new_state+start_index) = bitvec_to_int( head->state+start_index,
												  stop_index-start_index+1 );
		for (i = start_index+1; i < num_env+num_sys - (stop_index-start_index); i++)
			*(new_state+i) = *(head->state+i+stop_index-start_index);
		
		free( head->state );
		head->state = new_state;

		head = head->next;
	}

	return num_env+num_sys - (stop_index-start_index);
}


/* N.B., we assume that noonbool_var_list is sorted with respect to
   the expanded variable list (evar_list followed by svar_list). */
int aut_expand_bool( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
					 ptree_t *nonbool_var_list )
{
	int mapped_len, num_nonbool;
	vartype *new_state;
	int *offw;

	if (head == NULL)  /* Empty automaton */
		return -1;

	mapped_len = tree_size( evar_list ) + tree_size( svar_list );

	num_nonbool = tree_size( nonbool_var_list );
	if (num_nonbool == 0)
		return 0;  /* Empty call */

	offw = get_offsets_list( evar_list, svar_list, nonbool_var_list );
	if (offw == NULL && num_nonbool > 0)
		return -1;

	while (head) {
		new_state = expand_nonbool_state( head->state, offw, num_nonbool, mapped_len );
		if (new_state == NULL) {
			fprintf( stderr, "Error aut_expand_bool: failed to expand nonbool values in automaton.\n" );
			return -1;
		}

		free( head->state );
		head->state = new_state;

		head = head->next;
	}

	if (offw != NULL)
		free( offw );
	return 0;
}


anode_t *forward_prune( anode_t *head, anode_t **U, int U_len )
{
	bool touched;
	int i, j;
	anode_t *node;

	if (head == NULL || U_len < 0)  /* Empty automata are not permitted. */
		return NULL;
	if (U == NULL || U_len == 0)
		return head;

	/* Loop and prune */
	do {
		touched = False;
		for (i = 0; i < U_len; i++) {
			if (*(U+i) == NULL)
				continue;

			/* Look for a predecessor */
			node = head;
			while (node) {
				for (j = 0; j < node->trans_len; j++) {
					if (*(node->trans+j) == *(U+i))
						break;
				}
				if (j < node->trans_len)
					break;
				node = node->next;
			}
			if (node == NULL) {  /* No Pred found */
				touched = True;
				U = realloc( U, (U_len + (*(U+i))->trans_len)*sizeof(anode_t *) );
				if (U == NULL) {
					perror( "forward_prune, realloc" );
					return NULL;
				}
				for (j = 0; j < (*(U+i))->trans_len; j++)
					*(U+U_len+j) = *((*(U+i))->trans+j);
				U_len += (*(U+i))->trans_len;
				head = delete_anode( head, *(U+i) );
				for (j = 0; j < U_len; j++) {  /* Delete duplicate pointers in U */
					if (j == i)
						continue;
					if (*(U+i) == *(U+j))
						*(U+j) = NULL;
				}
				*(U+i) = NULL;  /* Mark entry as deleted */
				break;
			}

		}
	} while (touched);

	free( U );
	return head;
}
