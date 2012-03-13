/* interactive.c -- Functions to interact with gr1c (sub)level sets.
 *                  Also see solve.c and solve_operators.c
 *
 *
 * SCL; Mar 2012.
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "ptree.h"
#include "solve.h"


extern ptree_t *evar_list;
extern ptree_t *svar_list;
extern ptree_t *env_init;
extern ptree_t *sys_init;
extern ptree_t *env_trans;
extern ptree_t *sys_trans;
extern ptree_t **env_goals;
extern ptree_t **sys_goals;
extern int num_egoals;
extern int num_sgoals;


/* Internal routines for setting typical cube arrays. */
extern void cube_env( int *cube, int num_env, int num_sys );
extern void cube_sys( int *cube, int num_env, int num_sys );
extern void cube_prime_env( int *cube, int num_env, int num_sys );
extern void cube_prime_sys( int *cube, int num_env, int num_sys );

/* Functions for enumerating states represented by a cube array from
   CUDD.  See solve.c for documentation. */
extern void increment_cube( bool *cube, int *gcube, int len );
extern bool saturated_cube( bool *cube, int *gcube, int len );
extern void initialize_cube( bool *cube, int *gcube, int len );
extern void state2cube( bool *state, int *cube, int len );

/* More functions not yet globally exported.  See solve.c and
   solve_operators.c for definitions and documentation. */
extern DdNode *state2cof( DdManager *manager, int *cube, int cube_len,
						  bool *state, DdNode *trans, int offset, int len );
extern bool **get_env_moves( DdManager *manager, int *cube,
							 bool *state, DdNode *etrans,
							 int num_env, int num_sys, int *emoves_len );
extern DdNode *compute_existsmodal( DdManager *manager, DdNode *C,
									DdNode *etrans, DdNode *strans,
									int num_env, int num_sys, int *cube );
extern DdNode *check_realizable_internal( DdManager *manager, DdNode *W,
										  unsigned char init_flags, unsigned char verbose );


/***************************
 ** Commands (incomplete) **/
#define INTCOM_WINNING 1
#define INTCOM_ENVNEXT 2
#define INTCOM_SYSNEXT 3
#define INTCOM_RESTRICT 4
#define INTCOM_RELAX 5
#define INTCOM_UNRESTRICT 6
#define INTCOM_UNRELAX 7
#define INTCOM_UNREACH 8
#define INTCOM_GETINDEX 9
#define INTCOM_SETINDEX 10
#define INTCOM_REWIN 11
#define INTCOM_RELEVELS 12
/***************************/

/**** Command arguments ****/
/* In the case of pointers, it is expected that command_loop will
   allocate the memory, and levelset_interactive (or otherwise the
   function that invoked command_loop) will free it. */
bool *intcom_state1, *intcom_state2;
int intcom_index;
char *intcom_name;


/* Read space-separated values from given string.  Return the state
   vector on success, NULL if error.  N.B., the given string will be
   manipulated and not restored, up to the position of the last read
   value. */
bool *read_state_str( char *input, int len )
{
	bool *state;
	int i;
	char *start;
	char *end;
	int input_len = strlen( input );
	if (len < 1 || input_len < 1)
		return NULL;
	state = malloc( sizeof(bool)*len );
	if (state == NULL) {
		perror( "read_state_str, malloc" );
		return NULL;
	}

	start = input;
	end = input;
	for (i = 0; i < len && *end != '\0'; i++) {
		*(state+i) = strtol( start, &end, 10 );
		start = end;
	}
	if (i < len) {
		fprintf( stderr, "Error read_state_str: not all of requested state components found.\n" );
		free( state );
		return NULL;
	}

	return state;
}

