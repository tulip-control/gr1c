/* patching.c -- Definitions for signatures appearing in patching.h.
 *
 *
 * SCL; 2012-2015
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "logging.h"
#include "automaton.h"
#include "ptree.h"
#include "patching.h"
#include "solve_support.h"
#include "gr1c_util.h"


extern specification_t spc;


/* Pretty print state vector, default is to list nonzero variables.
   Output written to fp if not NULL, otherwise to stdout.  If num_env
   (respectively, num_sys) is -1, then only the system variables
   (resp. environment variables) are printed.  */
void pprint_state( vartype *state, int num_env, int num_sys, FILE *fp )
{
    int i, nnz = 0;
    ptree_t *node;

    if (fp == NULL)
        fp = stdout;

    node = spc.evar_list;
    for (i = 0; i < num_env; i++) {
        if (*(state+i)) {
            fprintf( fp, " %s", node->name );
            nnz++;
        }
        node = node->left;
    }

    if (num_sys == -1) {
        if (nnz == 0)
            fprintf( fp, " (nil)" );
        return;
    }

    if (num_env == -1)
        num_env = tree_size( spc.evar_list );

    node = spc.svar_list;
    for (i = num_env; i < num_sys+num_env; i++) {
        if (*(state+i)) {
            fprintf( fp, " %s", node->name );
            nnz++;
        }
        node = node->left;
    }

    if (nnz == 0)
        fprintf( fp, " (nil)" );
}


