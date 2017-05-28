/* sim.c -- Definitions for signatures appearing in sim.h.
 *
 *
 * SCL; 2012-2015
 */


#include <stdlib.h>
#include <time.h>

#include "sim.h"
#include "common.h"
#include "gr1c_util.h"
#include "automaton.h"
#include "logging.h"
#include "solve_support.h"
#include "solve_metric.h"


extern specification_t spc;


anode_t *sim_rhc( DdManager *manager, DdNode *W,
                  DdNode *etrans, DdNode *strans, DdNode **sgoals,
                  char *metric_vars, int horizon, vartype *init_state,
                  int num_it, unsigned char verbose )
{
    int *offw, num_metric_vars;
    anode_t *play;
    int num_env, num_sys;
    vartype *candidate_state, *next_state;
    int current_goal = 0;
    int current_it = 0, i, j;
    vartype **env_moves;
    int emoves_len, emove_index;
    DdNode *strans_into_W;
    double Max, Min, next_min;

    /* Number of stacks is equal to the horizon. */
    anode_t *node, *prev_node, **hstacks = NULL;
    int hdepth;
    vartype *fnext_state, *finit_state;

    anode_t **MEM = NULL;
    int MEM_len = 0, MEM_index;

    int *cube;  /* length will be twice total number of variables (to
                   account for both variables and their primes). */
    DdNode *tmp, *tmp2, *ddval;

    /* Variables used during CUDD generation (state enumeration). */
    DdGen *gen;
    CUDD_VALUE_TYPE gvalue;
    int *gcube;
    DdNode **vars, **pvars;

    if (init_state == NULL || horizon < 1)
        return NULL;

    offw = get_offsets( metric_vars, &num_metric_vars );
    if (offw == NULL)
        return NULL;

    srand( time(NULL) );
    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

    vars = malloc( (num_env+num_sys)*sizeof(DdNode *) );
    pvars = malloc( (num_env+num_sys)*sizeof(DdNode *) );
    for (i = 0; i < num_env+num_sys; i++) {
        *(vars+i) = Cudd_bddIthVar( manager, i );
        *(pvars+i) = Cudd_bddIthVar( manager, i+num_env+num_sys );
    }
    if (!Cudd_SetVarMap( manager, vars, pvars, num_env+num_sys )) {
        fprintf( stderr,
                 "Error: failed to define variable map in CUDD manager.\n" );
        return NULL;
    }
    free( vars );
    free( pvars );

    hstacks = malloc( horizon*sizeof(anode_t *) );
    if (hstacks == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }
    for (i = 0; i < horizon; i++)
        *(hstacks+i) = NULL;

    next_state = malloc( (num_env+num_sys)*sizeof(vartype) );
    candidate_state = malloc( (num_env+num_sys)*sizeof(vartype) );
    finit_state = malloc( (num_env+num_sys)*sizeof(vartype) );
    fnext_state = malloc( (num_env+num_sys)*sizeof(vartype) );
    if (next_state == NULL || candidate_state == NULL || finit_state == NULL
        || fnext_state == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    cube = malloc( 2*(num_env+num_sys)*sizeof(int) );
    if (cube == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    tmp = Cudd_bddVarMap( manager, W );
    if (tmp == NULL) {
        fprintf( stderr,
                 "Error sim_rhc: Error in swapping variables with primed"
                 " forms.\n" );
        return NULL;
    }
    Cudd_Ref( tmp );
    strans_into_W = Cudd_bddAnd( manager, strans, tmp );
    Cudd_Ref( strans_into_W );
    Cudd_RecursiveDeref( manager, tmp );

    play = insert_anode( NULL, current_it, -1, False,
                         init_state, num_env+num_sys );
    while (current_it < num_it) {
        if (verbose)
            logprint( "Beginning simulation iteration %d...", current_it );
        current_it++;

        /* Check if time to switch attention to next goal. */
        state_to_cube( init_state, cube, num_env+num_sys );
        ddval = Cudd_Eval( manager, *(sgoals+current_goal), cube );
        if (!Cudd_IsComplement( ddval )) {
            current_goal = (current_goal+1) % spc.num_sgoals;
            free( MEM );
            MEM = NULL;
            MEM_len = 0;
        }

        for (MEM_index = 0; MEM_index < MEM_len; MEM_index++) {
            if (statecmp( (*(MEM+MEM_index))->state, init_state,
                          num_env+num_sys ))
                break;
        }
        if (MEM_index >= MEM_len) {
            MEM_len++;
            MEM_index = MEM_len-1;
            MEM = realloc( MEM, MEM_len*sizeof(anode_t *) );
            *(MEM+MEM_index) = insert_anode( NULL, 0, -1, False,
                                             init_state, num_env+num_sys );
        }

        env_moves = get_env_moves( manager, cube,
                                   init_state, etrans,
                                   num_env, num_sys,
                                   &emoves_len );
        emove_index = rand() % emoves_len;

        tmp = state_to_cof( manager, cube, 2*(num_env+num_sys), init_state,
                         strans_into_W, 0, num_env+num_sys );
        tmp2 = state_to_cof( manager, cube, 2*(num_env+num_sys),
                          *(env_moves+emove_index), tmp,
                          num_env+num_sys, num_env );
        Cudd_RecursiveDeref( manager, tmp );

        tmp = Cudd_bddAnd( manager, *(sgoals+current_goal), W );
        Cudd_Ref( tmp );

        next_min = -1.;
        Cudd_AutodynDisable( manager );
        Cudd_ForeachCube( manager, tmp2, gen, gcube, gvalue ) {
            for (i = 0; i < num_env; i++)
                *(candidate_state+i) = *(*(env_moves+emove_index)+i);
            initialize_cube( candidate_state+num_env,
                             gcube+num_sys+2*num_env, num_sys );
            while (!saturated_cube( candidate_state+num_env,
                                    gcube+num_sys+2*num_env, num_sys )) {
                if (find_anode( *hstacks, 0,
                                candidate_state, num_env+num_sys ) == NULL) {
                    *hstacks = insert_anode( *hstacks, 0, -1, False,
                                             candidate_state, num_env+num_sys );

                    node = (*(MEM+MEM_index))->next;
                    while (node) {
                        if (statecmp( node->state, candidate_state,
                                      num_env+num_sys ))
                            break;
                        node = node->next;
                    }
                    if (node == NULL) {
                        bounds_state( manager, tmp, candidate_state,
                                      offw, num_metric_vars, &Min, &Max, 0 );
                        if (next_min == -1. || Min < next_min) {
                            next_min = Min;
                            for (i = 0; i < num_env+num_sys; i++)
                                *(next_state+i) = *(candidate_state+i);
                        }
                    }
                }

                increment_cube( candidate_state+num_env,
                                gcube+num_sys+2*num_env, num_sys );
            }
            if (find_anode( *hstacks, 0,
                            candidate_state, num_env+num_sys ) == NULL) {
                *hstacks = insert_anode( *hstacks, 0, -1, False,
                                         candidate_state, num_env+num_sys );

                node = (*(MEM+MEM_index))->next;
                while (node) {
                    if (statecmp( node->state, candidate_state,
                                  num_env+num_sys ))
                        break;
                    node = node->next;
                }
                if (node == NULL) {
                    bounds_state( manager, tmp, candidate_state,
                                  offw, num_metric_vars, &Min, &Max, 0 );
                    if (next_min == -1. || Min < next_min) {
                        next_min = Min;
                        for (i = 0; i < num_env+num_sys; i++)
                            *(next_state+i) = *(candidate_state+i);
                    }
                }
            }
        }
        Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
        Cudd_RecursiveDeref( manager, tmp2 );
        Cudd_RecursiveDeref( manager, tmp );

        if (verbose)
            logprint( "\t%d possible states at horizon 1.",
                      aut_size( *hstacks ) );

        for (i = 0; i < emoves_len; i++)
            free( *(env_moves+i) );
        free( env_moves );

        for (hdepth = 1; hdepth < horizon; hdepth++) {

            node = *(hstacks+hdepth-1);
            while (node) {
                for (i = 0; i < num_env+num_sys; i++)
                    *(finit_state+i) = *(node->state+i);

                env_moves = get_env_moves( manager, cube,
                                           finit_state, etrans,
                                           num_env, num_sys,
                                           &emoves_len );
                for (emove_index = 0; emove_index < emoves_len; emove_index++) {

                    tmp = state_to_cof( manager, cube, 2*(num_env+num_sys),
                                     finit_state, strans_into_W,
                                     0, num_env+num_sys );
                    tmp2 = state_to_cof( manager, cube, 2*(num_env+num_sys),
                                      *(env_moves+emove_index), tmp,
                                      num_env+num_sys, num_env );
                    Cudd_RecursiveDeref( manager, tmp );

                    tmp = Cudd_bddAnd( manager, *(sgoals+current_goal), W );
                    Cudd_Ref( tmp );

                    Cudd_AutodynDisable( manager );
                    Cudd_ForeachCube( manager, tmp2, gen, gcube, gvalue ) {
                        for (i = 0; i < num_env; i++)
                            *(fnext_state+i) = *(*(env_moves+emove_index)+i);
                        initialize_cube( fnext_state+num_env,
                                         gcube+num_sys+2*num_env, num_sys );
                        while (!saturated_cube( fnext_state+num_env,
                                                gcube+num_sys+2*num_env,
                                                num_sys )) {
                            for (j = 0; j <= hdepth; j++) {
                                if (find_anode( *(hstacks+j), 0,
                                                fnext_state, num_env+num_sys )
                                    != NULL)
                                    break;
                            }
                            if (j > hdepth) {
                                /* First time to find this state */
                                *(hstacks+hdepth)
                                    = insert_anode( *(hstacks+hdepth), 0, -1,
                                                    False,
                                                    fnext_state,
                                                    num_env+num_sys );

                                prev_node = (*(MEM+MEM_index))->next;
                                while (prev_node) {
                                    if (statecmp( prev_node->state, fnext_state,
                                                  num_env+num_sys ))
                                        break;
                                    prev_node = prev_node->next;
                                }
                                if (prev_node == NULL) {
                                    bounds_state( manager, tmp, fnext_state,
                                                  offw, num_metric_vars,
                                                  &Min, &Max, 0 );
                                    if (next_min == -1. || Min < next_min) {
                                        next_min = Min;
                                        for (i = 0; i < num_env+num_sys; i++)
                                            *(next_state+i) = *(fnext_state+i);
                                    }
                                }
                            }

                            increment_cube( fnext_state+num_env,
                                            gcube+num_sys+2*num_env, num_sys );
                        }
                        for (j = 0; j <= hdepth; j++) {
                            if (find_anode( *(hstacks+j), 0,
                                            fnext_state, num_env+num_sys )
                                != NULL)
                                break;
                        }
                        if (j > hdepth) {
                            *(hstacks+hdepth)
                                = insert_anode( *(hstacks+hdepth), 0, -1, False,
                                                fnext_state, num_env+num_sys );

                            prev_node = (*(MEM+MEM_index))->next;
                            while (prev_node) {
                                if (statecmp( prev_node->state, fnext_state,
                                              num_env+num_sys ))
                                    break;
                                prev_node = prev_node->next;
                            }
                            if (prev_node == NULL) {
                                bounds_state( manager, tmp, fnext_state,
                                              offw, num_metric_vars,
                                              &Min, &Max, 0 );
                                if (next_min == -1. || Min < next_min) {
                                    next_min = Min;
                                    for (i = 0; i < num_env+num_sys; i++)
                                        *(next_state+i) = *(fnext_state+i);
                                }
                            }
                        }
                    }
                    Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
                    Cudd_RecursiveDeref( manager, tmp2 );
                    Cudd_RecursiveDeref( manager, tmp );
                }

                for (i = 0; i < emoves_len; i++)
                    free( *(env_moves+i) );
                free( env_moves );
                node = node->next;
            }

            if (verbose)
                logprint( "\t%d possible states at horizon %d.",
                          aut_size( *(hstacks+hdepth) ), hdepth+1 );
        }

        node = *(MEM+MEM_index);
        while (node->next)
            node = node->next;
        node->next = insert_anode( NULL, 0, -1, False,
                                   next_state, num_env+num_sys );

        if (horizon > 1 && find_anode( *hstacks, 0,
                                       next_state, num_env+num_sys ) == NULL) {
            /* Treat horizon of 1 as special case. */
            for (j = 1; j < horizon; j++) {
                if ((node = find_anode( *(hstacks+j), 0,
                                        next_state, num_env+num_sys )) != NULL)
                    break;
            }
            if (j >= horizon) {
                fprintf( stderr, "ERROR: failed to backtrack in sim_rhc().\n" );
                return NULL;
            }

            tmp = Cudd_bddAnd( manager, etrans, strans );
            Cudd_Ref( tmp );

            while (j > 0) {
                j--;
                prev_node = *(hstacks+j);
                while (prev_node) {
                    for (i = 0; i < num_env+num_sys; i++)
                        *(cube+i) = *(prev_node->state+i);
                    for (i = 0; i < num_env+num_sys; i++)
                        *(cube+num_env+num_sys+i) = *(node->state+i);
                    ddval = Cudd_Eval( manager, tmp, cube );
                    if (!Cudd_IsComplement( ddval )) {
                        node = prev_node;
                        break;
                    }

                    prev_node = prev_node->next;
                }
                if (prev_node == NULL) {
                    fprintf( stderr,
                             "ERROR: failed to backtrack in sim_rhc().\n" );
                    return NULL;
                }
            }
            for (i = 0; i < num_env+num_sys; i++)
                *(next_state+i) = *(node->state+i);

            Cudd_RecursiveDeref( manager, tmp );
        }

        play = insert_anode( play, current_it, -1, False,
                             next_state, num_env+num_sys );
        play = append_anode_trans( play, current_it-1, init_state,
                                   num_env+num_sys, current_it, next_state );
        for (i = 0; i < num_env+num_sys; i++)
            *(init_state+i) = *(next_state+i);
    }


    Cudd_RecursiveDeref( manager, strans_into_W );
    free( next_state );
    free( candidate_state );
    free( finit_state );
    free( fnext_state );
    free( hstacks );
    free( cube );
    free( offw );
    return play;
}
