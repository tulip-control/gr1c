/* solve.c -- Definitions for signatures appearing in solve.h.
 *            Also consider solve_operators.c
 *
 *
 * SCL; 2012-2015
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "logging.h"
#include "solve.h"
#include "solve_support.h"
#include "automaton.h"


extern specification_t spc;


DdNode *check_realizable_internal( DdManager *manager, DdNode *W,
                                   unsigned char init_flags,
                                   unsigned char verbose );


void logprint_state( vartype *state ) {
    int i;
    int num_env, num_sys;
    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );
    for (i = 0; i < num_env+num_sys; i++) {
        logprint( "\t%s = %d;",
                  (i < num_env ?
                   (get_list_item( spc.evar_list, i ))->name
                   : (get_list_item( spc.svar_list, i-num_env ))->name),
                  *(state+i) );
    }
}


anode_t *synthesize( DdManager *manager,  unsigned char init_flags,
                     unsigned char verbose )
{
    anode_t *strategy = NULL;
    anode_t *this_node_stack = NULL;
    anode_t *node, *new_node;
    bool initial;
    vartype *state;
    vartype **env_moves;
    int emoves_len;

    ptree_t *var_separator;
    DdNode *W;
    DdNode *strans_into_W;

    DdNode *einit, *sinit, *etrans, *strans, **egoals, **sgoals;

    DdNode *ddval;  /* Store result of evaluating a BDD */
    DdNode ***Y = NULL;
    DdNode *Y_i_primed;
    int *num_sublevels;
    DdNode ****X_ijr = NULL;

    DdNode *tmp, *tmp2;
    int i, j, r, k;  /* Generic counters */
    int offset;
    bool env_nogoal_flag = False;  /* Indicate environment has no goals */
    int loop_mode;
    int next_mode;

    int num_env, num_sys;
    int *cube;  /* length will be twice total number of variables (to
                   account for both variables and their primes). */

    /* Variables used during CUDD generation (state enumeration). */
    DdGen *gen;
    CUDD_VALUE_TYPE gvalue;
    int *gcube;

    /* Set environment goal to True (i.e., any state) if none was
       given. This simplifies the implementation below. */
    if (spc.num_egoals == 0) {
        env_nogoal_flag = True;
        spc.num_egoals = 1;
        spc.env_goals = malloc( sizeof(ptree_t *) );
        *spc.env_goals = init_ptree( PT_CONSTANT, NULL, 1 );
    }

    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

    /* State vector (i.e., valuation of the variables) */
    state = malloc( sizeof(vartype)*(num_env+num_sys) );
    if (state == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    /* Allocate cube array, used later for quantifying over variables. */
    cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
    if (cube == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
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
            free( state );
            free( cube );
            return NULL;
        }
        var_separator->left = spc.svar_list;
    }

    /* Generate BDDs for the various parse trees from the problem spec. */
    if (spc.env_init != NULL) {
        einit = ptree_BDD( spc.env_init, spc.evar_list, manager );
    } else {
        einit = Cudd_ReadOne( manager );
        Cudd_Ref( einit );
    }
    if (spc.sys_init != NULL) {
        sinit = ptree_BDD( spc.sys_init, spc.evar_list, manager );
    } else {
        sinit = Cudd_ReadOne( manager );
        Cudd_Ref( sinit );
    }
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

    if (var_separator == NULL) {
        spc.evar_list = NULL;
    } else {
        var_separator->left = NULL;
    }

    W = compute_winning_set_BDD( manager, etrans, strans, egoals, sgoals,
                                 verbose );
    if (W == NULL) {
        fprintf( stderr,
                 "Error synthesize: failed to construct winning set.\n" );
        free( state );
        free( cube );
        return NULL;
    }
    Y = compute_sublevel_sets( manager, W, etrans, strans,
                               egoals, spc.num_egoals,
                               sgoals, spc.num_sgoals,
                               &num_sublevels, &X_ijr, verbose );
    if (Y == NULL) {
        fprintf( stderr,
                 "Error synthesize: failed to construct sublevel sets.\n" );
        free( state );
        free( cube );
        return NULL;
    }

    /* The sublevel sets are exactly as resulting from the vanilla
       fixed point formula.  Thus for each system goal i, Y_0 = \emptyset,
       and Y_1 is a union of i-goal states and environment-blocking states.

       For the purpose of synthesis, it is enough to delete Y_0 and
       replace Y_1 with the intersection of i-goal states and the
       winning set, and then shift the indices down (so that Y_1 is
       now called Y_0, Y_2 is now called Y_1, etc.) */
    for (i = 0; i < spc.num_sgoals; i++) {
        Cudd_RecursiveDeref( manager, *(*(Y+i)) );
        Cudd_RecursiveDeref( manager, *(*(Y+i)+1) );
        for (r = 0; r < spc.num_egoals; r++)
            Cudd_RecursiveDeref( manager, *(*(*(X_ijr+i))+r) );
        free( *(*(X_ijr+i)) );

        *(*(Y+i)+1) = Cudd_bddAnd( manager, *(sgoals+i), W );
        Cudd_Ref( *(*(Y+i)+1) );

        (*(num_sublevels+i))--;
        for (j = 0; j < *(num_sublevels+i); j++) {
            *(*(Y+i)+j) = *(*(Y+i)+j+1);
            *(*(X_ijr+i)+j) = *(*(X_ijr+i)+j+1);
        }

        assert( *(num_sublevels+i) > 0 );
        *(Y+i) = realloc( *(Y+i), (*(num_sublevels+i))*sizeof(DdNode *) );
        *(X_ijr+i) = realloc( *(X_ijr+i),
                              (*(num_sublevels+i))*sizeof(DdNode **) );
        if (*(Y+i) == NULL || *(X_ijr+i) == NULL) {
            perror( __FILE__ ",  realloc" );
            exit(-1);
        }
    }

    /* Make primed form of W and take conjunction with system
       transition (safety) formula, for use while stepping down Y_i
       sets.  Note that we assume the variable map has been
       appropriately defined in the CUDD manager, after the call to
       compute_winning_set_BDD above. */
    tmp = Cudd_bddVarMap( manager, W );
    if (tmp == NULL) {
        fprintf( stderr,
                 "Error synthesize: Error in swapping variables with primed"
                 " forms.\n" );
        free( state );
        free( cube );
        return NULL;
    }
    Cudd_Ref( tmp );
    strans_into_W = Cudd_bddAnd( manager, strans, tmp );
    Cudd_Ref( strans_into_W );
    Cudd_RecursiveDeref( manager, tmp );

    /* From each initial state, build strategy by propagating forward
       toward the next goal (current target goal specified by "mode"
       of a state), and iterating until every reached state and mode
       combination has already been encountered (whence the
       strategy already built). */
    if (init_flags == ALL_INIT
        || (init_flags == ONE_SIDE_INIT && spc.sys_init == NULL)) {
        if (init_flags == ALL_INIT) {
            if (verbose > 1)
                logprint( "Enumerating initial states, given init_flags ="
                          " ALL_INIT" );
            tmp = Cudd_bddAnd( manager, einit, sinit );
        } else {
            if (verbose > 1)
                logprint( "Enumerating initial states, given init_flags ="
                          " ONE_SIDE_INIT and empty SYSINIT" );
            tmp = einit;
            Cudd_Ref( tmp );
        }
        Cudd_Ref( tmp );
        Cudd_AutodynDisable( manager );
        Cudd_ForeachCube( manager, tmp, gen, gcube, gvalue ) {
            initialize_cube( state, gcube, num_env+num_sys );
            while (!saturated_cube( state, gcube, num_env+num_sys )) {
                this_node_stack = insert_anode( this_node_stack, 0, -1, False,
                                                state, num_env+num_sys );
                if (this_node_stack == NULL) {
                    fprintf( stderr,
                             "Error synthesize: building list of initial"
                             " states.\n" );
                    return NULL;
                }
                increment_cube( state, gcube, num_env+num_sys );
            }
            this_node_stack = insert_anode( this_node_stack, 0, -1, False,
                                            state, num_env+num_sys );
            if (this_node_stack == NULL) {
                fprintf( stderr,
                         "Error synthesize: building list of initial"
                         " states.\n" );
                return NULL;
            }
        }
        Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
        Cudd_RecursiveDeref( manager, tmp );

    } else if (init_flags == ALL_ENV_EXIST_SYS_INIT) {
        if (verbose > 1)
            logprint( "Enumerating initial states, given init_flags ="
                      " ALL_ENV_EXIST_SYS_INIT" );

        /* Generate initial environment states */
        for (i = 0; i < num_env; i++)
            *(cube+i) = 2;
        for (i = num_env; i < 2*(num_sys+num_env); i++)
            *(cube+i) = 1;
        tmp2 = Cudd_CubeArrayToBdd( manager, cube );
        if (tmp2 == NULL) {
            fprintf( stderr, "Error in generating cube for quantification.\n" );
            return NULL;
        }
        Cudd_Ref( tmp2 );
        tmp = Cudd_bddExistAbstract( manager, einit, tmp2 );
        if (tmp == NULL) {
            fprintf( stderr, "Error in performing quantification.\n" );
            return NULL;
        }
        Cudd_Ref( tmp );
        Cudd_RecursiveDeref( manager, tmp2 );

        Cudd_AutodynDisable( manager );
        Cudd_ForeachCube( manager, tmp, gen, gcube, gvalue ) {
            initialize_cube( state, gcube, num_env );
            while (!saturated_cube( state, gcube, num_env )) {
                this_node_stack = insert_anode( this_node_stack, 0, -1, False,
                                                state, num_env+num_sys );
                if (this_node_stack == NULL) {
                    fprintf( stderr,
                             "Error synthesize: building list of initial"
                             " states.\n" );
                    return NULL;
                }
                increment_cube( state, gcube, num_env );
            }
            this_node_stack = insert_anode( this_node_stack, 0, -1, False,
                                            state, num_env+num_sys );
            if (this_node_stack == NULL) {
                fprintf( stderr,
                         "Error synthesize: building list of initial"
                         " states.\n" );
                return NULL;
            }
        }
        Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
        Cudd_RecursiveDeref( manager, tmp );

        /* For each initial environment state, find a system state in
           the winning set W. */
        node = this_node_stack;
        while (node) {
            for (i = num_env; i < 2*(num_env+num_sys); i++)
                *(cube+i) = 2;
            for (i = 0; i < num_env; i++)
                *(cube+i) = *(node->state+i);

            tmp2 = Cudd_CubeArrayToBdd( manager, cube );
            if (tmp2 == NULL) {
                fprintf( stderr, "Error in generating cube for cofactor.\n" );
                return NULL;
            }
            Cudd_Ref( tmp2 );

            tmp = Cudd_Cofactor( manager, W, tmp2 );
            if (tmp == NULL) {
                fprintf( stderr, "Error in computing cofactor.\n" );
                return NULL;
            }
            Cudd_Ref( tmp );
            Cudd_RecursiveDeref( manager, tmp2 );

            tmp2 = Cudd_bddAnd( manager, tmp, sinit );
            Cudd_Ref( tmp2 );
            Cudd_RecursiveDeref( manager, tmp );

            Cudd_AutodynDisable( manager );
            gen = Cudd_FirstCube( manager, tmp2, &gcube, &gvalue );
            if (gen == NULL) {
                fprintf( stderr, "Error synthesize: failed to find cube.\n" );
                return NULL;
            }
            if (Cudd_IsGenEmpty( gen )) {
                fprintf( stderr,
                         "Error synthesize: unexpected losing initial"
                         " environment state found.\n" );
                return NULL;
            }
            initialize_cube( state, gcube, num_env+num_sys );
            for (i = num_env; i < num_env+num_sys; i++)
                *(node->state+i) = *(state+i);
            Cudd_GenFree( gen );
            Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
            Cudd_RecursiveDeref( manager, tmp2 );

            node = node->next;
        }

    } else if (init_flags == ONE_SIDE_INIT) {
        /* N.B., case of sys_init==NULL is treated above */
        if (verbose > 1)
            logprint( "Enumerating initial states, given init_flags ="
                      " ONE_SIDE_INIT and empty ENVINIT" );

        tmp = Cudd_bddAnd( manager, sinit, W );
        Cudd_Ref( tmp );
        Cudd_AutodynDisable( manager );
        gen = Cudd_FirstCube( manager, tmp, &gcube, &gvalue );
        if (gen == NULL) {
            fprintf( stderr, "Error synthesize: failed to find cube.\n" );
            return NULL;
        }
        if (Cudd_IsGenEmpty( gen )) {
            fprintf( stderr,
                     "Error synthesize: no winning initial state found.\n" );
            return NULL;
        }
        initialize_cube( state, gcube, num_env+num_sys );
        this_node_stack = insert_anode( this_node_stack, 0, -1, False,
                                        state, num_env+num_sys );
        if (this_node_stack == NULL) {
            fprintf( stderr,
                     "Error synthesize: building list of initial states.\n" );
            return NULL;
        }
        Cudd_GenFree( gen );
        Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
        Cudd_RecursiveDeref( manager, tmp );
    } else {
        fprintf( stderr, "Error: Unrecognized init_flags %d", init_flags );
        return NULL;
    }

    /* Insert all stacked, initial nodes into strategy. */
    node = this_node_stack;
    while (node) {
        if (verbose > 1) {
            logprint( "Insert initial state: {" );
            logprint_state( node->state );
            logprint( "}" );
        }
        strategy = insert_anode( strategy, node->mode, node->rgrad, True,
                                 node->state, num_env+num_sys );
        if (strategy == NULL) {
            fprintf( stderr,
                     "Error synthesize: inserting state node into"
                     " strategy.\n" );
            return NULL;
        }
        node = node->next;
    }

    if (verbose > 1) {
        logprint( "Constructing enumerative strategy..." );
        logprint( "Beginning with node stack size %d.",
                  aut_size( this_node_stack ) );
    }
    while (this_node_stack) {
        /* Find smallest Y_j set containing node. */
        for (k = num_env+num_sys; k < 2*(num_env+num_sys); k++)
            *(cube+k) = 2;
        state_to_cube( this_node_stack->state, cube, num_env+num_sys );
        loop_mode = this_node_stack->mode;
        do {
            j = *(num_sublevels+this_node_stack->mode);
            do {
                j--;
                ddval = Cudd_Eval( manager,
                                   *(*(Y+this_node_stack->mode)+j), cube );
                if (Cudd_IsComplement( ddval )) {
                    j++;
                    break;
                }
            } while (j > 0);
            if (j == 0) {
                if (this_node_stack->mode == spc.num_sgoals-1) {
                    this_node_stack->mode = 0;
                } else {
                    (this_node_stack->mode)++;
                }
            } else {
                break;
            }
        } while (loop_mode != this_node_stack->mode);
        if (this_node_stack->mode == loop_mode) {
            node = find_anode( strategy, this_node_stack->mode,
                               this_node_stack->state, num_env+num_sys );
            if (node->trans_len > 0) {
                /* This state and mode combination is already in strategy. */
                this_node_stack = pop_anode( this_node_stack );
                continue;
            }
        } else {

            node = find_anode( strategy, loop_mode, this_node_stack->state,
                               num_env+num_sys );
            if (node->trans_len > 0) {
                /* This state and mode combination is already in strategy. */
                this_node_stack = pop_anode( this_node_stack );
                continue;
            } else {
                if (verbose > 1) {
                    logprint( "Delete node with mode %d and state: {",
                              node->mode );
                    logprint_state( node->state );
                    logprint( "}" );
                }
                initial = node->initial;
                strategy = delete_anode( strategy, node );
                new_node = find_anode( strategy, this_node_stack->mode,
                                       this_node_stack->state,
                                       num_env+num_sys );
                if (new_node == NULL) {
                    if (verbose > 1) {
                        logprint( "Insert node with mode %d and state: {",
                                  this_node_stack->mode );
                        logprint_state( this_node_stack->state );
                        logprint( "}" );
                    }
                    strategy = insert_anode( strategy,
                                             this_node_stack->mode, -1, initial,
                                             this_node_stack->state,
                                             num_env+num_sys );
                    if (strategy == NULL) {
                        fprintf( stderr,
                                 "Error synthesize: inserting state node into"
                                 " strategy.\n" );
                        return NULL;
                    }
                    new_node = find_anode( strategy, this_node_stack->mode,
                                           this_node_stack->state,
                                           num_env+num_sys );
                } else if (new_node->trans_len > 0) {
                    replace_anode_trans( strategy, node, new_node );
                    this_node_stack = pop_anode( this_node_stack );
                    continue;
                }
                replace_anode_trans( strategy, node, new_node );
            }

            node = new_node;
        }
        this_node_stack = pop_anode( this_node_stack );
        node->rgrad = j;

        if (num_env > 0) {
            env_moves = get_env_moves( manager, cube,
                                       node->state, etrans,
                                       num_env, num_sys,
                                       &emoves_len );
        } else {
            emoves_len = 1;  /* This allows one iteration of the for-loop */
        }
        for (k = 0; k < emoves_len; k++) {
            /* Note that we assume the variable map has been
               appropriately defined in the CUDD manager, after the
               call to compute_winning_set above. */
            if (j == 0) {
                Y_i_primed = Cudd_bddVarMap( manager, **(Y+node->mode) );
            } else {
                Y_i_primed = Cudd_bddVarMap( manager, *(*(Y+node->mode)+j-1) );
            }
            if (Y_i_primed == NULL) {
                fprintf( stderr,
                         "Error synthesize: Error in swapping variables with"
                         " primed forms.\n" );
                return NULL;
            }
            Cudd_Ref( Y_i_primed );

            tmp = Cudd_bddAnd( manager, strans_into_W, Y_i_primed );
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
                fprintf( stderr, "Error synthesize: failed to find cube.\n" );
                return NULL;
            }
            if (Cudd_IsGenEmpty( gen )) {
                /* Cannot step closer to system goal, so must be in
                   goal state or able to block environment goal. */
                Cudd_GenFree( gen );
                Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
                if (j > 0) {
                    for (offset = 1; offset >= 0; offset--) {
                    for (r = 0; r < spc.num_egoals; r++) {
                        Cudd_RecursiveDeref( manager, tmp );
                        Cudd_RecursiveDeref( manager, Y_i_primed );
                        Y_i_primed
                            = Cudd_bddVarMap( manager,
                                              *(*(*(X_ijr+node->mode)+j - offset)+r) );
                        if (Y_i_primed == NULL) {
                            fprintf( stderr,
                                     "Error synthesize: Error in swapping"
                                     " variables with primed forms.\n" );
                            return NULL;
                        }
                        Cudd_Ref( Y_i_primed );
                        tmp = Cudd_bddAnd( manager, strans_into_W, Y_i_primed );
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

                        if (!Cudd_bddLeq( manager, tmp,
                                          Cudd_Not( Cudd_ReadOne( manager ) ) )
                            || !Cudd_bddLeq( manager,
                                             Cudd_Not( Cudd_ReadOne( manager ) ),
                                             tmp ))
                            break;
                    }
                    if (r < spc.num_egoals)
                        break;
                    }
                    if (r >= spc.num_egoals) {
                        fprintf( stderr,
                                 "Error synthesize: unexpected losing"
                                 " state.\n" );
                        return NULL;
                    }
                } else {
                    Cudd_RecursiveDeref( manager, tmp );
                    Cudd_RecursiveDeref( manager, Y_i_primed );
                    Y_i_primed = Cudd_ReadOne( manager );
                    Cudd_Ref( Y_i_primed );
                    tmp = Cudd_bddAnd( manager, strans_into_W, Y_i_primed );
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
                             "Error synthesize: failed to find cube.\n" );
                    return NULL;
                }
                if (Cudd_IsGenEmpty( gen )) {
                    Cudd_GenFree( gen );
                    fprintf( stderr,
                             "Error synthesize: unexpected losing state.\n" );
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

            state_to_cube( state, cube, num_env+num_sys );
            ddval = Cudd_Eval( manager, **(Y+node->mode), cube );
            if (Cudd_IsComplement( ddval )) {
                next_mode = node->mode;
            } else {
                if (node->mode == spc.num_sgoals-1) {
                    next_mode = 0;
                } else {
                    next_mode = node->mode + 1;
                }
            }

            new_node = find_anode( strategy, next_mode,
                                   state, num_env+num_sys );
            if (new_node == NULL) {
                if (verbose > 1) {
                    logprint( "Insert node with mode %d and state: {",
                              next_mode );
                    logprint_state( state );
                    logprint( "}" );
                }
                strategy = insert_anode( strategy, next_mode, -1, False,
                                         state, num_env+num_sys );
                if (strategy == NULL) {
                    fprintf( stderr,
                             "Error synthesize: inserting new node into"
                             " strategy.\n" );
                    return NULL;
                }
                this_node_stack = insert_anode( this_node_stack, next_mode, -1,
                                                False,
                                                state, num_env+num_sys );
                if (this_node_stack == NULL) {
                    fprintf( stderr,
                             "Error synthesize: pushing node onto stack"
                             " failed.\n" );
                    return NULL;
                }
            }

            strategy = append_anode_trans( strategy,
                                           node->mode, node->state,
                                           num_env+num_sys,
                                           next_mode, state );
            if (strategy == NULL) {
                fprintf( stderr,
                         "Error synthesize: inserting new transition into"
                         " strategy.\n" );
                return NULL;
            }

            Cudd_RecursiveDeref( manager, Y_i_primed );
        }
        if (num_env > 0) {
            for (k = 0; k < emoves_len; k++)
                free( *(env_moves+k) );
            free( env_moves );
        } else {
            emoves_len = 0;
        }
    }

    /* Pre-exit clean-up */
    Cudd_RecursiveDeref( manager, W );
    Cudd_RecursiveDeref( manager, strans_into_W );
    Cudd_RecursiveDeref( manager, einit );
    Cudd_RecursiveDeref( manager, sinit );
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
    free( cube );
    free( state );
    for (i = 0; i < spc.num_sgoals; i++) {
        for (j = 0; j < *(num_sublevels+i); j++) {
            Cudd_RecursiveDeref( manager, *(*(Y+i)+j) );
            for (r = 0; r < spc.num_egoals; r++) {
                Cudd_RecursiveDeref( manager, *(*(*(X_ijr+i)+j)+r) );
            }
            free( *(*(X_ijr+i)+j) );
        }
        if (*(num_sublevels+i) > 0) {
            free( *(Y+i) );
            free( *(X_ijr+i) );
        }
    }
    if (spc.num_sgoals > 0) {
        free( Y );
        free( X_ijr );
        free( num_sublevels );
    }
    if (env_nogoal_flag) {
        spc.num_egoals = 0;
        delete_tree( *spc.env_goals );
        free( spc.env_goals );
    }


    return strategy;
}


