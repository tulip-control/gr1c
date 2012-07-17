/* patching_support.c -- More definitions for signatures in patching.h.
 *
 *
 * SCL; 2012.
 */


#include <stdlib.h>
#include <stdio.h>
#include "cudd.h"

#include "patching.h"
#include "solve_support.h"


extern int num_egoals;


anode_t *synthesize_reachgame( DdManager *manager, int num_env, int num_sys,
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
		perror( "synthesize_reachgame, malloc" );
		return NULL;
	}
	cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
	if (cube == NULL) {
		perror( "synthesize_reachgame, malloc" );
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
		perror( "synthesize_reachgame, malloc" );
		return NULL;
	}
	*Y = Exit_BDD;
	Cudd_Ref( *Y );

	X_jr = malloc( num_sublevels*sizeof(DdNode **) );
	if (X_jr == NULL) {
		perror( "synthesize_reachgame, malloc" );
		return NULL;
	}
	
	*X_jr = malloc( num_egoals*sizeof(DdNode *) );
	if (*X_jr == NULL) {
		perror( "synthesize_reachgame, malloc" );
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
			perror( "synthesize_reachgame, realloc" );
			return NULL;
		}
		
		*(X_jr + num_sublevels-1) = malloc( num_egoals*sizeof(DdNode *) );
		if (*(X_jr + num_sublevels-1) == NULL) {
			perror( "synthesize_reachgame, malloc" );
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
					perror( "synthesize_reachgame, realloc" );
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
	   defined in the CUDD manager before invocation of synthesize_reachgame. */
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
			fprintf( stderr, "Error synthesize_reachgame: inserting state node into strategy.\n" );
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
			fprintf( stderr, "Error synthesize_reachgame: swapping variables with primed forms.\n" );
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
				fprintf( stderr, "Error synthesize_reachgame: failed to find cube.\n" );
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
							fprintf( stderr, "Error synthesize_reachgame: Error in swapping variables with primed forms.\n" );
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
						fprintf( stderr, "Error synthesize_reachgame: unexpected losing state.\n" );
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
					fprintf( stderr, "Error synthesize_reachgame: failed to find cube.\n" );
					return NULL;
				}
				if (Cudd_IsGenEmpty( gen )) {
					Cudd_GenFree( gen );
					fprintf( stderr, "Error synthesize_reachgame: unexpected losing state.\n" );
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
					fprintf( stderr, "Error synthesize_reachgame: inserting new node into strategy.\n" );
					return NULL;
				}
				this_node_stack = insert_anode( this_node_stack, -1, -1,
												state, num_env+num_sys );
				if (this_node_stack == NULL) {
					fprintf( stderr, "Error synthesize_reachgame: pushing node onto stack failed.\n" );
					return NULL;
				}
			} 

			strategy = append_anode_trans( strategy,
										   node->mode, node->state,
										   num_env+num_sys,
										   -1, state );
			if (strategy == NULL) {
				fprintf( stderr, "Error synthesize_reachgame: inserting new transition into strategy.\n" );
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
