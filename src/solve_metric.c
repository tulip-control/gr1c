/* solve_metric.c -- Mostly definitions for signatures in solve_metric.h.
 *
 *
 * SCL; 2012-2015
 */


#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "common.h"
#include "logging.h"
#include "gr1c_util.h"
#include "ptree.h"
#include "solve.h"
#include "solve_support.h"
#include "solve_metric.h"


extern specification_t spc;


int *get_offsets( char *metric_vars, int *num_vars )
{
    char *var_str, *tok = NULL;
    int *offw = NULL;
    ptree_t *var, *var_tail;
    int start_index, stop_index;

    if (metric_vars == NULL)
        return NULL;
    var_str = strdup( metric_vars );
    if (var_str == NULL) {
        perror( __FILE__ ",  strdup" );
        exit(-1);
    }

    *num_vars = 0;
    tok = strtok( var_str, " " );
    if (tok == NULL) {
        free( var_str );
        return NULL;
    }
    do {
        (*num_vars)++;
        offw = realloc( offw, 2*(*num_vars)*sizeof(int) );
        if (offw == NULL) {
            perror( __FILE__ ",  realloc" );
            exit(-1);
        }

        var = spc.evar_list;
        start_index = 0;
        while (var) {
            if (strstr( var->name, tok ) == var->name)
                break;
            var = var->left;
            start_index++;
        }
        if (var == NULL) {
            var = spc.svar_list;
            while (var) {
                if (strstr( var->name, tok ) == var->name)
                    break;
                var = var->left;
                start_index++;
            }
            if (var == NULL) {
                fprintf( stderr, "Could not find match for \"%s\"\n", tok );
                free( offw );
                free( var_str );
                return NULL;
            }
        }

        var_tail = var;
        stop_index = start_index;
        while (var_tail->left) {
            if (strstr( var_tail->left->name, tok ) != var_tail->left->name )
                break;
            var_tail = var_tail->left;
            stop_index++;
        }

        *(offw+2*((*num_vars)-1)) = start_index;
        *(offw+2*((*num_vars)-1)+1) = stop_index-start_index+1;
    } while ((tok = strtok( NULL, " " )));

    free( var_str );
    return offw;
}


