/* util.c -- Small handy routines; declarations appear in gr1c_util.h
 *
 *
 * SCL; 2012-2015
 */


#define _ISOC99_SOURCE
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "ptree.h"
#include "solve.h"
#include "solve_support.h"
#include "gr1c_util.h"
#include "logging.h"


int bitvec_to_int( vartype *vec, int vec_len )
{
    int i;
    int result = 0;
    for (i = 0; i < vec_len; i++) {
        if (*(vec+i))
            result += (1 << i);
    }
    return result;
}


vartype *int_to_bitvec( int x, int vec_len )
{
    int i;
    vartype *vec;
    if (vec_len < 1)
        return NULL;
    vec = malloc( vec_len*sizeof(vartype) );
    if (vec == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }
    for (i = 0; i < vec_len; i++) {
        if (x & (1 << i)) {
            *(vec+i) = 1;
        } else {
            *(vec+i) = 0;
        }
    }
    return vec;
}


ptree_t *expand_nonbool_variables( ptree_t **evar_list, ptree_t **svar_list,
                                   unsigned char verbose )
{
    ptree_t *nonbool_var_list = NULL;
    ptree_t *tmppt, *expt, *prevpt, *var_separator;

    /* Make all variables boolean */
    if ((*evar_list) == NULL) {
        /* that this is the deterministic case
           is indicated by var_separator = NULL. */
        var_separator = NULL;
        (*evar_list) = (*svar_list);
    } else {
        var_separator = get_list_item( (*evar_list), -1 );
        if (var_separator == NULL) {
            fprintf( stderr,
                     "Error: get_list_item failed on environment variables"
                     " list.\n" );
            return NULL;
        }
        var_separator->left = (*svar_list);
    }
    tmppt = (*evar_list);
    while (tmppt) {
        if (tmppt->value >= 0) {
            if (nonbool_var_list == NULL) {
                nonbool_var_list = init_ptree( PT_VARIABLE,
                                               tmppt->name, tmppt->value );
            } else {
                append_list_item( nonbool_var_list, PT_VARIABLE,
                                  tmppt->name, tmppt->value );
            }

        }
        tmppt = tmppt->left;
    }
    if (var_separator == NULL) {
        (*evar_list) = NULL;
    } else {
        var_separator->left = NULL;
    }

    /* Expand the variable list */
    if ((*evar_list) != NULL) {
        tmppt = (*evar_list);
        if (tmppt->value >= 0) {  /* Handle special case of head node */
            expt = var_to_bool( tmppt->name, tmppt->value );
            (*evar_list) = expt;
            prevpt = get_list_item( expt, -1 );
            prevpt->left = tmppt->left;
            tmppt->left = NULL;
            delete_tree( tmppt );
            tmppt = prevpt;
        } else {
            prevpt = tmppt;
        }
        tmppt = tmppt->left;
        while (tmppt) {
            if (tmppt->value >= 0) {
                expt = var_to_bool( tmppt->name, tmppt->value );
                prevpt->left = expt;
                prevpt = get_list_item( expt, -1 );
                prevpt->left = tmppt->left;
                tmppt->left = NULL;
                delete_tree( tmppt );
                tmppt = prevpt;
            } else {
                prevpt = tmppt;
            }
            tmppt = tmppt->left;
        }
    }

    if ((*svar_list) != NULL) {
        tmppt = (*svar_list);
        if (tmppt->value >= 0) {  /* Handle special case of head node */
            expt = var_to_bool( tmppt->name, tmppt->value );
            (*svar_list) = expt;
            prevpt = get_list_item( expt, -1 );
            prevpt->left = tmppt->left;
            tmppt->left = NULL;
            delete_tree( tmppt );
            tmppt = prevpt;
        } else {
            prevpt = tmppt;
        }
        tmppt = tmppt->left;
        while (tmppt) {
            if (tmppt->value >= 0) {
                expt = var_to_bool( tmppt->name, tmppt->value );
                prevpt->left = expt;
                prevpt = get_list_item( expt, -1 );
                prevpt->left = tmppt->left;
                tmppt->left = NULL;
                delete_tree( tmppt );
                tmppt = prevpt;
            } else {
                prevpt = tmppt;
            }
            tmppt = tmppt->left;
        }
    }

    return nonbool_var_list;
}


