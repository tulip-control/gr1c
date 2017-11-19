/* solve_operators.c -- Functions for several mu-calculus operations.
 *                      Also consider solve.c
 *
 *
 * SCL; 2012-2015
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "logging.h"
#include "solve.h"
#include "solve_support.h"


extern specification_t spc;


DdNode *compute_winning_set( DdManager *manager, unsigned char verbose )
{
    int i;
    ptree_t *var_separator;
    DdNode *W;  /* Characteristic function of winning set */
    DdNode *etrans, *strans, **egoals, **sgoals;
    bool env_nogoal_flag = False;  /* Indicate environment has no goals */

    /* Set environment goal to True (i.e., any state) if none was
       given. This simplifies the implementation below. */
    if (spc.num_egoals == 0) {
        env_nogoal_flag = True;
        spc.num_egoals = 1;
        spc.env_goals = malloc( sizeof(ptree_t *) );
        *spc.env_goals = init_ptree( PT_CONSTANT, NULL, 1 );
    }

    /* Chain together environment and system variable lists for
       working with BDD library. */
    if (spc.evar_list == NULL) {
        var_separator = NULL;
        spc.evar_list = spc.svar_list;  /* that this is the deterministic case
                                           is indicated by var_separator = NULL. */
    } else {
        var_separator = get_list_item( spc.evar_list, -1 );
        if (var_separator == NULL) {
            fprintf( stderr,
                     "Error: get_list_item failed on environment variables"
                     " list.\n" );
            return NULL;
        }
        var_separator->left = spc.svar_list;
    }

    /* Generate BDDs for the various parse trees from the problem spec. */
    if (verbose > 1)
        logprint( "Building environment transition BDD..." );
    etrans = ptree_BDD( spc.env_trans, spc.evar_list, manager );
    if (verbose > 1) {
        logprint( "Done." );
        logprint( "Building system transition BDD..." );
    }
    strans = ptree_BDD( spc.sys_trans, spc.evar_list, manager );
    if (verbose > 1)
        logprint( "Done." );

    /* Build goal BDDs, if present. */
    if (spc.num_egoals > 0) {
        egoals = malloc( spc.num_egoals*sizeof(DdNode *) );
        for (i = 0; i < spc.num_egoals; i++)
            *(egoals+i) = ptree_BDD( *(spc.env_goals+i), spc.evar_list, manager );
    } else {
        egoals = NULL;
    }
    if (spc.num_sgoals > 0) {
        sgoals = malloc( spc.num_sgoals*sizeof(DdNode *) );
        for (i = 0; i < spc.num_sgoals; i++)
            *(sgoals+i) = ptree_BDD( *(spc.sys_goals+i), spc.evar_list, manager );
    } else {
        sgoals = NULL;
    }

    /* Break the link that appended the system variables list to the
       environment variables list. */
    if (var_separator == NULL) {
        spc.evar_list = NULL;
    } else {
        var_separator->left = NULL;
    }

    W = compute_winning_set_BDD( manager, etrans, strans, egoals, sgoals,
                                 verbose );

    Cudd_RecursiveDeref( manager, etrans );
    Cudd_RecursiveDeref( manager, strans );
    for (i = 0; i < spc.num_egoals; i++)
        Cudd_RecursiveDeref( manager, *(egoals+i) );
    for (i = 0; i < spc.num_sgoals; i++)
        Cudd_RecursiveDeref( manager, *(sgoals+i) );
    if (spc.num_egoals > 0)
        free( egoals );
    if (spc.num_sgoals > 0)
        free( sgoals );
    if (env_nogoal_flag) {
        spc.num_egoals = 0;
        delete_tree( *spc.env_goals );
        free( spc.env_goals );
    }

    return W;
}


