/* patching_support.c -- More definitions for signatures in patching.h.
 *
 *
 * SCL; 2012-2015
 */


#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "patching.h"
#include "solve_support.h"


extern specification_t spc;


anode_t *synthesize_reachgame_BDD( DdManager *manager, int num_env, int num_sys,
                                   DdNode *Entry, DdNode *Exit,
                                   DdNode *etrans, DdNode *strans,
                                   DdNode **egoals, DdNode *N_BDD,
                                   unsigned char verbose )
{
    anode_t *strategy = NULL;
    anode_t *this_node_stack = NULL;
    anode_t *node, *new_node;
    vartype *state;
    int *cube;
    vartype **env_moves;
    int emoves_len;

    DdNode *strans_into_N;
    DdNode **Y = NULL, *Y_exmod;
    DdNode *X = NULL, *X_prev = NULL;
    DdNode *Y_i_primed;
    int num_sublevels;
    DdNode ***X_jr = NULL;

    DdNode *tmp, *tmp2;
    int i, j, r, k;  /* Generic counters */
    int offset;
    DdNode *ddval;  /* Store result of evaluating a BDD */

    /* Variables used during CUDD generation (state enumeration). */
    DdGen *gen;
    CUDD_VALUE_TYPE gvalue;
    int *gcube;

    state = malloc( sizeof(vartype)*(num_env+num_sys) );
    if (state == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }
    cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
    if (cube == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    num_sublevels = 1;
    Y = malloc( num_sublevels*sizeof(DdNode *) );
    if (Y == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }
    *Y = Exit;
    Cudd_Ref( *Y );

    X_jr = malloc( num_sublevels*sizeof(DdNode **) );
    if (X_jr == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    *X_jr = malloc( spc.num_egoals*sizeof(DdNode *) );
    if (*X_jr == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }
    for (r = 0; r < spc.num_egoals; r++) {
        *(*X_jr+r) = Cudd_Not( Cudd_ReadOne( manager ) );
        Cudd_Ref( *(*X_jr+r) );
    }

    while (True) {
        num_sublevels++;
        Y = realloc( Y, num_sublevels*sizeof(DdNode *) );
        X_jr = realloc( X_jr, num_sublevels*sizeof(DdNode **) );
        if (Y == NULL || X_jr == NULL) {
            perror( __FILE__ ",  realloc" );
            exit(-1);
        }

        *(X_jr + num_sublevels-1) = malloc( spc.num_egoals*sizeof(DdNode *) );
        if (*(X_jr + num_sublevels-1) == NULL) {
            perror( __FILE__ ",  malloc" );
            exit(-1);
        }

        Y_exmod = compute_existsmodal( manager, *(Y+num_sublevels-2),
                                       etrans, strans,
                                       num_env, num_sys, cube );
        if (Y_exmod == NULL)
            return NULL;  /* Fatal error */
        tmp = Cudd_bddAnd( manager, Y_exmod, N_BDD );
        Cudd_Ref( tmp );
        Cudd_RecursiveDeref( manager, Y_exmod );
        Y_exmod = tmp;

        *(Y+num_sublevels-1) = Cudd_Not( Cudd_ReadOne( manager ) );
        Cudd_Ref( *(Y+num_sublevels-1) );
        for (r = 0; r < spc.num_egoals; r++) {

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
                if (X == NULL)
                    return NULL;  /* Fatal error */
                tmp = Cudd_bddAnd( manager, X, N_BDD );
                Cudd_Ref( tmp );
                Cudd_RecursiveDeref( manager, X );
                X = tmp;

                tmp2 = Cudd_bddOr( manager, Exit, Y_exmod );
                Cudd_Ref( tmp2 );

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

            *(*(X_jr+num_sublevels-1) + r) = X;
            Cudd_Ref( *(*(X_jr+num_sublevels-1) + r) );

            tmp = *(Y+num_sublevels-1);
            *(Y+num_sublevels-1) = Cudd_bddOr( manager,
                                               *(Y+num_sublevels-1), X );
            Cudd_Ref( *(Y+num_sublevels-1) );
            Cudd_RecursiveDeref( manager, tmp );

            Cudd_RecursiveDeref( manager, X );
            X = NULL;
            Cudd_RecursiveDeref( manager, X_prev );
            X_prev = NULL;
        }
        Cudd_RecursiveDeref( manager, Y_exmod );

        tmp = *(Y+num_sublevels-1);
        *(Y+num_sublevels-1) = Cudd_bddOr( manager, *(Y+num_sublevels-1),
                                           *(Y+num_sublevels-2) );
        Cudd_Ref( *(Y+num_sublevels-1) );
        Cudd_RecursiveDeref( manager, tmp );

        tmp = Cudd_bddOr( manager, Entry, *(Y+num_sublevels-1) );
        Cudd_Ref( tmp );
        if (Cudd_bddLeq( manager, tmp, *(Y+num_sublevels-1) )
            && Cudd_bddLeq( manager, *(Y+num_sublevels-1), tmp )) {
            Cudd_RecursiveDeref( manager, tmp );
            if (Cudd_bddLeq( manager, *(Y+num_sublevels-1),
                             *(Y+num_sublevels-2) )
                && Cudd_bddLeq( manager, *(Y+num_sublevels-2),
                                *(Y+num_sublevels-1) )) {
                Cudd_RecursiveDeref( manager, *(Y+num_sublevels-1) );
                for (r = 0; r < spc.num_egoals; r++) {
                    Cudd_RecursiveDeref( manager,
                                         *(*(X_jr+num_sublevels-1) + r) );
                }
                free( *(X_jr+num_sublevels-1) );
                num_sublevels--;
                Y = realloc( Y, num_sublevels*sizeof(DdNode *) );
                X_jr = realloc( X_jr, num_sublevels*sizeof(DdNode **) );
                if (Y == NULL || X_jr == NULL) {
                    perror( __FILE__ ",  realloc" );
                    exit(-1);
                }
            }
            break;
        }
        Cudd_RecursiveDeref( manager, tmp );

        if (Cudd_bddLeq( manager, *(Y+num_sublevels-1),
                         *(Y+num_sublevels-2) )
            && Cudd_bddLeq( manager, *(Y+num_sublevels-2),
                            *(Y+num_sublevels-1) )) {
            return NULL;  /* Local synthesis failed */
        }
    }


    /* Note that we assume the variable map has been appropriately defined
       in the CUDD manager before invocation of synthesize_reachgame_BDD. */
    tmp = Cudd_bddVarMap( manager, N_BDD );
    if (tmp == NULL) {
        fprintf( stderr,
                 "Error synthesize_reachgame_BDD: Error in swapping variables"
                 " with primed forms.\n" );
        return NULL;
    }
    Cudd_Ref( tmp );
    strans_into_N = Cudd_bddAnd( manager, strans, tmp );
    Cudd_Ref( strans_into_N );
    Cudd_RecursiveDeref( manager, tmp );

    /* Synthesize local strategy */
    Cudd_AutodynDisable( manager );
    Cudd_ForeachCube( manager, Entry, gen, gcube, gvalue ) {
        initialize_cube( state, gcube, num_env+num_sys );
        while (!saturated_cube( state, gcube, num_env+num_sys )) {
            this_node_stack = insert_anode( this_node_stack, -1, -1, False,
                                            state, num_env+num_sys );
            if (this_node_stack == NULL) {
                fprintf( stderr,
                         "Error synthesize_reachgame_BDD: building list of"
                         " initial states.\n" );
                return NULL;
            }
            increment_cube( state, gcube, num_env+num_sys );
        }
        this_node_stack = insert_anode( this_node_stack, -1, -1, False,
                                        state, num_env+num_sys );
        if (this_node_stack == NULL) {
            fprintf( stderr,
                     "Error synthesize_reachgame_BDD: building list of"
                     " initial states.\n" );
            return NULL;
        }
    }
    Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );

    /* Insert all stacked, initial nodes into strategy. */
    node = this_node_stack;
    while (node) {
        strategy = insert_anode( strategy, -1, node->rgrad, False,
                                 node->state, num_env+num_sys );
        if (strategy == NULL) {
            fprintf( stderr,
                     "Error synthesize_reachgame_BDD: inserting state node"
                     " into strategy.\n" );
            return NULL;
        }
        node = node->next;
    }

    while (this_node_stack) {
        /* Find smallest Y_j set containing node. */
        for (k = num_env+num_sys; k < 2*(num_env+num_sys); k++)
            *(cube+k) = 2;
        state_to_cube( this_node_stack->state, cube, num_env+num_sys );
        j = num_sublevels;
        do {
            j--;
            ddval = Cudd_Eval( manager, *(Y+j), cube );
            if (Cudd_IsComplement( ddval )) {
                j++;
                break;
            }
        } while (j > 0);
        node = find_anode( strategy, -1, this_node_stack->state,
                           num_env+num_sys );
        node->rgrad = j;
        this_node_stack = pop_anode( this_node_stack );
        if (node->trans_len > 0 || j == 0) {
            /* This state and mode combination is already in strategy. */
            continue;
        }

        Y_i_primed = Cudd_bddVarMap( manager, *(Y+j-1) );
        if (Y_i_primed == NULL) {
            fprintf( stderr,
                     "Error synthesize_reachgame_BDD: swapping variables with"
                     " primed forms.\n" );
            return NULL;
        }
        Cudd_Ref( Y_i_primed );

        if (num_env > 0) {
            env_moves = get_env_moves( manager, cube,
                                       node->state, etrans,
                                       num_env, num_sys,
                                       &emoves_len );
        } else {
            emoves_len = 1;  /* This allows one iteration of the for-loop */
        }
        for (k = 0; k < emoves_len; k++) {
            tmp = Cudd_bddAnd( manager, strans_into_N, Y_i_primed );
            Cudd_Ref( tmp );
            tmp2 = state_to_cof( manager, cube, 2*(num_env+num_sys),
                              node->state,
                              tmp, 0, num_env+num_sys );
            Cudd_RecursiveDeref( manager, tmp );
            if (num_env > 0) {
                tmp = state_to_cof( manager, cube, 2*(num_env+num_sys),
                                 *(env_moves+k),
                                 tmp2, num_env+num_sys, num_env );
                Cudd_RecursiveDeref( manager, tmp2 );
            } else {
                tmp = tmp2;
            }

            Cudd_AutodynDisable( manager );
            gen = Cudd_FirstCube( manager, tmp, &gcube, &gvalue );
            if (gen == NULL) {
                fprintf( stderr,
                         "Error synthesize_reachgame_BDD: failed to find"
                         " cube.\n" );
                return NULL;
            }
            if (Cudd_IsGenEmpty( gen )) {
                /* Cannot step closer to target set, so must be able
                   to block environment liveness. */
                Cudd_GenFree( gen );
                Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
                if (j > 0) {
                    for (offset = 1; offset >= 0; offset--) {
                    for (r = 0; r < spc.num_egoals; r++) {
                        Cudd_RecursiveDeref( manager, tmp );
                        Cudd_RecursiveDeref( manager, Y_i_primed );
                        Y_i_primed = Cudd_bddVarMap( manager, *(*(X_jr+j - offset)+r) );
                        if (Y_i_primed == NULL) {
                            fprintf( stderr,
                                     "Error synthesize_reachgame_BDD: Error"
                                     " in swapping variables with primed"
                                     " forms.\n" );
                            return NULL;
                        }
                        Cudd_Ref( Y_i_primed );
                        tmp = Cudd_bddAnd( manager, strans_into_N, Y_i_primed );
                        Cudd_Ref( tmp );
                        tmp2 = state_to_cof( manager, cube, 2*(num_env+num_sys),
                                          node->state,
                                          tmp, 0, num_sys+num_env );
                        Cudd_RecursiveDeref( manager, tmp );
                        if (num_env > 0) {
                            tmp = state_to_cof( manager, cube, 2*(num_env+num_sys),
                                             *(env_moves+k),
                                             tmp2, num_sys+num_env, num_env );
                            Cudd_RecursiveDeref( manager, tmp2 );
                        } else {
                            tmp = tmp2;
                        }

                        if (!Cudd_bddLeq( manager,
                                          tmp,
                                          Cudd_Not( Cudd_ReadOne( manager ) ) )
                            || ! Cudd_bddLeq( manager,
                                              Cudd_Not( Cudd_ReadOne( manager ) ),
                                              tmp ))
                            break;
                    }
                    if (r < spc.num_egoals)
                        break;
                    }
                    if (r >= spc.num_egoals) {
                        fprintf( stderr,
                                 "Error synthesize_reachgame_BDD: unexpected"
                                 " losing state.\n" );
                        return NULL;
                    }
                } else {
                    Cudd_RecursiveDeref( manager, tmp );
                    Cudd_RecursiveDeref( manager, Y_i_primed );
                    Y_i_primed = Cudd_ReadOne( manager );
                    Cudd_Ref( Y_i_primed );
                    tmp = Cudd_bddAnd( manager, strans_into_N, Y_i_primed );
                    Cudd_Ref( tmp );
                    tmp2 = state_to_cof( manager, cube, 2*(num_env+num_sys),
                                      node->state,
                                      tmp, 0, num_sys+num_env );
                    Cudd_RecursiveDeref( manager, tmp );
                    if (num_env > 0) {
                        tmp = state_to_cof( manager, cube, 2*(num_env+num_sys),
                                         *(env_moves+k),
                                         tmp2, num_sys+num_env, num_env );
                        Cudd_RecursiveDeref( manager, tmp2 );
                    } else {
                        tmp = tmp2;
                    }
                }

                Cudd_AutodynDisable( manager );
                gen = Cudd_FirstCube( manager, tmp, &gcube, &gvalue );
                if (gen == NULL) {
                    fprintf( stderr,
                             "Error synthesize_reachgame_BDD: failed to find"
                             " cube.\n" );
                    return NULL;
                }
                if (Cudd_IsGenEmpty( gen )) {
                    Cudd_GenFree( gen );
                    fprintf( stderr,
                             "Error synthesize_reachgame_BDD: unexpected"
                             " losing state.\n" );
                    return NULL;
                }
                for (i = 0; i < 2*(num_env+num_sys); i++)
                    *(cube+i) = *(gcube+i);
                Cudd_GenFree( gen );
                Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
            } else {
                for (i = 0; i < 2*(num_env+num_sys); i++)
                    *(cube+i) = *(gcube+i);
                Cudd_GenFree( gen );
                Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
            }

            Cudd_RecursiveDeref( manager, tmp );
            initialize_cube( state, cube+num_env+num_sys, num_env+num_sys );
            for (i = 0; i < num_env; i++)
                *(state+i) = *(*(env_moves+k)+i);

            new_node = find_anode( strategy, -1, state, num_env+num_sys );
            if (new_node == NULL) {
                strategy = insert_anode( strategy, -1, -1, False,
                                         state, num_env+num_sys );
                if (strategy == NULL) {
                    fprintf( stderr,
                             "Error synthesize_reachgame_BDD: inserting new"
                             " node into strategy.\n" );
                    return NULL;
                }
                this_node_stack = insert_anode( this_node_stack, -1, -1, False,
                                                state, num_env+num_sys );
                if (this_node_stack == NULL) {
                    fprintf( stderr,
                             "Error synthesize_reachgame_BDD: pushing node"
                             " onto stack failed.\n" );
                    return NULL;
                }
            }

            strategy = append_anode_trans( strategy, -1, node->state,
                                           num_env+num_sys,
                                           -1, state );
            if (strategy == NULL) {
                fprintf( stderr,
                         "Error synthesize_reachgame_BDD: inserting new"
                         " transition into strategy.\n" );
                return NULL;
            }
        }
        if (num_env > 0) {
            for (k = 0; k < emoves_len; k++)
                free( *(env_moves+k) );
            free( env_moves );
        } else {
            emoves_len = 0;
        }
        Cudd_RecursiveDeref( manager, Y_i_primed );
    }


    /* Pre-exit clean-up */
    Cudd_RecursiveDeref( manager, strans_into_N );
    free( cube );
    free( state );
    for (i = 0; i < num_sublevels; i++) {
        Cudd_RecursiveDeref( manager, *(Y+i) );
        for (j = 0; j < spc.num_egoals; j++) {
            Cudd_RecursiveDeref( manager, *(*(X_jr+i)+j) );
        }
        free( *(X_jr+i) );
    }
    free( Y );
    free( X_jr );

    return strategy;
}


anode_t *synthesize_reachgame( DdManager *manager, int num_env, int num_sys,
                               anode_t **Entry, int Entry_len,
                               anode_t **Exit, int Exit_len,
                               DdNode *etrans, DdNode *strans, DdNode **egoals,
                               DdNode *N_BDD,
                               unsigned char verbose )
{
    DdNode *Entry_BDD;
    DdNode *Exit_BDD;
    anode_t *strategy;
    DdNode *tmp, *tmp2;
    int i;

    /* Build characteristic functions (as BDDs) for Entry and Exit sets. */
    Entry_BDD = Cudd_Not( Cudd_ReadOne( manager ) );
    Cudd_Ref( Entry_BDD );
    for (i = 0; i < Entry_len; i++) {
        tmp2 = state_to_BDD( manager, (*(Entry+i))->state, 0, num_env+num_sys );
        tmp = Cudd_bddOr( manager, Entry_BDD, tmp2 );
        Cudd_Ref( tmp );
        Cudd_RecursiveDeref( manager, Entry_BDD );
        Cudd_RecursiveDeref( manager, tmp2 );
        Entry_BDD = tmp;
    }
    tmp2 = NULL;

    Exit_BDD = Cudd_Not( Cudd_ReadOne( manager ) );
    Cudd_Ref( Exit_BDD );
    for (i = 0; i < Exit_len; i++) {
        tmp2 = state_to_BDD( manager, (*(Exit+i))->state, 0, num_env+num_sys );
        tmp = Cudd_bddOr( manager, Exit_BDD, tmp2 );
        Cudd_Ref( tmp );
        Cudd_RecursiveDeref( manager, Exit_BDD );
        Cudd_RecursiveDeref( manager, tmp2 );
        Exit_BDD = tmp;
    }
    tmp2 = NULL;

    strategy =  synthesize_reachgame_BDD( manager, num_env, num_sys,
                                          Entry_BDD, Exit_BDD, etrans, strans,
                                          egoals, N_BDD, verbose );

    /* Pre-exit clean-up */
    Cudd_RecursiveDeref( manager, Exit_BDD );
    Cudd_RecursiveDeref( manager, Entry_BDD );

    return strategy;
}
