/* patching.c -- Definitions for signatures appearing in patching.h.
 *
 *
 * SCL; Apr 2012.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cudd.h"

#include "common.h"
#include "patching.h"


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

/* See interactive.c for definition and documentation. */
extern int read_state_str( char *input, bool **state, int max_len );
extern DdNode *state2BDD( DdManager *manager, bool *state, int offset, int len );

/* See solve_operators.c for definition. */
extern DdNode *compute_existsmodal( DdManager *manager, DdNode *C,
									DdNode *etrans, DdNode *strans,
									int num_env, int num_sys, int *cube );

/* See solve.c for documentation. */
extern void increment_cube( bool *cube, int *gcube, int len );
extern bool saturated_cube( bool *cube, int *gcube, int len );
extern void initialize_cube( bool *cube, int *gcube, int len );
extern void state2cube( bool *state, int *cube, int len );
extern DdNode *state2cof( DdManager *manager, int *cube, int cube_len,
						  bool *state, DdNode *trans, int offset, int len );
extern bool **get_env_moves( DdManager *manager, int *cube,
							 bool *state, DdNode *etrans,
							 int num_env, int num_sys, int *emoves_len );


anode_t *find_anode_ID( anode_t *head, int ID )
{
	int i;
	if (head == NULL || i < 0)
		return NULL;
	for (i = 0; i < ID && head != NULL; i++)
		head = head->next;
	return head;
}

bool statecmp( bool *state1, bool *state2, int state_len )
{
	int i;
	for (i = 0; i < state_len; i++) {
		if (*(state1+i) != *(state2+i))
			return False;
	}
	return True;
}


#define INPUT_STRING_LEN 256

