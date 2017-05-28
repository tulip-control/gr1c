/* Unit tests for patching algorithm implementations.
 *
 * SCL; 2012-2014.
 */

#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "tests_common.h"
#include "solve_support.h"
#include "ptree.h"
#include "automaton.h"
#include "patching.h"

specification_t spc;


#define STRING_MAXLEN 60
int main( int argc, char **argv )
{
    ptree_t *var_list = NULL, *node;
    int num_env, num_sys;
    DdManager *manager = NULL;
    DdNode *etrans = NULL, *strans = NULL, **egoals = NULL;
    anode_t **Exit = NULL, **Entry = NULL;
    int Exit_len, Entry_len;
    DdNode *N_BDD = NULL;
    anode_t *strategy = NULL;
    vartype state[2];

    int i;
    DdNode *tmp, *tmp2;  /* Temporary BDDs */
    DdNode **vars, **pvars;
    anode_t *start_node, *stop_node;

    SPC_INIT( spc );

    /*************************************************************
       # **Test fixture**

       ENV: x;
       SYS: y;

       ENVINIT: x;
       ENVTRANS:;
       ENVGOAL: []<>x;

       SYSINIT: y;
       SYSTRANS:;
       SYSGOAL: []<>y&x & []<>!y;
       # Note that system liveness is not applicable for synthesize_reachgame().
    *************************************************************/

    num_env = num_sys = 1;
    var_list = append_list_item( NULL, PT_VARIABLE, "x", -1 );
    if (var_list == NULL) {
        ERRPRINT( "failed to create new (type ptree) list of variables." );
        abort();
    }
    if (append_list_item( var_list, PT_VARIABLE, "y", -1 ) == NULL) {
        ERRPRINT( "failed to append variable to list (type ptree)." );
        abort();
    }
    if (tree_size( var_list ) != num_env+num_sys) {
        ERRPRINT2( "variable list (type ptree) expected size is %d, "
                   "but detected %d.",
                   num_env+num_sys,
                   tree_size( var_list ) );
        abort();
    }

    manager = Cudd_Init( 2*(num_env+num_sys),
                         0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_SetMaxCacheHard( manager, (unsigned int)-1 );
    Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );

    /* Define a map in the manager to easily swap variables with
       their primed selves. */
    vars = malloc( (num_env+num_sys)*sizeof(DdNode *) );
    pvars = malloc( (num_env+num_sys)*sizeof(DdNode *) );
    for (i = 0; i < num_env+num_sys; i++) {
        *(vars+i) = Cudd_bddIthVar( manager, i );
        *(pvars+i) = Cudd_bddIthVar( manager, i+num_env+num_sys );
    }
    if (!Cudd_SetVarMap( manager, vars, pvars, num_env+num_sys )) {
        ERRPRINT( "failed to define variable map in CUDD manager." );
        abort();
    }
    free( vars );
    free( pvars );

    etrans = Cudd_ReadOne( manager );
    Cudd_Ref( etrans );
    strans = Cudd_ReadOne( manager );
    Cudd_Ref( strans );

    spc.num_egoals = 1;
    egoals = malloc( spc.num_egoals*sizeof(DdNode *) );
    if (egoals == NULL) {
        perror( __FILE__ ",  malloc" );
        abort();
    }
    node = pusht_terminal( NULL, PT_VARIABLE, "x", -1 );
    *egoals = ptree_BDD( node, var_list, manager );

    /* In this test for synthesize_reachgame(), we do not patch an existing
       strategy.  Thus the nodes in the Entry and Exit sets are without
       predecessors.  In terms of states, Entry contains [0,0], and
       Exit contains [1,1]. */
    Entry_len = Exit_len = 1;
    Entry = malloc( sizeof(anode_t *)*Entry_len );
    if (Entry == NULL) {
        perror( __FILE__ ",  malloc" );
        abort();
    }
    Exit = malloc( sizeof(anode_t *)*Exit_len );
    if (Exit == NULL) {
        perror( __FILE__ ",  malloc" );
        abort();
    }

    state[0] = state[1] = 0;
    *Entry = insert_anode( NULL, 0, -1, False, state, num_env+num_sys );
    if (*Entry == NULL) {
        ERRPRINT2( "failed to create node without predecessors "
                   "and with state [%d, %d].",
                   state[0], state[1] );
        abort();
    }

    state[0] = state[1] = 1;
    *Exit = insert_anode( NULL, 0, -1, False, state, num_env+num_sys );
    if (*Exit == NULL) {
        ERRPRINT2( "failed to create node without predecessors "
                   "and with state [%d, %d].",
                   state[0], state[1] );
        abort();
    }

    /* N contains the states [1,1] and [0,0]. */
    N_BDD = Cudd_Not( Cudd_ReadOne( manager ) );
    Cudd_Ref( N_BDD );
    state[0] = state[1] = 0;
    tmp2 = state_to_BDD( manager, state, 0, num_env+num_sys );
    tmp = Cudd_bddOr( manager, N_BDD, tmp2 );
    Cudd_Ref( tmp );
    Cudd_RecursiveDeref( manager, N_BDD );
    Cudd_RecursiveDeref( manager, tmp2 );
    N_BDD = tmp;
    state[0] = state[1] = 1;
    tmp2 = state_to_BDD( manager, state, 0, num_env+num_sys );
    tmp = Cudd_bddOr( manager, N_BDD, tmp2 );
    Cudd_Ref( tmp );
    Cudd_RecursiveDeref( manager, N_BDD );
    Cudd_RecursiveDeref( manager, tmp2 );
    N_BDD = tmp;

    strategy = synthesize_reachgame( manager, num_env, num_sys,
                                     Entry, Entry_len, Exit, Exit_len,
                                     etrans, strans, egoals, N_BDD, 1 );

    if (strategy == NULL) {
        ERRPRINT( "failed to solve a small reachability game." );
        abort();
    }
    if (aut_size( strategy ) != 2) {
        ERRPRINT1( "local automaton has size %d but expected size 2.",
                   aut_size( strategy ) );
        abort();
    }

    /* Find node at which strategy for this small reachability game
       begins, and where it ends. */
    start_node = stop_node = NULL;
    if ((strategy->state[0] == 0 && strategy->state[0] == 0)
        && (strategy->next->state[0] == 1 && strategy->next->state[0] == 1)) {
        start_node = strategy;
        stop_node = strategy->next;
    } else if ((strategy->state[0] == 1 && strategy->state[0] == 1)
        && (strategy->next->state[0] == 0 && strategy->next->state[0] == 0)) {
        start_node = strategy->next;
        stop_node = strategy;
    } else {
        ERRPRINT( "local automaton contains unexpected node labelings." );
        abort();
    }

    /* Verify transitions */
    if (!(start_node->trans_len == 2
          && ((*(start_node->trans) == start_node
               && *(start_node->trans+1) == stop_node)
              || (*(start_node->trans) == stop_node
                  && *(start_node->trans+1) == start_node)))) {
        ERRPRINT( "local automaton start node has unexpected transitions." );
        abort();
    }
    if (stop_node->trans_len != 0) {
        ERRPRINT( "local automaton end node has unexpected transitions." );
        abort();
    }

    /* Clean-up and check for leaks */
    Cudd_RecursiveDeref( manager, etrans );
    Cudd_RecursiveDeref( manager, strans );
    for (i = 0; i < spc.num_egoals; i++)
        Cudd_RecursiveDeref( manager, *(egoals+i) );
    free( egoals );
    Cudd_RecursiveDeref( manager, N_BDD );
    if (Cudd_CheckZeroRef( manager ) != 0) {
        ERRPRINT1( "Leaked BDD references; Cudd_CheckZeroRef -> %d.",
                   Cudd_CheckZeroRef( manager ) );
        abort();
    }
    Cudd_Quit( manager );
    delete_tree( var_list );
    delete_aut( strategy );
    for (i = 0; i < Entry_len; i++)
        delete_aut( *(Entry+i) );
    free( Entry );
    for (i = 0; i < Exit_len; i++)
        delete_aut( *(Exit+i) );
    free( Exit );
    return 0;
}
