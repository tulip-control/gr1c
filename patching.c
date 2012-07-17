/* patching.c -- Definitions for signatures appearing in patching.h.
 *
 *
 * SCL; 2012.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cudd.h"

#include "common.h"
#include "automaton.h"
#include "ptree.h"
#include "patching.h"
#include "solve_support.h"


extern ptree_t *evar_list;
extern ptree_t *svar_list;
extern ptree_t **env_goals;
extern int num_egoals;
extern int num_sgoals;
extern ptree_t **env_trans_array;
extern ptree_t **sys_trans_array;
extern int et_array_len;
extern int st_array_len;


anode_t *synthesize_patch( DdManager *manager, int num_env, int num_sys,
						   anode_t **Entry, int Entry_len,
						   anode_t **Exit, int Exit_len,
						   DdNode *etrans, DdNode *strans, DdNode **egoals,
						   DdNode *N_BDD,
						   unsigned char verbose )
{
	DdNode *Exit_BDD;
	DdNode *Entry_BDD;

	anode_t *strategy = NULL;
	anode_t *this_node_stack = NULL;
	anode_t *node, *new_node;
	bool *state;
	int *cube;
	bool **env_moves;
	int emoves_len;

	DdNode *strans_into_N;
	DdNode **Y = NULL, *Y_exmod;
	DdNode *X = NULL, *X_prev = NULL;
	DdNode *Y_i_primed;
	int num_sublevels;
	DdNode ***X_jr = NULL;

	DdNode *tmp, *tmp2;
	int i, j, r, k;  /* Generic counters */
	DdNode *ddval;  /* Store result of evaluating a BDD */
	
	/* Variables used during CUDD generation (state enumeration). */
	DdGen *gen;
	CUDD_VALUE_TYPE gvalue;
	int *gcube;

	state = malloc( sizeof(bool)*(num_env+num_sys) );
	if (state == NULL) {
		perror( "synthesize_patch, malloc" );
		return NULL;
	}
	cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
	if (cube == NULL) {
		perror( "synthesize_patch, malloc" );
		return NULL;
	}

	/* Build characteristic functions (as BDDs) for Entry and Exit sets. */
	Entry_BDD = Cudd_Not( Cudd_ReadOne( manager ) );
	Cudd_Ref( Entry_BDD );
	for (i = 0; i < Entry_len; i++) {
		ddval = state2BDD( manager, (*(Entry+i))->state, 0, num_env+num_sys );
		tmp = Cudd_bddOr( manager, Entry_BDD, ddval );
		Cudd_Ref( tmp );
		Cudd_RecursiveDeref( manager, Entry_BDD );
		Cudd_RecursiveDeref( manager, ddval );
		Entry_BDD = tmp;
	}
	ddval = NULL;
	
	Exit_BDD = Cudd_Not( Cudd_ReadOne( manager ) );
	Cudd_Ref( Exit_BDD );
	for (i = 0; i < Exit_len; i++) {
		ddval = state2BDD( manager, (*(Exit+i))->state, 0, num_env+num_sys );
		tmp = Cudd_bddOr( manager, Exit_BDD, ddval );
		Cudd_Ref( tmp );
		Cudd_RecursiveDeref( manager, Exit_BDD );
		Cudd_RecursiveDeref( manager, ddval );
		Exit_BDD = tmp;
	}
	ddval = NULL;
	
	num_sublevels = 1;
	Y = malloc( num_sublevels*sizeof(DdNode *) );
	if (Y == NULL) {
		perror( "synthesize_patch, malloc" );
		return NULL;
	}
	*Y = Exit_BDD;
	Cudd_Ref( *Y );

	X_jr = malloc( num_sublevels*sizeof(DdNode **) );
	if (X_jr == NULL) {
		perror( "synthesize_patch, malloc" );
		return NULL;
	}
	
	*X_jr = malloc( num_egoals*sizeof(DdNode *) );
	if (*X_jr == NULL) {
		perror( "synthesize_patch, malloc" );
		return NULL;
	}
	for (r = 0; r < num_egoals; r++) {
		*(*X_jr+r) = Cudd_Not( Cudd_ReadOne( manager ) );
		Cudd_Ref( *(*X_jr+r) );
	}

	while (True) {
		num_sublevels++;
		Y = realloc( Y, num_sublevels*sizeof(DdNode *) );
		X_jr = realloc( X_jr, num_sublevels*sizeof(DdNode **) );
		if (Y == NULL || X_jr == NULL) {
			perror( "synthesize_patch, realloc" );
			return NULL;
		}
		
		*(X_jr + num_sublevels-1) = malloc( num_egoals*sizeof(DdNode *) );
		if (*(X_jr + num_sublevels-1) == NULL) {
			perror( "synthesize_patch, malloc" );
			return NULL;
		}

		Y_exmod = compute_existsmodal( manager, *(Y+num_sublevels-2),
									   etrans, strans,
									   num_env, num_sys, cube );
		if (Y_exmod == NULL)
			return NULL;  /* Fatal error */
		tmp = Cudd_bddAnd( manager, Y_exmod, N_BDD );
		Cudd_Ref( tmp );
		Cudd_RecursiveDeref( manager, Y_exmod );
		Y_exmod = tmp;

		*(Y+num_sublevels-1) = Cudd_Not( Cudd_ReadOne( manager ) );
		Cudd_Ref( *(Y+num_sublevels-1) );
		for (r = 0; r < num_egoals; r++) {
			
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
				if (X == NULL)
					return NULL;  /* Fatal error */
				tmp = Cudd_bddAnd( manager, X, N_BDD );
				Cudd_Ref( tmp );
				Cudd_RecursiveDeref( manager, X );
				X = tmp;
								
				tmp2 = Cudd_bddOr( manager, Exit_BDD, Y_exmod );
				Cudd_Ref( tmp2 );

				tmp = Cudd_bddAnd( manager, X, Cudd_Not( *(egoals+r) ) );
				Cudd_Ref( tmp );
				Cudd_RecursiveDeref( manager, X );

				X = Cudd_bddOr( manager, tmp2, tmp );
				Cudd_Ref( X );
				Cudd_RecursiveDeref( manager, tmp );
				Cudd_RecursiveDeref( manager, tmp2 );

				tmp = X;
				X = Cudd_bddAnd( manager, X, X_prev );
				Cudd_Ref( X );
				Cudd_RecursiveDeref( manager, tmp );

			} while (!(Cudd_bddLeq( manager, X, X_prev )*Cudd_bddLeq( manager, X_prev, X )));

			*(*(X_jr+num_sublevels-1) + r) = X;
			Cudd_Ref( *(*(X_jr+num_sublevels-1) + r) );

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

		if (Cudd_bddLeq( manager, Entry_BDD, *(Y+num_sublevels-1) )) {
			if (Cudd_bddLeq( manager, *(Y+num_sublevels-1), *(Y+num_sublevels-2) )*Cudd_bddLeq( manager, *(Y+num_sublevels-2), *(Y+num_sublevels-1) )) {
				Cudd_RecursiveDeref( manager, *(Y+num_sublevels-1) );
				for (r = 0; r < num_egoals; r++) {
					Cudd_RecursiveDeref( manager, *(*(X_jr+num_sublevels-1) + r) );
				}
				free( *(X_jr+num_sublevels-1) );
				num_sublevels--;
				Y = realloc( Y, num_sublevels*sizeof(DdNode *) );
				X_jr = realloc( X_jr, num_sublevels*sizeof(DdNode **) );
				if (Y == NULL || X_jr == NULL) {
					perror( "synthesize_patch, realloc" );
					return NULL;
				}
			}
			break;
		}

		if (Cudd_bddLeq( manager, *(Y+num_sublevels-1), *(Y+num_sublevels-2) )*Cudd_bddLeq( manager, *(Y+num_sublevels-2), *(Y+num_sublevels-1) )) {
			return NULL;  /* Local synthesis failed */
		}
		Cudd_RecursiveDeref( manager, Y_exmod );
	}


	/* Note that we assume the variable map has been appropriately
	   defined in the CUDD manager before invocation of synthesize_patch. */
	tmp = Cudd_bddVarMap( manager, N_BDD );
	if (tmp == NULL) {
		fprintf( stderr, "Error synthesize: Error in swapping variables with primed forms.\n" );
		return NULL;
	}
	Cudd_Ref( tmp );
	strans_into_N = Cudd_bddAnd( manager, strans, tmp );
	Cudd_Ref( strans_into_N );
	Cudd_RecursiveDeref( manager, tmp );

	/* Synthesize local strategy */
	for (i = 0; i < Entry_len; i++)
		this_node_stack = insert_anode( this_node_stack, -1, -1, (*(Entry+i))->state, num_env+num_sys );

	/* Insert all stacked, initial nodes into strategy. */
	node = this_node_stack;
	while (node) {
		strategy = insert_anode( strategy, node->mode, node->rgrad,
								 node->state, num_env+num_sys );
		if (strategy == NULL) {
			fprintf( stderr, "Error synthesize_patch: inserting state node into strategy.\n" );
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
		node = find_anode( strategy, this_node_stack->mode, this_node_stack->state,
						   num_env+num_sys );
		if (node->trans_len > 0 || j == 0) {
			/* This state and mode combination is already in strategy. */
			node->rgrad = j;
			this_node_stack = pop_anode( this_node_stack );
			continue;
		}

		/* Note that we assume the variable map has been appropriately
		   defined in the CUDD manager, after the call to
		   compute_winning_set above. */
		Y_i_primed = Cudd_bddVarMap( manager, *(Y+j-1) );
		if (Y_i_primed == NULL) {
			fprintf( stderr, "Error synthesize_patch: swapping variables with primed forms.\n" );
			return NULL;
		}
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
			tmp = Cudd_bddAnd( manager, strans_into_N, Y_i_primed );
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
				fprintf( stderr, "Error synthesize_patch: failed to find cube.\n" );
				return NULL;
			}
			if (Cudd_IsGenEmpty( gen )) {
				/* Cannot step closer to system goal, so must be in
				   goal state or able to block environment goal. */
				Cudd_GenFree( gen );
				Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
				if (j > 0) {
					for (r = 0; r < num_egoals; r++) {
						Cudd_RecursiveDeref( manager, tmp );
						Cudd_RecursiveDeref( manager, Y_i_primed );
						Y_i_primed = Cudd_bddVarMap( manager, *(*(X_jr+j)+r) );
						if (Y_i_primed == NULL) {
							fprintf( stderr, "Error synthesize_patch: Error in swapping variables with primed forms.\n" );
							return NULL;
						}
						Cudd_Ref( Y_i_primed );
						tmp = Cudd_bddAnd( manager, strans_into_N, Y_i_primed );
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
					
						if (!(Cudd_bddLeq( manager, tmp, Cudd_Not( Cudd_ReadOne( manager ) ) )*Cudd_bddLeq( manager, Cudd_Not( Cudd_ReadOne( manager ) ), tmp )))
							break;
					}
					if (r >= num_egoals) {
						fprintf( stderr, "Error synthesize_patch: unexpected losing state.\n" );
						return NULL;
					}
				} else {
					Cudd_RecursiveDeref( manager, tmp );
					Cudd_RecursiveDeref( manager, Y_i_primed );
					Y_i_primed = Cudd_ReadOne( manager );
					Cudd_Ref( Y_i_primed );
					tmp = Cudd_bddAnd( manager, strans_into_N, Y_i_primed );
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
				}

				Cudd_AutodynDisable( manager );
				gen = Cudd_FirstCube( manager, tmp, &gcube, &gvalue );
				if (gen == NULL) {
					fprintf( stderr, "Error synthesize_patch: failed to find cube.\n" );
					return NULL;
				}
				if (Cudd_IsGenEmpty( gen )) {
					Cudd_GenFree( gen );
					fprintf( stderr, "Error synthesize_patch: unexpected losing state.\n" );
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

			new_node = find_anode( strategy, -1, state, num_env+num_sys );
			if (new_node == NULL) {
				strategy = insert_anode( strategy, -1, -1,
										 state, num_env+num_sys );
				if (strategy == NULL) {
					fprintf( stderr, "Error synthesize_patch: inserting new node into strategy.\n" );
					return NULL;
				}
				this_node_stack = insert_anode( this_node_stack, -1, -1,
												state, num_env+num_sys );
				if (this_node_stack == NULL) {
					fprintf( stderr, "Error synthesize_patch: pushing node onto stack failed.\n" );
					return NULL;
				}
			} 

			strategy = append_anode_trans( strategy,
										   node->mode, node->state,
										   num_env+num_sys,
										   -1, state );
			if (strategy == NULL) {
				fprintf( stderr, "Error synthesize_patch: inserting new transition into strategy.\n" );
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

	
	/* Pre-exit clean-up */
	Cudd_RecursiveDeref( manager, Exit_BDD );
	Cudd_RecursiveDeref( manager, Entry_BDD );
	free( cube );
	free( state );

	return strategy;
}


#define INPUT_STRING_LEN 256

anode_t *patch_localfixpoint( DdManager *manager, FILE *strategy_fp, FILE *change_fp, unsigned char verbose )
{
	ptree_t *var_separator;
	DdNode *etrans, *strans, **egoals;
	DdNode *etrans_part, *strans_part;
	int num_env, num_sys;

	DdNode *vertex1, *vertex2; /* ...regarding vertices of the game graph. */
	char line[INPUT_STRING_LEN];
	bool *state;

	bool env_nogoal_flag = False;  /* Indicate environment has no goals */

	int i, j, k;  /* Generic counters */
	DdNode *tmp, *tmp2;
	int num_read;
	anode_t *strategy, *local_strategy;
	int strategy_size = 0;
	anode_t *node, *head;
	int node_counter;
	bool **N = NULL;  /* "neighborhood" of states */
	int N_len = 0;
	int goal_mode;
	DdNode *N_BDD = NULL;  /* Characteristic function for set of states N. */
	int min_rgrad;  /* Minimum reachability gradient value of affected nodes. */
	int Exit_rgrad;  /* Maximum value among reached Exit nodes. */
	int local_max_rgrad;
	int local_min_rgrad;
	bool break_flag;

	anode_t **Exit;
	anode_t **Entry;
	int Exit_len, Entry_len;

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

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	strategy = aut_aut_load( num_env+num_sys, strategy_fp );
	if (strategy == NULL) {
		return NULL;
	}
	strategy_size = aut_size( strategy );
	if (verbose)
		printf( "Read in strategy of size %d\n", strategy_size );

	affected = malloc( sizeof(anode_t **)*num_sgoals );
	if (affected == NULL) {
		perror( "patch_localfixpoint, malloc" );
		return NULL;
	}
	affected_len = malloc( sizeof(int)*num_sgoals );
	if (affected_len == NULL) {
		perror( "patch_localfixpoint, malloc" );
		return NULL;
	}
	for (i = 0; i < num_sgoals; i++) {
		*(affected+i) = NULL;
		*(affected_len+i) = 0;
	}

	cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
	if (cube == NULL) {
		perror( "patch_localfixpoint, malloc" );
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

	/* Process game graph changes file */
	break_flag = False;
	while (fgets( line, INPUT_STRING_LEN, change_fp )) {
		/* Blank or comment line? */
		if (strlen( line ) < 1 || *line == '\n' || *line == '#')
			continue;

		num_read = read_state_str( line, &state, num_env+num_sys );
		if (num_read == 0) {
			/* This must be the first command, so break from loop and
			   build local transition rules from N. */
			break_flag = True;
			break;
		} else if (num_read < num_env+num_sys) {
			fprintf( stderr, "Error patch_localfixpoint: malformed game change file.\n" );
			return NULL;
		}

		N_len++;
		N = realloc( N, sizeof(bool *)*N_len );
		if (N == NULL) {
			perror( "patch_localfixpoint, realloc" );
			return NULL;
		}
		*(N+N_len-1) = state;
	}

	if (verbose) {
		printf( "States in N:\n" );
		for (i = 0; i < N_len; i++) {
			printf( "   " );
			for (j = 0; j < num_env+num_sys; j++)
				printf( " %d", *(*(N+i)+j) );
			printf( "\n" );
		}
		fflush( stdout );
	}

	/* Build characteristic function for N */
	N_BDD = Cudd_Not( Cudd_ReadOne( manager ) );
	Cudd_Ref( N_BDD );
	for (i = 0; i < N_len; i++) {
		ddval = state2BDD( manager, *(N+i), 0, num_env+num_sys );
		tmp = Cudd_bddOr( manager, N_BDD, ddval );
		Cudd_Ref( tmp );
		Cudd_RecursiveDeref( manager, N_BDD );
		Cudd_RecursiveDeref( manager, ddval );
		N_BDD = tmp;
	}
	ddval = NULL;

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

	/* Generate BDDs for parse trees from the problem spec transition
	   rules that are relevant given restriction to N. */
	if (verbose) {
		printf( "Building local environment transition BDD..." );
		fflush( stdout );
	}
	etrans = Cudd_ReadOne( manager );
	Cudd_Ref( etrans );
	if (verbose) {
		printf( "\nRelevant env trans:\n" );
	}
	for (i = 0; i < et_array_len; i++) {
		etrans_part = ptree_BDD( *(env_trans_array+i), evar_list, manager );
		for (j = 0; j < N_len; j++) {
			for (k = 0; k < num_env+num_sys; k++) {
				*(cube+k) = *(*(N+j)+k);
				*(cube+num_env+num_sys+k) = 2;
			}
			ddval = Cudd_CubeArrayToBdd( manager, cube );
			if (ddval == NULL) {
				fprintf( stderr, "Error patch_localfixpoint: building characteristic function of N." );
				return NULL;
			}
			Cudd_Ref( ddval );
		
			tmp2 = Cudd_Cofactor( manager, etrans_part, ddval );
			if (tmp2 == NULL) {
				fprintf( stderr, "Error patch_localfixpoint: computing cofactor." );
				return NULL;
			}
			Cudd_Ref( tmp2 );

			if (!(Cudd_bddLeq( manager, tmp2, Cudd_ReadOne( manager ) )*Cudd_bddLeq( manager, Cudd_ReadOne( manager ), tmp2 ))) {
				Cudd_RecursiveDeref( manager, tmp2 );
				Cudd_RecursiveDeref( manager, ddval );
				break;
			}
			
			Cudd_RecursiveDeref( manager, tmp2 );
			Cudd_RecursiveDeref( manager, ddval );
		}
		if (j < N_len) {
			if (verbose) {
				printf( "    " );
				print_formula( *(env_trans_array+i), stdout );
				printf( "\n" );
			}
			tmp = Cudd_bddAnd( manager, etrans, etrans_part );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, etrans );
			etrans = tmp;
		}
		Cudd_RecursiveDeref( manager, etrans_part );
		
	}
	if (verbose) {
		printf( "\nDone.\n" );
		printf( "Building local system transition BDD..." );
		fflush( stdout );
	}

	strans = Cudd_ReadOne( manager );
	Cudd_Ref( strans );
	if (verbose) {
		printf( "\nRelevant sys trans:\n" );
	}
	for (i = 0; i < st_array_len; i++) {
		strans_part = ptree_BDD( *(sys_trans_array+i), evar_list, manager );
		for (j = 0; j < N_len; j++) {
			for (k = 0; k < num_env+num_sys; k++) {
				*(cube+k) = *(*(N+j)+k);
				*(cube+num_env+num_sys+k) = 2;
			}
			ddval = Cudd_CubeArrayToBdd( manager, cube );
			if (ddval == NULL) {
				fprintf( stderr, "Error patch_localfixpoint: building characteristic function of N." );
				return NULL;
			}
			Cudd_Ref( ddval );
		
			tmp2 = Cudd_Cofactor( manager, strans_part, ddval );
			if (tmp2 == NULL) {
				fprintf( stderr, "Error patch_localfixpoint: computing cofactor." );
				return NULL;
			}
			Cudd_Ref( tmp2 );

			if (!(Cudd_bddLeq( manager, tmp2, Cudd_ReadOne( manager ) )*Cudd_bddLeq( manager, Cudd_ReadOne( manager ), tmp2 ))) {
				Cudd_RecursiveDeref( manager, tmp2 );
				Cudd_RecursiveDeref( manager, ddval );
				break;
			}
			
			Cudd_RecursiveDeref( manager, tmp2 );
			Cudd_RecursiveDeref( manager, ddval );
		}
		if (j < N_len) {
			if (verbose) {
				printf( "    " );
				print_formula( *(sys_trans_array+i), stdout );
				printf( "\n" );
			}
			tmp = Cudd_bddAnd( manager, strans, strans_part );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, strans );
			strans = tmp;
		}
		Cudd_RecursiveDeref( manager, strans_part );
		
	}
	if (verbose) {
		printf( "\nDone.\n" );
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

	if (var_separator == NULL) {
		evar_list = NULL;
	} else {
		var_separator->left = NULL;
	}

	/* Was the earlier file loop broken because a command was discovered? */
	if (break_flag) {
		do {
			/* Blank or comment line? */
			if (strlen( line ) < 1 || *line == '\n' || *line == '#')
				continue;

			if (!strncmp( line, "restrict ", strlen( "restrict " ) ) || !strncmp( line, "relax ", strlen( "relax " ) )) {
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
				for (j = 0; j < num_sgoals; j++) {
					node = find_anode( strategy, j, state, num_env+num_sys );
					if (node != NULL) {
						if (!strncmp( line, "restrict ", strlen( "restrict " ) )
							&& (num_read == 2*(num_env+num_sys))) {
							for (i = 0; i < node->trans_len; i++) {
								if (statecmp( (*(node->trans+i))->state,
											  state+num_env+num_sys, num_env+num_sys )) {
									(*(affected_len + node->mode))++;
									*(affected + node->mode) = realloc( *(affected + node->mode), sizeof(anode_t *)*(*(affected_len + node->mode)) );
									if (*(affected + node->mode) == NULL) {
										perror( "patch_localfixpoint, realloc" );
										return NULL;
									}
									/* If affected state is not in N, then fail. */
									for (i = 0; i < N_len; i++) {
										if (statecmp( state, *(N+i), num_env+num_sys ))
											break;
									}
									if (i == N_len) {
										fprintf( stderr, "Error patch_localfixpoint: affected state not contained in N.\n" );
										return NULL;
									}
									*(*(affected + node->mode) + *(affected_len + node->mode)-1) = node;
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
								(*(affected_len + node->mode))++;
								*(affected + node->mode) = realloc( *(affected + node->mode), sizeof(anode_t *)*(*(affected_len + node->mode)) );
								if (*(affected + node->mode) == NULL) {
									perror( "patch_localfixpoint, realloc" );
									return NULL;
								}
								/* If affected state is not in N, then fail. */
								for (i = 0; i < N_len; i++) {
									if (statecmp( state, *(N+i), num_env+num_sys ))
										break;
								}
								if (i == N_len) {
									fprintf( stderr, "Error patch_localfixpoint: affected state not contained in N.\n" );
									return NULL;
								}
								*(*(affected + node->mode) + *(affected_len + node->mode)-1) = node;
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
					for (i = 0; i < head->trans_len; i++) {
						if (statecmp( state, (*(head->trans+i))->state+num_env, num_sys )) {
							(*(affected_len + head->mode))++;
							*(affected + head->mode) = realloc( *(affected + head->mode), sizeof(anode_t *)*(*(affected_len + head->mode)) );
							if (*(affected + head->mode) == NULL) {
								perror( "patch_localfixpoint, realloc" );
								return NULL;
							}
							/* If affected state is not in N, then fail. */
							for (i = 0; i < N_len; i++) {
								if (statecmp( head->state, *(N+i), num_env+num_sys ))
									break;
							}
							if (i == N_len) {
								fprintf( stderr, "Error patch_localfixpoint: affected state not contained in N.\n" );
								return NULL;
							}
							*(*(affected + head->mode) + *(affected_len + head->mode)-1) = head;
							break;
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

			} else {
				fprintf( stderr, "Error patch_localfixpoint: unrecognized line in given edge change file.\n" );
				return NULL;
			}
			
			if (verbose)
				fflush( stdout );
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
		fprintf( stderr, "Error: failed to define variable map in CUDD manager.\n" );
		return NULL;
	}
	free( vars );
	free( pvars );

	/* Pre-allocate space for Entry and Exit sets; the number of
	   elements actually used is tracked by Entry_len and Exit_len,
	   respectively. */
	Exit = malloc( sizeof(anode_t *)*strategy_size );
	if (Exit == NULL) {
		perror( "patch_localfixpoint, malloc" );
		return NULL;
	}
	Entry = malloc( sizeof(anode_t *)*strategy_size );
	if (Entry == NULL) {
		perror( "patch_localfixpoint, malloc" );
		return NULL;
	}
	for (goal_mode = 0; goal_mode < num_sgoals; goal_mode++) {

		/* Ignore goal modes that are unaffected by the change. */
		if (*(affected_len + goal_mode) == 0)
			continue;

		if (verbose) {
			printf( "Processing for goal mode %d...\n", goal_mode );
			fflush( stdout );
		}

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
					/* Test for nominal membership in Exit set; i.e.,
					   current node is in N, see if it leads out. */
					for (j = 0; j < head->trans_len; j++) {
						for (k = 0; k < N_len; k++)
							if ((*(head->trans+j))->mode == goal_mode
								&& statecmp( (*(head->trans+j))->state, *(N+k), num_env+num_sys ))
								break;

						if (k == N_len)
							break;
					}
					if (j < head->trans_len) {
						Exit_len++;
						*(Exit+Exit_len-1) = head;
					}
				} else {  
					/* Test for nominal membership in Entry set; i.e.,
					   current node is not in N, see if it leads into N. */
					for (j = 0; j < head->trans_len; j++) {
						for (k = 0; k < N_len; k++)
							if ((*(head->trans+j))->mode == goal_mode
								&& statecmp( (*(head->trans+j))->state, *(N+k), num_env+num_sys ))
								break;
						if (k < N_len)
							break;
					}
					if (j < head->trans_len) {
						Entry_len++;
						*(Entry+Entry_len-1) = *(head->trans+j);
					}
				}
			}
			head = head->next;
		}

		if (verbose) {
			printf( "Exit set before pruning:\n" );
			for (i = 0; i < Exit_len; i++) {
				printf( "   " );
				for (j = 0; j < num_env+num_sys; j++)
					printf( " %d", *((*(Exit+i))->state+j) );
				printf( "\n" );
			}
			fflush( stdout );
		}

		/* Find minimum reachability gradient value among nodes in the
		   Entry set, and remove any Exit nodes greater than or equal
		   to it. */
		min_rgrad = -1;
		for (i = 0; i < Entry_len; i++) {
			if ((*(Entry+i))->rgrad < min_rgrad || min_rgrad == -1)
				min_rgrad = (*(Entry+i))->rgrad;
		}
		if (verbose) {
			printf( "Minimum reachability gradient value in Exit: %d\n", min_rgrad );
			fflush( stdout );
		}
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
			printf( "Entry set:\n" );
			for (i = 0; i < Entry_len; i++) {
				printf( "   " );
				for (j = 0; j < num_env+num_sys; j++)
					printf( " %d", *((*(Entry+i))->state+j) );
				printf( "\n" );
			}
			printf( "Exit set:\n" );
			for (i = 0; i < Exit_len; i++) {
				printf( "   " );
				for (j = 0; j < num_env+num_sys; j++)
					printf( " %d", *((*(Exit+i))->state+j) );
				printf( "\n" );
			}
			fflush( stdout );
		}

		local_strategy = synthesize_patch( manager, num_env, num_sys,
										   Entry, Entry_len,
										   Exit, Exit_len,
										   etrans, strans, egoals, N_BDD,
										   verbose );
		if (local_strategy == NULL)
			break;

		node = local_strategy;
		local_min_rgrad = local_max_rgrad = -1;
		while (node) {
			node->mode = goal_mode+10;
			if (node->rgrad > local_max_rgrad)
				local_max_rgrad = node->rgrad;
			if (node->rgrad < local_min_rgrad || local_min_rgrad == -1)
				local_min_rgrad = node->rgrad;
			node = node->next;
		}

		/* Connect local strategy to original */
		for (i = 0; i < Entry_len; i++) {
			node = local_strategy;
			while (node && !statecmp( (*(Entry+i))->state, node->state, num_env+num_sys ))
				node = node->next;
			if (node == NULL) {
				fprintf( stderr, "Error patch_localfixpoint: expected Entry node missing from local strategy, in goal mode %d\n", goal_mode );
				return NULL;
			}

			replace_anode_trans( strategy, *(Entry+i), node );
		}

		Exit_rgrad = -1;
		for (i = 0; i < Exit_len; i++) {
			node = local_strategy;
			while (node && !statecmp( (*(Exit+i))->state, node->state, num_env+num_sys ))
				node = node->next;
			if (node == NULL)
				continue;

			if ((*(Exit+i))->rgrad > Exit_rgrad)
				Exit_rgrad = (*(Exit+i))->rgrad;

			node->trans = realloc( node->trans, sizeof(anode_t *)*(node->trans_len + (*(Exit+i))->trans_len) );
			if (node->trans == NULL) {
				perror( "patch_localfixpoint, realloc" );
				return NULL;
			}
			for (j = 0; j < (*(Exit+i))->trans_len; j++)
				*(node->trans + node->trans_len + j) = *((*(Exit+i))->trans + j);
			node->trans_len += (*(Exit+i))->trans_len;
		}

		for (i = 0; i < *(affected_len+goal_mode); i++) {
			strategy = delete_anode( strategy, *(*(affected+goal_mode)+i) );
			replace_anode_trans( strategy, *(*(affected+goal_mode)+i), NULL );
		}

		node = strategy;
		while (node) {
			if (node->mode != goal_mode) {
				node = node->next;
				continue;
			}
			for (i = 0; i < N_len; i++)
				if (statecmp( node->state, *(N+i), num_env+num_sys ))
					break;
			if (i < N_len) {
				strategy = delete_anode( strategy, node );
				replace_anode_trans( strategy, node, NULL );
				node = strategy;
			} else {
				node = node->next;
			}
		}

		/* Scale reachability gradient values to make room for patch. */
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
	}

	if (goal_mode != num_sgoals) {  /* Did a local patching attempt fail? */
		delete_aut( strategy );
		strategy = NULL;
	} else {
		strategy = aut_prune_deadends( strategy );
	}


	/* Pre-exit clean-up */
	free( cube );
	Cudd_RecursiveDeref( manager, etrans );
	Cudd_RecursiveDeref( manager, strans );
	for (i = 0; i < num_egoals; i++)
		Cudd_RecursiveDeref( manager, *(egoals+i) );
	if (num_egoals > 0)
		free( egoals );
	if (env_nogoal_flag) {
		num_egoals = 0;
		delete_tree( *env_goals );
		free( env_goals );
	}
	Cudd_RecursiveDeref( manager, N_BDD );
	for (i = 0; i < N_len; i++)
		free( *(N+i) );
	free( N );
	for (i = 0; i < num_sgoals; i++)
		free( *(affected+i) );
	free( affected );
	free( affected_len );
	free( Exit );
	free( Entry );

	return strategy;
}