DdNode *check_realizable( DdManager *manager, unsigned char init_flags,
                          unsigned char verbose )
{
    DdNode *W = compute_winning_set( manager, verbose );
    return check_realizable_internal( manager, W, init_flags, verbose );
}


DdNode *check_realizable_internal( DdManager *manager, DdNode *W,
                                   unsigned char init_flags,
                                   unsigned char verbose )
{
    bool realizable;
    DdNode *tmp, *tmp2, *tmp3;
    DdNode *einit = NULL, *sinit = NULL;

    ptree_t *var_separator;
    int num_env, num_sys;
    int *cube;  /* length will be twice total number of variables (to
                   account for both variables and their primes). */
    DdNode *ddcube;

    if (verbose > 1) {
        logprint_startline();
        logprint_raw( "check_realizable_internal invoked with init_flags: " );
        LOGPRINT_INIT_FLAGS( init_flags );
        logprint_endline();
    }

    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

    /* Allocate cube array, used later for quantifying over variables. */
    cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
    if (cube == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
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
            free( cube );
            return NULL;
        }
        var_separator->left = spc.svar_list;
    }

    if (spc.env_init != NULL) {
        einit = ptree_BDD( spc.env_init, spc.evar_list, manager );
    } else {
        einit = Cudd_ReadOne( manager );
        Cudd_Ref( einit );
    }
    if (spc.sys_init != NULL) {
        sinit = ptree_BDD( spc.sys_init, spc.evar_list, manager );
    } else {
        sinit = Cudd_ReadOne( manager );
        Cudd_Ref( sinit );
    }

    /* Break the link that appended the system variables list to the
       environment variables list. */
    if (var_separator == NULL) {
        spc.evar_list = NULL;
    } else {
        var_separator->left = NULL;
    }

    /* Does winning set contain all initial states?

       We assume that the initial condition formulae, i.e., env_init
       and sys_init, are appropriate for the given init_flags.  This
       can be checked with check_gr1c_form() (cf. gr1c_util.h). */
    if (init_flags == ALL_INIT) {

        tmp = Cudd_bddAnd( manager, einit, sinit );
        Cudd_Ref( tmp );
        tmp2 = Cudd_bddAnd( manager, tmp, W );
        Cudd_Ref( tmp2 );
        if (tmp == Cudd_Not( Cudd_ReadOne( manager ) )
            || !Cudd_bddLeq( manager, tmp, tmp2 )
            || !Cudd_bddLeq( manager, tmp2, tmp )) {
            realizable = False;
        } else {
            realizable = True;
        }
        Cudd_RecursiveDeref( manager, tmp );
        Cudd_RecursiveDeref( manager, tmp2 );

    } else if (init_flags == ALL_ENV_EXIST_SYS_INIT) {

        tmp = Cudd_bddAnd( manager, sinit, einit );
        Cudd_Ref( tmp );

        tmp3 = Cudd_bddAnd( manager, tmp, W );
        Cudd_Ref( tmp3 );

        cube_sys( cube, num_env, num_sys );
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

        tmp = Cudd_bddExistAbstract( manager, tmp3, ddcube );
        if (tmp == NULL) {
            fprintf( stderr, "Error in performing quantification." );
            return NULL;
        }
        Cudd_Ref( tmp );
        Cudd_RecursiveDeref( manager, ddcube );
        Cudd_RecursiveDeref( manager, tmp3 );

        if (!Cudd_bddLeq( manager, tmp, tmp2 )
            || !Cudd_bddLeq( manager, tmp2, tmp )) {
            realizable = False;
        } else {
            realizable = True;
        }
        Cudd_RecursiveDeref( manager, tmp );
        Cudd_RecursiveDeref( manager, tmp2 );

    } else if (init_flags == ONE_SIDE_INIT) {
        if (spc.sys_init == NULL) {

            tmp = Cudd_bddAnd( manager, einit, W );
            Cudd_Ref( tmp );
            if (!Cudd_bddLeq( manager, tmp, einit )
                || !Cudd_bddLeq( manager, einit, tmp )) {
                realizable = False;
            } else {
                realizable = True;
            }
            Cudd_RecursiveDeref( manager, tmp );

        } else {
            /* If env_init and sys_init were both NULL, we would still
               want to treat env_init as True.  Cf. ONE_SIDE_INIT
               option for init_flags in documentation for
               check_realizable(). */

            tmp = Cudd_bddAnd( manager, sinit, W );
            Cudd_Ref( tmp );
            if (!Cudd_bddLeq( manager, tmp, Cudd_Not( Cudd_ReadOne( manager ) ) )
                || !Cudd_bddLeq( manager, Cudd_Not( Cudd_ReadOne( manager ) ), tmp )) {
                realizable = True;
            } else {
                realizable = False;
            }
            Cudd_RecursiveDeref( manager, tmp );
        }

    } else {
        fprintf( stderr, "Error: Unrecognized init_flags %d", init_flags );
        free( cube );
        return NULL;
    }

    Cudd_RecursiveDeref( manager, einit );
    Cudd_RecursiveDeref( manager, sinit );
    free( cube );

    if (realizable) {
        return W;
    } else {
        Cudd_RecursiveDeref( manager, W );
        return NULL;
    }
}
