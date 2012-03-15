/* automaton.c -- Definitions for signatures appearing in automaton.h.
 *
 *
 * SCL; Mar 2012.
 */


#include <stdlib.h>
#include <stdio.h>

#include "automaton.h"


int tulip_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list, FILE *fp )
{
	int i;
	anode_t *node;
	int node_counter = 0;
	ptree_t *var;
	int num_env, num_sys;

	if (fp == NULL)
		fp = stdout;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	fprintf( fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
	fprintf( fp, "<tulipcon xmlns=\"http://tulip-control.sourceforge.net/ns/0\" version=\"0\">\n" );
	fprintf( fp, "  <spec>\n    <env_init></env_init><env_safety></env_safety><env_prog></env_prog><sys_init></sys_init><sys_safety></sys_safety><sys_prog></sys_prog>\n  </spec>\n" );
	
	fprintf( fp, "  <aut>\n" );
	node = head;
	while (node) {
		fprintf( fp, "    <node>\n      <id>%d</id><name></name>\n      <child_list>", node_counter );
		for (i = 0; i < node->trans_len; i++)
			fprintf( fp, " %d",
					 find_anode_index( head,
									   (*(node->trans+i))->mode,
									   (*(node->trans+i))->state, num_env+num_sys ) );
		fprintf( fp, "</child_list>\n      <state>\n" );
		for (i = 0; i < num_env; i++) {
			var = get_list_item( evar_list, i );
			fprintf( fp, "        <item key=\"%s\" value=\"%d\" />\n",
					 var->name, *(node->state+i) );
		}
		for (i = 0; i < num_sys; i++) {
			var = get_list_item( svar_list, i );
			fprintf( fp, "        <item key=\"%s\" value=\"%d\" />\n",
					 var->name, *(node->state+num_env+i) );
		}
		fprintf( fp, "      </state>\n    </node>\n" );
		node_counter++;
		node = node->next;
	}

	fprintf( fp, "  </aut>\n" );
	fprintf( fp, "  <extra>strategy synthesized with gr1c, version " GR1C_VERSION "</extra>\n</tulipcon>\n" );

	return 0;
}


void list_aut_dump( anode_t *head, int state_len, FILE *fp )
{
	anode_t *node = head;
	int node_counter = 0;
	int i;
	int width_count;
	if (fp == NULL)
		fp = stdout;
	while (node) {
		width_count = 0;
		fprintf( fp, "%4d : ", node_counter );
		for (i = 0; i < state_len; i++) {
			fprintf( fp, "%d", *(node->state+i) );
			width_count++;
			if (width_count == 4) {
				fprintf( fp, " " );
				width_count = 0;
			}
		}
		fprintf( fp, " - %2d - [", node->mode );
		for (i = 0; i < node->trans_len; i++)
			fprintf( fp, " %d",
					 find_anode_index( head,
									   (*(node->trans+i))->mode,
									   (*(node->trans+i))->state, state_len ) );
		fprintf( fp, "]\n" );
		node = node->next;
		node_counter++;
	}
}


anode_t *insert_anode( anode_t *head, int mode, bool *state, int state_len )
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
	new_head->trans = NULL;
	new_head->trans_len = 0;

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