anode_t *patch_localfixpoint( DdManager *manager, FILE *strategy_fp, FILE *change_fp, unsigned char verbose )
{
	ptree_t *var_separator;
	DdNode *einit, *sinit, *etrans, *strans, **egoals, **sgoals;
	int num_env, num_sys;

	DdNode *vertex1, *vertex2; /* ...regarding vertices of the game graph. */
	char line[INPUT_STRING_LEN];
	bool *state;

	bool env_nogoal_flag = False;  /* Indicate environment has no goals */

	int i, j, k;  /* Generic counters */
	DdNode *tmp, *tmp2;
	int num_read;
	char *start, *end;
	anode_t *strategy, *local_strategy;
	anode_t *this_node_stack;
	anode_t *node, *new_node, *head;
	int node_counter;
	int *N = NULL;  /* "neighborhood" of nodes (a list of node IDs). */
	int N_len = 0;
	int N_mode = -1;  /* Goal mode of the automaton nodes in the neighborhood */
	int min_rindex;  /* Minimum reachability index of affected nodes. */

	DdNode *local_target;  /* Called B in the manuscript */
	DdNode *entry_states;  /* Called C in the manuscript */

	anode_t **affected = NULL;  /* Array of pointers to nodes directly
								   affected by edge set change.
								   Called U_i in the manuscript. */
	int affected_len = 0;

	DdNode **Y, *Y_exmod;
	int num_sublevels;
	DdNode *X = NULL, *X_prev = NULL;
	DdNode **vars, **pvars;
	int *cube;
	DdNode *Y_i_primed;
	DdNode *strans_into_W;
	DdNode *ddval;
	bool **env_moves;
	int emoves_len;

	/* Variables used during CUDD generation (state enumeration). */
	DdGen *gen;
	CUDD_VALUE_TYPE gvalue;
	int *gcube;

	if (change_fp == NULL)
		return NULL;  /* Require game changes to be listed in an open stream. */

	if (strategy_fp == NULL)
		strategy_fp = stdin;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	strategy = aut_aut_load( num_env+num_sys, strategy_fp );
	if (strategy == NULL) {
		return NULL;
	}
	
	/* Set environment goal to True (i.e., any state) if none was
	   given. This simplifies the implementation below. */
	if (num_egoals == 0) {
		env_nogoal_flag = True;
		num_egoals = 1;
		env_goals = malloc( sizeof(ptree_t *) );
		*env_goals = init_ptree( PT_CONSTANT, NULL, 1 );
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

	/* Process game graph changes */
	N_len = 1;
	N = malloc( sizeof(int)*N_len);
	if (N == NULL) {
		perror( "patch_localfixpoint, malloc" );
		return NULL;
	}
	while (fgets( line, INPUT_STRING_LEN, change_fp )) {
		if (strlen( line ) < 1 || *line == '\n')  /* Blank line? */
			continue;

		*(N+N_len-1) = strtol( line, &end, 10 );
		if (end != line) {
			do {
				N_len++;
				N = realloc( N, sizeof(int)*N_len );
				if (N == NULL) {
					perror( "patch_localfixpoint, realloc" );
					return NULL;
				}
				start = end;
				*(N+N_len-1) = strtol( start, &end, 10 );
			} while (end != start && *end != '\0');

			node = find_anode_ID( strategy, *N );
			if (node == NULL) {
				fprintf( stderr, "Error: invalid node ID in neighborhood.\n" );
				return NULL;
			}
			N_mode = node->mode;

			if (verbose) {
				printf( "Neighborhood goal mode of %d with node IDs:", N_mode );
				for (i = 0; i < N_len-1; i++)
					printf( " %d", *(N+i) );
				printf( "\n" );
			}
		} else if (!strncmp( line, "restrict ", strlen( "restrict " ) ) || !strncmp( line, "relax ", strlen( "relax " ) )) {
			if (N_mode < 0) {
				fprintf( stderr, "Error: command encountered before goal mode set.\n" );
				return NULL;
			}

			if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
				num_read = read_state_str( line+strlen( "restrict" )+1, &state, 2*(num_env+num_sys) );
			} else { /* "relax " */
				num_read = read_state_str( line+strlen( "relax" )+1, &state, 2*(num_env+num_sys) );
			}
			if (num_read != 2*(num_env+num_sys) && num_read != 2*num_env+num_sys) {
				if (num_read > 0)
					free( state );
				fprintf( stderr, "Error: invalid arguments to restrict or relax command.\n" );
				return NULL;
			}
			if (verbose) {
				if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
					if (num_read == 2*(num_env+num_sys)) {
						printf( "Removing controlled edge from" );
					} else {
						printf( "Removing uncontrolled edge from" );
					}
				} else { /* "relax " */
					if (num_read == 2*(num_env+num_sys)) {
						printf( "Adding controlled edge from" );
					} else {
						printf( "Adding uncontrolled edge from" );
					}
				}
				for (i = 0; i < num_env+num_sys; i++)
					printf( " %d", *(state+i) );
				printf( " to" );
				for (i = num_env+num_sys; i < num_read; i++)
					printf( " %d", *(state+i) );
				printf( "\n" );
			}

			/* Find nodes in strategy that are affected by this change */
			node = find_anode( strategy, N_mode, state, num_env+num_sys );
			if (node != NULL) {
				if (!strncmp( line, "restrict ", strlen( "restrict " ) )
					&& (num_read == 2*(num_env+num_sys))) {
					for (i = 0; i < node->trans_len; i++) {
						if (statecmp( (*(node->trans+i))->state,
									  state+num_env+num_sys, num_env+num_sys )) {
							affected_len++;
							affected = realloc( affected, sizeof(anode_t *)*affected_len );
							if (affected == NULL) {
								perror( "patch_localfixpoint, realloc" );
								return NULL;
							}
							*(affected+affected_len-1) = node;
							if (verbose) {
								printf( "New affected node ID: %d\n",
										find_anode_index( strategy, N_mode, state, num_env+num_sys ) );
							}
							break;
						}
					}
				} else if (!strncmp( line, "relax ", strlen( "relax " ) )
						   && (num_read == 2*num_env+num_sys)) {
					for (i = 0; i < node->trans_len; i++) {
						if (statecmp( (*(node->trans+i))->state,
									  state+num_env+num_sys, num_env ))
							break;
					}
					if (i == node->trans_len) {
						affected_len++;
						affected = realloc( affected, sizeof(anode_t *)*affected_len );
						if (affected == NULL) {
							perror( "patch_localfixpoint, realloc" );
							return NULL;
						}
						*(affected+affected_len-1) = node;
						if (verbose) {
							printf( "New affected node ID: %d\n",
									find_anode_index( strategy, N_mode, state, num_env+num_sys ) );
						}
					}
				}
			}

			vertex1 = state2BDD( manager, state, 0, num_env+num_sys );
			vertex2 = state2BDD( manager, state+num_env+num_sys, num_env+num_sys, num_read-(num_env+num_sys) );
			tmp = Cudd_Not( vertex1 );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, vertex1 );
			vertex1 = tmp;
			if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
				tmp = Cudd_Not( vertex2 );
				Cudd_Ref( tmp );
				Cudd_RecursiveDeref( manager, vertex2 );
				vertex2 = tmp;
			}
			tmp = Cudd_bddOr( manager, vertex1, vertex2 );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, vertex1 );
			Cudd_RecursiveDeref( manager, vertex2 );
			if (num_read == 2*num_env+num_sys) {
				if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
					tmp2 = Cudd_bddAnd( manager, tmp, etrans );
				} else { /* "relax " */
					tmp2 = Cudd_bddOr( manager, tmp, etrans );
				}
				Cudd_Ref( tmp2 );
				Cudd_RecursiveDeref( manager, etrans );
				etrans = tmp2;
			} else { /* num_read == 2*(num_env+num_sys) */
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
			if (N_mode < 0) {
				fprintf( stderr, "Error: command encountered before goal mode set.\n" );
				return NULL;
			}

			num_read = read_state_str( line+strlen( "blocksys" )+1, &state, num_sys );
			if (num_read != num_sys) {
				if (num_read > 0)
					free( state );
				fprintf( stderr, "Error: invalid arguments to blocksys command.\n%d\n%s\n", num_read, line );
				return NULL;
			}
			if (verbose) {
				printf( "Removing system moves into" );
				for (i = 0; i < num_sys; i++)
					printf( " %d", *(state+i) );
				printf( "\n" );
			}

			/* Find nodes in strategy that are affected by this change */
			head = strategy;
			node_counter = 0;
			while (head) {
				if (head->mode == N_mode) {
					for (i = 0; i < head->trans_len; i++) {
						if (statecmp( state, (*(head->trans+i))->state+num_env, num_sys )) {
							affected_len++;
							affected = realloc( affected, sizeof(anode_t *)*affected_len );
							if (affected == NULL) {
								perror( "patch_localfixpoint, realloc" );
								return NULL;
							}
							*(affected+affected_len-1) = head;
							if (verbose) {
								printf( "New affected node ID: %d\n", node_counter );
							}
							break;
						}
					}
				}
				head = head->next;
				node_counter++;
			}

			vertex2 = state2BDD( manager, state, 2*num_env+num_sys, num_sys );
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

		} else if (!strncmp( line, "blockenv ", strlen( "blockenv " ) )) {
			if (N_mode < 0) {
				fprintf( stderr, "Error: command encountered before goal mode set.\n" );
				return NULL;
			}

			num_read = read_state_str( line+strlen( "blockenv" )+1, &state, num_env );
			if (num_read != num_env) {
				if (num_read > 0)
					free( state );
				fprintf( stderr, "Error: invalid arguments to blockenv command.\n" );
				return NULL;
			}
			if (verbose) {
				printf( "Removing environment moves into" );
				for (i = 0; i < num_env; i++)
					printf( " %d", *(state+i) );
				printf( "\n" );
			}

			/* Find nodes in strategy that are affected by this change */
			head = strategy;
			node_counter = 0;
			while (head) {
				if (head->mode == N_mode) {
					for (i = 0; i < head->trans_len; i++) {
						if (statecmp( state, (*(head->trans+i))->state, num_env )) {
							affected_len++;
							affected = realloc( affected, sizeof(anode_t *)*affected_len );
							if (affected == NULL) {
								perror( "patch_localfixpoint, realloc" );
								return NULL;
							}
							*(affected+affected_len-1) = head;
							if (verbose) {
								printf( "New affected node ID: %d\n", node_counter );
							}
							break;
						}
					}
				}
				head = head->next;
				node_counter++;
			}

			vertex2 = state2BDD( manager, state, num_env+num_sys, num_env );
			tmp = Cudd_Not( vertex2 );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, vertex2 );
			vertex2 = tmp;

			tmp = Cudd_bddAnd( manager, etrans, vertex2 );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, etrans );
			etrans = tmp;

			Cudd_RecursiveDeref( manager, vertex2 );
			free( state );

		} else {
			fprintf( stderr, "WARNING: unrecognized line in given edge change file.\n" );
			continue;
		}

		if (verbose)
			fflush( stdout );
	}
	N_len--;  /* Remove unused tail */
	N = realloc( N, sizeof(int)*N_len );
	if (N == NULL) {
		perror( "patch_localfixpoint, realloc" );
		return NULL;
	}

	min_rindex = -1;
	for (i = 0; i < affected_len; i++) {
		if (min_rindex == -1 || (*(affected+i))->rindex < min_rindex)
			min_rindex = (*(affected+i))->rindex;
	}

	/* Create set to be reached by the local strategy and set of
	   initial states (all of which must be feasible). */
	local_target = Cudd_Not( Cudd_ReadOne( manager ) );
	Cudd_Ref( local_target );
	entry_states = Cudd_Not( Cudd_ReadOne( manager ) );
	Cudd_Ref( entry_states );
	for (i = 0; i < N_len; i++) {
		node = find_anode_ID( strategy, *(N+i) );
		if (node == NULL) {
			fprintf( stderr, "Error: node ID %d in neighborhood not found in given strategy.\n", *(N+i) );
			return NULL;
		}
		if (node->rindex >= min_rindex) {
			head = strategy;
			node_counter = 0;
			while (head) {
				for (j = 0; j < N_len && node_counter != *(N+j); j++) ;
				if (j == N_len) {
					for (j = 0; j < head->trans_len; j++) {
						if (node == *(head->trans+j)) {
							vertex1 = state2BDD( manager, node->state, 0, num_env+num_sys );
							tmp = Cudd_bddOr( manager, entry_states, vertex1 );
							Cudd_Ref( tmp );
							Cudd_RecursiveDeref( manager, vertex1 );
							Cudd_RecursiveDeref( manager, entry_states );
							entry_states = tmp;
							break;
						}
					}
					if (j < head->trans_len && node == *(head->trans+j))
						break;
				}
				head = head->next;
				node_counter++;
			}
		} else {
			vertex1 = state2BDD( manager, node->state, 0, num_env+num_sys );
			tmp = Cudd_bddOr( manager, local_target, vertex1 );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, vertex1 );
			Cudd_RecursiveDeref( manager, local_target );
			local_target = tmp;
		}
	}

	cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
	if (cube == NULL) {
		perror( "patch_localfixpoint, malloc" );
		return NULL;
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
		fprintf( stderr, "Error: failed to define variable map in CUDD manager.\n" );
		return NULL;
	}
	free( vars );
	free( pvars );

	num_sublevels = 1;
	Y = malloc( sizeof(DdNode *)*num_sublevels );
	if (Y == NULL) {
		perror( "patch_localfixpoint, malloc" );
		return NULL;
	}
	*Y = local_target;
	Cudd_Ref( *Y );

	do {
		num_sublevels++;
		Y = realloc( Y, sizeof(DdNode *)*num_sublevels );
		if (Y == NULL) {
			perror( "patch_localfixpoint, realloc" );
			return NULL;
		}

		Y_exmod = compute_existsmodal( manager, *(Y+num_sublevels-2),
									   etrans, strans,
									   num_env, num_sys, cube );
		*(Y+num_sublevels-1) = Cudd_Not( Cudd_ReadOne( manager ) );
		Cudd_Ref( *(Y+num_sublevels-1) );
		for (j = 0; j < num_egoals; j++) {

			/* (Re)initialize X */
			if (X != NULL)
				Cudd_RecursiveDeref( manager, X );
			X = Cudd_ReadOne( manager );
			Cudd_Ref( X );

			/* Greatest fixpoint for X, for this env goal */
			do {
				if (X_prev != NULL)
					Cudd_RecursiveDeref( manager, X_prev );
				X_prev = X;
				X = compute_existsmodal( manager, X_prev, etrans, strans,
										 num_env, num_sys, cube );
				if (X == NULL) {
					/* fatal error */
					return NULL;
				}
								
				tmp2 = Cudd_bddOr( manager, local_target, Y_exmod );
				Cudd_Ref( tmp2 );

				tmp = Cudd_bddOr( manager, tmp2, Cudd_Not( *(egoals+j) ) );
				Cudd_Ref( tmp );
				Cudd_RecursiveDeref( manager, tmp2 );

				tmp2 = X;
				X = Cudd_bddAnd( manager, tmp, X );
				Cudd_Ref( X );
				Cudd_RecursiveDeref( manager, tmp );
				Cudd_RecursiveDeref( manager, tmp2 );

				tmp = X;
				X = Cudd_bddAnd( manager, X, X_prev );
				Cudd_Ref( X );
				Cudd_RecursiveDeref( manager, tmp );

			} while (!(Cudd_bddLeq( manager, X, X_prev )*Cudd_bddLeq( manager, X_prev, X )));
			tmp = *(Y+num_sublevels-1);
			*(Y+num_sublevels-1) = Cudd_bddOr( manager, *(Y+num_sublevels-1), X );
			Cudd_Ref( *(Y+num_sublevels-1) );
			Cudd_RecursiveDeref( manager, tmp );

			Cudd_RecursiveDeref( manager, X );
			X = NULL;
			Cudd_RecursiveDeref( manager, X_prev );
			X_prev = NULL;
		}

		tmp = *(Y+num_sublevels-1);
		*(Y+num_sublevels-1) = Cudd_bddOr( manager, *(Y+num_sublevels-1), *(Y+num_sublevels-2) );
		Cudd_Ref( *(Y+num_sublevels-1) );
		Cudd_RecursiveDeref( manager, tmp );

		/* Have all entry points been reached, or a fixed point? */
		if (Cudd_bddLeq( manager, entry_states, *(Y+num_sublevels-1) ))
			break;
	} while (!(Cudd_bddLeq( manager, *(Y+num_sublevels-1), *(Y+num_sublevels-2))*Cudd_bddLeq( manager, *(Y+num_sublevels-2), *(Y+num_sublevels-1))));

	if (Cudd_bddLeq( manager, entry_states, *(Y+num_sublevels-1) )) {
		/* Build local strategy and then patch original */
		local_strategy = NULL;

		state = malloc( sizeof(bool)*(num_env+num_sys) );
		if (state == NULL) {
			perror( "patch_localfixpoint, malloc" );
			return NULL;
		}

		tmp = Cudd_bddVarMap( manager, *(Y+num_sublevels-1) );
		if (tmp == NULL) {
			fprintf( stderr, "Error: could not swap local winning set variables to primed form.\n" );
			return NULL;
		}
		Cudd_Ref( tmp );
		strans_into_W = Cudd_bddAnd( manager, strans, tmp );
		Cudd_Ref( strans_into_W );
		Cudd_RecursiveDeref( manager, tmp );
		
		this_node_stack = NULL;
		/* An alternative to using cube generation by CUDD would be to
		   enumerate from the original strategy, as done above to
		   construct the BDD entry_states. */
		Cudd_AutodynDisable( manager );
		Cudd_ForeachCube( manager, entry_states, gen, gcube, gvalue ) {
			initialize_cube( state, gcube, num_env+num_sys );
			while (!saturated_cube( state, gcube, num_env+num_sys )) {
				this_node_stack = insert_anode( this_node_stack, N_mode, -1, state, num_env+num_sys );
				if (this_node_stack == NULL) {
					fprintf( stderr, "Error: failed to build local strategy initial nodes.\n" );
					return NULL;
				}
				increment_cube( state, gcube, num_env+num_sys );
			}
			this_node_stack = insert_anode( this_node_stack, N_mode, -1, state, num_env+num_sys );
			if (this_node_stack == NULL) {
				fprintf( stderr, "Error: could not enumerate initial nodes.\n" );
				return NULL;
			}
		}
		Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );

		/* Insert all stacked, initial nodes into strategy. */
		node = this_node_stack;
		while (node) {
			local_strategy = insert_anode( local_strategy, N_mode, node->rindex,
										   node->state, num_env+num_sys );
			if (local_strategy == NULL) {
				fprintf( stderr, "Error: failed to build local strategy initial nodes.\n" );
				return NULL;
			}
			node = node->next;
		}

		while (this_node_stack) {
			/* Find smallest Y_j set containing node. */
			for (k = num_env+num_sys; k < 2*(num_env+num_sys); k++)
				*(cube+k) = 2;
			state2cube( this_node_stack->state, cube, num_env+num_sys );
			j = num_sublevels;
			do {
				j--;
				ddval = Cudd_Eval( manager, *(Y+j), cube );
				if (ddval->type.value < .1) {
					j++;
					break;
				}
			} while (j > 0);

			node = find_anode( local_strategy, N_mode, this_node_stack->state, num_env+num_sys );
			if (node->trans_len > 0 || j == 0) {
				/* This state is a re-entry point, or is already in
				   local strategy. */
				this_node_stack = pop_anode( this_node_stack );
				continue;
			}
			
			this_node_stack = pop_anode( this_node_stack );
			Y_i_primed = Cudd_bddVarMap( manager, *(Y+j-1) );
			Cudd_Ref( Y_i_primed );
			
			if (num_env > 0) {
				env_moves = get_env_moves( manager, cube,
										   node->state, etrans,
										   num_env, num_sys,
										   &emoves_len );
			} else {
				emoves_len = 1;  /* This allows one iteration of the for-loop */
			}
			for (k = 0; k < emoves_len; k++) {
				tmp = Cudd_bddAnd( manager, strans_into_W, Y_i_primed );
				Cudd_Ref( tmp );
				tmp2 = state2cof( manager, cube, 2*(num_env+num_sys),
								  node->state,
								  tmp, 0, num_env+num_sys );
				Cudd_RecursiveDeref( manager, tmp );
				if (num_env > 0) {
					tmp = state2cof( manager, cube, 2*(num_env+num_sys),
									 *(env_moves+k),
									 tmp2, num_env+num_sys, num_env );
					Cudd_RecursiveDeref( manager, tmp2 );
				} else {
					tmp = tmp2;
				}

				Cudd_AutodynDisable( manager );
				gen = Cudd_FirstCube( manager, tmp, &gcube, &gvalue );
				if (gen == NULL) {
					fprintf( stderr, "Error: failed to find cube.\n" );
					return NULL;
				}
				if (Cudd_IsGenEmpty( gen )) {
					/* Cannot step closer to local target set, so must
					   be able to block environment goal. */
					Cudd_GenFree( gen );
					Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
					Cudd_RecursiveDeref( manager, tmp );
					Cudd_RecursiveDeref( manager, Y_i_primed );
					if (j > 0) {
						Y_i_primed = Cudd_bddVarMap( manager, *(Y+j) );
						if (Y_i_primed == NULL) {
							fprintf( stderr, "Error: could not swap local target set variables to primed form.\n" );
							return NULL;
						}
					} else {
						Y_i_primed = Cudd_ReadOne( manager );
					}
					Cudd_Ref( Y_i_primed );
					tmp = Cudd_bddAnd( manager, strans_into_W, Y_i_primed );
					Cudd_Ref( tmp );
					tmp2 = state2cof( manager, cube, 2*(num_env+num_sys),
									  node->state,
									  tmp, 0, num_sys+num_env );
					Cudd_RecursiveDeref( manager, tmp );
					if (num_env > 0) {
						tmp = state2cof( manager, cube, 2*(num_env+num_sys),
										 *(env_moves+k),
										 tmp2, num_sys+num_env, num_env );
						Cudd_RecursiveDeref( manager, tmp2 );
					} else {
						tmp = tmp2;
					}

					Cudd_AutodynDisable( manager );
					gen = Cudd_FirstCube( manager, tmp, &gcube, &gvalue );
					if (gen == NULL) {
						fprintf( stderr, "Error: failed to find cube.\n" );
						return NULL;
					}
					if (Cudd_IsGenEmpty( gen )) {
						Cudd_GenFree( gen );
						fprintf( stderr, "Error: unexpected losing state.\n" );
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

				state2cube( state, cube, num_env+num_sys );

				new_node = find_anode( local_strategy, N_mode, state, num_env+num_sys );
				if (new_node == NULL) {
					local_strategy = insert_anode( local_strategy, N_mode, -1,
												   state, num_env+num_sys );
					if (local_strategy == NULL) {
						fprintf( stderr, "Error: inserting new node into local strategy.\n" );
						return NULL;
					}
					this_node_stack = insert_anode( this_node_stack, N_mode, -1,
													state, num_env+num_sys );
					if (this_node_stack == NULL) {
						fprintf( stderr, "Error: pushing node onto stack failed.\n" );
						return NULL;
					}
				}

				local_strategy = append_anode_trans( local_strategy,
													 N_mode, node->state,
													 num_env+num_sys,
													 N_mode, state );
				if (local_strategy == NULL) {
					fprintf( stderr, "Error: inserting new transition into strategy.\n" );
					return NULL;
				}
			}
			if (num_env > 0) {
				for (k = 0; k < emoves_len; k++)
					free( *(env_moves+k) );
				free( env_moves );
			} else {
				emoves_len = 0;
			}
			Cudd_RecursiveDeref( manager, Y_i_primed );
		}
		Cudd_RecursiveDeref( manager, strans_into_W );
		free( state );

		/* Connect local strategy to original */
		head = local_strategy;
		while (head) {
			node = find_anode( strategy, N_mode, head->state, num_env+num_sys );
			if (node != NULL) {
				if (head->trans_len == 0) {
					replace_anode_trans( local_strategy, head, node );
					if (head == local_strategy) {
						local_strategy = head->next;
						free( head->state );
						free( head );
						head = local_strategy;
					} else {
						node = local_strategy;
						while (node->next != head)
							node = node->next;
						node->next = head->next;
						free( head->state );
						free( head );
						head = node->next;
					}
				} else {
					replace_anode_trans( strategy, node, head );
					strategy = delete_anode( strategy, node );
					head = head->next;
				}
			} else {
				head = head->next;
			}
		}

		head = strategy;
		while (head->next)
			head = head->next;
		head->next = local_strategy;
		
	} else { /* Unrealizable */
		delete_aut( strategy );
		strategy = NULL;
	}

	/* Pre-exit clean-up */
	free( cube );
	Cudd_RecursiveDeref( manager, local_target );
	Cudd_RecursiveDeref( manager, entry_states );
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
	if (env_nogoal_flag) {
		num_egoals = 0;
		delete_tree( *env_goals );
		free( env_goals );
	}
	for (i = 0; i < num_sublevels; i++)
		Cudd_RecursiveDeref( manager, *(Y+i) );
	free( Y );
	free( N );
	free( affected );

	return strategy;
}