DdNode *compute_winning_set_BDD( DdManager *manager,
                                 DdNode *etrans, DdNode *strans,
                                 DdNode **egoals, DdNode **sgoals,
                                 unsigned char verbose )
{
    DdNode *X = NULL, *X_prev = NULL;
    DdNode *Y = NULL, *Y_exmod = NULL, *Y_prev = NULL;
    DdNode **Z = NULL, **Z_prev = NULL;
    bool Z_changed;  /* Use to detect occurrence of fixpoint for all Z_i */

    /* Fixpoint iteration counters */
    int num_it_Z, num_it_Y, num_it_X;

    DdNode *tmp, *tmp2;
    int i, j;  /* Generic counters */

    DdNode **vars, **pvars;
    int num_env, num_sys;
    int *cube;  /* length will be twice total number of variables (to
                   account for both variables and their primes). */

    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

    /* Allocate cube array, used later for quantifying over variables. */
    cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
    if (cube == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    /* Define a map in the manager to easily swap variables with their
       primed selves. */
    vars = malloc( (num_env+num_sys)*sizeof(DdNode *) );
    pvars = malloc( (num_env+num_sys)*sizeof(DdNode *) );
    for (i = 0; i < num_env+num_sys; i++) {
        *(vars+i) = Cudd_bddIthVar( manager, i );
        *(pvars+i) = Cudd_bddIthVar( manager, i+num_env+num_sys );
    }
    if (!Cudd_SetVarMap( manager, vars, pvars, num_env+num_sys )) {
        fprintf( stderr,
                 "Error: failed to define variable map in CUDD manager.\n" );
        free( cube );
        return NULL;
    }
    free( vars );
    free( pvars );

    if (spc.num_sgoals > 0) {
        Z = malloc( spc.num_sgoals*sizeof(DdNode *) );
        Z_prev = malloc( spc.num_sgoals*sizeof(DdNode *) );
        for (i = 0; i < spc.num_sgoals; i++) {
            *(Z+i) = NULL;
            *(Z_prev+i) = NULL;
        }
    }

    /* Initialize */
    for (i = 0; i < spc.num_sgoals; i++) {
        *(Z+i) = Cudd_ReadOne( manager );
        Cudd_Ref( *(Z+i) );
    }

    num_it_Z = 0;
    do {
        num_it_Z++;
        if (verbose > 1) {
            logprint( "Z iteration %d", num_it_Z );
            logprint( "Cudd_ReadMemoryInUse (bytes): %d",
                      Cudd_ReadMemoryInUse( manager ) );
        }

        for (i = 0; i < spc.num_sgoals; i++) {
            if (*(Z_prev+i) != NULL)
                Cudd_RecursiveDeref( manager, *(Z_prev+i) );
            *(Z_prev+i) = *(Z+i);
        }

        for (i = 0; i < spc.num_sgoals; i++) {
            if (i == spc.num_sgoals-1) {
                *(Z+i) = compute_existsmodal( manager, *Z_prev, etrans, strans,
                                              num_env, num_sys, cube );
            } else {
                *(Z+i) = compute_existsmodal( manager, *(Z_prev+i+1),
                                              etrans, strans, num_env, num_sys,
                                              cube );
            }
            if (*(Z+i) == NULL) {
                /* fatal error */
                return NULL;
            }

            /* (Re)initialize Y */
            if (Y != NULL)
                Cudd_RecursiveDeref( manager, Y );
            Y = Cudd_Not( Cudd_ReadOne( manager ) );
            Cudd_Ref( Y );

            num_it_Y = 0;
            do {
                num_it_Y++;
                if (verbose > 1) {
                    logprint( "\tY iteration %d", num_it_Y );
                    logprint( "\tCudd_ReadMemoryInUse (bytes): %d",
                              Cudd_ReadMemoryInUse( manager ) );
                }

                if (Y_prev != NULL)
                    Cudd_RecursiveDeref( manager, Y_prev );
                Y_prev = Y;
                if (Y_exmod != NULL)
                    Cudd_RecursiveDeref( manager, Y_exmod );
                Y_exmod = compute_existsmodal( manager, Y_prev, etrans, strans,
                                               num_env, num_sys, cube );
                if (Y_exmod == NULL) {
                    /* fatal error */
                    return NULL;
                }

                Y = Cudd_Not( Cudd_ReadOne( manager ) );
                Cudd_Ref( Y );
                for (j = 0; j < spc.num_egoals; j++) {

                    /* (Re)initialize X */
                    if (X != NULL)
                        Cudd_RecursiveDeref( manager, X );
                    X = Cudd_ReadOne( manager );
                    Cudd_Ref( X );

                    /* Greatest fixpoint for X, for this env goal */
                    num_it_X = 0;
                    do {
                        num_it_X++;
                        if (verbose > 1) {
                            logprint( "\t\tX iteration %d", num_it_X );
                            logprint( "\t\tCudd_ReadMemoryInUse (bytes): %d",
                                      Cudd_ReadMemoryInUse( manager ) );
                        }

                        if (X_prev != NULL)
                            Cudd_RecursiveDeref( manager, X_prev );
                        X_prev = X;
                        X = compute_existsmodal( manager, X_prev,
                                                 etrans, strans,
                                                 num_env, num_sys, cube );
                        if (X == NULL) {
                            /* fatal error */
                            return NULL;
                        }

                        tmp = Cudd_bddAnd( manager, *(sgoals+i), *(Z+i) );
                        Cudd_Ref( tmp );
                        tmp2 = Cudd_bddOr( manager, tmp, Y_exmod );
                        Cudd_Ref( tmp2 );
                        Cudd_RecursiveDeref( manager, tmp );

                        tmp = Cudd_bddAnd( manager,
                                           X, Cudd_Not( *(egoals+j) ) );
                        Cudd_Ref( tmp );
                        Cudd_RecursiveDeref( manager, X );

                        X = Cudd_bddOr( manager, tmp2, tmp );
                        Cudd_Ref( X );
                        Cudd_RecursiveDeref( manager, tmp );
                        Cudd_RecursiveDeref( manager, tmp2 );

                        tmp = X;
                        X = Cudd_bddAnd( manager, X, X_prev );
                        Cudd_Ref( X );
                        Cudd_RecursiveDeref( manager, tmp );

                    } while (!Cudd_bddLeq( manager, X, X_prev )
                             || !Cudd_bddLeq( manager, X_prev, X ));

                    tmp = Y;
                    Y = Cudd_bddOr( manager, Y, X );
                    Cudd_Ref( Y );
                    Cudd_RecursiveDeref( manager, tmp );

                    Cudd_RecursiveDeref( manager, X );
                    X = NULL;
                    Cudd_RecursiveDeref( manager, X_prev );
                    X_prev = NULL;
                }

                tmp2 = Y;
                Y = Cudd_bddOr( manager, Y, Y_prev );
                Cudd_Ref( Y );
                Cudd_RecursiveDeref( manager, tmp2 );

            } while (!Cudd_bddLeq( manager, Y, Y_prev )
                     || !Cudd_bddLeq( manager, Y_prev, Y ));

            Cudd_RecursiveDeref( manager, *(Z+i) );
            *(Z+i) = Cudd_bddAnd( manager, Y, *(Z_prev+i) );
            Cudd_Ref( *(Z+i) );

            Cudd_RecursiveDeref( manager, Y );
            Y = NULL;
            Cudd_RecursiveDeref( manager, Y_prev );
            Y_prev = NULL;
            Cudd_RecursiveDeref( manager, Y_exmod );
            Y_exmod = NULL;

        }

        Z_changed = False;
        for (i = 0; i < spc.num_sgoals; i++) {
            if (!Cudd_bddLeq( manager, *(Z+i), *(Z_prev+i) )
                || !Cudd_bddLeq( manager, *(Z_prev+i), *(Z+i) )) {
                Z_changed = True;
                break;
            }
        }
    } while (Z_changed);

    /* Pre-exit clean-up */
    tmp = *Z;
    Cudd_RecursiveDeref( manager, *Z_prev );
    for (i = 1; i < spc.num_sgoals; i++) {
        Cudd_RecursiveDeref( manager, *(Z+i) );
        Cudd_RecursiveDeref( manager, *(Z_prev+i) );
    }
    free( Z );
    free( Z_prev );
    free( cube );

    return tmp;
}


DdNode ***compute_sublevel_sets( DdManager *manager,
                                 DdNode *W,
                                 DdNode *etrans, DdNode *strans,
                                 DdNode **egoals, int num_env_goals,
                                 DdNode **sgoals, int num_sys_goals,
                                 int **num_sublevels,
                                 DdNode *****X_ijr,
                                 unsigned char verbose )
{
    DdNode ***Y = NULL, *Y_exmod = NULL;
    DdNode *X = NULL, *X_prev = NULL;

    DdNode **vars, **pvars;
    int num_env, num_sys;
    int *cube;

    DdNode *tmp, *tmp2;
    int i, r;

    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

    cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
    if (cube == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    /* Define a map in the manager to easily swap variables with their
       primed selves. */
    vars = malloc( (num_env+num_sys)*sizeof(DdNode *) );
    pvars = malloc( (num_env+num_sys)*sizeof(DdNode *) );
    for (i = 0; i < num_env+num_sys; i++) {
        *(vars+i) = Cudd_bddIthVar( manager, i );
        *(pvars+i) = Cudd_bddIthVar( manager, i+num_env+num_sys );
    }
    if (!Cudd_SetVarMap( manager, vars, pvars, num_env+num_sys )) {
        fprintf( stderr,
                 "Error: failed to define variable map in CUDD manager.\n" );
        free( cube );
        return NULL;
    }
    free( vars );
    free( pvars );

    if (num_sys_goals > 0) {
        Y = malloc( num_sys_goals*sizeof(DdNode **) );
        *num_sublevels = malloc( num_sys_goals*sizeof(int) );
        if (Y == NULL || *num_sublevels == NULL) {
            perror( __FILE__ ",  malloc" );
            exit(-1);
        }
        *X_ijr = malloc( num_sys_goals*sizeof(DdNode ***) );
        if (*X_ijr == NULL) {
            perror( __FILE__ ",  malloc" );
            exit(-1);
        }

        for (i = 0; i < num_sys_goals; i++) {
            *(*num_sublevels+i) = 1;
            *(Y+i) = malloc( *(*num_sublevels+i)*sizeof(DdNode *) );
            if (*(Y+i) == NULL) {
                perror( __FILE__ ",  malloc" );
                exit(-1);
            }
            **(Y+i) = Cudd_Not( Cudd_ReadOne( manager ) );
            Cudd_Ref( **(Y+i) );

            *(*X_ijr+i) = malloc( *(*num_sublevels+i)*sizeof(DdNode **) );
            if (*(*X_ijr+i) == NULL) {
                perror( __FILE__ ",  malloc" );
                exit(-1);
            }
            **(*X_ijr+i) = malloc( num_env_goals*sizeof(DdNode *) );
            if (**(*X_ijr+i) == NULL) {
                perror( __FILE__ ",  malloc" );
                exit(-1);
            }
            for (r = 0; r < num_env_goals; r++) {
                *(**(*X_ijr+i) + r) = Cudd_Not( Cudd_ReadOne( manager ) );
                Cudd_Ref( *(**(*X_ijr+i) + r) );
            }
        }
    } else {
        free( cube );
        return NULL;
    }

    /* Build list of Y_i sets from iterations of the fixpoint formula. */
    for (i = 0; i < num_sys_goals; i++) {
        while (True) {
            (*(*num_sublevels+i))++;
            *(Y+i) = realloc( *(Y+i), *(*num_sublevels+i)*sizeof(DdNode *) );
            *(*X_ijr+i) = realloc( *(*X_ijr+i),
                                   *(*num_sublevels+i)*sizeof(DdNode **) );
            if (*(Y+i) == NULL || *(*X_ijr+i) == NULL) {
                perror( __FILE__ ",  realloc" );
                exit(-1);
            }

            *(*(*X_ijr+i) + *(*num_sublevels+i)-1)
                = malloc( num_env_goals*sizeof(DdNode *) );
            if (*(*(*X_ijr+i) + *(*num_sublevels+i)-1) == NULL) {
                perror( __FILE__ ",  malloc" );
                exit(-1);
            }

            Y_exmod = compute_existsmodal( manager,
                                           *(*(Y+i)+*(*num_sublevels+i)-2),
                                           etrans, strans, num_env, num_sys,
                                           cube );

            *(*(Y+i)+*(*num_sublevels+i)-1) = Cudd_Not(Cudd_ReadOne( manager ));
            Cudd_Ref( *(*(Y+i)+*(*num_sublevels+i)-1) );
            for (r = 0; r < num_env_goals; r++) {

                /* (Re)initialize X */
                if (X != NULL)
                    Cudd_RecursiveDeref( manager, X );
                X = Cudd_ReadOne( manager );
                Cudd_Ref( X );

                /* Greatest fixpoint for X, for this env goal */
                do {
                    if (X_prev != NULL)
                        Cudd_RecursiveDeref( manager, X_prev );
                    X_prev = X;
                    X = compute_existsmodal( manager, X_prev, etrans, strans,
                                             num_env, num_sys, cube );
                    if (X == NULL) {
                        /* fatal error */
                        return NULL;
                    }

                    tmp = Cudd_bddAnd( manager, *(sgoals+i), W );
                    Cudd_Ref( tmp );
                    tmp2 = Cudd_bddOr( manager, tmp, Y_exmod );
                    Cudd_Ref( tmp2 );
                    Cudd_RecursiveDeref( manager, tmp );

                    tmp = Cudd_bddAnd( manager, X, Cudd_Not( *(egoals+r) ) );
                    Cudd_Ref( tmp );
                    Cudd_RecursiveDeref( manager, X );

                    X = Cudd_bddOr( manager, tmp2, tmp );
                    Cudd_Ref( X );
                    Cudd_RecursiveDeref( manager, tmp );
                    Cudd_RecursiveDeref( manager, tmp2 );

                    tmp = X;
                    X = Cudd_bddAnd( manager, X, X_prev );
                    Cudd_Ref( X );
                    Cudd_RecursiveDeref( manager, tmp );

                } while (!Cudd_bddLeq( manager, X, X_prev )
                         || !Cudd_bddLeq( manager, X_prev, X ));

                *(*(*(*X_ijr+i) + *(*num_sublevels+i)-1) + r) = X;
                Cudd_Ref( *(*(*(*X_ijr+i) + *(*num_sublevels+i)-1) + r) );

                tmp = *(*(Y+i)+*(*num_sublevels+i)-1);
                *(*(Y+i)+*(*num_sublevels+i)-1)
                    = Cudd_bddOr( manager, *(*(Y+i)+*(*num_sublevels+i)-1), X );
                Cudd_Ref( *(*(Y+i)+*(*num_sublevels+i)-1) );
                Cudd_RecursiveDeref( manager, tmp );

                Cudd_RecursiveDeref( manager, X );
                X = NULL;
                Cudd_RecursiveDeref( manager, X_prev );
                X_prev = NULL;
            }

            tmp = *(*(Y+i)+*(*num_sublevels+i)-1);
            *(*(Y+i)+*(*num_sublevels+i)-1)
                = Cudd_bddOr( manager, *(*(Y+i)+*(*num_sublevels+i)-1),
                              *(*(Y+i)+*(*num_sublevels+i)-2) );
            Cudd_Ref( *(*(Y+i)+*(*num_sublevels+i)-1) );
            Cudd_RecursiveDeref( manager, tmp );

            if (Cudd_bddLeq( manager, *(*(Y+i)+*(*num_sublevels+i)-1),
                             *(*(Y+i)+*(*num_sublevels+i)-2))
                && Cudd_bddLeq( manager, *(*(Y+i)+*(*num_sublevels+i)-2),
                                *(*(Y+i)+*(*num_sublevels+i)-1) )) {
                Cudd_RecursiveDeref( manager, *(*(Y+i)+*(*num_sublevels+i)-1) );
                for (r = 0; r < num_env_goals; r++) {
                    Cudd_RecursiveDeref( manager, *(*(*(*X_ijr+i)
                                                      + *(*num_sublevels+i)-1)
                                                    + r) );
                }
                free( *(*(*X_ijr+i) + *(*num_sublevels+i)-1) );
                (*(*num_sublevels+i))--;
                *(Y+i) = realloc( *(Y+i),
                                  *(*num_sublevels+i)*sizeof(DdNode *) );
                *(*X_ijr+i) = realloc( *(*X_ijr+i),
                                       *(*num_sublevels+i)*sizeof(DdNode **) );
                if (*(Y+i) == NULL || *(*X_ijr+i) == NULL) {
                    perror( __FILE__ ",  realloc" );
                    exit(-1);
                }
                break;
            }
            Cudd_RecursiveDeref( manager, Y_exmod );
        }
        Cudd_RecursiveDeref( manager, Y_exmod );
    }

    free( cube );
    return Y;
}
