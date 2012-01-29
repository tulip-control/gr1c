/* ptree.c -- Definitions for signatures appearing in ptree.h.
 *
 *
 * SCL; 29 Jan 2012.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ptree.h"


ptree_t *init_ptree( int type, char *name, int value )
{
	ptree_t *head = malloc( sizeof(ptree_t) );
	if (head == NULL) {
		perror( "init_ptree, malloc" );
		return NULL;
	}
	head->left = head->right = NULL;
	head->type = type;
	if (type == PT_VARIABLE || type == PT_NEXT_VARIABLE) {
		if (name == NULL) {
			head->name = NULL;
		} else {
			head->name = strdup( name );
			if (head->name == NULL) {
				perror( "init_ptree, strdup" );
				return NULL;
			}
		}
	} else if (type == PT_CONSTANT) {
		head->value = value;
	}
	return head;
}


void delete_tree( ptree_t *head )
{
	if (head == NULL)
		return;
	if (head->left != NULL) {
		delete_tree( head->left );
		head->left = NULL;
	}
	if (head->right != NULL) {
		delete_tree( head->right );
		head->right = NULL;
	}
	if (head->name != NULL) {
		free( head->name );
	}
	free( head );
}


int tree_size( ptree_t *head )
{
	if (head == NULL)
		return 0;
	return 1 + tree_size( head->left ) + tree_size( head->right );
}


void inorder_print( ptree_t *head )
{
	if (head == NULL)
		return;

	inorder_print( head->left );
	
	switch (head->type) {
	case PT_EMPTY:
		printf( "(empty)\n" );
		break;

	case PT_VARIABLE:
		printf( "%s\n", head->name );
		break;

	case PT_NEXT_VARIABLE:
		printf( "%s'\n", head->name );
		break;

	case PT_CONSTANT:
		printf( "%d\n", head->value );
		break;
			
	case PT_NEG:
		printf( "!\n" );
		break;

	case PT_AND:
		printf( "&\n" );
		break;

	case PT_OR:
		printf( "|\n" );
		break;

	case PT_IMPLIES:
		printf( "->\n" );
		break;
		
	case PT_EQUALS:
		printf( "=\n" );
		break;

	default:
		fprintf( stderr, "inorder_print: Unrecognized type, %d\n", head->type );
		break;
	}

	inorder_print( head->right );
}


ptree_t *append_list_item( ptree_t *head, int type, char *name, int value )
{
	if (head == NULL) {
		return init_ptree( type, name, value );
	}

	if (head->left == NULL) {
		head->left = init_ptree( type, name, value );
		return head->left;
	} else {
		return append_list_item( head->left, type, name, value );
	}
}


ptree_t *remove_list_item( ptree_t *head, int index )
{
	ptree_t *child;
	int length = tree_size( head );
	int current_index = 1;

	/* Error-checking */
	if (head == NULL) {
		fprintf( stderr, "Warning: remove_list_item called with empty tree.\n" );
		return NULL;
	}
	if (index < -1 || index >= length) {
		fprintf( stderr, "Error: remove_list_item given invalid index, %d. Max possible is %d.\n", index, length-1 );
		return NULL;
	}

	if (index == 0 || (length == 1 && index == -1)) {
		/* Special case; remove root, return pointer to next item. */
		child = head->left;
		if (head->name != NULL) {
			free( head->name );
		}
		free( head );
		return child;
	}

	if (index == -1)  /* Special case; remove last item. */
		index = length-1;

	while (current_index != index) {
		head = head->left;
		current_index++;
	}
	child = head->left;
	head->left = child->left;
	if (child->name != NULL) {
		free( child->name );
	}
	free( child );

	return head;
}


int tree_dot_dump( ptree_t *head, char *filename )
{

	return -1;
}