/* Returns strategy with patched goal mode, or NULL if error. */
anode_t *localfixpoint_goalmode( DdManager *manager, int num_env, int num_sys,
                                 anode_t *strategy, int goal_mode,
                                 anode_t ***affected, int *affected_len,
                                 DdNode *etrans, DdNode *strans,
                                 DdNode **egoals,
                                 DdNode *N_BDD, vartype **N, int N_len,
                                 unsigned char verbose )
{
    int i, j, k;  /* Generic counters */
    anode_t **Exit;
    anode_t **Entry;
    int Exit_len, Entry_len;
    anode_t *local_strategy;
    anode_t *head, *node;
    int min_rgrad;  /* Minimum reach annotation value of affected nodes. */
    int Exit_rgrad;  /* Maximum value among reached Exit nodes. */
    int local_max_rgrad;
    int local_min_rgrad;

    /* Pre-allocate space for Entry and Exit sets; the number of
       elements actually used is tracked by Entry_len and Exit_len,
       respectively. */
    Exit = malloc( sizeof(anode_t *)*N_len );
    if (Exit == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }
    Entry = malloc( sizeof(anode_t *)*N_len );
    if (Entry == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    /* Ignore goal modes that are unaffected by the change. */
    if (*(affected_len + goal_mode) == 0) {
        free( Exit );
        free( Entry );
        return strategy;
    }

    if (verbose)
        logprint( "Processing for goal mode %d...", goal_mode );

    /* Build Entry set and initial Exit set */
    Exit_len = Entry_len = 0;
    head = strategy;
    while (head) {
        /* Though we use linear search in N, if we could sort N
           earlier, or somehow ensure the input (from the game
           edge change file) is already ordered, then this could
           be made into a binary search. */
        if (head->mode == goal_mode) {
            for (i = 0; i < N_len; i++)
                if (statecmp( head->state, *(N+i), num_env+num_sys ))
                    break;

            if (i < N_len) {  /* Was a match found? */
                Exit_len++;
                *(Exit+Exit_len-1) = head;
            } else {
                /* Test for nominal membership in Entry set; i.e.,
                   current node is not in N, check if it leads into N. */
                for (j = 0; j < head->trans_len; j++) {
                    for (k = 0; k < N_len; k++)
                        if ((*(head->trans+j))->mode == goal_mode
                            && statecmp( (*(head->trans+j))->state, *(N+k),
                                         num_env+num_sys ))
                            break;
                    if (k < N_len) {
                        for (k = 0; k < Entry_len; k++)
                            if (statecmp((*(Entry+k))->state,
                                         (*(head->trans+j))->state,
                                         num_env+num_sys ))
                                break;
                        if (k == Entry_len) {
                            Entry_len++;
                            *(Entry+Entry_len-1) = *(head->trans+j);
                        }
                    }
                }
            }
        }
        head = head->next;
    }

    /* Find minimum reach annotation value among nodes in the
       Entry and U_i sets, and remove any initial Exit nodes greater
       than or equal to it. */
    min_rgrad = -1;
    for (i = 0; i < Entry_len; i++) {
        if ((*(Entry+i))->rgrad < min_rgrad || min_rgrad == -1)
            min_rgrad = (*(Entry+i))->rgrad;
    }
    for (i = 0; i < *(affected_len+goal_mode); i++) {
        if ((*(*(affected+goal_mode)+i))->rgrad < min_rgrad || min_rgrad == -1)
            min_rgrad = (*(*(affected+goal_mode)+i))->rgrad;
    }
    if (verbose)
        logprint( "Minimum reach annotation value in Entry or U_i: %d",
                  min_rgrad );
    i = 0;
    while (i < Exit_len) {
        if ((*(Exit+i))->rgrad >= min_rgrad) {
            if (Exit_len > 1) {
                *(Exit+i) = *(Exit+Exit_len-1);
                Exit_len--;
            } else {
                Exit_len = 0;
                break;
            }
        } else {
            i++;
        }
    }

    if (verbose) {
        logprint( "Entry set:" );
        for (i = 0; i < Entry_len; i++) {
            logprint_startline();
            pprint_state( (*(Entry+i))->state, num_env, num_sys,
                          getlogstream() );
            logprint_endline();
        }
        logprint( "Exit set:" );
        for (i = 0; i < Exit_len; i++) {
            logprint_startline();
            pprint_state( (*(Exit+i))->state, num_env, num_sys,
                          getlogstream() );
            logprint_endline();
        }
    }

    local_strategy = synthesize_reachgame( manager, num_env, num_sys,
                                           Entry, Entry_len, Exit, Exit_len,
                                           etrans, strans, egoals, N_BDD,
                                           verbose );
    if (local_strategy == NULL) {
        free( Exit );
        free( Entry );
        return NULL;
    }

    if (verbose > 1) {
        logprint( "Local strategy for goal mode %d:", goal_mode );
        logprint_startline();
        dot_aut_dump( local_strategy, spc.evar_list, spc.svar_list, DOT_AUT_ATTRIB,
                      getlogstream() );
        logprint_endline();
    }

    node = local_strategy;
    local_min_rgrad = local_max_rgrad = -1;
    while (node) {
        if (node->rgrad > local_max_rgrad)
            local_max_rgrad = node->rgrad;
        if (node->rgrad < local_min_rgrad || local_min_rgrad == -1)
            local_min_rgrad = node->rgrad;
        node = node->next;
    }

    /* Connect local strategy to original */
    for (i = 0; i < Entry_len; i++) {
        node = local_strategy;
        while (node && !statecmp( (*(Entry+i))->state, node->state,
                                  num_env+num_sys ))
            node = node->next;
        if (node == NULL) {
            fprintf( stderr,
                     "Error localfixpoint_goalmode: expected Entry node"
                     " missing from local strategy, in goal mode %d\n",
                     goal_mode );
            return NULL;
        }

        replace_anode_trans( strategy, *(Entry+i), node );
    }

    Exit_rgrad = -1;
    node = local_strategy;
    while (node) {
        if (node->trans_len == 0) {  /* Terminal node of the local strategy? */
            for (i = 0; i < Exit_len; i++) {
                if (statecmp( node->state, (*(Exit+i))->state,
                              num_env+num_sys ))
                    break;
            }
            if (i == Exit_len) {
                fprintf( stderr,
                         "Error localfixpoint_goalmode: terminal node in"
                         " local strategy does not have a\nmatching Exit"
                         " node, in goal mode %d\n",
                         goal_mode );
                return NULL;
            }

            if ((*(Exit+i))->rgrad > Exit_rgrad)
                Exit_rgrad = (*(Exit+i))->rgrad;

            if (forward_modereach( *(Exit+i), goal_mode, N, N_len,
                                   -1, num_env+num_sys )) {
                fprintf( stderr,
                         "Error localfixpoint_goalmode: forward graph"
                         " reachability computation failed\nfrom Exit node"
                         " in goal mode %d\n",
                         goal_mode );
                return NULL;
            }

            (*(Exit+i))->mode = -1;
            node->mode = -2;  /* Mark as to-be-deleted */

            replace_anode_trans( local_strategy, node, *(Exit+i) );
        }
        node = node->next;
    }

    /* Delete useless nodes from N_i (whose function is now replaced
       by the local strategy). */
    node = strategy;
    while (node) {
        if (node->mode != goal_mode) {
            if (node->mode == -1)
                node->mode = goal_mode;
            node = node->next;
            continue;
        }
        for (i = 0; i < N_len; i++)
            if (statecmp( node->state, *(N+i), num_env+num_sys ))
                break;
        if (i < N_len)
            node->mode = -2;
        node = node->next;
    }
    node = strategy;
    while (node) {
        if (node->mode == -2) {
            replace_anode_trans( strategy, node, NULL );
            strategy = delete_anode( strategy, node );
            node = strategy;
        } else {
            node = node->next;
        }
    }
    node = local_strategy;
    while (node) {
        if (node->mode == -2) {
            local_strategy = delete_anode( local_strategy, node );
            node = local_strategy;
        } else {
            node = node->next;
        }
    }

    /* Scale reach annotation values to make room for patch. */
    i = 1;
    while ((min_rgrad-Exit_rgrad)*i < (local_max_rgrad-local_min_rgrad))
        i++;
    node = strategy;
    while (node) {
        node->rgrad *= i;
        node = node->next;
    }

    node = local_strategy;
    while (node) {
        node->mode = goal_mode;
        node->rgrad += Exit_rgrad*i;
        node = node->next;
    }

    head = strategy;
    while (head->next)
        head = head->next;
    head->next = local_strategy;

    free( Entry );
    free( Exit );

    return strategy;
}


#define INPUT_STRING_LEN 1024
anode_t *patch_localfixpoint( DdManager *manager,
                              FILE *strategy_fp, FILE *change_fp,
                              int original_num_env, int original_num_sys,
                              ptree_t *nonbool_var_list, int *offw,
                              unsigned char verbose )
{
    ptree_t *var_separator;
    DdNode *etrans, *strans, **egoals;
    DdNode *etrans_part, *strans_part;
    int num_env, num_sys;
    int num_nonbool;
    int num_enonbool;  /* Number of env variables with nonboolean domain */

    /* Two copies of offw, for quickly mapping state-next-state arrays */
    int *doffw = NULL;

    DdNode *vertex1, *vertex2; /* ...regarding vertices of the game graph. */
    char line[INPUT_STRING_LEN];
    vartype *state, *state_frag;

    bool env_nogoal_flag = False;  /* Indicate environment has no goals */

    int i, j, k;  /* Generic counters */
    DdNode *tmp, *tmp2;
    int num_read;
    anode_t *strategy, *result_strategy;
    anode_t *node, *head;
    int node_counter;
    vartype **N = NULL;  /* "neighborhood" of states */
    int N_len = 0;
    int goal_mode;
    DdNode *N_BDD = NULL;  /* Characteristic function for set of states N. */
    bool break_flag;

    anode_t ***affected = NULL;  /* Array of pointers to arrays of
                                    nodes directly affected by edge
                                    set change.  Called U_i in the
                                    manuscript. */
    int *affected_len = NULL;  /* Lengths of arrays in affected */

    DdNode **vars, **pvars;
    int *cube;
    DdNode *ddval;

    if (change_fp == NULL)
        return NULL;  /* Require game changes to be listed in an open stream. */

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

    num_nonbool = tree_size( spc.nonbool_var_list );
    if (num_nonbool > 0) {
        if (verbose > 1)
            logprint( "Expanding nonbool variables in the given strategy"
                      " automaton..." );
        if (aut_expand_bool( strategy,
                             spc.evar_list, spc.svar_list, spc.nonbool_var_list )) {
            fprintf( stderr,
                     "Error patch_localfixpoint: Failed to expand"
                     " nonboolean variables in given automaton." );
            return NULL;
        }
        if (verbose > 1) {
            logprint( "Given strategy after variable expansion:" );
            logprint_startline();
            dot_aut_dump( strategy, spc.evar_list, spc.svar_list, DOT_AUT_ATTRIB,
                          getlogstream() );
            logprint_endline();
        }

        num_enonbool = 0;
        while (*(offw+2*num_enonbool) < num_env)
            num_enonbool++;

        doffw = malloc( sizeof(int)*4*num_nonbool );
        if (doffw == NULL) {
            perror( __FILE__ ",  malloc" );
            exit(-1);
        }
        for (i = 0; i < 2*num_nonbool; i++)
            *(doffw+2*num_nonbool+i) = *(doffw+i) = *(offw+i);
    }

    affected = malloc( sizeof(anode_t **)*spc.num_sgoals );
    if (affected == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }
    affected_len = malloc( sizeof(int)*spc.num_sgoals );
    if (affected_len == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }
    for (i = 0; i < spc.num_sgoals; i++) {
        *(affected+i) = NULL;
        *(affected_len+i) = 0;
    }

    cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
    if (cube == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    /* Set environment goal to True (i.e., any state) if none was
       given. This simplifies the implementation below. */
    if (spc.num_egoals == 0) {
        env_nogoal_flag = True;
        spc.num_egoals = 1;
        spc.env_goals = malloc( sizeof(ptree_t *) );
        *spc.env_goals = init_ptree( PT_CONSTANT, NULL, 1 );
    }

    /* Process game graph changes file */
    break_flag = False;
    while (fgets( line, INPUT_STRING_LEN, change_fp )) {
        /* Blank or comment line? */
        if (strlen( line ) < 1 || *line == '\n' || *line == '#')
            continue;

        num_read = read_state_str( line, &state,
                                   original_num_env+original_num_sys );
        if (num_read == 0) {
            /* This must be the first command, so break from loop and
               build local transition rules from N. */
            break_flag = True;
            break;
        } else if (num_read < original_num_env+original_num_sys) {
            fprintf( stderr,
                     "Error patch_localfixpoint: malformed game change"
                     " file.\n" );
            free( doffw );
            free( affected );
            free( affected_len );
            free( cube );
            return NULL;
        }

        N_len++;
        N = realloc( N, sizeof(vartype *)*N_len );
        if (N == NULL) {
            perror( __FILE__ ",  realloc" );
            exit(-1);
        }
        if (num_nonbool > 0) {
            *(N+N_len-1) = expand_nonbool_state( state, offw, num_nonbool,
                                                 num_env+num_sys );
            if (*(N+N_len-1) == NULL) {
                fprintf( stderr,
                         "Error patch_localfixpoint: failed to expand"
                         " nonbool values in edge change file\n" );
                free( doffw );
                free( affected );
                free( affected_len );
                free( cube );
                return NULL;
            }
            free( state );
        } else {
            *(N+N_len-1) = state;
        }
    }

    if (verbose) {
        logprint( "States in N (%d total):", N_len );
        for (i = 0; i < N_len; i++) {
            logprint_startline();
            pprint_state( *(N+i), num_env, num_sys, getlogstream() );
            logprint_endline();
        }
    }

    /* Build characteristic function for N */
    N_BDD = Cudd_Not( Cudd_ReadOne( manager ) );
    Cudd_Ref( N_BDD );
    for (i = 0; i < N_len; i++) {
        ddval = state_to_BDD( manager, *(N+i), 0, num_env+num_sys );
        tmp = Cudd_bddOr( manager, N_BDD, ddval );
        Cudd_Ref( tmp );
        Cudd_RecursiveDeref( manager, N_BDD );
        Cudd_RecursiveDeref( manager, ddval );
        N_BDD = tmp;
    }
    ddval = NULL;

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
            free( doffw );
            free( affected );
            free( affected_len );
            free( cube );
            return NULL;
        }
        var_separator->left = spc.svar_list;
    }

    /* Generate BDDs for parse trees from the problem spec transition
       rules that are relevant given restriction to N. */
    if (verbose)
        logprint( "Building local environment transition BDD..." );
    etrans = Cudd_ReadOne( manager );
    Cudd_Ref( etrans );
    if (verbose) {
        logprint_startline();
        logprint_raw( "Relevant env trans (one per line):" );
    }
    for (i = 0; i < spc.et_array_len; i++) {
        etrans_part = ptree_BDD( *(spc.env_trans_array+i), spc.evar_list, manager );
        for (j = 0; j < N_len; j++) {
            for (k = 0; k < num_env+num_sys; k++) {
                *(cube+k) = *(*(N+j)+k);
                *(cube+num_env+num_sys+k) = 2;
            }
            ddval = Cudd_CubeArrayToBdd( manager, cube );
            if (ddval == NULL) {
                fprintf( stderr,
                         "Error patch_localfixpoint: building characteristic"
                         " function of N." );
                return NULL;
            }
            Cudd_Ref( ddval );

            tmp2 = Cudd_Cofactor( manager, etrans_part, ddval );
            if (tmp2 == NULL) {
                fprintf( stderr,
                         "Error patch_localfixpoint: computing cofactor." );
                return NULL;
            }
            Cudd_Ref( tmp2 );

            if (!Cudd_bddLeq( manager, tmp2, Cudd_ReadOne( manager ) )
                || !Cudd_bddLeq( manager, Cudd_ReadOne( manager ), tmp2 )) {
                Cudd_RecursiveDeref( manager, tmp2 );
                Cudd_RecursiveDeref( manager, ddval );
                break;
            }

            Cudd_RecursiveDeref( manager, tmp2 );
            Cudd_RecursiveDeref( manager, ddval );
        }
        if (j < N_len) {
            if (verbose) {
                logprint_raw( "\n" );
                print_formula( *(spc.env_trans_array+i), getlogstream(),
                               FORMULA_SYNTAX_GR1C );
            }
            tmp = Cudd_bddAnd( manager, etrans, etrans_part );
            Cudd_Ref( tmp );
            Cudd_RecursiveDeref( manager, etrans );
            etrans = tmp;
        }
        Cudd_RecursiveDeref( manager, etrans_part );

    }
    if (verbose) {
        logprint_endline();
        logprint( "Done." );
        logprint( "Building local system transition BDD..." );
    }

    strans = Cudd_ReadOne( manager );
    Cudd_Ref( strans );
    if (verbose) {
        logprint_startline();
        logprint_raw( "Relevant sys trans (one per line):" );
    }
    for (i = 0; i < spc.st_array_len; i++) {
        strans_part = ptree_BDD( *(spc.sys_trans_array+i), spc.evar_list, manager );
        for (j = 0; j < N_len; j++) {
            for (k = 0; k < num_env+num_sys; k++) {
                *(cube+k) = *(*(N+j)+k);
                *(cube+num_env+num_sys+k) = 2;
            }
            ddval = Cudd_CubeArrayToBdd( manager, cube );
            if (ddval == NULL) {
                fprintf( stderr,
                         "Error patch_localfixpoint: building characteristic"
                         " function of N." );
                return NULL;
            }
            Cudd_Ref( ddval );

            tmp2 = Cudd_Cofactor( manager, strans_part, ddval );
            if (tmp2 == NULL) {
                fprintf( stderr,
                         "Error patch_localfixpoint: computing cofactor." );
                return NULL;
            }
            Cudd_Ref( tmp2 );

            if (!Cudd_bddLeq( manager, tmp2, Cudd_ReadOne( manager ) )
                || !Cudd_bddLeq( manager, Cudd_ReadOne( manager ), tmp2 )) {
                Cudd_RecursiveDeref( manager, tmp2 );
                Cudd_RecursiveDeref( manager, ddval );
                break;
            }

            Cudd_RecursiveDeref( manager, tmp2 );
            Cudd_RecursiveDeref( manager, ddval );
        }
        if (j < N_len) {
            if (verbose) {
                logprint_raw( "\n" );
                print_formula( *(spc.sys_trans_array+i), getlogstream(),
                               FORMULA_SYNTAX_GR1C );
            }
            tmp = Cudd_bddAnd( manager, strans, strans_part );
            Cudd_Ref( tmp );
            Cudd_RecursiveDeref( manager, strans );
            strans = tmp;
        }
        Cudd_RecursiveDeref( manager, strans_part );

    }
    if (verbose) {
        logprint_endline();
        logprint( "Done." );
    }

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

    /* Was the earlier file loop broken because a command was discovered? */
    if (break_flag) {
        do {
            /* Blank or comment line? */
            if (strlen( line ) < 1 || *line == '\n' || *line == '#')
                continue;

            if (!strncmp( line, "restrict ", strlen( "restrict " ) )
                || !strncmp( line, "relax ", strlen( "relax " ) )) {
                if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
                    num_read
                        = read_state_str( line+strlen( "restrict" )+1,
                                          &state_frag,
                                          2*(original_num_env
                                             +original_num_sys) );
                } else { /* "relax " */
                    num_read
                        = read_state_str( line+strlen( "relax" )+1,
                                          &state_frag,
                                          2*(original_num_env
                                             +original_num_sys) );
                }
                if (num_read != 2*(original_num_env+original_num_sys)
                    && num_read != 2*original_num_env+original_num_sys) {
                    if (num_read > 0)
                        free( state_frag );
                    fprintf( stderr,
                             "Error: invalid arguments to restrict or relax"
                             " command.\n" );
                    return NULL;
                }

                if (num_nonbool > 0) {
                    if (num_read == 2*(original_num_env+original_num_sys)) {
                        state = expand_nonbool_state( state_frag, doffw,
                                                      2*num_nonbool,
                                                      2*(num_env+num_sys) );
                        if (state == NULL) {
                            fprintf( stderr,
                                     "Error patch_localfixpoint: failed to"
                                     " expand nonbool values in edge change"
                                     " file\n" );
                            return NULL;
                        }
                        free( state_frag );
                    } else { /* num_read==2*original_num_env+original_num_sys */
                        state = expand_nonbool_state( state_frag, doffw,
                                                      num_nonbool+num_enonbool,
                                                      2*num_env+num_sys );
                        if (state == NULL) {
                            fprintf( stderr,
                                     "Error patch_localfixpoint: failed to"
                                     " expand nonbool values in edge change"
                                     " file\n" );
                            return NULL;
                        }
                        free( state_frag );
                    }
                } else {
                    state = state_frag;
                }

                if (verbose) {
                    logprint_startline();
                    if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
                        if (num_read == 2*(original_num_env+original_num_sys)) {
                            logprint_raw( "Removing controlled edge from" );
                        } else {
                            logprint_raw( "Removing uncontrolled edge from" );
                        }
                    } else { /* "relax " */
                        if (num_read == 2*(original_num_env+original_num_sys)) {
                            logprint_raw( "Adding controlled edge from" );
                        } else {
                            logprint_raw( "Adding uncontrolled edge from" );
                        }
                    }
                    pprint_state( state, num_env, num_sys, getlogstream() );
                    logprint_raw( " to" );
                    pprint_state( state+num_env+num_sys,
                                  num_env, num_read-(2*num_env+num_sys),
                                  getlogstream() );
                    logprint_endline();
                }

                /* Find nodes in strategy that are affected by this change */
                for (j = 0; j < spc.num_sgoals; j++) {
                    node = find_anode( strategy, j, state, num_env+num_sys );
                    while (node != NULL) {
                        if (!strncmp( line, "restrict ", strlen( "restrict " ) )
                            && (num_read
                                == 2*(original_num_env+original_num_sys))) {
                            for (i = 0; i < node->trans_len; i++) {
                                if (statecmp( (*(node->trans+i))->state,
                                              state+num_env+num_sys,
                                              num_env+num_sys )) {
                                    (*(affected_len + node->mode))++;
                                    *(affected + node->mode)
                                        = realloc( *(affected + node->mode),
                                                   sizeof(anode_t *)
                                                   *(*(affected_len
                                                       + node->mode)) );
                                    if (*(affected + node->mode) == NULL) {
                                        perror( __FILE__ ", "
                                                " realloc" );
                                        exit(-1);
                                    }
                                    /* If affected state is not in N,
                                       then fail. */
                                    for (i = 0; i < N_len; i++) {
                                        if (statecmp( state, *(N+i),
                                                      num_env+num_sys ))
                                            break;
                                    }
                                    if (i == N_len) {
                                        fprintf( stderr,
                                                 "Error patch_localfixpoint:"
                                                 " affected state not"
                                                 " contained in N.\n" );
                                        return NULL;
                                    }
                                    *(*(affected + node->mode)
                                      + *(affected_len + node->mode)-1) = node;
                                    break;
                                }
                            }
                        } else if (!strncmp( line, "relax ", strlen("relax ") )
                                   && (num_read
                                       == 2*original_num_env+original_num_sys)
                            ) {
                            for (i = 0; i < node->trans_len; i++) {
                                if (statecmp( (*(node->trans+i))->state,
                                              state+num_env+num_sys, num_env ))
                                    break;
                            }
                            if (i == node->trans_len) {
                                (*(affected_len + node->mode))++;
                                *(affected + node->mode)
                                    = realloc( *(affected + node->mode),
                                               sizeof(anode_t *)
                                               *(*(affected_len
                                                   + node->mode)) );
                                if (*(affected + node->mode) == NULL) {
                                    perror( __FILE__ ",  realloc" );
                                    exit(-1);
                                }
                                /* If affected state is not in N, then fail. */
                                for (i = 0; i < N_len; i++) {
                                    if (statecmp( state, *(N+i),
                                                  num_env+num_sys ))
                                        break;
                                }
                                if (i == N_len) {
                                    fprintf( stderr,
                                             "Error patch_localfixpoint:"
                                             " affected state not contained"
                                             " in N.\n" );
                                    return NULL;
                                }
                                *(*(affected + node->mode)
                                  + *(affected_len + node->mode)-1) = node;
                            }
                        }

                        node = find_anode( node->next, j, state, num_env+num_sys );
                    }
                }

                vertex1 = state_to_BDD( manager, state, 0, num_env+num_sys );
                vertex2 = state_to_BDD( manager, state+num_env+num_sys,
                                     num_env+num_sys,
                                     num_read-(num_env+num_sys) );
                if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
                    tmp = Cudd_Not( vertex2 );
                    Cudd_Ref( tmp );
                    Cudd_RecursiveDeref( manager, vertex2 );
                    vertex2 = tmp;
                    tmp = Cudd_bddOr( manager, Cudd_Not( vertex1 ), vertex2 );
                } else { /* "relax " */
                    tmp = Cudd_bddAnd( manager, vertex1, vertex2 );
                }
                Cudd_Ref( tmp );
                Cudd_RecursiveDeref( manager, vertex1 );
                Cudd_RecursiveDeref( manager, vertex2 );
                if (num_read == 2*original_num_env+original_num_sys) {
                    if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
                        tmp2 = Cudd_bddAnd( manager, tmp, etrans );
                    } else { /* "relax " */
                        tmp2 = Cudd_bddOr( manager, tmp, etrans );
                    }
                    Cudd_Ref( tmp2 );
                    Cudd_RecursiveDeref( manager, etrans );
                    etrans = tmp2;
                } else { /* num_read == 2*(original_num_env+original_num_sys) */
                    if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
                        tmp2 = Cudd_bddAnd( manager, tmp, strans );
                    } else { /* "relax " */
                        tmp2 = Cudd_bddOr( manager, tmp, strans );
                    }
                    Cudd_Ref( tmp2 );
                    Cudd_RecursiveDeref( manager, strans );
                    strans = tmp2;
                }
                Cudd_RecursiveDeref( manager, tmp );
                free( state );

            } else if (!strncmp( line, "blocksys ", strlen( "blocksys " ) )) {
                num_read = read_state_str( line+strlen( "blocksys" )+1,
                                           &state_frag, original_num_sys );
                if (num_read != original_num_sys) {
                    if (num_read > 0)
                        free( state );
                    fprintf( stderr,
                             "Error: invalid arguments to blocksys"
                             " command.\n%d\n%s\n", num_read, line );
                    return NULL;
                }
                if (num_nonbool > 0) {
                    for (i = 0; i < num_nonbool-num_enonbool; i++)
                        *(offw+2*num_enonbool+2*i) -= num_env;
                    state = expand_nonbool_state( state_frag,
                                                  offw+2*num_enonbool,
                                                  num_nonbool-num_enonbool,
                                                  num_sys );
                    for (i = 0; i < num_nonbool-num_enonbool; i++)
                        *(offw+2*num_enonbool+2*i) += num_env;
                    if (state == NULL) {
                        fprintf( stderr,
                                 "Error patch_localfixpoint: failed to expand"
                                 " nonbool values in edge change file\n" );
                        return NULL;
                    }
                    free( state_frag );
                } else {
                    state = state_frag;
                }
                if (verbose) {
                    logprint_startline();
                    logprint_raw( "Removing system moves into" );
                    pprint_state( state, -1, num_sys, getlogstream() );
                    logprint_endline();
                }

                /* Find nodes in strategy that are affected by this change */
                head = strategy;
                node_counter = 0;
                while (head) {
                    for (i = 0; i < head->trans_len; i++) {
                        if (statecmp( state, (*(head->trans+i))->state+num_env,
                                      num_sys )) {
                            (*(affected_len + head->mode))++;
                            *(affected + head->mode)
                                = realloc( *(affected + head->mode),
                                           sizeof(anode_t *)
                                           *(*(affected_len + head->mode)) );
                            if (*(affected + head->mode) == NULL) {
                                perror( __FILE__ ",  realloc" );
                                exit(-1);
                            }
                            /* If affected state is not in N, then fail. */
                            for (i = 0; i < N_len; i++) {
                                if (statecmp( head->state, *(N+i),
                                              num_env+num_sys ))
                                    break;
                            }
                            if (i == N_len) {
                                fprintf( stderr,
                                         "Error patch_localfixpoint: affected"
                                         " state not contained in N.\n" );
                                return NULL;
                            }
                            *(*(affected + head->mode)
                              + *(affected_len + head->mode)-1) = head;
                            break;
                        }
                    }
                    head = head->next;
                    node_counter++;
                }

                vertex2 = state_to_BDD( manager, state,
                                     2*num_env+num_sys, num_sys );
                tmp = Cudd_Not( vertex2 );
                Cudd_Ref( tmp );
                Cudd_RecursiveDeref( manager, vertex2 );
                vertex2 = tmp;

                tmp = Cudd_bddAnd( manager, strans, vertex2 );
                Cudd_Ref( tmp );
                Cudd_RecursiveDeref( manager, strans );
                strans = tmp;

                Cudd_RecursiveDeref( manager, vertex2 );
                free( state );

            } else {
                fprintf( stderr,
                         "Error patch_localfixpoint: unrecognized line in"
                         " given edge change file.\n" );
                return NULL;
            }
        } while (fgets( line, INPUT_STRING_LEN, change_fp ));
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

    for (goal_mode = 0; goal_mode < spc.num_sgoals; goal_mode++) {

        result_strategy = localfixpoint_goalmode( manager, num_env, num_sys,
                                                  strategy, goal_mode,
                                                  affected, affected_len,
                                                  etrans, strans, egoals,
                                                  N_BDD, N, N_len,
                                                  verbose );
        if (result_strategy == NULL)
            break;
        strategy = result_strategy;
    }

    if (goal_mode != spc.num_sgoals) {  /* Did a local patching attempt fail? */
        delete_aut( strategy );
        strategy = NULL;
    } else {
        strategy = aut_prune_deadends( strategy );
    }


    /* Pre-exit clean-up */
    free( cube );
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
    Cudd_RecursiveDeref( manager, N_BDD );
    for (i = 0; i < N_len; i++)
        free( *(N+i) );
    free( N );
    for (i = 0; i < spc.num_sgoals; i++)
        free( *(affected+i) );
    free( affected );
    free( affected_len );
    if (num_nonbool > 0)
        free( doffw );

    return strategy;
}
