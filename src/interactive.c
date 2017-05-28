/* interactive.c -- Functions to interact with gr1c (sub)level sets.
 *                  Also consider solve.c and solve_operators.c
 *
 * N.B., interaction gives access to the "sublevel" sets from the
 * vanilla fixed point formula, as returned by compute_sublevel_sets().
 * By contrast, synthesize() changes Y_0 and Y_1 for each system goal
 * so as to make the result more easily amenable to constructing a
 * strategy automaton.  This difference may later be changed so that
 * levelset_interactive() behaves more like synthesize().
 *
 *
 * SCL; 2012-2015
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef USE_READLINE
  #include <readline/readline.h>
  #include <readline/history.h>
#endif

#include "ptree.h"
#include "solve.h"
#include "solve_support.h"
#include "logging.h"


extern specification_t spc;


/***************************
 ** Commands (incomplete) **/
#define INTCOM_WINNING 1
#define INTCOM_ENVNEXT 2
#define INTCOM_SYSNEXT 3
#define INTCOM_RESTRICT 4
#define INTCOM_RELAX 5
#define INTCOM_CLEAR 6
#define INTCOM_BLKENV 7
#define INTCOM_BLKSYS 8
#define INTCOM_GETINDEX 9
#define INTCOM_REWIN 11
#define INTCOM_RELEVELS 12
#define INTCOM_SYSNEXTA 13
/***************************/

/* Help string */
#define INTCOM_HELP_STR "winning STATE\n" \
    "envnext STATE\n" \
    "sysnext STATE1 STATE2ENV GOALMODE\n" \
    "sysnexta STATE1 STATE2ENV\n" \
    "restrict STATE1 STATE2\n" \
    "relax STATE1 STATE2\n" \
    "clear\n" \
    "blocksys STATESYS\n" \
    "blockenv STATEENV\n" \
    "getindex STATE GOALMODE\n" \
    "refresh winning\n" \
    "refresh levels\n" \
    "addvar env (sys) VARIABLE\n" \
    "envvar\n" \
    "sysvar\n" \
    "var\n" \
    "numgoals\n" \
    "printgoal GOALMODE\n" \
    "printegoals\n" \
    "enable (disable) autoreorder\n" \
    "quit"

/**** Command arguments ****/
/* In the case of pointers, it is expected that command_loop will
   allocate the memory, and levelset_interactive (or otherwise the
   function that invoked command_loop) will free it. */
vartype *intcom_state;
int intcom_index;  /* May be used to pass strategy goal mode or length
                      of intcom_state. */


