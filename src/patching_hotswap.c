/* patching_hotswap.c -- More definitions for signatures in patching.h.
 *
 *
 * SCL; 2013-2015
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "logging.h"
#include "automaton.h"
#include "ptree.h"
#include "patching.h"
#include "solve.h"
#include "solve_support.h"
#include "solve_metric.h"


extern specification_t spc;


/* Defined in patching.c */
extern void pprint_state( vartype *state, int num_env, int num_sys, FILE *fp );

/* Defined in patching_support.c */
extern anode_t *synthesize_reachgame_BDD( DdManager *manager,
                                          int num_env, int num_sys,
                                          DdNode *Entry, DdNode *Exit,
                                          DdNode *etrans, DdNode *strans,
                                          DdNode **egoals, DdNode *N_BDD,
                                          unsigned char verbose );


anode_t *add_metric_sysgoal( DdManager *manager, FILE *strategy_fp,
                             int original_num_env, int original_num_sys,
                             int *offw, int num_metric_vars,
                             ptree_t *new_sysgoal, unsigned char verbose )
{
    ptree_t *var_separator;
    DdNode *etrans, *strans, **egoals, **sgoals;
    bool env_nogoal_flag = False;  /* Indicate environment has no goals */

    anode_t *strategy = NULL, *component_strategy;
    int num_env, num_sys;
    anode_t *node1, *node2;

    double *Min, *Max;  /* One (Min, Max) pair per original system goal */

    int Gi_len[2];
    anode_t **Gi[2];  /* Array of two pointers to arrays of nodes in
                         the given strategy between which we wish to
                         insert the new system goal. */
    int istar[2];  /* Indices of system goals between which to insert. */
    DdNode *Gi_BDD = NULL;
    DdNode *new_sgoal = NULL;
    anode_t **new_reached = NULL;
    int new_reached_len = 0;

    anode_t **Gi_succ = NULL;
    int Gi_succ_len = 0;

    int i, j, k;  /* Generic counters */
    bool found_flag;
    int node_counter;
    DdNode *tmp, *tmp2;
    DdNode **vars, **pvars;

    if (strategy_fp == NULL)
        strategy_fp = stdin;

    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

    strategy = aut_aut_load( original_num_env+original_num_sys, strategy_fp );
    if (strategy == NULL) {
        return NULL;
    }
    if (verbose)
        logprint( "Read in strategy of size %d", aut_size( strategy ) );

    if (verbose > 1)
        logprint( "Expanding nonbool variables in the given strategy"
                  " automaton..." );
    if (aut_expand_bool( strategy, spc.evar_list, spc.svar_list, spc.nonbool_var_list )) {
        fprintf( stderr,
                 "Error add_metric_sysgoal: Failed to expand nonboolean"
                 " variables in given automaton." );
        return NULL;
    }
    if (verbose > 1) {
        logprint( "Given strategy after variable expansion:" );
        logprint_startline();
        dot_aut_dump( strategy, spc.evar_list, spc.svar_list, DOT_AUT_ATTRIB,
                      getlogstream() );
        logprint_endline();
    }

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

    new_sgoal = ptree_BDD( new_sysgoal, spc.evar_list, manager );

    if (var_separator == NULL) {
        spc.evar_list = NULL;
    } else {
        var_separator->left = NULL;
    }


    /* Find which original system goal is closest to the new one */
    if (offw != NULL && num_metric_vars > 0) {
        Min = malloc( spc.num_sgoals*sizeof(double) );
        Max = malloc( spc.num_sgoals*sizeof(double) );
        if (Min == NULL || Max == NULL) {
            perror( __FILE__ ",  malloc" );
            exit(-1);
        }
        for (i = 0; i < spc.num_sgoals; i++) {
            if (bounds_DDset( manager, *(sgoals+i), new_sgoal,
                              offw, num_metric_vars, Min+i, Max+i, verbose )) {
                fprintf( stderr,
                         "Error add_metric_sysgoal: bounds_DDset() failed to"
                         " compute distance from original goal %d",
                         i );
                return NULL;
            }
        }
        istar[0] = 0;
        for (i = 1; i < spc.num_sgoals; i++) {
            if (*(Min+i) < *(Min+istar[0]))
                istar[0] = i;
        }
        if (verbose)
            logprint( "add_metric_sysgoal: The original system goal with"
                      " index %d has minimum distance",
                      istar[0] );
    } else {
        Min = Max = NULL;
        istar[0] = spc.num_sgoals-1;

        if (verbose)
            logprint( "No metric variables given, so using sys goal mode i* = %d", istar[0] );
    }

    if (istar[0] == spc.num_sgoals-1) {
        istar[1] = 0;
    } else {
        istar[1] = istar[0]+1;
    }
    Gi[0] = Gi[1] = NULL;
    Gi_len[0] = Gi_len[1] = 0;

    /* Find i* and i*+1 nodes */
    for (i = 0; i < 2; i++) {
        if (verbose > 1)
            logprint( "Adding nodes to set G_{%d}:", istar[i] );
        node_counter = 0;
        node1 = strategy;
        while (node1) {
            node2 = strategy;
            found_flag = False;
            while (node2 && !found_flag) {
                for (j = 0; j < node2->trans_len; j++) {
                    if (*(node2->trans+j) == node1) {
                        if ((node2->mode < node1->mode
                             && node2->mode <= istar[i]
                             && node1->mode > istar[i])
                            || (node2->mode > node1->mode
                                && (node1->mode > istar[i]
                                    || node2->mode <= istar[i]))) {

                            Gi_len[i]++;
                            Gi[i] = realloc( Gi[i],
                                             Gi_len[i]*sizeof(anode_t *) );
                            if (Gi[i] == NULL) {
                                perror( __FILE__ ",  realloc" );
                                exit(-1);
                            }

                            if (verbose > 1) {
                                logprint_startline();
                                logprint_raw( "\t%d : ", node_counter );
                                pprint_state( node1->state, num_env, num_sys,
                                              getlogstream() );
                                logprint_raw( " - %d", node1->mode );
                                logprint_endline();
                            }
                            *(Gi[i]+Gi_len[i]-1) = node1;

                            found_flag = True;
                        }
                        break;
                    }
                }

                node2 = node2->next;
            }

            node_counter++;
            node1 = node1->next;
        }
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
        return NULL;
    }
    free( vars );
    free( pvars );


    /* Build characteristic function for G_{i*} set. */
    Gi_BDD = Cudd_Not( Cudd_ReadOne( manager ) );
    Cudd_Ref( Gi_BDD );
    for (i = 0; i < Gi_len[0]; i++) {
        tmp2 = state_to_BDD( manager, (*(Gi[0]+i))->state, 0, num_env+num_sys );
        tmp = Cudd_bddOr( manager, Gi_BDD, tmp2 );
        Cudd_Ref( tmp );
        Cudd_RecursiveDeref( manager, Gi_BDD );
        Cudd_RecursiveDeref( manager, tmp2 );
        Gi_BDD = tmp;
    }
    tmp2 = NULL;

    tmp = Cudd_ReadOne( manager );
    Cudd_Ref( tmp );
    component_strategy = synthesize_reachgame_BDD( manager, num_env, num_sys,
                                                   Gi_BDD, new_sgoal,
                                                   etrans, strans, egoals, tmp,
                                                   verbose );
    Cudd_RecursiveDeref( manager, tmp );
    if (component_strategy == NULL) {
        delete_aut( strategy );
        return NULL;  /* Failure */
    }
    if (verbose > 1) {
        logprint( "Component strategy to reach the new system goal from"
                  " G_{i*}:" );
        logprint_startline();
        dot_aut_dump( component_strategy, spc.evar_list, spc.svar_list,
                      DOT_AUT_BINARY | DOT_AUT_ATTRIB, getlogstream() );
        logprint_endline();
    }

    Cudd_RecursiveDeref( manager, Gi_BDD );
    Gi_BDD = NULL;


    /* Find which states were actually reached before building
       component strategy back into the given (original) strategy. */
    node1 = component_strategy;
    while (node1) {
        if (node1->trans_len == 0) {
            new_reached_len++;
            new_reached = realloc(new_reached,
                                  new_reached_len*sizeof(anode_t *) );
            if (new_reached == NULL) {
                perror( __FILE__ ",  realloc" );
                exit(-1);
            }
            *(new_reached+new_reached_len-1) = node1;
        }

        node1 = node1->next;
    }


    node1 = component_strategy;
    while (node1) {
        /* Temporary mode label for this component. */
        node1->mode = spc.num_sgoals+1;
        node1 = node1->next;
    }

    /* From original to first component... */
    for (i = 0; i < Gi_len[0]; i++) {
        if ((*(Gi[0]+i))->trans_len > 0) {
            Gi_succ = realloc( Gi_succ,
                               (Gi_succ_len + (*(Gi[0]+i))->trans_len)
                               *sizeof(anode_t *) );
            if (Gi_succ == NULL) {
                perror( __FILE__ ",  realloc" );
                exit(-1);
            }
            for (j = 0; j < (*(Gi[0]+i))->trans_len; j++) {
                for (k = 0; k < Gi_len[0]; k++) {
                    if (*((*(Gi[0]+i))->trans+j) == *(Gi[0]+k))
                        break;
                }
                if (k < Gi_len[0])
                    continue;  /* Do not prune members of Gi */
                *(Gi_succ+Gi_succ_len) = *((*(Gi[0]+i))->trans+j);
                Gi_succ_len++;
            }
        }

        (*(Gi[0]+i))->trans_len = 0;
        free( (*(Gi[0]+i))->trans );
        (*(Gi[0]+i))->trans = NULL;

        /* Temporary mode label for this component. */
        (*(Gi[0]+i))->mode = spc.num_sgoals+1;

        node1 = component_strategy;
        while (node1) {
            if (statecmp( node1->state, (*(Gi[0]+i))->state,
                          num_env+num_sys )) {
                (*(Gi[0]+i))->trans_len = node1->trans_len;
                (*(Gi[0]+i))->trans = node1->trans;
                node1->trans = NULL;
                node1->trans_len = 0;
                break;
            }
            node1 = node1->next;
        }
        if (node1 == NULL) {
            fprintf( stderr,
                     "Error add_metric_sysgoal: component automaton is not"
                     " compatible with original." );
            delete_aut( strategy );
            return NULL;
        }

        /* Delete the replaced node */
        replace_anode_trans( strategy, node1, *(Gi[0]+i) );
        replace_anode_trans( component_strategy, node1, *(Gi[0]+i) );
        component_strategy = delete_anode( component_strategy, node1 );
    }

    node1 = strategy;
    while (node1->next)
        node1 = node1->next;
    node1->next = component_strategy;

    if (verbose > 1) {
        logprint( "Partially patched strategy before de-expanding variables:" );
        logprint_startline();
        dot_aut_dump( strategy, spc.evar_list, spc.svar_list,
                      DOT_AUT_BINARY | DOT_AUT_ATTRIB, getlogstream() );
        logprint_endline();
    }


    tmp = Cudd_ReadOne( manager );
    Cudd_Ref( tmp );
    component_strategy = synthesize_reachgame( manager, num_env, num_sys,
                                               new_reached, new_reached_len,
                                               Gi[1], Gi_len[1],
                                               etrans, strans, egoals, tmp,
                                               verbose );
    Cudd_RecursiveDeref( manager, tmp );
    if (component_strategy == NULL) {
        delete_aut( strategy );
        return NULL;  /* Failure */
    }
    if (verbose > 1) {
        logprint( "Component strategy to reach G_{i*+1} from the new system"
                  " goal:" );
        logprint_startline();
        dot_aut_dump( component_strategy, spc.evar_list, spc.svar_list,
                      DOT_AUT_BINARY | DOT_AUT_ATTRIB, getlogstream() );
        logprint_endline();
    }

    /* From first component to second component... */
    for (i = 0; i < new_reached_len; i++) {
        /* Temporary mode label for this component. */
        (*(new_reached+i))->mode = -1;
        node1 = component_strategy;
        while (node1) {
            if (statecmp( node1->state, (*(new_reached+i))->state,
                          num_env+num_sys )) {
                (*(new_reached+i))->trans_len = node1->trans_len;
                (*(new_reached+i))->trans = node1->trans;
                node1->trans = NULL;
                node1->trans_len = 0;
                (*(new_reached+i))->rgrad = node1->rgrad;
                break;
            }
            node1 = node1->next;
        }
        if (node1 == NULL) {
            fprintf( stderr,
                     "Error add_metric_sysgoal: component automata are not"
                     " compatible." );
            delete_aut( strategy );
            return NULL;
        }

        replace_anode_trans( strategy, node1, *(new_reached+i) );
        replace_anode_trans( component_strategy, node1, *(new_reached+i) );
        component_strategy = delete_anode( component_strategy, node1 );
    }

    /* From second component into original... */
    node1 = component_strategy;
    while (node1) {
        if (node1->trans_len == 0) {

            for (i = 0; i < Gi_len[1]; i++) {
                if (statecmp( (*(Gi[1]+i))->state, node1->state,
                              num_env+num_sys )) {

                    node1->trans_len = (*(Gi[1]+i))->trans_len;
                    node1->trans = (*(Gi[1]+i))->trans;
                    (*(Gi[1]+i))->trans_len = 0;
                    (*(Gi[1]+i))->trans = NULL;
                    node1->rgrad = (*(Gi[1]+i))->rgrad;
                    node1->mode = (*(Gi[1]+i))->mode;
                    break;
                }
            }
            if (i == Gi_len[1]) {
                fprintf( stderr,
                         "Error add_metric_sysgoal: component automata are"
                         " not compatible." );
                delete_aut( strategy );
                return NULL;
            }

            replace_anode_trans( strategy, *(Gi[1]+i), node1 );
            replace_anode_trans( component_strategy, *(Gi[1]+i), node1 );
            strategy = delete_anode( strategy, *(Gi[1]+i) );

        }
        node1 = node1->next;
    }

    node1 = strategy;
    while (node1->next)
        node1 = node1->next;
    node1->next = component_strategy;

    strategy = forward_prune( strategy, Gi_succ, Gi_succ_len );
    if (strategy == NULL) {
        fprintf( stderr, "Error add_metric_sysgoal: pruning failed.\n" );
        return NULL;
    }
    Gi_succ = NULL;

    /* Update labels, thus completing the insertion process */
    if (istar[0] == spc.num_sgoals-1) {
        node1 = strategy;
        while (node1) {
            if (node1->mode == spc.num_sgoals+1) {
                node1->mode = spc.num_sgoals;
            } else if (node1->mode == -1) {
                node1->mode = 0;
            }
            node1 = node1->next;
        }
    } else {  /* istar[0] < istar[1] */
        for (i = spc.num_sgoals-1; i >= istar[1]; i--) {
            node1 = strategy;
            while (node1) {
                if (node1->mode == i)
                    (node1->mode)++;
                node1 = node1->next;
            }
        }
        node1 = strategy;
        while (node1) {
            if (node1->mode == spc.num_sgoals+1) {
                node1->mode = istar[1];
            } else if (node1->mode == -1) {
                node1->mode = istar[1]+1;
            }
            node1 = node1->next;
        }
    }


    /* Pre-exit clean-up */
    free( Min );
    free( Max );
    free( Gi[0] );
    free( Gi[1] );
    free( new_reached );
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

    return strategy;
}


