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
#define INTCOM_CLEAR 6
#define INTCOM_UNREACH 8
#define INTCOM_GETINDEX 9
#define INTCOM_SETINDEX 10
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
	"unreach STATE\n" \
	"getindex STATE GOALMODE\n" \
	"setindex STATE GOALMODE\n" \
	"refresh winning\n" \
	"refresh levels\n" \
	"addvar env (sys) VARIABLE\n" \
	"envvar\n" \
	"sysvar\n" \
	"var\n" \
	"numgoals\n" \
	"printgoal GOALMODE\n" \
	"enable (disable) autoreorder\n" \
	"quit"

/**** Command arguments ****/
/* In the case of pointers, it is expected that command_loop will
   allocate the memory, and levelset_interactive (or otherwise the
   function that invoked command_loop) will free it. */
bool *intcom_state;
int intcom_index;  /* May be used to pass strategy goal mode or length
					  of intcom_state. */


/* Read space-separated values from given string. Allocate space for
   the resulting array and store a pointer to it in argument
   "state". Read at most max_len values, and return the number of
   values read (thus, the length of the array pointed to by state at
   exit). */
int read_state_str( char *input, bool **state, int max_len )
{
	int i;
	char *start;
	char *end;
	if (max_len < 1 || strlen( input ) < 1) {
		*state = NULL;
		return 0;
	}
	*state = malloc( sizeof(bool)*max_len );
	if (*state == NULL) {
		perror( "read_state_str, malloc" );
		return 0;
	}

	start = input;
	end = input;
	for (i = 0; i < max_len && *end != '\0'; i++) {
		*((*state)+i) = strtol( start, &end, 10 );
		if (start == end)
			break;
		start = end;
	}
	
	if (i == 0) {
		free( *state );
		*state = NULL;
		return 0;
	}

	*state = realloc( *state, sizeof(bool)*i );
	if (*state == NULL) {
		perror( "read_state_str, realloc" );
		return 0;
	}
	return i;
}