char *fgets_wrap( char *prompt, int max_len, FILE *infp, FILE *outfp )
{
    char *input;

    if (max_len < 1)
        return NULL;
    input = malloc( max_len*sizeof(char) );
    if (input == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    if (prompt != NULL && *prompt != '\0')
        fprintf( outfp, "%s", prompt );
    if (fgets( input, max_len, infp ) == NULL) {
        free( input );
        return NULL;
    }

    return input;
}


int command_loop( DdManager *manager, FILE *infp, FILE *outfp )
{
    int num_env, num_sys;

    ptree_t *tmppt;
    int var_index;
    char *input;
    int num_read;

    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

#ifdef USE_READLINE
    while ((input = readline( GR1C_INTERACTIVE_PROMPT ))) {
#else
    while ((input = fgets_wrap( GR1C_INTERACTIVE_PROMPT, 256, infp, outfp))) {
#endif
        if (*input == '\0') {
            free( input );
            continue;
        }
        if (!strncmp( input, "quit", strlen( "quit" ) )) {
            break;
        } else if (!strncmp( input, "help", strlen( "help" ) )) {
            fprintf( outfp, INTCOM_HELP_STR );
        } else if (!strncmp( input, "enable autoreorder",
                             strlen( "enable autoreorder" ) )) {
            Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
        } else if (!strncmp( input, "disable autoreorder",
                             strlen( "disable autoreorder" ) )) {
            Cudd_AutodynDisable( manager );
        } else if (!strncmp( input, "numgoals", strlen( "numgoals" ) )) {
            fprintf( outfp, "%d", spc.num_sgoals );
        } else if (!strncmp( input, "envvar", strlen( "envvar" ) )) {
            var_index = 0;
            if (spc.evar_list == NULL) {
                fprintf( outfp, "(none)" );
            } else {
                tmppt = spc.evar_list;
                while (tmppt) {
                    if (tmppt->left == NULL) {
                        fprintf( outfp, "%s (%d)", tmppt->name, var_index );
                    } else {
                        fprintf( outfp, "%s (%d), ", tmppt->name, var_index );
                    }
                    tmppt = tmppt->left;
                    var_index++;
                }
            }
        } else if (!strncmp( input, "sysvar", strlen( "sysvar" ) )) {
            var_index = num_env;
            if (spc.svar_list == NULL) {
                fprintf( outfp, "(none)" );
            } else {
                tmppt = spc.svar_list;
                while (tmppt) {
                    if (tmppt->left == NULL) {
                        fprintf( outfp, "%s (%d)", tmppt->name, var_index );
                    } else {
                        fprintf( outfp, "%s (%d), ", tmppt->name, var_index );
                    }
                    tmppt = tmppt->left;
                    var_index++;
                }
            }
        } else if (!strncmp( input, "var", strlen( "var" ) )) {
            var_index = 0;
            if (spc.evar_list != NULL) {
                tmppt = spc.evar_list;
                while (tmppt) {
                    fprintf( outfp, "%s (%d), ", tmppt->name, var_index );
                    tmppt = tmppt->left;
                    var_index++;
                }
            }
            if (spc.svar_list != NULL) {
                tmppt = spc.svar_list;
                while (tmppt) {
                    if (tmppt->left == NULL) {
                        fprintf( outfp, "%s (%d)", tmppt->name, var_index );
                    } else {
                        fprintf( outfp, "%s (%d), ", tmppt->name, var_index );
                    }
                    tmppt = tmppt->left;
                    var_index++;
                }
            }
        } else if (!strncmp( input, "refresh winning",
                             strlen( "refresh winning" ) )) {
            return INTCOM_REWIN;
        } else if (!strncmp( input, "refresh levels",
                             strlen( "refresh levels" ) )) {
            return INTCOM_RELEVELS;
        } else if (!strncmp( input, "printgoal ", strlen( "printgoal " ) )) {

            *(input+strlen( "printgoal" )) = '\0';
            num_read = read_state_str( input+strlen( "printgoal" )+1,
                                       &intcom_state, 1 );
            if (num_read < 1) {
                fprintf( outfp, "Invalid arguments." );
            } else {
                if (*intcom_state < 0 || *intcom_state > spc.num_sgoals-1) {
                    fprintf( outfp, "Invalid mode: %d", *intcom_state );
                } else {
                    print_formula( *(spc.sys_goals+*intcom_state), stdout,
                                   FORMULA_SYNTAX_GR1C );
                }
                free( intcom_state );
            }

        } else if (!strncmp( input, "printegoals", strlen( "printegoals" ) )) {

            for (var_index = 0; var_index < spc.num_egoals; var_index++) {
                print_formula( *(spc.env_goals+var_index), stdout,
                               FORMULA_SYNTAX_GR1C );
                fprintf( outfp, "\n" );
            }
            fprintf( outfp, "---" );

        } else if (!strncmp( input, "winning ", strlen( "winning " ) )) {

            *(input+strlen( "winning" )) = '\0';
            num_read = read_state_str( input+strlen( "winning" )+1,
                                       &intcom_state, num_env+num_sys );
            if (num_read == num_env+num_sys) {
                free( input );
                return INTCOM_WINNING;
            } else {
                if (num_read > 0)
                    free( intcom_state );
                fprintf( outfp, "Invalid arguments." );
            }

        } else if (!strncmp( input, "envnext ", strlen( "envnext " ) )) {

            *(input+strlen( "envnext" )) = '\0';
            num_read = read_state_str( input+strlen( "envnext" )+1,
                                       &intcom_state, num_env+num_sys );
            if (num_read == num_env+num_sys) {
                free( input );
                return INTCOM_ENVNEXT;
            } else {
                if (num_read > 0)
                    free( intcom_state );
                fprintf( outfp, "Invalid arguments." );
            }

        } else if (!strncmp( input, "sysnext ", strlen( "sysnext " ) )) {

            *(input+strlen( "sysnext" )) = '\0';
            num_read = read_state_str( input+strlen( "sysnext" )+1,
                                       &intcom_state, 2*num_env+num_sys+1 );
            if (num_read != 2*num_env+num_sys+1) {
                if (num_read > 0)
                    free( intcom_state );
                free( input );
                fprintf( outfp, "Invalid arguments.\n" );
                continue;
            }
            if (*(intcom_state+2*num_env+num_sys) < 0
                || *(intcom_state+2*num_env+num_sys) > spc.num_sgoals-1) {
                fprintf( outfp,
                         "Invalid mode: %d",
                         *(intcom_state+2*num_env+num_sys) );
                free( intcom_state );
            } else {
                free( input );
                intcom_index  = *(intcom_state+2*num_env+num_sys);
                intcom_state = realloc( intcom_state,
                                        (2*num_env+num_sys)*sizeof(vartype) );
                if (intcom_state == NULL) {
                    perror( __FILE__ ",  realloc" );
                    exit(-1);
                }
                return INTCOM_SYSNEXT;
            }

        } else if (!strncmp( input, "sysnexta ", strlen( "sysnexta " ) )) {

            *(input+strlen( "sysnexta" )) = '\0';
            num_read = read_state_str( input+strlen( "sysnexta" )+1,
                                       &intcom_state, 2*num_env+num_sys );
            if (num_read == 2*num_env+num_sys) {
                free( input );
                return INTCOM_SYSNEXTA;
            } else {
                if (num_read > 0)
                    free( intcom_state );
                fprintf( outfp, "Invalid arguments." );
            }

        } else if (!strncmp( input, "restrict ", strlen( "restrict " ) )) {

            *(input+strlen( "restrict" )) = '\0';
            num_read = read_state_str( input+strlen( "restrict" )+1,
                                       &intcom_state, 2*(num_env+num_sys) );
            if ((num_read == 2*(num_env+num_sys))
                || (num_read == 2*num_env+num_sys)) {
                free( input );
                intcom_index = num_read;
                return INTCOM_RESTRICT;
            } else {
                if (num_read > 0)
                    free( intcom_state );
                fprintf( outfp, "Invalid arguments." );
            }

        } else if (!strncmp( input, "relax ", strlen( "relax " ) )) {

            *(input+strlen( "relax" )) = '\0';
            num_read = read_state_str( input+strlen( "relax" )+1, &intcom_state,
                                       2*(num_env+num_sys) );
            if ((num_read == 2*(num_env+num_sys))
                || (num_read == 2*num_env+num_sys)) {
                free( input );
                intcom_index = num_read;
                return INTCOM_RELAX;
            } else {
                if (num_read > 0)
                    free( intcom_state );
                fprintf( outfp, "Invalid arguments." );
            }

        } else if (!strncmp( input, "clear", strlen( "clear" ) )) {
            free( input );
            return INTCOM_CLEAR;
        } else if (!strncmp( input, "blockenv ", strlen( "blockenv " ) )) {

            *(input+strlen( "blockenv" )) = '\0';
            num_read = read_state_str( input+strlen( "blockenv" )+1,
                                       &intcom_state, num_env );
            if (num_read == num_env) {
                free( input );
                return INTCOM_BLKENV;
            } else {
                if (num_read > 0)
                    free( intcom_state );
                fprintf( outfp, "Invalid arguments." );
            }

        } else if (!strncmp( input, "blocksys ", strlen( "blocksys " ) )) {

            *(input+strlen( "blocksys" )) = '\0';
            num_read = read_state_str( input+strlen( "blocksys" )+1,
                                       &intcom_state, num_sys );
            if (num_read == num_sys) {
                free( input );
                return INTCOM_BLKSYS;
            } else {
                if (num_read > 0)
                    free( intcom_state );
                fprintf( outfp, "Invalid arguments." );
            }

        } else if (!strncmp( input, "getindex ", strlen( "getindex " ) )) {

            *(input+strlen( "getindex" )) = '\0';
            num_read = read_state_str( input+strlen( "getindex" )+1,
                                       &intcom_state, num_env+num_sys+1 );
            if (num_read != num_env+num_sys+1) {
                if (num_read > 0)
                    free( intcom_state );
                free( input );
                fprintf( outfp, "Invalid arguments.\n" );
                continue;
            }
            if (*(intcom_state+num_env+num_sys) < 0
                || *(intcom_state+num_env+num_sys) > spc.num_sgoals-1) {
                fprintf( outfp,
                         "Invalid mode: %d", *(intcom_state+num_env+num_sys) );
                free( intcom_state );
            } else {
                free( input );
                intcom_index = *(intcom_state+num_env+num_sys);
                intcom_state = realloc( intcom_state,
                                        (num_env+num_sys)*sizeof(vartype) );
                if (intcom_state == NULL) {
                    perror( __FILE__ ",  realloc" );
                    exit(-1);
                }
                return INTCOM_GETINDEX;
            }

        } else {
            fprintf( outfp, "Unrecognized command: %s", input );
        }

        fprintf( outfp, "\n" );
        free( input );
        fflush( outfp );
    }
    if (input != NULL)
        free( input );

    return 0;
}


int levelset_interactive( DdManager *manager, unsigned char init_flags,
                          FILE *infp, FILE *outfp,
                          unsigned char verbose )
{
    int command;
    vartype *state;
    vartype **env_moves;
    int emoves_len;

    ptree_t *var_separator;
    DdNode *W;
    DdNode *strans_into_W;

    DdNode *etrans, *strans, **egoals, **sgoals;

    DdNode *etrans_patched, *strans_patched;
    DdNode *vertex1, *vertex2; /* ...regarding vertices of the game graph. */

    DdNode *ddval;  /* Store result of evaluating a BDD */
    DdNode ***Y = NULL;
    DdNode *Y_i_primed;
    int *num_sublevels = NULL;
    DdNode ****X_ijr = NULL;

    DdNode *tmp, *tmp2;
    int i, j, r;  /* Generic counters */
    bool env_nogoal_flag = False;  /* Indicate environment has no goals */

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
            return -1;
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

    if (var_separator == NULL) {
            spc.evar_list = NULL;
    } else {
        var_separator->left = NULL;
    }

    etrans_patched = etrans;
    Cudd_Ref( etrans_patched );
    strans_patched = strans;
    Cudd_Ref( strans_patched );

    W = compute_winning_set_BDD( manager,
                                 etrans, strans, egoals, sgoals, verbose );
    if (W == NULL) {
        fprintf( stderr,
                 "Error levelset_interactive: failed to construct winning"
                 " set.\n" );
        free( state );
        free( cube );
        return -1;
    }

    command = INTCOM_RELEVELS;  /* Initialization, force sublevel computation */
    do {
        switch (command) {
        case INTCOM_REWIN:
            if (W != NULL)
                Cudd_RecursiveDeref( manager, W );
            W = compute_winning_set_BDD( manager,
                                         etrans_patched, strans_patched,
                                         egoals, sgoals, verbose );
            if (W == NULL) {
                fprintf( stderr,
                         "Error levelset_interactive: failed to construct"
                         " winning set.\n" );
                return -1;
            }
            break;

        case INTCOM_RELEVELS:
            if (W != NULL)
                Cudd_RecursiveDeref( manager, W );
            W = compute_winning_set_BDD( manager,
                                         etrans_patched, strans_patched,
                                         egoals, sgoals, verbose );
            if (W == NULL) {
                fprintf( stderr,
                         "Error levelset_interactive: failed to construct"
                         " winning set.\n" );
                return -1;
            }
            if (Y != NULL) {
                for (i = 0; i < spc.num_sgoals; i++) {
                    for (j = 0; j < *(num_sublevels+i); j++)
                        Cudd_RecursiveDeref( manager, *(*(Y+i)+j) );
                    if (*(num_sublevels+i) > 0)
                        free( *(Y+i) );
                }
                if (spc.num_sgoals > 0) {
                    free( Y );
                    free( num_sublevels );
                }
            }
            Y = compute_sublevel_sets( manager, W,
                                       etrans_patched, strans_patched,
                                       egoals, spc.num_egoals,
                                       sgoals, spc.num_sgoals,
                                       &num_sublevels, &X_ijr, verbose );
            if (Y == NULL) {
                fprintf( stderr,
                         "Error levelset_interactive: failed to construct"
                         " sublevel sets.\n" );
                return -1;
            }
            break;

        case INTCOM_ENVNEXT:
            if (num_env > 0) {
                env_moves = get_env_moves( manager, cube, intcom_state,
                                           etrans_patched, num_env, num_sys,
                                           &emoves_len );
                free( intcom_state );
            } else {
                fprintf( outfp, "(none)\n" );
                free( intcom_state );
                break;
            }

            for (i = 0; i < emoves_len; i++) {
                if (num_env > 0)
                    fprintf( outfp, "%d", **(env_moves+i) );
                for (j = 1; j < num_env; j++)
                    fprintf( outfp, " %d", *(*(env_moves+i)+j) );
                fprintf( outfp, "\n" );
            }
            fprintf( outfp, "---\n" );

            if (emoves_len > 0) {
                for (i = 0; i < emoves_len; i++)
                    free( *(env_moves+i) );
                free( env_moves );
            }
            break;

        case INTCOM_SYSNEXT:

            tmp = Cudd_bddVarMap( manager, W );
            if (tmp == NULL) {
                fprintf( stderr,
                         "Error levelset_interactive: Error in swapping"
                         " variables with primed forms.\n" );
                return -1;
            }
            Cudd_Ref( tmp );
            strans_into_W = Cudd_bddAnd( manager, strans_patched, tmp );
            Cudd_Ref( strans_into_W );
            Cudd_RecursiveDeref( manager, tmp );

            state_to_cube( intcom_state, cube, num_env+num_sys );
            j = *(num_sublevels+intcom_index);
            do {
                j--;
                ddval = Cudd_Eval( manager, *(*(Y+intcom_index)+j), cube );
                if (Cudd_IsComplement( ddval )) {
                    j++;
                    break;
                }
            } while (j > 0);
            if (j == 0) {
                Y_i_primed = Cudd_bddVarMap( manager,
                                             *(*(Y+intcom_index)) );
            } else {
                Y_i_primed = Cudd_bddVarMap( manager,
                                             *(*(Y+intcom_index)+j-1) );
            }
            if (Y_i_primed == NULL) {
                fprintf( stderr,
                         "levelset_interactive: Error in swapping variables"
                         " with primed forms.\n" );
                return -1;
            }
            Cudd_Ref( Y_i_primed );

            tmp = Cudd_bddAnd( manager, strans_into_W, Y_i_primed );
            Cudd_Ref( tmp );
            tmp2 = state_to_cof( manager, cube, 2*(num_env+num_sys),
                              intcom_state, tmp, 0, num_env+num_sys );
            Cudd_RecursiveDeref( manager, tmp );
            if (num_env > 0) {
                tmp = state_to_cof( manager, cube, 2*(num_sys+num_env),
                                 intcom_state+num_env+num_sys,
                                 tmp2, num_env+num_sys, num_env );
                Cudd_RecursiveDeref( manager, tmp2 );
            } else {
                tmp = tmp2;
            }
            if (verbose) {
                logprint( "From state " );
                for (i = 0; i < num_env+num_sys; i++)
                    logprint( " %d", *(intcom_state+i) );
                logprint( "; given env move " );
                for (i = 0; i < num_env; i++)
                    logprint( " %d", *(intcom_state+num_env+num_sys+i) );
                logprint( "; goal %d", intcom_index );
            }

            Cudd_AutodynDisable( manager );
            /* Mark first element to detect whether any cubes were generated. */
            *state = -1;
            Cudd_ForeachCube( manager, tmp, gen, gcube, gvalue ) {
                initialize_cube( state, gcube+2*num_env+num_sys, num_sys );
                while (!saturated_cube( state, gcube+2*num_env+num_sys,
                                        num_sys )) {
                    fprintf( outfp, "%d", *state );
                    for (i = 1; i < num_sys; i++)
                        fprintf( outfp, " %d", *(state+i) );
                    fprintf( outfp, "\n" );
                    increment_cube( state, gcube+2*num_env+num_sys, num_sys );
                }
                fprintf( outfp, "%d", *state );
                for (i = 1; i < num_sys; i++)
                    fprintf( outfp, " %d", *(state+i) );
                fprintf( outfp, "\n" );
            }
            Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
            if (*state == -1) {
                /* Cannot step closer to system goal, so must be in
                   goal state or able to block environment goal. */
                Cudd_RecursiveDeref( manager, tmp );
                Cudd_RecursiveDeref( manager, Y_i_primed );
                if (j > 0) {
                    Y_i_primed = Cudd_bddVarMap( manager,
                                                 *(*(Y+intcom_index)+j) );
                    if (Y_i_primed == NULL) {
                        fprintf( stderr,
                                 "levelset_interactive: Error in swapping"
                                 " variables with primed forms.\n" );
                        return -1;
                    }
                } else {
                    Y_i_primed = Cudd_ReadOne( manager );
                }
                Cudd_Ref( Y_i_primed );
                tmp = Cudd_bddAnd( manager, strans_into_W, Y_i_primed );
                Cudd_Ref( tmp );
                tmp2 = state_to_cof( manager, cube, 2*(num_env+num_sys),
                                  intcom_state, tmp, 0, num_env+num_sys );
                Cudd_RecursiveDeref( manager, tmp );
                if (num_env > 0) {
                    tmp = state_to_cof( manager, cube, 2*(num_sys+num_env),
                                     intcom_state+num_env+num_sys,
                                     tmp2, num_env+num_sys, num_env );
                    Cudd_RecursiveDeref( manager, tmp2 );
                } else {
                    tmp = tmp2;
                }

                Cudd_AutodynDisable( manager );
                Cudd_ForeachCube( manager, tmp, gen, gcube, gvalue ) {
                    initialize_cube( state, gcube+2*num_env+num_sys, num_sys );
                    while (!saturated_cube( state, gcube+2*num_env+num_sys,
                                            num_sys )) {
                        fprintf( outfp, "%d", *state );
                        for (i = 1; i < num_sys; i++)
                            fprintf( outfp, " %d", *(state+i) );
                        fprintf( outfp, "\n" );
                        increment_cube( state, gcube+2*num_env+num_sys,
                                        num_sys );
                    }
                    fprintf( outfp, "%d", *state );
                    for (i = 1; i < num_sys; i++)
                        fprintf( outfp, " %d", *(state+i) );
                    fprintf( outfp, "\n" );
                }
                Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
            }
            fprintf( outfp, "---\n" );

            free( intcom_state );
            Cudd_RecursiveDeref( manager, tmp );
            Cudd_RecursiveDeref( manager, Y_i_primed );
            Cudd_RecursiveDeref( manager, strans_into_W );
            break;

        case INTCOM_SYSNEXTA:
            tmp = state_to_cof( manager, cube, 2*(num_env+num_sys),
                             intcom_state, strans_patched, 0, num_env+num_sys );
            if (num_env > 0) {
                tmp2 = state_to_cof( manager, cube, 2*(num_sys+num_env),
                                  intcom_state+num_env+num_sys,
                                  tmp, num_env+num_sys, num_env );
                Cudd_RecursiveDeref( manager, tmp );
            } else {
                tmp2 = tmp;
            }
            free( intcom_state );

            Cudd_AutodynDisable( manager );
            Cudd_ForeachCube( manager, tmp2, gen, gcube, gvalue ) {
                initialize_cube( state, gcube+2*num_env+num_sys, num_sys );
                while (!saturated_cube( state, gcube+2*num_env+num_sys,
                                        num_sys )) {
                    fprintf( outfp, "%d", *state );
                    for (i = 1; i < num_sys; i++)
                        fprintf( outfp, " %d", *(state+i) );
                    fprintf( outfp, "\n" );
                    increment_cube( state, gcube+2*num_env+num_sys, num_sys );
                }
                fprintf( outfp, "%d", *state );
                for (i = 1; i < num_sys; i++)
                    fprintf( outfp, " %d", *(state+i) );
                fprintf( outfp, "\n" );
            }
            Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
            fprintf( outfp, "---\n" );

            Cudd_RecursiveDeref( manager, tmp2 );
            break;

        case INTCOM_WINNING:
            if (verbose) {
                logprint( "Winning set membership check for state " );
                for (i = 0; i < num_env+num_sys; i++)
                    logprint( " %d", *(intcom_state+i) );
            }
            state_to_cube( intcom_state, cube, num_env+num_sys );
            free( intcom_state );
            ddval = Cudd_Eval( manager, W, cube );
            if (Cudd_IsComplement( ddval )) {
                fprintf( outfp, "False\n" );
            } else {
                fprintf( outfp, "True\n" );
            }
            break;

        case INTCOM_GETINDEX:
            if (verbose) {
                logprint( "Reachability index for goal %d of state ",
                          intcom_index );
                for (i = 0; i < num_env+num_sys; i++)
                    logprint( " %d", *(intcom_state+i) );
            }

            /* Check whether state is in winning set. */
            state_to_cube( intcom_state, cube, num_env+num_sys );
            free( intcom_state );
            ddval = Cudd_Eval( manager, W, cube );
            if (Cudd_IsComplement( ddval )) {
                fprintf( outfp, "Inf\n" );
                break;
            }

            /* Yes, so return a finite value. */
            j = *(num_sublevels+intcom_index);
            do {
                j--;
                ddval = Cudd_Eval( manager, *(*(Y+intcom_index)+j), cube );
                if (Cudd_IsComplement( ddval )) {
                    j++;
                    break;
                }
            } while (j > 0);
            fprintf( outfp, "%d\n", j );
            break;

        case INTCOM_RESTRICT:
        case INTCOM_RELAX:
            if (verbose) {
                if (command == INTCOM_RESTRICT) {
                    if (intcom_index == 2*num_env+num_sys) {
                        logprint( "Removing uncontrolled edge from" );
                    } else { /* intcom_index == 2*(num_env+num_sys) */
                        logprint( "Removing controlled edge from" );
                    }
                } else { /* INTCOM_RELAX */
                    if (intcom_index == 2*num_env+num_sys) {
                        logprint( "Adding uncontrolled edge from" );
                    } else { /* intcom_index == 2*(num_env+num_sys) */
                        logprint( "Adding controlled edge from" );
                    }
                }
                for (i = 0; i < num_env+num_sys; i++)
                    logprint( " %d", *(intcom_state+i) );
                logprint( " to " );
                for (i = num_env+num_sys; i < intcom_index; i++)
                    logprint( " %d", *(intcom_state+i) );
            }

            vertex1 = state_to_BDD( manager, intcom_state, 0, num_env+num_sys );
            vertex2 = state_to_BDD( manager,
                                 intcom_state+num_env+num_sys, num_env+num_sys,
                                 intcom_index-(num_env+num_sys) );
            if (command == INTCOM_RESTRICT) {
                tmp = Cudd_Not( vertex2 );
                Cudd_Ref( tmp );
                Cudd_RecursiveDeref( manager, vertex2 );
                vertex2 = tmp;
                tmp = Cudd_bddOr( manager, Cudd_Not( vertex1 ), vertex2 );
            } else { /* INTCOM_RELAX */
                tmp = Cudd_bddAnd( manager, vertex1, vertex2 );
            }

            Cudd_Ref( tmp );
            Cudd_RecursiveDeref( manager, vertex1 );
            Cudd_RecursiveDeref( manager, vertex2 );
            if (intcom_index == 2*num_env+num_sys) {
                if (command == INTCOM_RESTRICT) {
                    tmp2 = Cudd_bddAnd( manager, tmp, etrans_patched );
                } else { /* INTCOM_RELAX */
                    tmp2 = Cudd_bddOr( manager, tmp, etrans_patched );
                }
                Cudd_Ref( tmp2 );
                Cudd_RecursiveDeref( manager, etrans_patched );
                etrans_patched = tmp2;
            } else { /* intcom_index == 2*(num_env+num_sys) */
                if (command == INTCOM_RESTRICT) {
                    tmp2 = Cudd_bddAnd( manager, tmp, strans_patched );
                } else { /* INTCOM_RELAX */
                    tmp2 = Cudd_bddOr( manager, tmp, strans_patched );
                }
                Cudd_Ref( tmp2 );
                Cudd_RecursiveDeref( manager, strans_patched );
                strans_patched = tmp2;
            }
            Cudd_RecursiveDeref( manager, tmp );
            free( intcom_state );
            break;

        case INTCOM_BLKENV:
            if (verbose) {
                logprint( "Removing environment moves into" );
                for (i = 0; i < num_env; i++)
                    logprint( " %d", *(intcom_state+i) );
            }
            vertex2 = state_to_BDD( manager,
                                 intcom_state, num_env+num_sys, num_env );
            tmp = Cudd_Not( vertex2 );
            Cudd_Ref( tmp );
            Cudd_RecursiveDeref( manager, vertex2 );
            vertex2 = tmp;

            tmp = Cudd_bddAnd( manager, etrans_patched, vertex2 );
            Cudd_Ref( tmp );
            Cudd_RecursiveDeref( manager, etrans_patched );
            etrans_patched = tmp;

            Cudd_RecursiveDeref( manager, vertex2 );
            free( intcom_state );
            break;

        case INTCOM_BLKSYS:
            if (verbose) {
                logprint( "Removing system moves into" );
                for (i = 0; i < num_sys; i++)
                    logprint( " %d", *(intcom_state+i) );
            }

            vertex2 = state_to_BDD( manager,
                                 intcom_state, 2*num_env+num_sys, num_sys );
            tmp = Cudd_Not( vertex2 );
            Cudd_Ref( tmp );
            Cudd_RecursiveDeref( manager, vertex2 );
            vertex2 = tmp;

            tmp = Cudd_bddAnd( manager, strans_patched, vertex2 );
            Cudd_Ref( tmp );
            Cudd_RecursiveDeref( manager, strans_patched );
            strans_patched = tmp;

            Cudd_RecursiveDeref( manager, vertex2 );
            free( intcom_state );
            break;

        case INTCOM_CLEAR:
            Cudd_RecursiveDeref( manager, etrans_patched );
            Cudd_RecursiveDeref( manager, strans_patched );
            etrans_patched = etrans;
            Cudd_Ref( etrans_patched );
            strans_patched = strans;
            Cudd_Ref( strans_patched );
            break;
        }

        fflush( outfp );
    } while ((command = command_loop( manager, infp, outfp )) > 0);

    /* Pre-exit clean-up */
    Cudd_RecursiveDeref( manager, etrans_patched );
    Cudd_RecursiveDeref( manager, strans_patched );
    Cudd_RecursiveDeref( manager, W );
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
    if (env_nogoal_flag) {
        spc.num_egoals = 0;
        delete_tree( *spc.env_goals );
        free( spc.env_goals );
    }
    if (spc.num_sgoals > 0) {
        free( Y );
        free( X_ijr );
        free( num_sublevels );
    }

    if (command < 0)  /* command_loop returned error code? */
        return command;
    return 1;
}