anode_t *rm_sysgoal( DdManager *manager, FILE *strategy_fp,
                     int original_num_env, int original_num_sys,
                     int delete_i, unsigned char verbose )
{
    ptree_t *var_separator;
    DdNode *etrans, *strans, **egoals;
    bool env_nogoal_flag = False;  /* Indicate environment has no goals */

    anode_t *strategy = NULL;
    int num_env, num_sys;
    int num_del_nodes = 0;

    anode_t *substrategy;
    anode_t **Entry;
    anode_t **Exit;
    int Entry_len = 0, Exit_len = 0;
    anode_t *node;

    int i, j;
    DdNode **vars, **pvars;
    DdNode *tmp;

    if (strategy_fp == NULL)
        strategy_fp = stdin;

    if (delete_i < 0 || delete_i >= spc.num_sgoals) {
        logprint( "Error rm_sysgoal: invoked with goal index %d outside"
                  " bounds [0,%d]",
                  delete_i,
                  spc.num_sgoals-1 );
        return NULL;
    }
    if (spc.num_sgoals < 3) {
        logprint( "Error rm_sysgoal: Current implementation requires at"
                  " least 3 initial system goals." );
        return NULL;
    }

    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

    strategy = aut_aut_load( original_num_env+original_num_sys, strategy_fp );
    if (strategy == NULL) {
        return NULL;
    }
    if (verbose)
        logprint( "Read in strategy of size %d", aut_size( strategy ) );

    if (verbose > 1)
        logprint( "Expanding nonbool variables in the given strategy"
                  " automaton..." );
    if (aut_expand_bool( strategy,
                         spc.evar_list, spc.svar_list, spc.nonbool_var_list )) {
        fprintf( stderr,
                 "Error rm_sysgoal: Failed to expand nonboolean variables"
                 " in given automaton." );
        return NULL;
    }
    if (verbose > 1) {
        logprint( "Given strategy after variable expansion:" );
        logprint_startline();
        dot_aut_dump( strategy, spc.evar_list, spc.svar_list, DOT_AUT_ATTRIB,
                      getlogstream() );
        logprint_endline();
    }

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

    if (var_separator == NULL) {
        spc.evar_list = NULL;
    } else {
        var_separator->left = NULL;
    }


    node = strategy;
    num_del_nodes = 0;
    while (node) {
        if (node->mode == delete_i || node->mode == (delete_i+1)%spc.num_sgoals)
            num_del_nodes++;
        node = node->next;
    }
    if (verbose)
        logprint( "rm_sysgoal: Found %d automaton nodes annotated with mode"
                  " to-be-deleted (%d) or successor (%d)",
                  num_del_nodes,
                  delete_i,
                  (delete_i+1)%spc.num_sgoals );

    if (num_del_nodes == 0) {
        if (verbose)
            logprint( "rm_sysgoal: Did not find nodes to be deleted "
                      "nor nodes of successor mode." );
        return NULL;
    }

    /* Pre-allocate space for Entry and Exit sets; the number of
       elements actually used is tracked by Entry_len and Exit_len,
       respectively. */
    Entry = malloc( sizeof(anode_t *)*num_del_nodes );
    if (Entry == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);

    }
    Exit = malloc( sizeof(anode_t *)*num_del_nodes );
    if (Exit == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    node = strategy;
    while (node) {
        if (node->mode == delete_i || node->mode == (delete_i+1)%spc.num_sgoals) {

            for (i = 0; i < node->trans_len; i++) {
                if ((*(node->trans+i))->mode != (delete_i+1)%spc.num_sgoals
                    && (*(node->trans+i))->mode != delete_i) {
                    for (j = 0; j < Exit_len; j++)
                        if (*(Exit+j) == *(node->trans+i))
                            break;
                    if (j == Exit_len) {
                        Exit_len++;
                        *(Exit+Exit_len-1) = *(node->trans+i);
                    }
                }
            }

        }
        if (node->mode != delete_i) {

            for (i = 0; i < node->trans_len; i++) {
                if ((*(node->trans+i))->mode == delete_i) {
                    for (j = 0; j < Entry_len; j++)
                        if (*(Entry+j) == *(node->trans+i))
                            break;
                    if (j == Entry_len) {
                        Entry_len++;
                        *(Entry+Entry_len-1) = *(node->trans+i);
                    }
                }
            }

        }

        node = node->next;
    }

    if (verbose) {
        logprint( "Entry set:" );
        for (i = 0; i < Entry_len; i++) {
            logprint_startline();
            fprintf( getlogstream(),
                     " node with mode %d and state ", (*(Entry+i))->mode );
            pprint_state( (*(Entry+i))->state, num_env, num_sys,
                          getlogstream() );
            logprint_endline();
        }
        logprint( "Exit set:" );
        for (i = 0; i < Exit_len; i++) {
            logprint_startline();
            fprintf( getlogstream(),
                     " node with mode %d and state ", (*(Exit+i))->mode );
            pprint_state( (*(Exit+i))->state, num_env, num_sys,
                          getlogstream() );
            logprint_endline();
        }
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
        free( Entry );
        free( Exit );
        return NULL;
    }
    free( vars );
    free( pvars );


    tmp = Cudd_ReadOne( manager );
    Cudd_Ref( tmp );
    substrategy = synthesize_reachgame( manager, num_env, num_sys,
                                        Entry, Entry_len, Exit, Exit_len,
                                        etrans, strans, egoals, tmp,
                                        verbose );
    Cudd_RecursiveDeref( manager, tmp );
    if (substrategy == NULL) {
        free( Exit );
        free( Entry );
        /* XXX Need to free other resources, e.g., etrans */
        return NULL;
    }
    if (verbose > 1) {
        logprint( "rm_sysgoal: substrategy from Entry to Exit:" );
        logprint_startline();
        dot_aut_dump( substrategy, spc.evar_list, spc.svar_list,
                      DOT_AUT_BINARY | DOT_AUT_ATTRIB, getlogstream() );
        logprint_endline();
    }

    /* Connect local strategy to original */
    for (i = 0; i < Entry_len; i++) {
        node = substrategy;
        while (node && !statecmp( (*(Entry+i))->state, node->state,
                                  num_env+num_sys ))
            node = node->next;
        if (node == NULL) {
            fprintf( stderr,
                     "Error rm_sysgoal: expected Entry node"
                     " missing from local strategy" );
            return NULL;
        }

        node->initial = (*(Entry+i))->initial;
        replace_anode_trans( strategy, *(Entry+i), node );
    }

    node = substrategy;
    while (node) {
        if (node->trans_len == 0) {  /* Terminal node of the local strategy? */
            for (i = 0; i < Exit_len; i++) {
                if (*(Exit+i) == NULL)
                    continue;
                if (statecmp( node->state, (*(Exit+i))->state,
                              num_env+num_sys ))
                    break;
            }
            if (i == Exit_len) {
                fprintf( stderr,
                         "Error rm_sysgoal: terminal node in"
                         " local strategy does not have a\nmatch in Exit\n" );
                return NULL;
            }

            node->mode = (*(Exit+i))->mode;
            node->rgrad = (*(Exit+i))->rgrad;
            node->initial = (*(Exit+i))->initial;
            node->trans = (*(Exit+i))->trans;
            node->trans_len = (*(Exit+i))->trans_len;
            (*(Exit+i))->trans = NULL;
            (*(Exit+i))->trans_len = 0;
            replace_anode_trans( strategy, *(Exit+i), node );
            replace_anode_trans( substrategy, *(Exit+i), node );
            strategy = delete_anode( strategy, *(Exit+i) );
            *(Exit+i) = NULL;
        }
        node = node->next;
    }

    /* Delete nodes of the deleted and superseded goal modes */
    node = strategy;
    while (node) {
        if (node->mode == delete_i || node->mode == (delete_i+1)%spc.num_sgoals) {
            replace_anode_trans( strategy, node, NULL );
            replace_anode_trans( substrategy, node, NULL );
            strategy = delete_anode( strategy, node );
            node = strategy;
        } else {
            node = node->next;
        }
    }

    node = substrategy;
    while (node) {
        if (node->mode == -1) {
            node->mode = (delete_i+1)%spc.num_sgoals;
        }
        node = node->next;
    }

    /* Append new substrategy */
    if (strategy == NULL) {
        strategy = substrategy;
    } else {
        node = strategy;
        while (node->next)
            node = node->next;
        node->next = substrategy;
    }


    Cudd_RecursiveDeref( manager, etrans );
    Cudd_RecursiveDeref( manager, strans );
    for (i = 0; i < spc.num_egoals; i++)
        Cudd_RecursiveDeref( manager, *(egoals+i) );
    if (spc.num_egoals > 0)
        free( egoals );
    if (env_nogoal_flag) {
        spc.num_egoals = 0;
        delete_tree( *spc.env_goals );
        free( spc.env_goals );
    }
    free( Exit );
    free( Entry );

    return strategy;
}
