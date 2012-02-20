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


void print_node( ptree_t *node, FILE *fp )
{
	if (fp == NULL)
		fp = stdout;

	switch (node->type) {
	case PT_EMPTY:
		fprintf( fp, "(empty)" );
		break;

	case PT_VARIABLE:
		fprintf( fp, "%s", node->name );
		break;

	case PT_NEXT_VARIABLE:
		fprintf( fp, "%s'", node->name );
		break;

	case PT_CONSTANT:
		fprintf( fp, "%d", node->value );
		break;
			
	case PT_NEG:
		fprintf( fp, "!" );
		break;

	case PT_AND:
		fprintf( fp, "&" );
		break;

	case PT_OR:
		fprintf( fp, "|" );
		break;

	case PT_IMPLIES:
		fprintf( fp, "->" );
		break;
		
	case PT_EQUALS:
		fprintf( fp, "=" );
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


ptree_t *merge_ptrees( ptree_t **heads, int len, int type )
{
	ptree_t *head, *node;
	int i;

	if (len <= 1 || heads == NULL)  /* Vacuous call. */
		return NULL;

	/* Check whether valid merging operator requested. */
	switch (type) {
	case PT_AND:
	case PT_OR:
	case PT_IMPLIES:
		break;
	default:
		return NULL;
	}

	head = init_ptree( type, NULL, -1 );
	if (head == NULL)
		return NULL;
	head->right = *(heads+len-1);

	node = head;
	for (i = len-1; i > 1; i--) {
		node->left = init_ptree( type, NULL, -1 );
		if (node->left == NULL) {
			fprintf( stderr, "Error: merge_ptrees failed to create enough new nodes.\n" );
			return NULL;
		}
		node = node->left;
		node->right = *(heads+i-1);
	}
	node->left = *heads;

	return head;
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


void tree_dot_dump_node( ptree_t *node, FILE *fp )
{
	fprintf( fp, "\"%ld;\\n", (size_t)node );
	print_node( node, fp );
	fprintf( fp, "\"\n" );
	if (node->left != NULL) {
		fprintf( fp, "\"%ld;\\n", (size_t)node );
		print_node( node, fp );
		fprintf( fp, "\" -> \"%ld;\\n", (size_t)(node->left) );
		print_node( node->left, fp );
		fprintf( fp, "\"\n" );
	}
	if (node->right != NULL) {
		fprintf( fp, "\"%ld;\\n", (size_t)node );
		print_node( node, fp );
		fprintf( fp, "\" -> \"%ld;\\n", (size_t)(node->right) );
		print_node( node->right, fp );
		fprintf( fp, "\"\n" );
	}
}


int tree_dot_dump( ptree_t *head, char *filename )
{
	

	FILE *fp = fopen( filename, "w" );
	if (fp == NULL) {
		perror( "tree_dot_dump, fopen" );
		return -1;
	}

	if (fprintf( fp, "digraph PT {\n" ) < -1) {
		fclose( fp );
		return -1;
	}

	inorder_trav( head, tree_dot_dump_node, fp );

	fprintf( fp, "}\n" );

	if (fclose( fp )) {
		perror( "tree_dot_dump, fclose" );
		return -1;
	}

	return 0;
}


void print_formula( ptree_t *head, FILE *fp )
{
	if (head == NULL) {
		fprintf( stderr, "Warning: print_formula called with NULL node." );
		return;
	}

	if (fp == NULL)
		fp = stdout;

	switch (head->type) {
	case PT_AND:
	case PT_OR:
	case PT_IMPLIES:
	case PT_EQUALS:
		fprintf( fp, "(" );
		print_formula( head->left, fp );
		break;

	case PT_NEG:
		fprintf( fp, "(!" );
		print_formula( head->right, fp );
		fprintf( fp, ")" );
		return;

	case PT_VARIABLE:
		fprintf( fp, "%s", head->name );
		return;

	case PT_NEXT_VARIABLE:
		fprintf( fp, "%s'", head->name );
		return;
		
	case PT_CONSTANT:
		if (head->value == 0) {
			fprintf( fp, "False" );
		} else if (head->value == 1) {
			fprintf( fp, "True" );
		} else {
			fprintf( fp, "%d", head->value );
		}
		return;

	default:
		fprintf( stderr, "Warning: print_formula called with node of unknown type" );
		return;
	}

	switch (head->type) {
	case PT_AND:
		fprintf( fp, "&" );
		break;
	case PT_OR:
		fprintf( fp, "|" );
		break;
	case PT_IMPLIES:
		fprintf( fp, "->" );
		break;
	case PT_EQUALS:
		fprintf( fp, "=" );
		break;
	}
	print_formula( head->right, fp );
	fprintf( fp, ")" );
	return;
}


DdNode *ptree_BDD( ptree_t *head, ptree_t *var_list, DdManager *manager )
{
	DdNode *lsub, *rsub, *fn;
	int index;

	switch (head->type) {
	case PT_AND:
	case PT_OR:
	case PT_IMPLIES:
		lsub = ptree_BDD( head->left, var_list, manager );
		rsub = ptree_BDD( head->right, var_list, manager );
		break;
	case PT_NEG:
		rsub = ptree_BDD( head->right, var_list, manager );
		break;
	case PT_VARIABLE:
		index = find_list_item( var_list, head->type, head->name, 0 );
		if (index < 0) {
			fprintf( stderr, "Error: ptree_BDD requested variable \"%s\", but it is not in given list.\n", head->name );
			exit(-1);
		}
		return Cudd_bddIthVar( manager, index );;
	}

	switch (head->type) {
	case PT_AND:
		fn = Cudd_bddAnd( manager, lsub, rsub );
		Cudd_Ref( fn );
		if (head->left->type != PT_VARIABLE)
			Cudd_RecursiveDeref( manager, lsub );
		if (head->right->type != PT_VARIABLE)
			Cudd_RecursiveDeref( manager, rsub );
		break;

	case PT_OR:
		fn = Cudd_bddOr( manager, lsub, rsub );
		Cudd_Ref( fn );
		if (head->left->type != PT_VARIABLE)
			Cudd_RecursiveDeref( manager, lsub );
		if (head->right->type != PT_VARIABLE)
			Cudd_RecursiveDeref( manager, rsub );
		break;

	case PT_IMPLIES:
		fn = Cudd_bddOr( manager, Cudd_Not(lsub), rsub );
		Cudd_Ref( fn );
		if (head->left->type != PT_VARIABLE)
			Cudd_RecursiveDeref( manager, lsub );
		if (head->right->type != PT_VARIABLE)
			Cudd_RecursiveDeref( manager, rsub );
		break;

	case PT_NEG:
		fn = Cudd_Not( rsub );
		Cudd_Ref( fn );
		if (head->right->type != PT_VARIABLE)
			Cudd_RecursiveDeref( manager, rsub );
		break;
	}

	return fn;
}


int find_list_item( ptree_t *head, int type, char *name, int value )
{
	int index = 0;
	while (head != NULL) {
		if (head->type == type) {
			if (head->type == PT_VARIABLE || head->type == PT_NEXT_VARIABLE) {
				/* If node is variable type, then names must match. */
				if (name != NULL && head->name != NULL && !strcmp( head->name, name ))
					break;
			} else if (head->type == PT_CONSTANT) {
				/* If node is constant (e.g., True), then values must match. */
				if (head->value == value)
					break;
			} else {
				/* Otherwise, it suffices to have the same type. */
				break;
			}
		}
		index += 1;
		head = head->left;
	}
	if (head == NULL) {
		return -1;  /* No matches found. */
	} else {
		return index;
	}
}


ptree_t *get_list_item( ptree_t *head, int index )
{
	if (head == NULL || index < -1)
		return NULL;

	if (index == -1) {  /* Special case of end item request. */
		while (head->left != NULL)
			head = head->left;
		return head;
	}

	while (index > 1) {
		if (head->left == NULL)
			return NULL;
		head = head->left;
		index--;
	}
	return head;
}
