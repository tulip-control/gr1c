/* Unit tests for conversion of formula parse trees to BDDs.
 *
 * SCL; 2012, 2015
 */

#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "ptree.h"
#include "tests_common.h"


int main( int argc, char **argv )
{
    DdManager *manager;
    DdNode *f;  /* Boolean formula (BDD) */
    DdNode *ddval;  /* Store result of evaluating a BDD */
    int *cube;
    char manual_eval;
    ptree_t *var_list;
    ptree_t *head;
    int i; /* Generic counter */

    /* Test fixture */
    var_list = append_list_item( NULL, PT_VARIABLE, "a", -1 );
    append_list_item( var_list, PT_VARIABLE, "b", -1 );
    append_list_item( var_list, PT_VARIABLE, "c", -1 );
    if (tree_size( var_list ) != 3) {
        ERRPRINT1( "List of 3 variables detected as having wrong size %d.",
                   tree_size( var_list ) );
        abort();
    }
    manager = Cudd_Init( tree_size( var_list ),
                         0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_SetMaxCacheHard( manager, (unsigned int)-1 );
    Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );


    /************************************************
     * (a & b) | !c
     ************************************************/
    head = NULL;
    head = pusht_terminal( head, PT_VARIABLE, "a", -1 );
    head = pusht_terminal( head, PT_VARIABLE, "b", -1 );
    head = pusht_operator( head, PT_AND );
    head = pusht_terminal( head, PT_VARIABLE, "c", -1 );
    head = pusht_operator( head, PT_NEG );
    head = pusht_operator( head, PT_OR );
    f = ptree_BDD( head, var_list, manager );
    cube = malloc( 3*sizeof(int) );
    if (cube == NULL) {
        perror( __FILE__ ",  malloc" );
        abort();
    }
    for (i = 0; i < 8; i++) {
        *cube = i&1;
        *(cube+1) = (i >> 1)&1;
        *(cube+2) = (i >> 2)&1;
        manual_eval = (((*cube)&(*(cube+1))) | !(*(cube+2)))&1;
        ddval = Cudd_Eval( manager, f, cube );
        if ((Cudd_IsComplement( ddval ) && manual_eval)
            || (!Cudd_IsComplement( ddval ) && !manual_eval)) {
            ERRPRINT( "BDD generated from parse tree of \"(a & b) | !c\" "
                      "gave incorrect output."  );
            abort();
        }
    }
    free( cube );
    Cudd_RecursiveDeref( manager, f );


    if (Cudd_CheckZeroRef( manager ) != 0) {
        ERRPRINT1( "Leaked BDD references; Cudd_CheckZeroRef -> %d.",
                   Cudd_CheckZeroRef( manager ) );
        abort();
    }
    Cudd_Quit( manager );
    delete_tree( var_list );
    return 0;
}
