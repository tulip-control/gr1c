/* Unit tests for formula parse trees.
 *
 * SCL; 2012, 2015, 2016
 */

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ptree.h"
#include "tests_common.h"


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
        ERRPRINT( "Unexpected discrepancy of branching structure "
                  "between parse trees." );
        abort();
    }
    if (head1->type != head2->type) {
        ERRPRINT( "Unexpected discrepancy of node types "
                  "between parse trees." );
        abort();
    }

    switch (head1->type) {

    case PT_CONSTANT:
        if (head1->value != head2->value) {
            ERRPRINT( "compare_ptrees: "
                      "values of constant-type nodes do not match" );
            abort();
        }
        break;

    case PT_VARIABLE:
    case PT_NEXT_VARIABLE:
        if (strcmp( head1->name, head2->name )) {
            ERRPRINT( "compare_ptrees: "
                      "names of variable-type nodes do not match" );
            abort();
        }
        if (head1->value != head2->value) {
            ERRPRINT( "compare_ptrees: "
                      "values of variable-type nodes do not match" );
            abort();
        }
        break;

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
    ptree_t *var_list;
    ptree_t *primed_var_list;
    char filename[STRING_MAXLEN];
    char *result;
    int fd;
    FILE *fp;
    int rv;  /* return value, for checking output */


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
            ERRPRINT1( "expected linked list (as ptree object) item "
                       "named \"%s\" not found.",
                       "b" );
        } else {
            ERRPRINT2( "linked list (as ptree object) item "
                       "named \"%s\" found at wrong index %d.",
                       "b", index );
        }
        abort();
    }
    index = find_list_item( head, PT_VARIABLE, "kitten", -1 );
    if (index != 2) {
        if (index < 0) {
            ERRPRINT1( "expected linked list (as ptree object) item "
                       "named \"%s\" not found.",
                       "kitten" );
        } else {
            ERRPRINT2( "linked list (as ptree object) item "
                       "named \"%s\" found at wrong index %d.",
                       "kitten", index );
        }
        abort();
    }
    node = get_list_item( head, index );
    if (node == NULL) {
        ERRPRINT( "error during index-based look-up of item "
                  "in linked list (as ptree object)." );
        abort();
    } else if (strncmp( node->name, "kitten", strlen("kitten") )) {
        ERRPRINT2( "found wrong item named \"%s\" while looking "
                   "for \"%s\" in linked list (as ptree object).",
                   node->name, "kitten" );
        abort();
    }
    if (tree_size( head ) != 3) {
        ERRPRINT1( "linked list of length 3 detected as having "
                   "wrong length %d.",
                   tree_size( head ) );
        abort();
    }
    node = remove_list_item( head, 0 );  /* Remove first item in list */
    if (node == NULL || *(node->name) != 'b') {
        ERRPRINT( "error removing first item in "
                  "linked list (as ptree object)." );
        abort();
    }
    head = node;
    /* At this point, the list is (in order)
     *   b , kitten
     */
    node = remove_list_item( head, -1 );  /* Remove last item in list */
    if (node == NULL || node != head) {
        ERRPRINT( "error removing last item in "
                  "linked list (as ptree object)." );
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
        ERRPRINT( "Error while manually building parse tree for \""
                  REF_FORMULA1_SMALL "\"." );
        abort();
    }
    head->left = init_ptree( PT_VARIABLE, "felix", -1 );
    if (head->left == NULL) {
        ERRPRINT( "Error while manually building parse tree for \""
                  REF_FORMULA1_SMALL "\"." );
        abort();
    }
    head->right = init_ptree( PT_NEG, NULL, -1 );
    if (head->right == NULL) {
        ERRPRINT( "Error while manually building parse tree for \""
                  REF_FORMULA1_SMALL "\"." );
        abort();
    }
    head->right->right = init_ptree( PT_OR, NULL, -1 );
    if (head->right->right == NULL) {
        ERRPRINT( "Error while manually building parse tree for \""
                  REF_FORMULA1_SMALL "\"." );
        abort();
    }
    head->right->right->left = init_ptree( PT_NEXT_VARIABLE, "the", -1 );
    if (head->right->right->left == NULL) {
        ERRPRINT( "Error while manually building parse tree for \""
                  REF_FORMULA1_SMALL "\"." );
        abort();
    }
    head->right->right->right = init_ptree( PT_VARIABLE, "cat", -1 );
    if (head->right->right->right == NULL) {
        ERRPRINT( "Error while manually building parse tree for \""
                  REF_FORMULA1_SMALL "\"." );
        abort();
    }

    if (tree_size( head ) != REF_FORMULA1_SIZE) {
        ERRPRINT2( "Manually built parse tree of size %d detected as "
                   "having wrong size %d.",
                   REF_FORMULA1_SIZE, tree_size( head ) );
        abort();
    }

    /* Dump the manually built tree to a temporary file. */
    strcpy( filename, "temp_ptree_dumpXXXXXX" );
    fd = mkstemp( filename );
    if (fd == -1) {
        perror( __FILE__ ",  mkstemp" );
        abort();
    }
    fp = fdopen( fd, "w+" );
    if (fp == NULL) {
        perror( __FILE__ ",  fdopen" );
        abort();
    }
    inorder_trav( head, print_node, fp );
    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ",  fseek" );
        abort();
    }

    /* Read back in the result of dumping the manually built tree and
       compare to reference. */
    result = malloc( STRING_MAXLEN*sizeof(char) );
    if (result == NULL) {
        perror( __FILE__ ",  malloc" );
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
        ERRPRINT1( "expected formula string \"" DUMP_FORMULA1_SMALL
                   "\" but got \"%s\".",
                   result );
        abort();
    }

    free( result );

    /* Manually build another small parse tree and try to merge. */
    head2 = init_ptree( PT_NEG, NULL, -1 );
    if (head2 == NULL) {
        ERRPRINT( "Error while manually building parse tree for \""
                  REF_FORMULA2_SMALL "\"." );
        abort();
    }
    head2->right = init_ptree( PT_NEXT_VARIABLE, "x", -1 );
    if (head2->right == NULL) {
        ERRPRINT( "Error while manually building parse tree for \""
                  REF_FORMULA2_SMALL "\"." );
        abort();
    }
    heads = malloc( 2*sizeof(ptree_t *) );
    *heads = head;
    *(heads+1) = head2;
    head = merge_ptrees( heads, 2, PT_AND );
    if (head == NULL) {
        ERRPRINT( "Error while conjuncting manually built subformulae \""
                  REF_FORMULA1_SMALL "\" and \"" REF_FORMULA2_SMALL "\"." );
        abort();
    }
    free( heads );

    if (tree_size( head ) != CONJUNCTED_SMALLS_SIZE) {
        ERRPRINT2( "Conjunction (from manually built subformulae) "
                   "of size %d detected as having wrong size %d.",
                   CONJUNCTED_SMALLS_SIZE, tree_size( head ) );
        abort();
    }

    /* Dump conjunction of manually built trees to a temporary file. */
    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ",  fseek" );
        abort();
    }
    if (ftruncate( fd, 0 )) {
        perror( __FILE__ ",  ftruncate" );
        abort();
    }
    inorder_trav( head, print_node, fp );
    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ",  fseek" );
        abort();
    }

    /* Read back in the result of dumping and compare to reference. */
    result = malloc( STRING_MAXLEN*sizeof(char) );
    if (result == NULL) {
        perror( __FILE__ ",  malloc" );
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
        ERRPRINT1( "expected formula string \"" DUMP_CONJUNCTED_SMALLS
                   "\" but got \"%s\".",
                   result );
        abort();
    }

    free( result );

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
        ERRPRINT2( "Stack-generated parse tree of size %d detected as "
                   "having wrong size %d.",
                   CONJUNCTED_SMALLS_SIZE, tree_size( head ) );
        abort();
    }

    /* Dump stack-generated parse tree to a temporary file. */
    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ",  fseek" );
        abort();
    }
    if (ftruncate( fd, 0 )) {
        perror( __FILE__ ",  ftruncate" );
        abort();
    }
    inorder_trav( head2, print_node, fp );
    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ",  fseek" );
        abort();
    }

    /* Read back in the result of dumping and compare to reference. */
    result = malloc( STRING_MAXLEN*sizeof(char) );
    if (result == NULL) {
        perror( __FILE__ ",  malloc" );
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
        ERRPRINT1( "expected formula string \"" DUMP_CONJUNCTED_SMALLS
                   "\" but got \"%s\".",
                   result );
        abort();
    }

    free( result );
    fclose( fp );
    if (remove( filename )) {
        perror( __FILE__ ",  remove" );
        abort();
    }

    /* Now compare trees directly; abort on discrepancy. */
    compare_ptrees( head, head2 );
    delete_tree( head2 );
    head2 = NULL;

    /* Several small tests of check_vars */
    var_list = init_ptree( PT_VARIABLE, "felix", -1 );
    append_list_item( var_list, PT_VARIABLE, "cat", -1 );
    primed_var_list = init_ptree( PT_VARIABLE, "the", -1 );
    result = check_vars( head, var_list, primed_var_list );
    if (result == NULL) {
        ERRPRINT( "check_vars() failed to catch extra variable: x'" );
        abort();
    }
    free( result );
    result = NULL;

    append_list_item( primed_var_list, PT_VARIABLE, "x", -1 );
    result = check_vars( head, var_list, primed_var_list );
    if (result != NULL) {
        ERRPRINT1( "check_vars() found unexpected variable: %s", result );
        abort();
    }
    delete_tree( var_list );
    delete_tree( primed_var_list );
    var_list = primed_var_list = NULL;


    /* Test copy_ptree. */
    head2 = copy_ptree( head );

    if (head == head2) {
        ERRPRINT( "copy_tree() did not provide new pointer." );
        abort();
    }
    compare_ptrees( head, head2 );

    delete_tree( head );
    delete_tree( head2 );

    /* Several trivial tests of finding min and max values in trees. */
    head = NULL;
    head = pusht_terminal( head, PT_CONSTANT, NULL, 1 );
    rv = min_tree_value( head );
    if (rv != 1) {
        ERRPRINT1( "min_tree_value() returned value %d, instead of expected 1",
                   rv );
        abort();
    }
    rv = max_tree_value( head );
    if (rv != 1) {
        ERRPRINT1( "max_tree_value() returned value %d, instead of expected 1",
                   rv );
        abort();
    }
    delete_tree( head );

    return 0;
}
