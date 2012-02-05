/* ptree.c -- Definitions for signatures appearing in ptree.h.
 *
 *
 * SCL; Jan, Feb 2012.
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


void print_node( ptree_t *node, FILE *f )
{
	if (f == NULL)
		f = stdout;

	switch (node->type) {
	case PT_EMPTY:
		fprintf( f, "(empty)" );
		break;

	case PT_VARIABLE:
		fprintf( f, "%s", node->name );
		break;

	case PT_NEXT_VARIABLE:
		fprintf( f, "%s'", node->name );
		break;

	case PT_CONSTANT:
		fprintf( f, "%d", node->value );
		break;
			
	case PT_NEG:
		fprintf( f, "!" );
		break;

	case PT_AND:
		fprintf( f, "&" );
		break;

	case PT_OR:
		fprintf( f, "|" );
		break;

	case PT_IMPLIES:
		fprintf( f, "->" );
		break;
		
	case PT_EQUALS:
		fprintf( f, "=" );
		break;

	default:
		fprintf( stderr, "inorder_print: Unrecognized type, %d\n", node->type );
		break;
	}
}


void inorder_trav( ptree_t *head,
				   void (* node_fn)(ptree_t *, void *), void *arg )
{
	if (head == NULL)
		return;

	inorder_trav( head->left, node_fn, arg );
	(*node_fn)( head, arg );
	inorder_trav( head->right, node_fn, arg );
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


ptree_t *pusht_terminal( ptree_t *head, int type, char *name, int value )
{
	ptree_t *new_head;

	new_head = init_ptree( type, name, value );
	new_head->left = head;
	return new_head;
}


ptree_t *pusht_operator( ptree_t *head, int type )
{
	ptree_t *new_head;

	/* Make sure doing this is possible. */
	if (head == NULL)
		return NULL;

	new_head = init_ptree( type, NULL, 0 );
	if (head->type == PT_EMPTY || head->type == PT_VARIABLE
		|| head->type == PT_NEXT_VARIABLE || head->type == PT_CONSTANT
		|| head->type == PT_NEG) { /* if terminal or unary */
		if (head->left != NULL && head->left->type == PT_EQUALS) {
			/* Handle special case of left node of = operator. (N.B.,
			   this cannot happen for other operators due to the order
			   that tokens are visiting while parsing input.) */
			new_head->left = head;
			new_head->right = head->left;
			head->left = head->left->left->left;
			new_head->right->left->left = NULL;
		} else {
			new_head->right = head;
			new_head->left = head->left;
			head->left = NULL;
		}
	} else { /* else, binary operator */
		if (head->left == NULL) {
			fprintf( stderr, "Error: parsing failed during call to pusht_operator." );
			exit(-1);
		}
		new_head->right = head;
		new_head->left = head->left->left;
		head->left->left = NULL;
	}
	
	return new_head;
}


void tree_dot_dump_node( ptree_t *node, FILE *f )
{
	fprintf( f, "\"%ld;\\n", (size_t)node );
	print_node( node, f );
	fprintf( f, "\"\n" );
	if (node->left != NULL) {
		fprintf( f, "\"%ld;\\n", (size_t)node );
		print_node( node, f );
		fprintf( f, "\" -> \"%ld;\\n", (size_t)(node->left) );
		print_node( node->left, f );
		fprintf( f, "\"\n" );
	}
	if (node->right != NULL) {
		fprintf( f, "\"%ld;\\n", (size_t)node );
		print_node( node, f );
		fprintf( f, "\" -> \"%ld;\\n", (size_t)(node->right) );
		print_node( node->right, f );
		fprintf( f, "\"\n" );
	}
}


int tree_dot_dump( ptree_t *head, char *filename )
{
	

	FILE *f = fopen( filename, "w" );
	if (f == NULL) {
		perror( "tree_dot_dump, fopen" );
		return -1;
	}

	if (fprintf( f, "digraph PT {\n" ) < -1) {
		fclose( f );
		return -1;
	}

	inorder_trav( head, tree_dot_dump_node, f );

	fprintf( f, "}\n" );

	if (fclose( f )) {
		perror( "tree_dot_dump, fclose" );
		return -1;
	}

	return 0;
}


void print_formula( ptree_t *head, FILE *f )
{
	if (head == NULL) {
		fprintf( stderr, "Warning: print_formula called with NULL node." );
		return;
	}

	if (f == NULL)
		f = stdout;

	switch (head->type) {
	case PT_AND:
	case PT_OR:
	case PT_IMPLIES:
	case PT_EQUALS:
		fprintf( f, "(" );
		print_formula( head->left, f );
		break;

	case PT_NEG:
		fprintf( f, "(!" );
		print_formula( head->right, f );
		fprintf( f, ")" );
		return;

	case PT_VARIABLE:
		fprintf( f, "%s", head->name );
		return;

	case PT_NEXT_VARIABLE:
		fprintf( f, "%s'", head->name );
		return;
		
	case PT_CONSTANT:
		if (head->value == 0) {
			fprintf( f, "False" );
		} else if (head->value == 1) {
			fprintf( f, "True" );
		} else {
			fprintf( f, "%d", head->value );
		}
		return;

	default:
		fprintf( stderr, "Warning: print_formula called with node of unknown type" );
		return;
	}

	switch (head->type) {
	case PT_AND:
		fprintf( f, "&" );
		break;
	case PT_OR:
		fprintf( f, "|" );
		break;
	case PT_IMPLIES:
		fprintf( f, "->" );
		break;
	case PT_EQUALS:
		fprintf( f, "=" );
		break;
	}
	print_formula( head->right, f );
	fprintf( f, ")" );
	return;
}