int command_loop( DdManager *manager, FILE *infp, FILE *outfp )
{
	int num_env, num_sys;

	ptree_t *tmppt;
	int var_index;
	char *input;
	int num_read;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	while (input = readline( "" )) {
		if (*input == '\0') {
			free( input );
			continue;
		}
		if (!strncmp( input, "quit", strlen( "quit" ) )) {
			break;
		} else if (!strncmp( input, "help", strlen( "help" ) )) {
			fprintf( outfp, INTCOM_HELP_STR );
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
		} else if (!strncmp( input, "refresh winning", strlen( "refresh winning" ) )) {
			return INTCOM_REWIN;
		} else if (!strncmp( input, "refresh levels", strlen( "refresh levels" ) )) {
			return INTCOM_RELEVELS;
		} else if (!strncmp( input, "printgoal ", strlen( "printgoal " ) )) {

			*(input+strlen( "printgoal" )) = '\0';
			num_read = read_state_str( input+strlen( "printgoal" )+1, &intcom_state, 1 );
			if (num_read < 1) {
				fprintf( outfp, "Invalid arguments." );
			} else {
				if (*intcom_state < 0 || *intcom_state > num_sgoals-1) {
					fprintf( outfp, "Invalid mode: %d", *intcom_state );
				} else {
					print_formula( *(sys_goals+*intcom_state), stdout );
				}
				free( intcom_state );
			}
			
		} else if (!strncmp( input, "winning ", strlen( "winning " ) )) {

			*(input+strlen( "winning" )) = '\0';
			num_read = read_state_str( input+strlen( "winning" )+1, &intcom_state,
									   num_env+num_sys );
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
			num_read = read_state_str( input+strlen( "envnext" )+1, &intcom_state,
									   num_env+num_sys );
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
			num_read = read_state_str( input+strlen( "sysnext" )+1, &intcom_state,
									   2*num_env+num_sys+1 );
			if (num_read != 2*num_env+num_sys+1) {
				if (num_read > 0)
					free( intcom_state );
				free( input );
				fprintf( outfp, "Invalid arguments.\n" );
				continue;
			}
			if (*(intcom_state+2*num_env+num_sys) < 0 || *(intcom_state+2*num_env+num_sys) > num_sgoals-1) {
				fprintf( outfp, "Invalid mode: %d", *(intcom_state+2*num_env+num_sys) );
				free( intcom_state );
			} else {
				free( input );
				intcom_index  = *(intcom_state+2*num_env+num_sys);
				intcom_state = realloc( intcom_state, (2*num_env+num_sys)*sizeof(bool) );
				if (intcom_state == NULL) {
					perror( "command_loop, realloc" );
					return -1;
				}
				return INTCOM_SYSNEXT;
			}
			
		} else if (!strncmp( input, "sysnexta ", strlen( "sysnexta " ) )) {

			*(input+strlen( "sysnexta" )) = '\0';
			num_read = read_state_str( input+strlen( "sysnexta" )+1, &intcom_state,
									   2*num_env+num_sys );
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
			num_read = read_state_str( input+strlen( "restrict" )+1, &intcom_state,
									   2*(num_env+num_sys) );
			if ((num_read == 2*(num_env+num_sys)) || (num_read == 2*num_env+num_sys)) {
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
			if ((num_read == 2*(num_env+num_sys)) || (num_read == 2*num_env+num_sys)) {
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
		} else if (!strncmp( input, "unreach ", strlen( "unreach " ) )) {

			*(input+strlen( "unreach" )) = '\0';
			num_read = read_state_str( input+strlen( "unreach" )+1, &intcom_state,
									   num_env+num_sys );
			if ((num_read == num_env+num_sys) || (num_read == num_env)) {
				free( input );
				return INTCOM_UNREACH;
			} else {
				if (num_read > 0)
					free( intcom_state );
				fprintf( outfp, "Invalid arguments." );
			}
			
		} else if (!strncmp( input, "getindex ", strlen( "getindex " ) )) {

			*(input+strlen( "getindex" )) = '\0';
			num_read = read_state_str( input+strlen( "getindex" )+1, &intcom_state,
									   num_env+num_sys+1 );
			if (num_read != num_env+num_sys+1) {
				if (num_read > 0)
					free( intcom_state );
				free( input );
				fprintf( outfp, "Invalid arguments.\n" );
				continue;
			}
			if (*(intcom_state+num_env+num_sys) < 0 || *(intcom_state+num_env+num_sys) > num_sgoals-1) {
				fprintf( outfp, "Invalid mode: %d", *(intcom_state+num_env+num_sys) );
				free( intcom_state );
			} else {
				free( input );
				intcom_index = *(intcom_state+num_env+num_sys);
				intcom_state = realloc( intcom_state, (num_env+num_sys)*sizeof(bool) );
				if (intcom_state == NULL) {
					perror( "command_loop, realloc" );
					return -1;
				}
				return INTCOM_GETINDEX;
			}
			
		} else if (!strncmp( input, "setindex ", strlen( "setindex " ) )) {

			*(input+strlen( "setindex" )) = '\0';
			num_read = read_state_str( input+strlen( "setindex" )+1, &intcom_state,
									   num_env+num_sys+1 );
			if (num_read != num_env+num_sys+1) {
				if (num_read > 0)
					free( intcom_state );
				free( input );
				fprintf( outfp, "Invalid arguments.\n" );
				continue;
			}
			if (*(intcom_state+num_env+num_sys) < 0 || *(intcom_state+num_env+num_sys) > num_sgoals-1) {
				fprintf( outfp, "Invalid mode: %d", *(intcom_state+num_env+num_sys) );
				free( intcom_state );
			} else {
				free( input );
				intcom_index = *(intcom_state+num_env+num_sys);
				intcom_state = realloc( intcom_state, (num_env+num_sys)*sizeof(bool) );
				if (intcom_state == NULL) {
					perror( "command_loop, realloc" );
					return -1;
				}
				return INTCOM_SETINDEX;
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
	bool *state;
	bool **env_moves;
	int emoves_len;

	ptree_t *var_separator;
	DdNode *W;
	DdNode *strans_into_W;

	DdNode *einit, *sinit, *etrans, *strans, **egoals, **sgoals;

	DdNode *etrans_patched, *strans_patched;
	DdNode *vertex1, *vertex2; /* ...regarding vertices of the game graph. */
	
	DdNode *ddval;  /* Store result of evaluating a BDD */
	DdNode ***Y = NULL;
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
		return -1;
	}

	/* Allocate cube array, used later for quantifying over variables. */
	cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
	if (cube == NULL) {
		perror( "levelset_interactive, malloc" );
		return -1;
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
			return -1;
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

	etrans_patched = etrans;
	Cudd_Ref( etrans_patched );
	strans_patched = strans;
	Cudd_Ref( strans_patched );

	W = compute_winning_set_BDD( manager, etrans, strans, egoals, sgoals, verbose );
	if (W == NULL) {
		fprintf( stderr, "Error levelset_interactive: failed to construct winning set.\n" );
		return -1;
	}
	W = check_realizable_internal( manager, W, init_flags, verbose );
	if (W == NULL)
		return 0;  /* not realizable */

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
				fprintf( stderr, "Error levelset_interactive: failed to construct winning set.\n" );
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
			Y = compute_sublevel_sets( manager, W,
									   etrans_patched, strans_patched,
									   egoals, sgoals,
									   &num_sublevels, verbose );
			if (Y == NULL) {
				fprintf( stderr, "Error levelset_interactive: failed to construct sublevel sets.\n" );
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
				for (j = 0; j < num_env; j++)
					fprintf( outfp, "%d ", *(*(env_moves+i)+j) );
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
				fprintf( stderr, "Error levelset_interactive: Error in swapping variables with primed forms.\n" );
				return -1;
			}
			Cudd_Ref( tmp );
			strans_into_W = Cudd_bddAnd( manager, strans_patched, tmp );
			Cudd_Ref( strans_into_W );
			Cudd_RecursiveDeref( manager, tmp );

			state2cube( intcom_state, cube, num_env+num_sys );
			j = *(num_sublevels+intcom_index);
			do {
				j--;
				ddval = Cudd_Eval( manager, *(*(Y+intcom_index)+j), cube );
				if (ddval->type.value < .1) {
					j++;
					break;
				}
			} while (j > 0);
			if (j == 0) {
				Y_i_primed = Cudd_bddVarMap( manager, *(*(Y+intcom_index)) );
			} else {
				Y_i_primed = Cudd_bddVarMap( manager, *(*(Y+intcom_index)+j-1) );
			}
			if (Y_i_primed == NULL) {
				fprintf( stderr, "levelset_interactive: Error in swapping variables with primed forms.\n" );
				return -1;
			}
			Cudd_Ref( Y_i_primed );

			tmp = Cudd_bddAnd( manager, strans_into_W, Y_i_primed );
			Cudd_Ref( tmp );
			tmp2 = state2cof( manager, cube, 2*(num_env+num_sys),
							  intcom_state, tmp, 0, num_env+num_sys );
			Cudd_RecursiveDeref( manager, tmp );
			if (num_env > 0) {
				tmp = state2cof( manager, cube, 2*(num_sys+num_env),
								 intcom_state+num_env+num_sys,
								 tmp2, num_env+num_sys, num_env );
				Cudd_RecursiveDeref( manager, tmp2 );
			} else {
				tmp = tmp2;
			}
			if (verbose) {
				printf( "From state " );
				for (i = 0; i < num_env+num_sys; i++)
					printf( " %d", *(intcom_state+i) );
				printf( "; given env move " );
				for (i = 0; i < num_env; i++)
					printf( " %d", *(intcom_state+num_env+num_sys+i) );
				printf( "; goal %d\n", intcom_index );
			}

			Cudd_AutodynDisable( manager );
			/* Mark first element to detect whether any cubes were generated. */
			*state = -1;
			Cudd_ForeachCube( manager, tmp, gen, gcube, gvalue ) {
				initialize_cube( state, gcube+2*num_env+num_sys, num_sys );
				while (!saturated_cube( state, gcube+2*num_env+num_sys, num_sys )) {
					for (i = 0; i < num_sys; i++)
						fprintf( outfp, "%d ", *(state+i) );
					fprintf( outfp, "\n" );
					increment_cube( state, gcube+2*num_env+num_sys, num_sys );
				}
				for (i = 0; i < num_sys; i++)
					fprintf( outfp, "%d ", *(state+i) );
				fprintf( outfp, "\n" );
			}
			Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
			if (*state == -1) {
				/* Cannot step closer to system goal, so must be in
				   goal state or able to block environment goal. */
				Cudd_RecursiveDeref( manager, tmp );
				Cudd_RecursiveDeref( manager, Y_i_primed );
				if (j > 0) {
					Y_i_primed = Cudd_bddVarMap( manager, *(*(Y+intcom_index)+j) );
					if (Y_i_primed == NULL) {
						fprintf( stderr, "levelset_interactive: Error in swapping variables with primed forms.\n" );
						return -1;
					}
				} else {
					Y_i_primed = Cudd_ReadOne( manager );
				}
				Cudd_Ref( Y_i_primed );
				tmp = Cudd_bddAnd( manager, strans_into_W, Y_i_primed );
				Cudd_Ref( tmp );
				tmp2 = state2cof( manager, cube, 2*(num_env+num_sys),
								  intcom_state, tmp, 0, num_env+num_sys );
				Cudd_RecursiveDeref( manager, tmp );
				if (num_env > 0) {
					tmp = state2cof( manager, cube, 2*(num_sys+num_env),
									 intcom_state+num_env+num_sys,
									 tmp2, num_env+num_sys, num_env );
					Cudd_RecursiveDeref( manager, tmp2 );
				} else {
					tmp = tmp2;
				}

				Cudd_AutodynDisable( manager );
				Cudd_ForeachCube( manager, tmp, gen, gcube, gvalue ) {
					initialize_cube( state, gcube+2*num_env+num_sys, num_sys );
					while (!saturated_cube( state, gcube+2*num_env+num_sys, num_sys )) {
						for (i = 0; i < num_sys; i++)
							fprintf( outfp, "%d ", *(state+i) );
						fprintf( outfp, "\n" );
						increment_cube( state, gcube+2*num_env+num_sys, num_sys );
					}
					for (i = 0; i < num_sys; i++)
						fprintf( outfp, "%d ", *(state+i) );
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
			tmp = state2cof( manager, cube, 2*(num_env+num_sys),
							 intcom_state, strans_patched, 0, num_env+num_sys );
			if (num_env > 0) {
				tmp2 = state2cof( manager, cube, 2*(num_sys+num_env),
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
				while (!saturated_cube( state, gcube+2*num_env+num_sys, num_sys )) {
					for (i = 0; i < num_sys; i++)
						fprintf( outfp, "%d ", *(state+i) );
					fprintf( outfp, "\n" );
					increment_cube( state, gcube+2*num_env+num_sys, num_sys );
				}
				for (i = 0; i < num_sys; i++)
					fprintf( outfp, "%d ", *(state+i) );
				fprintf( outfp, "\n" );
			}
			Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
			fprintf( outfp, "---\n" );

			Cudd_RecursiveDeref( manager, tmp2 );
			break;

		case INTCOM_WINNING:
			if (verbose) {
				printf( "Winning set membership check for state " );
				for (i = 0; i < num_env+num_sys; i++)
					printf( " %d", *(intcom_state+i) );
				printf( "\n" );
			}
			state2cube( intcom_state, cube, num_env+num_sys );
			free( intcom_state );
			ddval = Cudd_Eval( manager, W, cube );
			if (ddval->type.value < .1) {
				fprintf( outfp, "False\n" );
			} else {
				fprintf( outfp, "True\n" );
			}
			break;

		case INTCOM_GETINDEX:
			if (verbose) {
				printf( "Reachability index for goal %d of state ", intcom_index );
				for (i = 0; i < num_env+num_sys; i++)
					printf( " %d", *(intcom_state+i) );
				printf( "\n" );
			}
			state2cube( intcom_state, cube, num_env+num_sys );
			free( intcom_state );
			j = *(num_sublevels+intcom_index);
			do {
				j--;
				ddval = Cudd_Eval( manager, *(*(Y+intcom_index)+j), cube );
				if (ddval->type.value < .1) {
					j++;
					break;
				}
			} while (j > 0);
			fprintf( outfp, "%d\n", j );
			break;

		case INTCOM_RESTRICT:
		case INTCOM_RELAX:
			if (verbose) {
				if (intcom_index == 2*num_env+num_sys) {
					printf( "Removing uncontrolled edge from" );
				} else { /* intcom_index == 2*(num_env+num_sys) */
					printf( "Removing controlled edge from" );
				}
				for (i = 0; i < num_env+num_sys; i++)
					printf( " %d", *(intcom_state+i) );
				printf( " to " );
				for (i = num_env+num_sys; i < intcom_index; i++)
					printf( " %d", *(intcom_state+i) );
				printf( "\n" );
			}
			
			vertex1 = Cudd_ReadOne( manager );
			Cudd_Ref( vertex1 );
			for (i = 0; i < num_env+num_sys; i++) {
				if (*(intcom_state+i)) {
					tmp = Cudd_bddAnd( manager, vertex1,
									   Cudd_bddIthVar( manager, i ) );
				} else {
					tmp = Cudd_bddAnd( manager, vertex1,
									   Cudd_Not( Cudd_bddIthVar( manager, i ) ) );
				}
				Cudd_Ref( tmp );
				Cudd_RecursiveDeref( manager, vertex1 );
				vertex1 = tmp;
			}
			vertex2 = Cudd_ReadOne( manager );
			Cudd_Ref( vertex2 );
			for (i = num_env+num_sys; i < intcom_index; i++) {
				if (*(intcom_state+i)) {
					tmp = Cudd_bddAnd( manager, vertex2,
									   Cudd_bddIthVar( manager, i ) );
				} else {
					tmp = Cudd_bddAnd( manager, vertex2,
									   Cudd_Not( Cudd_bddIthVar( manager, i ) ) );
				}
				Cudd_Ref( tmp );
				Cudd_RecursiveDeref( manager, vertex2 );
				vertex2 = tmp;
			}
			tmp = Cudd_Not( vertex1 );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, vertex1 );
			vertex1 = tmp;
			if (command == INTCOM_RESTRICT) {
				tmp = Cudd_Not( vertex2 );
				Cudd_Ref( tmp );
				Cudd_RecursiveDeref( manager, vertex2 );
				vertex2 = tmp;
			}
			tmp = Cudd_bddOr( manager, vertex1, vertex2 );
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

		case INTCOM_UNREACH:
		case INTCOM_SETINDEX:
			free( intcom_state );
			fprintf( outfp, "(not implemented yet.)\n" );
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

	} while ((command = command_loop( manager, infp, outfp )) > 0);

	if (verbose) {
		printf( "== Cudd_PrintInfo(), called from levelset_interactive ================\n" );
		Cudd_PrintInfo( manager, stdout );
		printf( "======================================================================\n" );
	}

	/* Pre-exit clean-up */
	Cudd_RecursiveDeref( manager, etrans_patched );
	Cudd_RecursiveDeref( manager, strans_patched );
	Cudd_RecursiveDeref( manager, W );
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
