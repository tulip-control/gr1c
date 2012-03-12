/* interactive.c -- Functions to interact with gr1c (sub)level sets.
 *                  Also see solve.c and solve_operators.c
 *
 *
 * SCL; Mar 2012.
 */


#include <string.h>
#include <stdio.h>
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


int command_loop( DdManager *manager, FILE *infp, FILE *outfp )
{
	int num_env, num_sys;

	ptree_t *tmppt;
	int var_index;
	char *input;
	size_t input_len;

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
		} else {
			/* input_len = strlen( input ); */

			/* if (input_len > strlen( "winning " )) { */
			/* 	*(input+strlen( "winning" )) = '\0'; */
			/* 	if (!strncmp( input, "winning", strlen( "winning" ) )) { */
			/* 		free( input ); */
			/* 		return INTCOM_WINNING; */
			/* 	} */
			/* } */

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
	DdNode *W = check_realizable( manager, init_flags, verbose );
	DdNode *strans_into_W;

	DdNode *einit, *sinit, *etrans, *strans, **egoals, **sgoals;
	DdNode *X = NULL, *X_prev = NULL;
	
	DdNode *ddval;  /* Store result of evaluating a BDD */
	DdNode ***Y = NULL, *Y_exmod;
	DdNode *Y_i_primed;
	int *num_level_sets;

	DdNode *tmp, *tmp2;
	int i, j, k;  /* Generic counters */
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

	if (W == NULL)
		return 0;  /* not realizable */

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
		perror( "synthesize, malloc" );
		return NULL;
	}

	/* Allocate cube array, used later for quantifying over variables. */
	cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
	if (cube == NULL) {
		perror( "synthesize, malloc" );
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
		Y = malloc( num_sgoals*sizeof(DdNode **) );
		num_level_sets = malloc( num_sgoals*sizeof(int) );
		for (i = 0; i < num_sgoals; i++) {
			*(sgoals+i) = ptree_BDD( *(sys_goals+i), evar_list, manager );
			*(num_level_sets+i) = 1;
			*(Y+i) = malloc( *(num_level_sets+i)*sizeof(DdNode *) );
			if (*(Y+i) == NULL) {
				perror( "synthesize, malloc" );
				return NULL;
			}
			**(Y+i) = Cudd_bddAnd( manager, *(sgoals+i), W );
			Cudd_Ref( **(Y+i) );
		}
	} else {
		sgoals = NULL;
	}

	/* Make primed form of W and take conjunction with system
	   transition (safety) formula, for use while stepping down Y_i
	   sets.  Note that we assume the variable map has been
	   appropriately defined in the CUDD manager, after the call to
	   compute_winning_set above. */
	tmp = Cudd_bddVarMap( manager, W );
	if (tmp == NULL) {
		fprintf( stderr, "Error synthesize: Error in swapping variables with primed forms.\n" );
		return NULL;
	}
	Cudd_Ref( tmp );
	strans_into_W = Cudd_bddAnd( manager, strans, tmp );
	Cudd_Ref( strans_into_W );
	Cudd_RecursiveDeref( manager, tmp );
	
	/* Break the link that appended the system variables list to the
	   environment variables list. */
	if (var_separator == NULL) {
			evar_list = NULL;
	} else {
		var_separator->left = NULL;
	}

	command = INTCOM_RELEVELS;  /* Initialization, force sublevel computation */
	do {
		if (evar_list == NULL) {
			var_separator = NULL;
			evar_list = svar_list;
		} else {
			var_separator = get_list_item( evar_list, -1 );
			if (var_separator == NULL) {
				fprintf( stderr, "Error: get_list_item failed on environment variables list.\n" );
				return NULL;
			}
			var_separator->left = svar_list;
		}

		if (command == INTCOM_WINNING) {

			/* if (W != NULL) */
			/* 	Cudd_RecursiveDeref( manager, W ); */
			/* W = compute_winning_set_BDD( manager, */

		} else if (command == INTCOM_ENVNEXT) {

		}
		
		if (var_separator == NULL) {
			evar_list = NULL;
		} else {
			var_separator->left = NULL;
		}
	} while (command = command_loop( manager, infp, outfp ));

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
		for (j = 0; j < *(num_level_sets+i); j++)
			Cudd_RecursiveDeref( manager, *(*(Y+i)+j) );
		if (*(num_level_sets+i) > 0)
			free( *(Y+i) );
	}
	if (num_sgoals > 0) {
		free( Y );
		free( num_level_sets );
	}

	return 1;
}