int command_loop( DdManager *manager, FILE *infp, FILE *outfp )
{
	int num_env, num_sys;

	ptree_t *tmppt;
	int var_index;
	char *input;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	while (input = readline( "" )) {
		if (!strncmp( input, "quit", strlen( "quit" ) )) {
			break;
		} else if (!strncmp( input, "help", strlen( "help" ) )) {
			fprintf( outfp, "(not implemented yet.)" );
		} else if (!strncmp( input, "enable autoreorder", strlen( "enable autoreorder" ) )) {
			Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
		} else if (!strncmp( input, "disable autoreorder", strlen( "disable autoreorder" ) )) {
			Cudd_AutodynDisable( manager );
		} else if (!strncmp( input, "numgoals", strlen( "numgoals" ) )) {
			fprintf( outfp, "%d", num_sgoals );
		} else if (!strncmp( input, "envvar", strlen( "envvar" ) )) {
			var_index = 0;
			if (evar_list == NULL) {
				fprintf( outfp, "(none)" );
			} else {
				tmppt = evar_list;
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
			if (svar_list == NULL) {
				fprintf( outfp, "(none)" );
			} else {
				tmppt = svar_list;
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
			if (evar_list != NULL) {
				tmppt = evar_list;
				while (tmppt) {
					fprintf( outfp, "%s (%d), ", tmppt->name, var_index );
					tmppt = tmppt->left;
					var_index++;
				}
			}
			if (svar_list != NULL) {
				tmppt = svar_list;
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
		} else if (!strncmp( input, "winning ", strlen( "winning " ) )) {

			*(input+strlen( "winning" )) = '\0';
			intcom_state1 = read_state_str( input+strlen( "winning" )+1,
											num_env+num_sys );
			free( input );
			if (intcom_state1 == NULL)
				return -1;
			return INTCOM_WINNING;
			
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
	bool *state;
	bool **env_moves;
	int emoves_len;

	ptree_t *var_separator;
	DdNode *W;
	DdNode *strans_into_W;

	DdNode *einit, *sinit, *etrans, *strans, **egoals, **sgoals;
	
	DdNode *ddval;  /* Store result of evaluating a BDD */
	DdNode ***Y = NULL, *Y_exmod;
	DdNode *Y_i_primed;
	int *num_sublevels = NULL;

	DdNode *tmp, *tmp2;
	int i, j;  /* Generic counters */
	bool env_nogoal_flag = False;  /* Indicate environment has no goals */
	
	int num_env, num_sys;
	int *cube;  /* length will be twice total number of variables (to
				   account for both variables and their primes). */

	/* Variables used during CUDD generation (state enumeration). */
	DdGen *gen;
	CUDD_VALUE_TYPE gvalue;
	int *gcube;

	if (verbose) {
		printf( "== Cudd_PrintInfo(), called from levelset_interactive ================\n" );
		Cudd_PrintInfo( manager, stdout );
		printf( "======================================================================\n" );
	}

	/* Set environment goal to True (i.e., any state) if none was
	   given. This simplifies the implementation below. */
	if (num_egoals == 0) {
		env_nogoal_flag = True;
		num_egoals = 1;
		env_goals = malloc( sizeof(ptree_t *) );
		*env_goals = init_ptree( PT_CONSTANT, NULL, 1 );
	}

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	/* State vector (i.e., valuation of the variables) */
	state = malloc( sizeof(bool)*(num_env+num_sys) );
	if (state == NULL) {
		perror( "levelset_interactive, malloc" );
		return NULL;
	}

	/* Allocate cube array, used later for quantifying over variables. */
	cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
	if (cube == NULL) {
		perror( "levelset_interactive, malloc" );
		return NULL;
	}

	/* Chain together environment and system variable lists for
	   working with BDD library. */
	if (evar_list == NULL) {
		var_separator = NULL;
		evar_list = svar_list;  /* that this is the deterministic case
								   is indicated by var_separator = NULL. */
	} else {
		var_separator = get_list_item( evar_list, -1 );
		if (var_separator == NULL) {
			fprintf( stderr, "Error: get_list_item failed on environment variables list.\n" );
			return NULL;
		}
		var_separator->left = svar_list;
	}

	/* Generate BDDs for the various parse trees from the problem spec. */
	einit = ptree_BDD( env_init, evar_list, manager );
	sinit = ptree_BDD( sys_init, evar_list, manager );
	if (verbose) {
		printf( "Building environment transition BDD..." );
		fflush( stdout );
	}
	etrans = ptree_BDD( env_trans, evar_list, manager );
	if (verbose) {
		printf( "Done.\n" );
		printf( "Building system transition BDD..." );
		fflush( stdout );
	}
	strans = ptree_BDD( sys_trans, evar_list, manager );
	if (verbose) {
		printf( "Done.\n" );
		fflush( stdout );
	}

	/* Build goal BDDs, if present. */
	if (num_egoals > 0) {
		egoals = malloc( num_egoals*sizeof(DdNode *) );
		for (i = 0; i < num_egoals; i++)
			*(egoals+i) = ptree_BDD( *(env_goals+i), evar_list, manager );
	} else {
		egoals = NULL;
	}
	if (num_sgoals > 0) {
		sgoals = malloc( num_sgoals*sizeof(DdNode *) );
		for (i = 0; i < num_sgoals; i++)
			*(sgoals+i) = ptree_BDD( *(sys_goals+i), evar_list, manager );
	} else {
		sgoals = NULL;
	}

	if (var_separator == NULL) {
			evar_list = NULL;
	} else {
		var_separator->left = NULL;
	}

	W = compute_winning_set_BDD( manager, etrans, strans, egoals, sgoals, verbose );
	if (W == NULL) {
		fprintf( stderr, "Error levelset_interactive: failed to construct winning set.\n" );
		return -1;
	}
	W = check_realizable_internal( manager, W, init_flags, verbose );
	if (W == NULL)
		return 0;  /* not realizable */

	/* Make primed form of W and take conjunction with system
	   transition (safety) formula, for use while stepping down Y_i
	   sets.  Note that we assume the variable map has been
	   appropriately defined in the CUDD manager, after the call to
	   compute_winning_set above. */
	tmp = Cudd_bddVarMap( manager, W );
	if (tmp == NULL) {
		fprintf( stderr, "Error levelset_interactive: Error in swapping variables with primed forms.\n" );
		return NULL;
	}
	Cudd_Ref( tmp );
	strans_into_W = Cudd_bddAnd( manager, strans, tmp );
	Cudd_Ref( strans_into_W );
	Cudd_RecursiveDeref( manager, tmp );

	command = INTCOM_RELEVELS;  /* Initialization, force sublevel computation */
	do {
		switch (command) {
		case INTCOM_REWIN:
			if (W != NULL)
				Cudd_RecursiveDeref( manager, W );
			W = compute_winning_set_BDD( manager, etrans, strans, egoals, sgoals, verbose );
			if (W == NULL) {
				fprintf( stderr, "Error levelset_interactive: failed to construct winning set.\n" );
				return -1;
			}
			break;

		case INTCOM_RELEVELS:
			if (W != NULL)
				Cudd_RecursiveDeref( manager, W );
			W = compute_winning_set_BDD( manager, etrans, strans, egoals, sgoals, verbose );
			if (W == NULL) {
				fprintf( stderr, "Error levelset_interactive: failed to construct winning set.\n" );
				return -1;
			}
			if (Y != NULL) {
				for (i = 0; i < num_sgoals; i++) {
					for (j = 0; j < *(num_sublevels+i); j++)
						Cudd_RecursiveDeref( manager, *(*(Y+i)+j) );
					if (*(num_sublevels+i) > 0)
						free( *(Y+i) );
				}
				if (num_sgoals > 0) {
					free( Y );
					free( num_sublevels );
				}
			}
			Y = compute_sublevel_sets( manager, W, etrans, strans, egoals, sgoals,
									   &num_sublevels, verbose );
			if (Y == NULL) {
				fprintf( stderr, "Error levelset_interactive: failed to construct sublevel sets.\n" );
				return -1;
			}
			break;

		case INTCOM_ENVNEXT:
			
			break;

		case INTCOM_WINNING:
			state2cube( intcom_state1, cube, num_env+num_sys );
			ddval = Cudd_Eval( manager, W, cube );
			if (ddval->type.value < .1) {
				fprintf( outfp, "False\n" );
			} else {
				fprintf( outfp, "True\n" );
			}
			free( intcom_state1 );
			break;
		}

	} while ((command = command_loop( manager, infp, outfp )) > 0);

	if (verbose) {
		printf( "== Cudd_PrintInfo(), called from levelset_interactive ================\n" );
		Cudd_PrintInfo( manager, stdout );
		printf( "======================================================================\n" );
	}

	/* Pre-exit clean-up */
	Cudd_RecursiveDeref( manager, W );
	Cudd_RecursiveDeref( manager, strans_into_W );
	Cudd_RecursiveDeref( manager, einit );
	Cudd_RecursiveDeref( manager, sinit );
	Cudd_RecursiveDeref( manager, etrans );
	Cudd_RecursiveDeref( manager, strans );
	for (i = 0; i < num_egoals; i++)
		Cudd_RecursiveDeref( manager, *(egoals+i) );
	for (i = 0; i < num_sgoals; i++)
		Cudd_RecursiveDeref( manager, *(sgoals+i) );
	if (num_egoals > 0)
		free( egoals );
	if (num_sgoals > 0)
		free( sgoals );
	free( cube );
	free( state );
	if (env_nogoal_flag) {
		num_egoals = 0;
		delete_tree( *env_goals );
		free( env_goals );
	}
	for (i = 0; i < num_sgoals; i++) {
		for (j = 0; j < *(num_sublevels+i); j++)
			Cudd_RecursiveDeref( manager, *(*(Y+i)+j) );
		if (*(num_sublevels+i) > 0)
			free( *(Y+i) );
	}
	if (num_sgoals > 0) {
		free( Y );
		free( num_sublevels );
	}

	if (command < 0)  /* command_loop returned error code? */
		return command;
	return 1;
}
