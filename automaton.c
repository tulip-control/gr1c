/* automaton.c -- Definitions for core routines on automaton objects.
 *
 *
 * SCL; 2012.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "automaton.h"
#include "solve_support.h"


anode_t *insert_anode( anode_t *head, int mode, int rgrad, bool *state, int state_len )
{
	int i;
	anode_t *new_head = malloc( sizeof(anode_t) );
	if (new_head == NULL) {
		perror( "insert_anode, malloc" );
		return NULL;
	}

	new_head->state = malloc( state_len*sizeof(bool) );
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


anode_t *build_anode_trans( anode_t *head, int mode, bool *state, int state_len,
							int next_mode, bool **next_states, int next_len )
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
							 int mode, bool *state, int state_len,
							 int next_mode, bool *next_state )
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


anode_t *find_anode( anode_t *head, int mode, bool *state, int state_len )
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


int find_anode_index( anode_t *head, int mode, bool *state, int state_len )
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


int forward_modereach( anode_t *head, anode_t *node, int mode, bool **N, int N_len, int magic_mode, int state_len )
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