vartype *expand_nonbool_state( vartype *state, int *offw, int num_nonbool,
                               int mapped_len )
{
    int i, j, k;
    vartype *mapped_state, *state_frag;

    if (mapped_len <= 0 || num_nonbool < 0 || state == NULL)
        return NULL;

    mapped_state = malloc( mapped_len*sizeof(vartype) );
    if (mapped_state == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    i = j = k = 0;
    while (j < mapped_len) {
        if (i >= num_nonbool || j < *(offw+2*i)) {
            *(mapped_state+j) = *(state+k);

            j++;
        } else {
            state_frag = int_to_bitvec( *(state+k), *(offw+2*i+1) );
            for (j = *(offw+2*i); j < *(offw+2*i)+*(offw+2*i+1); j++)
                *(mapped_state+j) = *(state_frag+j-*(offw+2*i));
            free( state_frag );

            i++;
        }
        k++;
    }

    return mapped_state;
}


int expand_nonbool_GR1( ptree_t *evar_list, ptree_t *svar_list,
                        ptree_t **env_init, ptree_t **sys_init,
                        ptree_t ***env_trans_array, int *et_array_len,
                        ptree_t ***sys_trans_array, int *st_array_len,
                        ptree_t ***env_goals, int num_env_goals,
                        ptree_t ***sys_goals, int num_sys_goals,
                        unsigned char init_flags,
                        unsigned char verbose )
{
    int i;
    ptree_t *tmppt, *prevpt, *var_separator;
    int maxbitval;

    /* Make nonzero settings of "don't care" bits unreachable */
    tmppt = evar_list;
    while (tmppt) {
        if (tmppt->value == -1) {  /* Ignore Boolean variables */
            tmppt = tmppt->left;
            continue;
        }

        maxbitval = (int)(pow( 2, ceil(log2( tmppt->value > 0 ?
                                             tmppt->value+1 : 2 )) ));
        if (maxbitval-1 > tmppt->value) {
            if (verbose > 1)
                logprint( "In mapping %s to a bitvector, blocking values %d-%d",
                          tmppt->name, tmppt->value+1, maxbitval-1 );

            /* Initial conditions */
            if (init_flags == ONE_SIDE_INIT && *sys_init != NULL) {
                prevpt = *sys_init;
                *sys_init = init_ptree( PT_AND, NULL, 0 );
                (*sys_init)->right
                    = unreach_expanded_bool( tmppt->name, tmppt->value+1,
                                             maxbitval-1, PT_VARIABLE );
                (*sys_init)->left = prevpt;
            } else {
                if (*env_init == NULL)
                    *env_init = init_ptree( PT_CONSTANT, NULL, 1 );
                prevpt = *env_init;
                *env_init = init_ptree( PT_AND, NULL, 0 );
                (*env_init)->right
                    = unreach_expanded_bool( tmppt->name, tmppt->value+1,
                                             maxbitval-1, PT_VARIABLE );
                (*env_init)->left = prevpt;
            }

            /* Transition rules */
            (*et_array_len) += 2;
            (*env_trans_array) = realloc( (*env_trans_array),
                                          sizeof(ptree_t *)*(*et_array_len) );
            if ((*env_trans_array) == NULL ) {
                perror( __FILE__ ",  realloc" );
                exit(-1);
            }
            *((*env_trans_array)+(*et_array_len)-2)
                = unreach_expanded_bool( tmppt->name, tmppt->value+1,
                                         maxbitval-1, PT_VARIABLE );
            *((*env_trans_array)+(*et_array_len)-1)
                = unreach_expanded_bool( tmppt->name, tmppt->value+1,
                                         maxbitval-1, PT_NEXT_VARIABLE );
        }
        tmppt = tmppt->left;
    }

    tmppt = svar_list;
    while (tmppt) {
        if (tmppt->value == -1) {  /* Ignore Boolean variables */
            tmppt = tmppt->left;
            continue;
        }

        maxbitval = (int)(pow( 2, ceil(log2( tmppt->value > 0 ?
                                             tmppt->value+1 : 2 )) ));
        if (maxbitval-1 > tmppt->value) {
            if (verbose > 1)
                logprint( "In mapping %s to a bitvector, blocking values %d-%d",
                          tmppt->name, tmppt->value+1, maxbitval-1 );

            /* Initial conditions */
            if (init_flags == ONE_SIDE_INIT && *sys_init == NULL) {
                if (*env_init == NULL)
                    *env_init = init_ptree( PT_CONSTANT, NULL, 1 );
                prevpt = *env_init;
                *env_init = init_ptree( PT_AND, NULL, 0 );
                (*env_init)->right
                    = unreach_expanded_bool( tmppt->name, tmppt->value+1,
                                             maxbitval-1, PT_VARIABLE );
                (*env_init)->left = prevpt;
            } else {
                if (*sys_init == NULL)
                    *sys_init = init_ptree( PT_CONSTANT, NULL, 1 );
                prevpt = *sys_init;
                *sys_init = init_ptree( PT_AND, NULL, 0 );
                (*sys_init)->right
                    = unreach_expanded_bool( tmppt->name, tmppt->value+1,
                                             maxbitval-1, PT_VARIABLE );
                (*sys_init)->left = prevpt;
            }

            /* Transition rules */
            (*st_array_len) += 2;
            (*sys_trans_array) = realloc( (*sys_trans_array),
                                          sizeof(ptree_t *)*(*st_array_len) );
            if ((*sys_trans_array) == NULL ) {
                perror( __FILE__ ",  realloc" );
                exit(-1);
            }
            *((*sys_trans_array)+(*st_array_len)-2)
                = unreach_expanded_bool( tmppt->name, tmppt->value+1,
                                         maxbitval-1, PT_VARIABLE );
            *((*sys_trans_array)+(*st_array_len)-1)
                = unreach_expanded_bool( tmppt->name, tmppt->value+1,
                                         maxbitval-1, PT_NEXT_VARIABLE );
        }
        tmppt = tmppt->left;
    }

    if (evar_list == NULL) {
        var_separator = NULL;
        evar_list = svar_list;  /* that this is the deterministic case
                                   is indicated by var_separator = NULL. */
    } else {
        var_separator = get_list_item( evar_list, -1 );
        if (var_separator == NULL) {
            fprintf( stderr,
                     "Error: get_list_item failed on environment variables"
                     " list.\n" );
            return -1;
        }
        var_separator->left = svar_list;
    }
    tmppt = evar_list;
    while (tmppt) {
        if (tmppt->value >= 0) {
            if (*sys_init != NULL) {
                if (verbose > 1)
                    logprint( "Expanding nonbool variable %s in SYSINIT...",
                              tmppt->name );
                (*sys_init) = expand_to_bool( (*sys_init),
                                              tmppt->name, tmppt->value );
                if ((*sys_init) == NULL) {
                    fprintf( stderr,
                             "Error expand_nonbool_GR1: Failed to convert"
                             " non-Boolean variable to Boolean in SYSINIT.\n" );
                    return -1;
                }
                if (verbose > 1)
                    logprint( "Done." );
            }
            if (*env_init != NULL) {
                if (verbose > 1)
                    logprint( "Expanding nonbool variable %s in ENVINIT...",
                              tmppt->name );
                (*env_init) = expand_to_bool( (*env_init),
                                              tmppt->name, tmppt->value );
                if ((*env_init) == NULL) {
                    fprintf( stderr,
                             "Error expand_nonbool_GR1: Failed to convert"
                             " non-Boolean variable to Boolean in ENVINIT.\n" );
                    return -1;
                }
                if (verbose > 1)
                    logprint( "Done." );
            }
            for (i = 0; i < *et_array_len; i++) {
                if (verbose > 1)
                    logprint( "Expanding nonbool variable %s in ENVTRANS %d...",
                              tmppt->name, i );
                *((*env_trans_array)+i)
                    = expand_to_bool( *((*env_trans_array)+i),
                                      tmppt->name, tmppt->value );
                if (*((*env_trans_array)+i) == NULL) {
                    fprintf( stderr,
                             "Error expand_nonbool_GR1: Failed to convert"
                             " non-Boolean variable to Boolean.\n" );
                    return -1;
                }
                if (verbose > 1)
                    logprint( "Done." );
            }
            for (i = 0; i < *st_array_len; i++) {
                if (verbose > 1)
                    logprint( "Expanding nonbool variable %s in SYSTRANS %d...",
                              tmppt->name, i );
                *((*sys_trans_array)+i)
                    = expand_to_bool( *((*sys_trans_array)+i),
                                      tmppt->name, tmppt->value );
                if (*((*sys_trans_array)+i) == NULL) {
                    fprintf( stderr,
                             "Error expand_nonbool_GR1: Failed to convert"
                             " non-Boolean variable to Boolean.\n" );
                    return -1;
                }
                if (verbose > 1)
                    logprint( "Done." );
            }
            for (i = 0; i < num_env_goals; i++) {
                if (verbose > 1)
                    logprint( "Expanding nonbool variable %s in ENVGOAL %d...",
                              tmppt->name, i );
                *((*env_goals)+i)
                    = expand_to_bool( *((*env_goals)+i),
                                      tmppt->name, tmppt->value );
                if (*((*env_goals)+i) == NULL) {
                    fprintf( stderr,
                             "Error expand_nonbool_GR1: Failed to convert"
                             " non-Boolean variable to Boolean.\n" );
                    return -1;
                }
                if (verbose > 1)
                    logprint( "Done." );
            }
            for (i = 0; i < num_sys_goals; i++) {
                if (verbose > 1)
                    logprint( "Expanding nonbool variable %s in SYSGOAL %d...",
                              tmppt->name, i );
                *((*sys_goals)+i)
                    = expand_to_bool( *((*sys_goals)+i),
                                      tmppt->name, tmppt->value );
                if (*((*sys_goals)+i) == NULL) {
                    fprintf( stderr,
                             "Error expand_nonbool_GR1: Failed to convert"
                             " non-Boolean variable to Boolean.\n" );
                    return -1;
                }
                if (verbose > 1)
                    logprint( "Done." );
            }
        }

        tmppt = tmppt->left;
    }
    if (var_separator == NULL) {
        evar_list = NULL;
    } else {
        var_separator->left = NULL;
    }

    return 0;
}


void print_GR1_spec( ptree_t *evar_list, ptree_t *svar_list,
                     ptree_t *env_init, ptree_t *sys_init,
                     ptree_t **env_trans_array, int et_array_len,
                     ptree_t **sys_trans_array, int st_array_len,
                     ptree_t **env_goals, int num_env_goals,
                     ptree_t **sys_goals, int num_sys_goals,
                     FILE *outf )
{
    int i;
    ptree_t *tmppt;
    FILE *prev_logf;
    int prev_logoptions;

    if (outf != NULL) {
        prev_logf = getlogstream();
        prev_logoptions = getlogopt();
        setlogstream( outf );
        setlogopt( LOGOPT_NOTIME );
    }

    logprint_startline(); logprint_raw( "ENV:" );
    tmppt = evar_list;
    while (tmppt) {
        logprint_raw( " %s", tmppt->name );
        tmppt = tmppt->left;
    }
    logprint_raw( ";" ); logprint_endline();

    logprint_startline(); logprint_raw( "SYS:" );
    tmppt = svar_list;
    while (tmppt) {
        logprint_raw( " %s", tmppt->name );
        tmppt = tmppt->left;
    }
    logprint_raw( ";" ); logprint_endline();

    logprint_startline(); logprint_raw( "ENV INIT:  " );
    if (env_init)
        print_formula( env_init, getlogstream(), FORMULA_SYNTAX_GR1C );
    logprint_raw( ";" ); logprint_endline();

    logprint_startline(); logprint_raw( "SYS INIT:  " );
    if (sys_init)
        print_formula( sys_init, getlogstream(), FORMULA_SYNTAX_GR1C );
    logprint_raw( ";" ); logprint_endline();

    logprint_startline(); logprint_raw( "ENV TRANS:  " );
    if (et_array_len == 0) {
        logprint_raw( "(none)" );
    } else {
        logprint_raw( "[] " );
        print_formula( *env_trans_array, getlogstream(), FORMULA_SYNTAX_GR1C );
        for (i = 1; i < et_array_len; i++) {
            logprint_raw( " & [] " );
            print_formula( *(env_trans_array+i), getlogstream(),
                           FORMULA_SYNTAX_GR1C);
        }
    }
    logprint_raw( ";" ); logprint_endline();

    logprint_startline(); logprint_raw( "SYS TRANS:  " );
    if (st_array_len == 0) {
        logprint_raw( "(none)" );
    } else {
        logprint_raw( "[] " );
        print_formula( *sys_trans_array, getlogstream(), FORMULA_SYNTAX_GR1C );
        for (i = 1; i < st_array_len; i++) {
            logprint_raw( " & [] " );
            print_formula( *(sys_trans_array+i), getlogstream(),
                           FORMULA_SYNTAX_GR1C );
        }
    }
    logprint_raw( ";" ); logprint_endline();

    logprint_startline(); logprint_raw( "ENV GOALS:  " );
    if (num_env_goals == 0) {
        logprint_raw( "(none)" );
    } else {
        logprint_raw( "[]<> " );
        print_formula( *env_goals, getlogstream(), FORMULA_SYNTAX_GR1C );
        for (i = 1; i < num_env_goals; i++) {
            logprint_raw( " & []<> " );
            print_formula( *(env_goals+i), getlogstream(),
                           FORMULA_SYNTAX_GR1C );
        }
    }
    logprint_raw( ";" ); logprint_endline();

    logprint_startline(); logprint_raw( "SYS GOALS:  " );
    if (num_sys_goals == 0) {
        logprint_raw( "(none)" );
    } else {
        logprint_raw( "[]<> " );
        print_formula( *sys_goals, getlogstream(), FORMULA_SYNTAX_GR1C );
        for (i = 1; i < num_sys_goals; i++) {
            logprint_raw( " & []<> " );
            print_formula( *(sys_goals+i), getlogstream(),
                           FORMULA_SYNTAX_GR1C );
        }
    }
    logprint_raw( ";" ); logprint_endline();

    if (outf != NULL) {
        setlogstream( prev_logf );
        setlogopt( prev_logoptions );
    }
}


int check_gr1c_form( ptree_t *evar_list, ptree_t *svar_list,
                     ptree_t *env_init, ptree_t *sys_init,
                     ptree_t **env_trans_array, int et_array_len,
                     ptree_t **sys_trans_array, int st_array_len,
                     ptree_t **env_goals, int num_env_goals,
                     ptree_t **sys_goals, int num_sys_goals,
                     unsigned char init_flags )
{
    ptree_t *tmppt;
    char *tmpstr;
    int i;

    if (tree_size( evar_list ) == 0 && tree_size( svar_list ) == 0) {
        fprintf( stderr, "Error: No variables declared.\n" );
        return -1;
    }

    if (init_flags == ALL_ENV_EXIST_SYS_INIT) {
        if (env_init != NULL) {
            if ((tmpstr = check_vars( env_init, evar_list, NULL )) != NULL) {
                fprintf( stderr,
                         "Error: ENVINIT in GR(1) spec contains"
                         " unexpected variable: %s,\ngiven interpretation"
                         " from init_flags = ALL_ENV_EXIST_SYS_INIT.\n",
                         tmpstr );
                free( tmpstr );
                return -1;
            }
        }

        if (sys_init != NULL) {
            if ((tmpstr = check_vars( sys_init, svar_list, NULL )) != NULL) {
                fprintf( stderr,
                         "Error: SYSINIT in GR(1) spec contains"
                         " unexpected variable: %s,\ngiven interpretation"
                         " from init_flags = ALL_ENV_EXIST_SYS_INIT.\n",
                         tmpstr );
                free( tmpstr );
                return -1;
            }
        }

    } else if (init_flags == ONE_SIDE_INIT) {
        if (env_init != NULL && sys_init != NULL) {
            fprintf( stderr,
                     "Syntax error: init_flags = ONE_SIDE_INIT and"
                     " GR(1) specification has\nboth ENV_INIT and"
                     " SYS_INIT nonempty.\n" );
            return -1;
        }
    }

    tmppt = NULL;
    if (svar_list == NULL) {
        svar_list = evar_list;
    } else {
        tmppt = get_list_item( svar_list, -1 );
        tmppt->left = evar_list;
    }
    if (env_init != NULL
        && (tmpstr = check_vars( env_init, svar_list, NULL )) != NULL) {
        fprintf( stderr,
                 "Error: ENVINIT in GR(1) spec contains unexpected variable:"
                 " %s\n", tmpstr );
        free( tmpstr );
        return -1;
    } else if (sys_init != NULL
               && (tmpstr = check_vars( sys_init, svar_list, NULL )) != NULL) {
        fprintf( stderr,
                 "Error: SYSINIT in GR(1) spec contains unexpected variable:"
                 " %s\n", tmpstr );
        free( tmpstr );
        return -1;
    }
    for (i = 0; i < et_array_len; i++) {
        if ((tmpstr = check_vars( *(env_trans_array+i),
                                  svar_list, evar_list )) != NULL) {
            fprintf( stderr,
                     "Error: part %d of ENVTRANS in GR(1) spec contains"
                     " unexpected variable: %s\n",
                     i+1, tmpstr );
            free( tmpstr );
            return -1;
        }
    }
    for (i = 0; i < st_array_len; i++) {
        if ((tmpstr = check_vars( *(sys_trans_array+i),
                                  svar_list, svar_list )) != NULL) {
            fprintf( stderr,
                     "Error: part %d of SYSTRANS in GR(1) spec contains"
                     " unexpected variable: %s\n",
                     i+1, tmpstr );
            free( tmpstr );
            return -1;
        }
    }
    for (i = 0; i < num_env_goals; i++) {
        if ((tmpstr = check_vars( *(env_goals+i),
                                  svar_list, NULL )) != NULL) {
            fprintf( stderr,
                     "Error: part %d of ENVGOAL in GR(1) spec contains"
                     " unexpected variable: %s\n",
                     i+1, tmpstr );
            free( tmpstr );
            return -1;
        }
    }
    for (i = 0; i < num_sys_goals; i++) {
        if ((tmpstr = check_vars( *(sys_goals+i),
                                  svar_list, NULL )) != NULL) {
            fprintf( stderr,
                     "Error: part %d of SYSGOAL in GR(1) spec contains"
                     " unexpected variable: %s\n",
                     i+1, tmpstr );
            free( tmpstr );
            return -1;
        }
    }
    if (tmppt != NULL) {
        tmppt->left = NULL;
        tmppt = NULL;
    } else {
        svar_list = NULL;
    }

    return 0;
}


/* N.B., we assume that noonbool_var_list is sorted with respect to
   the expanded variable list (evar_list followed by svar_list). */
int *get_offsets_list( ptree_t *evar_list, ptree_t *svar_list,
                       ptree_t *nonbool_var_list )
{
    int *offw = NULL;
    ptree_t *var, *var_tail, *tmppt;
    int start_index, stop_index;
    int i;

    i = 0;
    tmppt = nonbool_var_list;
    while (tmppt) {
        i++;
        offw = realloc( offw, 2*i*sizeof(int) );
        if (offw == NULL) {
            perror( __FILE__ ",  realloc" );
            exit(-1);
        }

        var = evar_list;
        start_index = 0;
        while (var) {
            if (strstr( var->name, tmppt->name ) == var->name)
                break;
            var = var->left;
            start_index++;
        }
        if (var == NULL) {
            var = svar_list;
            while (var) {
                if (strstr( var->name, tmppt->name ) == var->name)
                    break;
                var = var->left;
                start_index++;
            }
            if (var == NULL) {
                fprintf( stderr,
                         "Error get_offsets_list: Could not find match for"
                         " \"%s\"\n", tmppt->name );
                free( offw );
                return NULL;
            }
        }

        var_tail = var;
        stop_index = start_index;
        while (var_tail->left) {
            if (strstr( var_tail->left->name, tmppt->name )
                != var_tail->left->name )
                break;
            var_tail = var_tail->left;
            stop_index++;
        }

        *(offw+2*(i-1)) = start_index;
        *(offw+2*(i-1)+1) = stop_index-start_index+1;

        tmppt = tmppt->left;
    }

    return offw;
}


void print_support( DdManager *manager, int state_len, DdNode *X, FILE *outf )
{
    FILE *prev_logf;
    int prev_logoptions;
    vartype *state;
    int i;
    DdGen *gen;
    CUDD_VALUE_TYPE gvalue;
    int *gcube;

    if (outf != NULL) {
        prev_logf = getlogstream();
        prev_logoptions = getlogopt();
        setlogstream( outf );
        setlogopt( LOGOPT_NOTIME );
    }

    /* State vector (i.e., valuation of the variables) */
    state = malloc( sizeof(vartype)*(state_len) );
    if (state == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    Cudd_AutodynDisable( manager );
    Cudd_ForeachCube( manager, X, gen, gcube, gvalue ) {
        initialize_cube( state, gcube, state_len );
        while (!saturated_cube( state, gcube, state_len )) {
            logprint_startline();
            for (i = 0; i < state_len; i++) {
                if (i > 0 && i % 4 == 0)
                    logprint_raw( " " );
                logprint_raw( "%d", *(state+i) );
            }
            logprint_endline();
            increment_cube( state, gcube, state_len );
        }
        logprint_startline();
        for (i = 0; i < state_len; i++) {
            if (i > 0 && i % 4 == 0)
                logprint_raw( " " );
            logprint_raw( "%d", *(state+i) );
        }
        logprint_endline();
    }
    Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );

    free( state );

    if (outf != NULL) {
        setlogstream( prev_logf );
        setlogopt( prev_logoptions );
    }
}
