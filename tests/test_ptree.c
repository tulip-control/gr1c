/* Unit tests for formula parse trees.
 *
 * SCL; April 2012.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ptree.h"
#include "tests/common.h"


#define REF_FORMULA1_SMALL "felix & !(the' | cat)"
#define REF_FORMULA1_SIZE 6
#define DUMP_FORMULA1_SMALL "felix&!the'|cat"

#define REF_FORMULA2_SMALL "!x'"
#define REF_CONJUNCTED_SMALLS "(felix & !(the' | cat)) & (!x')"
#define DUMP_CONJUNCTED_SMALLS "felix&!the'|cat&!x'"
#define CONJUNCTED_SMALLS_SIZE 9


void compare_ptrees( ptree_t *head1, ptree_t *head2 )
{
	if (head1 == NULL && head2 == NULL)
		return;
	if ((head1 != NULL && head2 == NULL) || (head1 == NULL && head2 != NULL)) {
		ERRPRINT( "Unexpected discrepancy between parse trees." );
		abort();
	}
	compare_ptrees( head1->left, head2->left );
	compare_ptrees( head1->right, head2->right );
}


#define STRING_MAXLEN 60
int main( int argc, char **argv )
{
	int index;
	int i;  /* Generic counter */
	ptree_t *head, *node;
	ptree_t *head2;
	ptree_t **heads;
	char filename[STRING_MAXLEN];
	char *result;
	FILE *fp;

	/* Repeatable random seed */
	srand( 0 );


	/************************************************
	 * Linked list use case
	 ************************************************/
	head = append_list_item( NULL, PT_VARIABLE, "a", -1 );
	if (head == NULL) {
		ERRPRINT( "failed to initialize linked list (as ptree object)." );
		abort();
	}
	node = append_list_item( head, PT_VARIABLE, "b", -1 );
	if (node == NULL) {
		ERRPRINT( "failed to append item to linked list (as ptree object)." );
		abort();
	}
	/* At this point, the list is (in order left-to-right)
	 *   a , b
	 */
	node = get_list_item( head, 0 );
	if (*(node->name) != 'a') {
		ERRPRINT( "expected item not found in linked list (as ptree object)." );
		abort();
	}
	node = get_list_item( head, 1 );
	if (*(node->name) != 'b') {
		ERRPRINT( "expected item not found in linked list (as ptree object)." );
		abort();
	}
	node = append_list_item( head, PT_VARIABLE, "kitten", -1 );
	if (node == NULL) {
		ERRPRINT( "failed to append item to linked list (as ptree object)." );
		abort();
	}
	/* At this point, the list is (in order)
	 *   a , b , kitten
	 */
	if ((index = find_list_item( head, PT_VARIABLE, "b", -1 )) != 1) {
		if (index < 0) {
			ERRPRINT1( "expected linked list (as ptree object) item named \"%s\" not found.", "b" );
		} else {
			ERRPRINT2( "linked list (as ptree object) item named \"%s\" found at wrong index %d.", "b", index );
		}
		abort();
	}
	index = find_list_item( head, PT_VARIABLE, "kitten", -1 );
	if (index != 2) {
		if (index < 0) {
			ERRPRINT1( "expected linked list (as ptree object) item named \"%s\" not found.", "kitten" );
		} else {
			ERRPRINT2( "linked list (as ptree object) item named \"%s\" found at wrong index %d.", "kitten", index );
		}
		abort();
	}
	node = get_list_item( head, index );
	if (node == NULL) {
		ERRPRINT( "error during index-based look-up of item in linked list (as ptree object)." );
		abort();
	} else if (strncmp( node->name, "kitten", strlen("kitten") )) {
		ERRPRINT2( "found wrong item named \"%s\" while looking for \"%s\" in linked list (as ptree object).", node->name, "kitten" );
		abort();
	}
	if (tree_size( head ) != 3) {
		ERRPRINT1( "linked list of length 3 detected as having wrong length %d.", tree_size( head ) );
		abort();
	}
	node = remove_list_item( head, 0 );  /* Remove first item in list */
	if (node == NULL || *(node->name) != 'b') {
		ERRPRINT( "error removing first item in linked list (as ptree object)." );
		abort();
	}
	head = node;
	/* At this point, the list is (in order)
	 *   b , kitten
	 */
	node = remove_list_item( head, -1 );  /* Remove last item in list */
	if (node == NULL || node != head) {
		ERRPRINT( "error removing last item in linked list (as ptree object)." );
		abort();
	}
	/* At this point, the list is (in order)
	 *   b
	 */
	delete_tree( head );
	head = NULL;


	/************************************************
	 * Elementary parse trees
	 ************************************************/
	/* (felix & !(the' | cat)) & (!x')

	   Strings of subformulae provided above with definitions
	   REF_FORMULA1_SMALL and REF_FORMULA2_SMALL.

	   Construct by hand, then use stack-based approach for same
	   formula, and compare results. */
	head = init_ptree( PT_AND, NULL, -1 );
	if (head == NULL) {
		ERRPRINT( "Error while manually building parse tree for \"" REF_FORMULA1_SMALL "\"." );
		abort();
	}
	head->left = init_ptree( PT_VARIABLE, "felix", -1 );
	if (head->left == NULL) {
		ERRPRINT( "Error while manually building parse tree for \"" REF_FORMULA1_SMALL "\"." );
		abort();
	}
	head->right = init_ptree( PT_NEG, NULL, -1 );
	if (head->right == NULL) {
		ERRPRINT( "Error while manually building parse tree for \"" REF_FORMULA1_SMALL "\"." );
		abort();
	}
	head->right->right = init_ptree( PT_OR, NULL, -1 );
	if (head->right->right == NULL) {
		ERRPRINT( "Error while manually building parse tree for \"" REF_FORMULA1_SMALL "\"." );
		abort();
	}
	head->right->right->left = init_ptree( PT_NEXT_VARIABLE, "the", -1 );
	if (head->right->right->left == NULL) {
		ERRPRINT( "Error while manually building parse tree for \"" REF_FORMULA1_SMALL "\"." );
		abort();
	}
	head->right->right->right = init_ptree( PT_VARIABLE, "cat", -1 );
	if (head->right->right->right == NULL) {
		ERRPRINT( "Error while manually building parse tree for \"" REF_FORMULA1_SMALL "\"." );
		abort();
	}
	
	if (tree_size( head ) != REF_FORMULA1_SIZE) {
		ERRPRINT2( "Manually built parse tree of size %d detected as having wrong size %d.", REF_FORMULA1_SIZE, tree_size( head ) );
		abort();
	}

	/* Dump the manually built tree to a temporary file. */
	strcpy( filename, "temp_ptree_dumpXXXXXX" );
	result = mktemp( filename );
	if (result == NULL) {
		perror( "test_ptree, mktemp" );
		abort();
	}
	fp = fopen( filename, "w+" );
	if (fp == NULL) {
		perror( "test_ptree, fopen" );
		abort();
	}
	inorder_trav( head, print_node, fp );
	if (fseek( fp, 0, SEEK_SET )) {
		perror( "test_ptree, fseek" );
		abort();
	}

	/* Read back in the result of dumping the manually built tree and
	   compare to reference. */
	result = malloc( STRING_MAXLEN*sizeof(char) );
	if (result == NULL) {
		perror( "test_ptree, malloc" );
		abort();
	}
	for (i = 0; i < STRING_MAXLEN-1 && !feof( fp ); i++)
		*(result+i) = (char)fgetc( fp );
	if (i < 1) {
		ERRPRINT( "Indexing error while reading back dumped parse tree." );
		abort();
	}
	*(result+i-1) = '\0';
	if (strcmp( result, DUMP_FORMULA1_SMALL )) {
		ERRPRINT1( "Error: expected formula string \"" DUMP_FORMULA1_SMALL "\" but got \"%s\".", result );
		abort();
	}

	free( result );
	fclose( fp );
	if (remove( filename )) {
		perror( "test_ptree, remove" );
		abort();
	}

	/* Manually build another small parse tree and try to merge. */
	head2 = init_ptree( PT_NEG, NULL, -1 );
	if (head2 == NULL) {
		ERRPRINT( "Error while manually building parse tree for \"" REF_FORMULA2_SMALL "\"." );
		abort();
	}
	head2->right = init_ptree( PT_NEXT_VARIABLE, "x", -1 );
	if (head2->right == NULL) {
		ERRPRINT( "Error while manually building parse tree for \"" REF_FORMULA2_SMALL "\"." );
		abort();
	}
	heads = malloc( 2*sizeof(ptree_t *) );
	*heads = head;
	*(heads+1) = head2;
	head = merge_ptrees( heads, 2, PT_AND );
	if (head == NULL) {
		ERRPRINT( "Error while conjuncting manually built subformulae \"" REF_FORMULA1_SMALL "\" and \"" REF_FORMULA2_SMALL "\"." );
		abort();
	}
	free( heads );

	if (tree_size( head ) != CONJUNCTED_SMALLS_SIZE) {
		ERRPRINT2( "Conjunction (from manually built subformulae) of size %d detected as having wrong size %d.", CONJUNCTED_SMALLS_SIZE, tree_size( head ) );
		abort();
	}
	
	/* Dump conjunction of manually built trees to a temporary file. */
	strcpy( filename, "temp_ptree_dumpXXXXXX" );
	result = mktemp( filename );
	if (result == NULL) {
		perror( "test_ptree, mktemp" );
		abort();
	}
	fp = fopen( filename, "w+" );
	if (fp == NULL) {
		perror( "test_ptree, fopen" );
		abort();
	}
	inorder_trav( head, print_node, fp );
	if (fseek( fp, 0, SEEK_SET )) {
		perror( "test_ptree, fseek" );
		abort();
	}

	/* Read back in the result of dumping and compare to reference. */
	result = malloc( STRING_MAXLEN*sizeof(char) );
	if (result == NULL) {
		perror( "test_ptree, malloc" );
		abort();
	}
	for (i = 0; i < STRING_MAXLEN-1 && !feof( fp ); i++)
		*(result+i) = (char)fgetc( fp );
	if (i < 1) {
		ERRPRINT( "Indexing error while reading back dumped parse tree." );
		abort();
	}
	*(result+i-1) = '\0';
	if (strcmp( result, DUMP_CONJUNCTED_SMALLS )) {
		ERRPRINT1( "Error: expected formula string \"" DUMP_CONJUNCTED_SMALLS "\" but got \"%s\".", result );
		abort();
	}

	free( result );
	fclose( fp );
	if (remove( filename )) {
		perror( "test_ptree, remove" );
		abort();
	}

	/* Construct parse tree using stack */
	head2 = NULL;
	head2 = pusht_terminal( head2, PT_VARIABLE, "felix", -1 );
	head2 = pusht_terminal( head2, PT_NEXT_VARIABLE, "the", -1 );
	head2 = pusht_terminal( head2, PT_VARIABLE, "cat", -1 );
	head2 = pusht_operator( head2, PT_OR );
	head2 = pusht_operator( head2, PT_NEG );
	head2 = pusht_operator( head2, PT_AND );
	head2 = pusht_terminal( head2, PT_NEXT_VARIABLE, "x", -1 );
	head2 = pusht_operator( head2, PT_NEG );
	head2 = pusht_operator( head2, PT_AND );
	
	if (tree_size( head2 ) != CONJUNCTED_SMALLS_SIZE) {
		ERRPRINT2( "Stack-generated parse tree of size %d detected as having wrong size %d.", CONJUNCTED_SMALLS_SIZE, tree_size( head ) );
		abort();
	}

	/* Dump stack-generated parse tree to a temporary file. */
	strcpy( filename, "temp_ptree_dumpXXXXXX" );
	result = mktemp( filename );
	if (result == NULL) {
		perror( "test_ptree, mktemp" );
		abort();
	}
	fp = fopen( filename, "w+" );
	if (fp == NULL) {
		perror( "test_ptree, fopen" );
		abort();
	}
	inorder_trav( head2, print_node, fp );
	if (fseek( fp, 0, SEEK_SET )) {
		perror( "test_ptree, fseek" );
		abort();
	}

	/* Read back in the result of dumping and compare to reference. */
	result = malloc( STRING_MAXLEN*sizeof(char) );
	if (result == NULL) {
		perror( "test_ptree, malloc" );
		abort();
	}
	for (i = 0; i < STRING_MAXLEN-1 && !feof( fp ); i++)
		*(result+i) = (char)fgetc( fp );
	if (i < 1) {
		ERRPRINT( "Indexing error while reading back dumped parse tree." );
		abort();
	}
	*(result+i-1) = '\0';
	if (strcmp( result, DUMP_CONJUNCTED_SMALLS )) {
		ERRPRINT1( "Error: expected formula string \"" DUMP_CONJUNCTED_SMALLS "\" but got \"%s\".", result );
		abort();
	}

	free( result );
	fclose( fp );
	if (remove( filename )) {
		perror( "test_ptree, remove" );
		abort();
	}

	/* Now compare trees directly; abort on discrepancy. */
	compare_ptrees( head, head2 );

	delete_tree( head );
	delete_tree( head2 );

	return 0;
}