int bounds_state( DdManager *manager, DdNode *T, vartype *ref_state,
                  int *offw, int num_metric_vars,
                  double *Min, double *Max, unsigned char verbose )
{
    vartype *state;
    double dist;
    int num_env, num_sys;
    int i;
    int *ref_mapped, *this_mapped;

    /* Variables used during CUDD generation (state enumeration). */
    DdGen *gen;
    CUDD_VALUE_TYPE gvalue;
    int *gcube;

    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

    /* State vector (i.e., valuation of the variables) */
    state = malloc( sizeof(vartype)*(num_env+num_sys) );
    if (state == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    /* Reference and particular integral vectors */
    ref_mapped = malloc( num_metric_vars*sizeof(int) );
    this_mapped = malloc( num_metric_vars*sizeof(int) );
    if (ref_mapped == NULL || this_mapped == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    for (i = 0; i < num_metric_vars; i++)
        *(ref_mapped+i) = bitvec_to_int( ref_state+(*(offw+2*i)),
                                         *(offw+2*i+1) );
    *Min = *Max = -1.;  /* Distance is non-negative; thus use -1 as "unset". */

    Cudd_AutodynDisable( manager );
    Cudd_ForeachCube( manager, T, gen, gcube, gvalue ) {
        initialize_cube( state, gcube, num_env+num_sys );
        while (!saturated_cube( state, gcube, num_env+num_sys )) {

            for (i = 0; i < num_metric_vars; i++)
                *(this_mapped+i) = bitvec_to_int( state+(*(offw+2*i)),
                                                  *(offw+2*i+1) );

            /* 2-norm derived metric */
            /* dist = 0.; */
            /* for (i = 0; i < num_metric_vars; i++) */
            /*     dist += pow(*(this_mapped+i) - *(ref_mapped+i), 2); */
            /* dist = sqrt( dist ); */

            /* 1-norm derived metric */
            dist = 0.;
            for (i = 0; i < num_metric_vars; i++)
                dist += fabs(*(this_mapped+i) - *(ref_mapped+i));
            if (*Min == -1. || dist < *Min)
                *Min = dist;
            if (*Max == -1. || dist > *Max)
                *Max = dist;

            increment_cube( state, gcube, num_env+num_sys );
        }

        for (i = 0; i < num_metric_vars; i++)
            *(this_mapped+i) = bitvec_to_int( state+(*(offw+2*i)),
                                              *(offw+2*i+1) );

        /* 2-norm derived metric */
        /* dist = 0.; */
        /* for (i = 0; i < num_metric_vars; i++) */
        /*     dist += pow(*(this_mapped+i) - *(ref_mapped+i), 2); */
        /* dist = sqrt( dist ); */

        /* 1-norm derived metric */
        dist = 0.;
        for (i = 0; i < num_metric_vars; i++)
            dist += fabs(*(this_mapped+i) - *(ref_mapped+i));
        if (*Min == -1. || dist < *Min)
            *Min = dist;
        if (*Max == -1. || dist > *Max)
            *Max = dist;

    }
    Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );

    free( state );
    free( ref_mapped );
    free( this_mapped );
    return 0;
}


int bounds_DDset( DdManager *manager, DdNode *T, DdNode *G,
                  int *offw, int num_metric_vars,
                  double *Min, double *Max, unsigned char verbose )
{
    vartype **states = NULL;
    int num_states = 0;
    vartype *state;
    double tMin, tMax;  /* Particular distance to goal set */
    int num_env, num_sys;
    int i, k;
    int *mapped_state;

    /* Variables used during CUDD generation (state enumeration). */
    DdGen *gen;
    CUDD_VALUE_TYPE gvalue;
    int *gcube;

    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

    state = malloc( (num_env+num_sys)*sizeof(vartype) );
    if (state == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    *Min = *Max = -1.;  /* Distance is non-negative; thus use -1 as "unset". */

    Cudd_AutodynDisable( manager );
    Cudd_ForeachCube( manager, T, gen, gcube, gvalue ) {
        initialize_cube( state, gcube, num_env+num_sys );
        while (!saturated_cube( state, gcube, num_env+num_sys )) {

            num_states++;
            states = realloc( states, num_states*sizeof(vartype *) );
            if (states == NULL) {
                perror( __FILE__ ",  realloc" );
                exit(-1);
            }
            *(states+num_states-1) = malloc( (num_env+num_sys)
                                             *sizeof(vartype) );
            if (*(states+num_states-1) == NULL) {
                perror( __FILE__ ",  malloc" );
                exit(-1);
            }
            for (i = 0; i < num_env+num_sys; i++)
                *(*(states+num_states-1)+i) = *(state+i);

            increment_cube( state, gcube, num_env+num_sys );
        }

        num_states++;
        states = realloc( states, num_states*sizeof(vartype *) );
        if (states == NULL) {
            perror( __FILE__ ",  realloc" );
            exit(-1);
        }

        *(states+num_states-1) = malloc( (num_env+num_sys)*sizeof(vartype) );
        if (*(states+num_states-1) == NULL) {
            perror( __FILE__ ",  malloc" );
            exit(-1);
        }
        for (i = 0; i < num_env+num_sys; i++)
            *(*(states+num_states-1)+i) = *(state+i);

    }
    Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );

    mapped_state = malloc( num_metric_vars*sizeof(int) );
    if (mapped_state == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }
    for (k = 0; k < num_states; k++) {
        bounds_state( manager, G, *(states+k), offw, num_metric_vars,
                      &tMin, &tMax, verbose );
        if (*Min == -1. || tMin < *Min)
            *Min = tMin;
        if (*Max == -1. || tMin > *Max)
            *Max = tMin;

        if (verbose > 1) {
            for (i = 0; i < num_metric_vars; i++)
                *(mapped_state+i) = bitvec_to_int( *(states+k)+(*(offw+2*i)),
                                                   *(offw+2*i+1) );
            logprint_startline();
            logprint_raw( "\t" );
            for (i = 0; i < num_metric_vars; i++)
                logprint_raw( "%d, ", *(mapped_state+i) );
            logprint_raw( "mi = %f", tMin );
            logprint_endline();
        }
    }

    free( mapped_state );
    for (i = 0; i < num_states; i++)
        free( *(states+i) );
    free( states );
    free( state );
    return 0;
}


/* Construct BDDs (characteristic functions of) etrans, strans,
   egoals, and sgoals as required by compute_winning_set_BDD() but
   save the result.  The motivating use-case is to compute these once
   and then provide them to later functions as needed. */
DdNode *compute_winning_set_saveBDDs( DdManager *manager,
                                      DdNode **etrans, DdNode **strans,
                                      DdNode ***egoals, DdNode ***sgoals,
                                      unsigned char verbose )
{
    int i;
    ptree_t *var_separator;
    DdNode *W;

    if (spc.num_egoals == 0) {
        spc.num_egoals = 1;
        spc.env_goals = malloc( sizeof(ptree_t *) );
        *spc.env_goals = init_ptree( PT_CONSTANT, NULL, 1 );
    }

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

    if (verbose > 1)
        logprint( "Building environment transition BDD..." );
    (*etrans) = ptree_BDD( spc.env_trans, spc.evar_list, manager );
    if (verbose > 1) {
        logprint( "Done." );
        logprint( "Building system transition BDD..." );
    }
    (*strans) = ptree_BDD( spc.sys_trans, spc.evar_list, manager );
    if (verbose > 1)
        logprint( "Done." );

    /* Build goal BDDs, if present. */
    if (spc.num_egoals > 0) {
        (*egoals) = malloc( spc.num_egoals*sizeof(DdNode *) );
        for (i = 0; i < spc.num_egoals; i++)
            *((*egoals)+i) = ptree_BDD( *(spc.env_goals+i), spc.evar_list, manager );
    } else {
        (*egoals) = NULL;
    }
    if (spc.num_sgoals > 0) {
        (*sgoals) = malloc( spc.num_sgoals*sizeof(DdNode *) );
        for (i = 0; i < spc.num_sgoals; i++)
            *((*sgoals)+i) = ptree_BDD( *(spc.sys_goals+i), spc.evar_list, manager );
    } else {
        (*sgoals) = NULL;
    }

    if (var_separator == NULL) {
        spc.evar_list = NULL;
    } else {
        var_separator->left = NULL;
    }

    W = compute_winning_set_BDD( manager,
                                 (*etrans), (*strans), (*egoals), (*sgoals),
                                 verbose );
    if (W == NULL) {
        fprintf( stderr,
                 "Error compute_winning_set_saveBDDs: failed to construct"
                 " winning set.\n" );
        return NULL;
    }

    return W;
}


int compute_minmax( DdManager *manager,DdNode **W,
                    DdNode **etrans, DdNode **strans, DdNode ***sgoals,
                    int **num_sublevels, double ***Min, double ***Max,
                    int *offw, int num_metric_vars,
                    unsigned char verbose )
{
    DdNode **egoals;
    DdNode ***Y = NULL;
    DdNode ****X_ijr = NULL;
    bool env_nogoal_flag = False;
    int i, j, r;
    DdNode *tmp, *tmp2;

    if (spc.num_egoals == 0)
        env_nogoal_flag = True;

    *W = compute_winning_set_saveBDDs( manager, etrans, strans, &egoals, sgoals,
                                       verbose );
    Y = compute_sublevel_sets( manager, *W, (*etrans), (*strans),
                               egoals, spc.num_egoals,
                               (*sgoals), spc.num_sgoals,
                               num_sublevels, &X_ijr, verbose );
    if (Y == NULL) {
        fprintf( stderr,
                 "Error compute_minmax: failed to construct sublevel sets.\n" );
        return -1;
    }

    *Min = malloc( spc.num_sgoals*sizeof(double *) );
    *Max = malloc( spc.num_sgoals*sizeof(double *) );
    if (*Min == NULL || *Max == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    for (i = 0; i < spc.num_sgoals; i++) {
        *(*Min + i) = malloc( (*(*num_sublevels+i)-1)*sizeof(double) );
        *(*Max + i) = malloc( (*(*num_sublevels+i)-1)*sizeof(double) );
        if (*(*Min + i) == NULL || *(*Max + i) == NULL) {
            perror( __FILE__ ",  malloc" );
            exit(-1);
        }

        *(*(*Min+i)) = *(*(*Max+i)) = 0;
        for (j = 1; j < *(*num_sublevels+i)-1; j++) {
            if (verbose > 1)
                logprint( "goal %d, level %d...", i, j );
            tmp = Cudd_bddAnd( manager,
                               *(*(Y+i)+j+1), Cudd_Not( *(*(Y+i)+j) ) );
            Cudd_Ref( tmp );
            tmp2 = Cudd_bddAnd( manager, *((*sgoals)+i), *W );
            Cudd_Ref( tmp2 );
            if (bounds_DDset( manager, tmp, tmp2, offw, num_metric_vars,
                              *(*Min+i)+j, *(*Max+i)+j, verbose )) {
                *(*(*Min+i)+j) = *(*(*Max+i)+j) = -1.;
            }
            Cudd_RecursiveDeref( manager, tmp );
            Cudd_RecursiveDeref( manager, tmp2 );
        }
    }


    /* Pre-exit clean-up */
    for (i = 0; i < spc.num_egoals; i++)
        Cudd_RecursiveDeref( manager, *(egoals+i) );
    if (spc.num_egoals > 0)
        free( egoals );

    for (i = 0; i < spc.num_sgoals; i++) {
        for (j = 0; j < *(*num_sublevels+i); j++) {
            Cudd_RecursiveDeref( manager, *(*(Y+i)+j) );
            for (r = 0; r < spc.num_egoals; r++) {
                Cudd_RecursiveDeref( manager, *(*(*(X_ijr+i)+j)+r) );
            }
            free( *(*(X_ijr+i)+j) );
        }
        if (*(*num_sublevels+i) > 0) {
            free( *(Y+i) );
            free( *(X_ijr+i) );
        }
    }
    if (env_nogoal_flag) {
        spc.num_egoals = 0;
        delete_tree( *spc.env_goals );
        free( spc.env_goals );
    }

    if (spc.num_sgoals > 0) {
        free( Y );
        free( X_ijr );
    }

    return 0;
}


int compute_horizon( DdManager *manager, DdNode **W,
                     DdNode **etrans, DdNode **strans, DdNode ***sgoals,
                     char *metric_vars, unsigned char verbose )
{
    int horizon = -1, horiz_j;
    int *num_sublevels;
    double **Min, **Max;
    int i, j, k;
    int *offw, num_metric_vars;

    offw = get_offsets( metric_vars, &num_metric_vars );
    if (offw == NULL)
        return -1;

    if (compute_minmax( manager, W, etrans, strans, sgoals,
                        &num_sublevels, &Min, &Max, offw, num_metric_vars,
                        verbose ))
        return -1;  /* Error in compute_minmax() */

    if (verbose) {
        for (i = 0; i < spc.num_sgoals; i++) {
            for (j = 0; j < *(num_sublevels+i)-1; j++) {
                logprint_startline();
                logprint_raw( "goal %d, level %d: ", i, j );
                logprint_raw( "%f, %f", *(*(Min+i)+j), *(*(Max+i)+j) );
                logprint_endline();
            }
        }
    }

    for (i = 0; i < spc.num_sgoals; i++) {
        for (j = 3; j < *(num_sublevels+i); j++) {
            horiz_j = 1;
            for (k = j-2; k >= 1; k--) {
                if (*(*(Max+i)+k-1) >= *(*(Min+i)+j-1))
                    horiz_j = j-k;
            }
            if (horiz_j > horizon)
                horizon = horiz_j;
        }
    }

    if (spc.num_sgoals > 0) {
        free( num_sublevels );
        for (i = 0; i < spc.num_sgoals; i++) {
            free( *(Min+i) );
            free( *(Max+i) );
        }
        free( Min );
        free( Max );
    }

    free( offw );
    return horizon;
}
