/* solve_support.c -- Definitions for signatures appearing in solve_support.h.
 *
 *
 * SCL; 2012-2015
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "solve_support.h"


int read_state_str( char *input, vartype **state, int max_len )
{
    int i;
    char *start;
    char *end;
    *state = NULL;
    if (max_len == 0 || strlen( input ) < 1)
        return 0;
    if (max_len > 0) {
        *state = malloc( sizeof(vartype)*max_len );
        if (*state == NULL) {
            perror( __FILE__ ",  malloc" );
            exit(-1);
        }
    }

    start = input;
    end = input;
    for (i = 0; (max_len < 0 || i < max_len) && *end != '\0'; i++) {
        if (max_len < 0) {
            *state = realloc( *state, sizeof(vartype)*(i+1) );
            if (*state == NULL) {
                perror( __FILE__ ",  realloc" );
                exit(-1);
            }
        }
        *((*state)+i) = strtol( start, &end, 10 );
        if (start == end)
            break;
        start = end;
    }

    if (i == 0) {
        free( *state );
        *state = NULL;
        return 0;
    }

    if (max_len > 0) {
        *state = realloc( *state, sizeof(vartype)*i );
        if (*state == NULL) {
            perror( __FILE__ ",  realloc" );
            exit(-1);
        }
    }
    return i;
}


void initialize_cube( vartype *cube, int *gcube, int len )
{
    int i;
    for (i = 0; i < len; i++) {
        if (*(gcube+i) == 2) {
            *(cube+i) = 0;
        } else {
            *(cube+i) = *(gcube+i);
        }
    }
}

void increment_cube( vartype *cube, int *gcube, int len )
{
    int i;
    for (i = len-1; i >= 0; i--) {
        if (*(gcube+i) == 2) {
            (*(cube+i))++;
            if (*(cube+i) > 1) {
                *(cube+i) = 0;
            } else {
                return;
            }
        }
    }
}

bool saturated_cube( vartype *cube, int *gcube, int len )
{
    int i;
    for (i = 0; i < len; i++) {
        if (*(gcube+i) == 2 && *(cube+i) == 0)
            return False;
    }
    return True;
}

void state_to_cube( vartype *state, int *cube, int len )
{
    int i;
    for (i = 0; i < len; i++)
        *(cube+i) = *(state+i);
    for (i = len; i < 2*len; i++)
        *(cube+i) = 2;
}


void cube_env( int *cube, int num_env, int num_sys )
{
    int i;
    for (i = num_env; i < 2*(num_env+num_sys); i++)
        *(cube+i) = 2;

    for (i = 0; i < num_env; i++)
        *(cube+i) = 1;
}

void cube_sys( int *cube, int num_env, int num_sys )
{
    int i;
    for (i = 0; i < 2*(num_env+num_sys); i++)
        *(cube+i) = 2;

    for (i = num_env; i < num_env+num_sys; i++)
        *(cube+i) = 1;
}

void cube_prime_env( int *cube, int num_env, int num_sys )
{
    int i;
    for (i = 0; i < 2*(num_env+num_sys); i++)
        *(cube+i) = 2;

    for (i = num_env+num_sys; i < 2*num_env+num_sys; i++)
        *(cube+i) = 1;
}

void cube_prime_sys( int *cube, int num_env, int num_sys )
{
    int i;
    for (i = 0; i < 2*num_env+num_sys; i++)
        *(cube+i) = 2;

    for (i = 2*num_env+num_sys; i < 2*(num_env+num_sys); i++)
        *(cube+i) = 1;
}


bool statecmp( vartype *state1, vartype *state2, int state_len )
{
    int i;
    for (i = 0; i < state_len; i++) {
        if (*(state1+i) != *(state2+i))
            return False;
    }
    return True;
}


DdNode *state_to_cof( DdManager *manager, int *cube, int cube_len,
                   vartype *state, DdNode *trans, int offset, int len )
{
    int i;
    DdNode *tmp;
    DdNode *ddcube;

    for (i = 0; i < cube_len; i++)
        *(cube+i) = 2;
    for (i = 0; i < len; i++)
        *(cube+offset+i) = *(state+i);

    ddcube = Cudd_CubeArrayToBdd( manager, cube );
    if (ddcube == NULL) {
        fprintf( stderr, "Error in generating cube for cofactor." );
        return NULL;
    }
    Cudd_Ref( ddcube );

    tmp = Cudd_Cofactor( manager, trans, ddcube );
    if (tmp == NULL) {
        fprintf( stderr, "Error in computing cofactor." );
        return NULL;
    }
    Cudd_Ref( tmp );
    Cudd_RecursiveDeref( manager, ddcube );
    return tmp;
}


DdNode *state_to_BDD( DdManager *manager, vartype *state, int offset, int len )
{
    DdNode *v, *tmp;
    int i;
    v = Cudd_ReadOne( manager );
    Cudd_Ref( v );
    for (i = 0; i < len; i++) {
        if (*(state+i)) {
            tmp = Cudd_bddAnd( manager, v,
                               Cudd_bddIthVar( manager, offset+i ) );
        } else {
            tmp = Cudd_bddAnd( manager, v,
                               Cudd_Not(Cudd_bddIthVar( manager, offset+i )) );
        }
        Cudd_Ref( tmp );
        Cudd_RecursiveDeref( manager, v );
        v = tmp;
    }
    return v;
}


vartype **get_env_moves( DdManager *manager, int *cube,
                         vartype *state, DdNode *etrans,
                         int num_env, int num_sys, int *emoves_len )
{
    DdNode *tmp, *tmp2, *ddcube;
    vartype **env_moves = NULL;
    DdGen *gen;
    CUDD_VALUE_TYPE gvalue;
    int *gcube;
    int i;

    tmp = state_to_cof( manager, cube, 2*(num_env+num_sys),
                     state, etrans, 0, num_env+num_sys );
    cube_prime_sys( cube, num_env, num_sys );
    ddcube = Cudd_CubeArrayToBdd( manager, cube );
    if (ddcube == NULL) {
        fprintf( stderr, "Error in generating cube for quantification." );
        return NULL;
    }
    Cudd_Ref( ddcube );
    tmp2 = Cudd_bddExistAbstract( manager, tmp, ddcube );
    if (tmp2 == NULL) {
        fprintf( stderr, "Error in performing quantification." );
        return NULL;
    }
    Cudd_Ref( tmp2 );
    Cudd_RecursiveDeref( manager, tmp );
    Cudd_RecursiveDeref( manager, ddcube );

    state = malloc( num_env*sizeof(vartype) );
    if (state == NULL) {
        fprintf( stderr, "Error in building next environment moves list." );
        return NULL;
    }

    *emoves_len = 0;
    Cudd_AutodynDisable( manager );
    Cudd_ForeachCube( manager, tmp2, gen, gcube, gvalue ) {
        initialize_cube( state, gcube+num_env+num_sys, num_env );
        while (!saturated_cube( state, gcube+num_env+num_sys, num_env )) {
            (*emoves_len)++;
            env_moves = realloc( env_moves, (*emoves_len)*sizeof(vartype *) );
            if (env_moves == NULL) {
                fprintf( stderr,
                         "Error in building next environment moves list." );
                return NULL;
            }
            *(env_moves+*emoves_len-1) = malloc( num_env*sizeof(vartype) );
            if (*(env_moves+*emoves_len-1) == NULL) {
                fprintf( stderr,
                         "Error in building next environment moves list." );
                return NULL;
            }

            for (i = 0; i < num_env; i++)
                *(*(env_moves+*emoves_len-1)+i) = *(state+i);

            increment_cube( state, gcube+num_env+num_sys, num_env );
        }
        (*emoves_len)++;
        env_moves = realloc( env_moves, (*emoves_len)*sizeof(vartype *) );
        if (env_moves == NULL) {
            fprintf( stderr, "Error in building next environment moves list." );
            return NULL;
        }
        *(env_moves+*emoves_len-1) = malloc( num_env*sizeof(vartype) );
        if (*(env_moves+*emoves_len-1) == NULL) {
            fprintf( stderr, "Error in building next environment moves list." );
            return NULL;
        }

        for (i = 0; i < num_env; i++)
            *(*(env_moves+*emoves_len-1)+i) = *(state+i);
    }
    free( state );
    Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
    Cudd_RecursiveDeref( manager, tmp2 );
    return env_moves;
}


/* Compute exists modal operator applied to set C. */
DdNode *compute_existsmodal( DdManager *manager, DdNode *C,
                             DdNode *etrans, DdNode *strans,
                             int num_env, int num_sys, int *cube )
{
    DdNode *tmp, *tmp2;
    DdNode *ddcube;

    C = Cudd_bddVarMap( manager, C );
    if (C == NULL) {
        fprintf( stderr,
                 "compute_existsmodal: Error in swapping variables with"
                 " primed forms." );
        return NULL;
    }
    Cudd_Ref( C );

    tmp = Cudd_bddAnd( manager, strans, C );
    Cudd_Ref( tmp );
    Cudd_RecursiveDeref( manager, C );
    cube_prime_sys( cube, num_env, num_sys );
    ddcube = Cudd_CubeArrayToBdd( manager, cube );
    if (ddcube == NULL) {
        fprintf( stderr,
                 "compute_existsmodal: Error in generating cube for"
                 " quantification." );
        return NULL;
    }
    Cudd_Ref( ddcube );
    tmp2 = Cudd_bddExistAbstract( manager, tmp, ddcube );
    if (tmp2 == NULL) {
        fprintf( stderr,
                 "compute_existsmodal: Error in performing quantification." );
        return NULL;
    }
    Cudd_Ref( tmp2 );
    Cudd_RecursiveDeref( manager, ddcube );
    Cudd_RecursiveDeref( manager, tmp );

    tmp = Cudd_bddOr( manager, Cudd_Not( etrans ), tmp2 );
    Cudd_Ref( tmp );
    Cudd_RecursiveDeref( manager, tmp2 );
    cube_prime_env( cube, num_env, num_sys );
    ddcube = Cudd_CubeArrayToBdd( manager, cube );
    if (ddcube == NULL) {
        fprintf( stderr,
                 "compute_existsmodal: Error in generating cube for"
                 " quantification." );
        return NULL;
    }
    Cudd_Ref( ddcube );
    tmp2 = Cudd_bddUnivAbstract( manager, tmp, ddcube );
    if (tmp2 == NULL) {
        fprintf( stderr,
                 "compute_existsmodal: Error in performing quantification." );
        return NULL;
    }
    Cudd_Ref( tmp2 );
    Cudd_RecursiveDeref( manager, ddcube );
    Cudd_RecursiveDeref( manager, tmp );
    return tmp2;
}
