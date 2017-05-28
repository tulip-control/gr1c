/* Unit tests for synthesis support routines.
 *
 * SCL; May 2012, April 2013.
 */

#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "ptree.h"
#include "automaton.h"
#include "solve_support.h"
#include "tests_common.h"


/* Abort on discrepancy. */
void compare_bcubes( vartype *cube1, vartype *cube2, int len )
{
    int i;
    for (i = 0; i < len; i++) {
        if (*(cube1+i) != *(cube2+i)) {
            ERRPRINT( "Given boolean cubes do not match." );
            abort();
        }
    }
}


int main( int argc, char **argv )
{
    vartype *ref_cube, *state;
    int *gcube;
    int len;
    ptree_t *head;

    DdManager *manager;
    DdNode *etrans;
    ptree_t *var_list;
    int num_env, num_sys;
    int *cube;
    int i, j;  /* Generic counters */
    int move_counter;
    vartype **env_moves;
    int emoves_len;

    /* Repeatable random seed */
    srand( 0 );


    /************************************************
     * Small, not otherwise specified
     ************************************************/
    len = 1024;
    state = malloc( len*sizeof(vartype) );
    if (state == NULL) {
        perror( __FILE__ ",  malloc" );
        abort();
    }
    ref_cube = malloc( len*sizeof(vartype) );
    if (ref_cube == NULL) {
        perror( __FILE__ ",  malloc" );
        abort();
    }

    for (i = 0; i < len; i++)
        *(state+i) = rand()%2;

    if (!statecmp( state, state, len )) {
        ERRPRINT( "state vector failed equality test with itself." );
        for (i = 0; i < len; i++)
            printf( "%d", *(state+i) );
        printf( "\n" );
        abort();
    }

    for (i = 0; i < len; i++)
        *(ref_cube+i) = 0;

    if (statecmp( state, ref_cube, len )) {
        ERRPRINT( "different state vectors incorrectly detected as equal." );
        for (i = 0; i < len; i++)
            printf( "%d", *(state+i) );
        printf( "\n\n" );
        for (i = 0; i < len; i++)
            printf( "%d", *(ref_cube+i) );
        printf( "\n" );
        abort();
    }

    free( state );
    free( ref_cube );


    /************************************************
     * Cube walking
     ************************************************/
    len = 4;
    state = malloc( len*sizeof(vartype) );
    if (state == NULL) {
        perror( __FILE__ ",  malloc" );
        abort();
    }
    ref_cube = malloc( len*sizeof(vartype) );
    if (ref_cube == NULL) {
        perror( __FILE__ ",  malloc" );
        abort();
    }
    gcube = malloc( len*sizeof(int) );
    if (gcube == NULL) {
        perror( __FILE__ ",  malloc" );
        abort();
    }

    /* gcube = [0, 2, 0, 2] */
    *gcube = *(gcube+2) = 0;
    *(gcube+1) = *(gcube+3) = 2;

    initialize_cube( state, gcube, len );
    *ref_cube = *(ref_cube+1) = *(ref_cube+2) = *(ref_cube+3) = 0;
    compare_bcubes( state, ref_cube, len );
    /* Should be [0,0,0,0] */

    if (saturated_cube( state, gcube, len )) {
        ERRPRINT( "Boolean cube unexpectedly detected as saturated." );
        abort();
    }

    increment_cube( state, gcube, len );
    *(ref_cube+3) = 1;
    compare_bcubes( state, ref_cube, len );
    /* Should be [0,0,0,1] */

    increment_cube( state, gcube, len );
    *(ref_cube+3) = 0;
    *(ref_cube+1) = 1;
    compare_bcubes( state, ref_cube, len );
    /* Should be [0,1,0,0] */

    increment_cube( state, gcube, len );
    *(ref_cube+3) = 1;
    compare_bcubes( state, ref_cube, len );
    /* Should be [0,1,0,1] and saturated */

    if (!saturated_cube( state, gcube, len )) {
        ERRPRINT( "Boolean cube unexpectedly detected as not saturated." );
        abort();
    }

    free( ref_cube );
    free( state );
    free( gcube );


    /************************************************
     * Environment (uncontrolled) move enumeration
     ************************************************/

    /* Test fixture */
    num_env = 3;
    num_sys = 1;
    var_list = append_list_item( NULL, PT_VARIABLE, "x1", -1 );
    append_list_item( var_list, PT_VARIABLE, "x2", -1 );
    append_list_item( var_list, PT_VARIABLE, "x3", -1 );
    append_list_item( var_list, PT_VARIABLE, "y", -1 );
    if (tree_size( var_list ) != 4) {
        ERRPRINT1( "List of 4 variables detected as having wrong size %d.",
                   tree_size( var_list ) );
        abort();
    }
    manager = Cudd_Init( 2*tree_size( var_list ),
                         0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_SetMaxCacheHard( manager, (unsigned int)-1 );
    Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );

    cube = malloc( sizeof(int)*2*(num_env+num_sys) );
    if (cube == NULL) {
        perror( __FILE__ ",  malloc" );
        abort();
    }

    state = malloc( sizeof(vartype)*(num_env+num_sys) );
    if (state == NULL) {
        perror( __FILE__ ",  malloc" );
        abort();
    }

    /* Single environment transition rule (safety formula):  [](y -> !x1') */
    head = NULL;
    head = pusht_terminal( head, PT_VARIABLE, "y", -1 );
    head = pusht_operator( head, PT_NEG );
    head = pusht_terminal( head, PT_NEXT_VARIABLE, "x1", -1 );
    head = pusht_operator( head, PT_NEG );
    head = pusht_operator( head, PT_OR );
    etrans = ptree_BDD( head, var_list, manager );
    delete_tree( head );
    head = NULL;

    /* First state is zero for all variables */
    /* COMMENT: a possible weakness in the checking approach used here
       is that we assume the environment moves will be generated in a
       particular order, despite this *not* being an explicit
       post-condition of get_env_moves. */
    for (i = 0; i < num_env+num_sys; i++)
        *(state+i) = 0;
    env_moves = get_env_moves( manager, cube, state, etrans,
                               num_env, num_sys, &emoves_len );
    if (emoves_len != 8) {
        ERRPRINT1( "Expected 8 possible environment moves, but detected %d.",
                   emoves_len );
        abort();
    }
    move_counter = 0;
    for (i = 0; i < emoves_len; i++) {
        for (j = 0; j < num_env; j++)
            *(state+num_env-1-j) = (move_counter>>j)&1;
        move_counter++;
        compare_bcubes( state, *(env_moves+i), num_env );
    }

    /* Second state is system (controlled) y=1, all other variables zero. */
    for (i = 0; i < num_env+num_sys-1; i++)
        *(state+i) = 0;
    *(state+num_env+num_sys-1) = 1;
    env_moves = get_env_moves( manager, cube, state, etrans,
                               num_env, num_sys, &emoves_len );
    if (emoves_len != 4) {
        ERRPRINT1( "Expected 4 possible environment moves, but detected %d.",
                   emoves_len );
        abort();
    }
    move_counter = 0;
    *state = 0;
    for (i = 0; i < emoves_len; i++) {
        for (j = 0; j < num_env-1; j++)
            *(state+num_env-1-j) = (move_counter>>j)&1;
        move_counter++;
        compare_bcubes( state, *(env_moves+i), num_env );
    }

    Cudd_RecursiveDeref( manager, etrans );
    if (Cudd_CheckZeroRef( manager ) != 0) {
        ERRPRINT1( "Leaked BDD references; Cudd_CheckZeroRef -> %d.",
                   Cudd_CheckZeroRef( manager ) );
        abort();
    }
    Cudd_Quit( manager );
    free( cube );
    free( state );
    delete_tree( var_list );
    return 0;
}
