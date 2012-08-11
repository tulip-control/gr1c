/* solve.c -- Definitions for signatures appearing in solve.h.
 *            Also see solve_operators.c
 *
 *
 * SCL; 2012.
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "solve.h"
#include "solve_support.h"
#include "automaton.h"


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


DdNode *check_realizable_internal( DdManager *manager, DdNode *W,
								   unsigned char init_flags, unsigned char verbose );


/* N.B., we assume there is at least one system goal.  This assumption
   will be removed in a future version (soon). */
anode_t *synthesize( DdManager *manager,  unsigned char init_flags,
					 unsigned char verbose )
{
	anode_t *strategy = NULL;
	anode_t *this_node_stack = NULL;
	anode_t *node, *new_node;
	bool *state;
	bool **env_moves;
	int emoves_len;

	ptree_t *var_separator;
	DdNode *W;
	DdNode *strans_into_W;

	DdNode *einit, *sinit, *etrans, *strans, **egoals, **sgoals;
	
	DdNode *ddval;  /* Store result of evaluating a BDD */
	DdNode ***Y = NULL;
	DdNode *Y_i_primed;
	int *num_sublevels;
	DdNode ****X_ijr = NULL;

	DdNode *tmp, *tmp2;
	int i, j, r, k;  /* Generic counters */
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
	if (verbose)
		logprint( "Building environment transition BDD..." );
	etrans = ptree_BDD( env_trans, evar_list, manager );
	if (verbose) {
		logprint( "Done." );
		logprint( "Building system transition BDD..." );
	}
	strans = ptree_BDD( sys_trans, evar_list, manager );
	if (verbose)
		logprint( "Done." );

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
		fprintf( stderr, "Error synthesize: failed to construct winning set.\n" );
		return NULL;
	}
	Y = compute_sublevel_sets( manager, W, etrans, strans,
							   egoals, num_egoals,
							   sgoals, num_sgoals,
							   &num_sublevels, &X_ijr, verbose );
	if (Y == NULL) {
		fprintf( stderr, "Error synthesize: failed to construct sublevel sets.\n" );
		return NULL;
	}

	/* Make primed form of W and take conjunction with system
	   transition (safety) formula, for use while stepping down Y_i
	   sets.  Note that we assume the variable map has been
	   appropriately defined in the CUDD manager, after the call to
	   compute_winning_set_BDD above. */
	tmp = Cudd_bddVarMap( manager, W );
	if (tmp == NULL) {
		fprintf( stderr, "Error synthesize: Error in swapping variables with primed forms.\n" );
		return NULL;
	}
	Cudd_Ref( tmp );
	strans_into_W = Cudd_bddAnd( manager, strans, tmp );
	Cudd_Ref( strans_into_W );
	Cudd_RecursiveDeref( manager, tmp );

	/* From each initial state, build strategy by propagating forward
	   toward the next goal (current target goal specified by "mode"
	   of a state), and iterating until every reached state and mode
	   combination has already been encountered (whence the
	   strategy already built). */
	if (init_flags == ALL_SYS_INIT) {
		tmp = Cudd_bddAnd( manager, einit, sinit );
		Cudd_Ref( tmp );
		Cudd_AutodynDisable( manager );
		Cudd_ForeachCube( manager, tmp, gen, gcube, gvalue ) {
			initialize_cube( state, gcube, num_env+num_sys );
			while (!saturated_cube( state, gcube, num_env+num_sys )) {
				this_node_stack = insert_anode( this_node_stack, 0, -1, state, num_env+num_sys );
				if (this_node_stack == NULL) {
					fprintf( stderr, "Error synthesize: building list of initial states.\n" );
					return NULL;
				}
				increment_cube( state, gcube, num_env+num_sys );
			}
			this_node_stack = insert_anode( this_node_stack, 0, -1, state, num_env+num_sys );
			if (this_node_stack == NULL) {
				fprintf( stderr, "Error synthesize: building list of initial states.\n" );
				return NULL;
			}
		}
		Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
		Cudd_RecursiveDeref( manager, tmp );
	} else { /* EXIST_SYS_INIT */
		tmp = Cudd_bddAnd( manager, einit, sinit );
		Cudd_Ref( tmp );
		tmp2 = Cudd_bddAnd( manager, tmp, W );
		Cudd_Ref( tmp2 );
		Cudd_RecursiveDeref( manager, tmp );
		Cudd_AutodynDisable( manager );
		Cudd_ForeachCube( manager, tmp2, gen, gcube, gvalue ) {
			initialize_cube( state, gcube, num_env+num_sys );
			while (!saturated_cube( state, gcube, num_env+num_sys )) {
				this_node_stack = insert_anode( this_node_stack, 0, -1, state, num_env+num_sys );
				if (this_node_stack == NULL) {
					fprintf( stderr, "Error synthesize: building list of initial states.\n" );
					return NULL;
				}
				increment_cube( state, gcube, num_env+num_sys );
			}
			this_node_stack = insert_anode( this_node_stack, 0, -1, state, num_env+num_sys );
			if (this_node_stack == NULL) {
				fprintf( stderr, "Error synthesize: building list of initial states.\n" );
				return NULL;
			}
		}
		Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
		Cudd_RecursiveDeref( manager, tmp2 );
	}

	/* Insert all stacked, initial nodes into strategy. */
	node = this_node_stack;
	while (node) {
		strategy = insert_anode( strategy, node->mode, node->rgrad,
								 node->state, num_env+num_sys );
		if (strategy == NULL) {
			fprintf( stderr, "Error synthesize: inserting state node into strategy.\n" );
			return NULL;
		}
		node = node->next;
	}

	while (this_node_stack) {
		/* Find smallest Y_j set containing node. */
		for (k = num_env+num_sys; k < 2*(num_env+num_sys); k++)
			*(cube+k) = 2;
		state2cube( this_node_stack->state, cube, num_env+num_sys );
		loop_mode = this_node_stack->mode;
		do {
			j = *(num_sublevels+this_node_stack->mode);
			do {
				j--;
				ddval = Cudd_Eval( manager, *(*(Y+this_node_stack->mode)+j), cube );
				if (ddval->type.value < .1) {
					j++;
					break;
				}
			} while (j > 0);
			if (j == 0) {
				if (this_node_stack->mode == num_sgoals-1) {
					this_node_stack->mode = 0;
				} else {
					(this_node_stack->mode)++;
				}
			} else {
				break;
			}
		} while (loop_mode != this_node_stack->mode);
		if (this_node_stack->mode == loop_mode) {
			node = find_anode( strategy, this_node_stack->mode, this_node_stack->state,
							   num_env+num_sys );
			if (node->trans_len > 0) {
				/* This state and mode combination is already in strategy. */
				this_node_stack = pop_anode( this_node_stack );
				continue;
			}
		} else {

			node = find_anode( strategy, loop_mode, this_node_stack->state,
							   num_env+num_sys );
			if (node->trans_len > 0) {
				/* This state and mode combination is already in strategy. */
				this_node_stack = pop_anode( this_node_stack );
				continue;
			} else {
				strategy = delete_anode( strategy, node );
				new_node = find_anode( strategy, this_node_stack->mode, this_node_stack->state, num_env+num_sys );
				if (new_node == NULL) {
					strategy = insert_anode( strategy, this_node_stack->mode, -1, this_node_stack->state, num_env+num_sys );
					if (strategy == NULL) {
						fprintf( stderr, "Error synthesize: inserting state node into strategy.\n" );
						return NULL;
					}
					new_node = find_anode( strategy, this_node_stack->mode, this_node_stack->state, num_env+num_sys );
				} else if (new_node->trans_len > 0) {
					replace_anode_trans( strategy, node, new_node );
					/* This state and mode combination is already in strategy. */
					this_node_stack = pop_anode( this_node_stack );
					continue;
				}
				replace_anode_trans( strategy, node, new_node );
			}

			node = new_node;
		}
		this_node_stack = pop_anode( this_node_stack );
		node->rgrad = j;

		/* Note that we assume the variable map has been appropriately
		   defined in the CUDD manager, after the call to
		   compute_winning_set above. */
		if (j == 0) {
			Y_i_primed = Cudd_bddVarMap( manager, *(*(Y+node->mode)) );
		} else {
			Y_i_primed = Cudd_bddVarMap( manager, *(*(Y+node->mode)+j-1) );
		}
		if (Y_i_primed == NULL) {
			fprintf( stderr, "Error synthesize: Error in swapping variables with primed forms.\n" );
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
				fprintf( stderr, "Error synthesize: failed to find cube.\n" );
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
						Y_i_primed = Cudd_bddVarMap( manager, *(*(*(X_ijr+node->mode)+j)+r) );
						if (Y_i_primed == NULL) {
							fprintf( stderr, "Error synthesize: Error in swapping variables with primed forms.\n" );
							return NULL;
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
					
						if (!(Cudd_bddLeq( manager, tmp, Cudd_Not( Cudd_ReadOne( manager ) ) )*Cudd_bddLeq( manager, Cudd_Not( Cudd_ReadOne( manager ) ), tmp )))
							break;
					}
					if (r >= num_egoals) {
						fprintf( stderr, "Error synthesize: unexpected losing state.\n" );
						return NULL;
					}
				} else {
					Cudd_RecursiveDeref( manager, tmp );
					Cudd_RecursiveDeref( manager, Y_i_primed );
					Y_i_primed = Cudd_ReadOne( manager );
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
				}
				next_mode = node->mode;

				Cudd_AutodynDisable( manager );
				gen = Cudd_FirstCube( manager, tmp, &gcube, &gvalue );
				if (gen == NULL) {
					fprintf( stderr, "Error synthesize: failed to find cube.\n" );
					return NULL;
				}
				if (Cudd_IsGenEmpty( gen )) {
					Cudd_GenFree( gen );
					fprintf( stderr, "Error synthesize: unexpected losing state.\n" );
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
			ddval = Cudd_Eval( manager, **(Y+node->mode), cube );
			if (ddval->type.value < .1) {
				next_mode = node->mode;
			} else {
				if (node->mode == num_sgoals-1) {
					next_mode = 0;
				} else {
					next_mode = node->mode + 1;
				}
			}

			new_node = find_anode( strategy, next_mode, state, num_env+num_sys );
			if (new_node == NULL) {
				strategy = insert_anode( strategy, next_mode, -1,
										 state, num_env+num_sys );
				if (strategy == NULL) {
					fprintf( stderr, "Error synthesize: inserting new node into strategy.\n" );
					return NULL;
				}
				this_node_stack = insert_anode( this_node_stack, next_mode, -1,
												state, num_env+num_sys );
				if (this_node_stack == NULL) {
					fprintf( stderr, "Error synthesize: pushing node onto stack failed.\n" );
					return NULL;
				}
			} 

			strategy = append_anode_trans( strategy,
										   node->mode, node->state,
										   num_env+num_sys,
										   next_mode, state );
			if (strategy == NULL) {
				fprintf( stderr, "Error synthesize: inserting new transition into strategy.\n" );
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
		for (j = 0; j < *(num_sublevels+i); j++) {
			Cudd_RecursiveDeref( manager, *(*(Y+i)+j) );
			for (r = 0; r < num_egoals; r++) {
				Cudd_RecursiveDeref( manager, *(*(*(X_ijr+i)+j)+r) );
			}
			free( *(*(X_ijr+i)+j) );
		}
		if (*(num_sublevels+i) > 0) {
			free( *(Y+i) );
			free( *(X_ijr+i) );
		}
	}
	if (num_sgoals > 0) {
		free( Y );
		free( X_ijr );
		free( num_sublevels );
	}

	return strategy;
}


DdNode *check_realizable( DdManager *manager, unsigned char init_flags,
						  unsigned char verbose )
{
	DdNode *W = compute_winning_set( manager, verbose );
	return check_realizable_internal( manager, W, init_flags, verbose );
}


DdNode *check_realizable_internal( DdManager *manager, DdNode *W,
								   unsigned char init_flags, unsigned char verbose )
{
	bool realizable;
	DdNode *tmp, *tmp2;
	DdNode *einit, *sinit;

	ptree_t *var_separator;
	int num_env, num_sys;
	int *cube;  /* length will be twice total number of variables (to
				   account for both variables and their primes). */
	DdNode *ddcube;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	/* Allocate cube array, used later for quantifying over variables. */
	cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
	if (cube == NULL) {
		perror( "check_realizable_internal, malloc" );
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

	einit = ptree_BDD( env_init, evar_list, manager );
	sinit = ptree_BDD( sys_init, evar_list, manager );

	/* Break the link that appended the system variables list to the
	   environment variables list. */
	if (var_separator == NULL) {
		evar_list = NULL;
	} else {
		var_separator->left = NULL;
	}

	/* Does winning set contain all initial states? */
	if (init_flags == ALL_SYS_INIT) {
		tmp = Cudd_bddAnd( manager, einit, sinit );
		Cudd_Ref( tmp );
		tmp2 = Cudd_bddAnd( manager, tmp, W );
		Cudd_Ref( tmp2 );
		if (!(Cudd_bddLeq( manager, tmp, tmp2 )*Cudd_bddLeq( manager, tmp2, tmp ))) {
			realizable = False;
		} else {
			realizable = True;
		}
		Cudd_RecursiveDeref( manager, tmp );
		Cudd_RecursiveDeref( manager, tmp2 );
	} else { /* EXIST_SYS_INIT */
		tmp = Cudd_bddAnd( manager, sinit, W );
		Cudd_Ref( tmp );
		cube_sys( cube, num_env, num_sys );
		ddcube = Cudd_CubeArrayToBdd( manager, cube );
		if (ddcube == NULL) {
			fprintf( stderr, "Error in generating cube for quantification." );
			return NULL;
		}
		Cudd_Ref( ddcube );
		tmp2 = Cudd_bddExistAbstract( manager, tmp, ddcube );
		if (tmp2 == NULL) {
			fprintf( stderr, "Error in performing quantification." );
			return NULL;
		}
		Cudd_Ref( tmp2 );
		Cudd_RecursiveDeref( manager, ddcube );
		Cudd_RecursiveDeref( manager, tmp );

		tmp = Cudd_bddOr( manager, Cudd_Not( einit ), tmp2 );
		Cudd_Ref( tmp );
		Cudd_RecursiveDeref( manager, tmp2 );
		cube_env( cube, num_env, num_sys );
		ddcube = Cudd_CubeArrayToBdd( manager, cube );
		if (ddcube == NULL) {
			fprintf( stderr, "Error in generating cube for quantification." );
			return NULL;
		}
		Cudd_Ref( ddcube );
		tmp2 = Cudd_bddUnivAbstract( manager, tmp, ddcube );
		if (tmp2 == NULL) {
			fprintf( stderr, "Error in performing quantification." );
			return NULL;
		}
		Cudd_Ref( tmp2 );
		Cudd_RecursiveDeref( manager, ddcube );
		Cudd_RecursiveDeref( manager, tmp );

		if (!(Cudd_bddLeq( manager, tmp2, Cudd_ReadOne( manager ) )*Cudd_bddLeq( manager, Cudd_ReadOne( manager ), tmp2 ))) {
			realizable = False;
		} else {
			realizable = True;
		}
		Cudd_RecursiveDeref( manager, tmp2 );
	}

	Cudd_RecursiveDeref( manager, einit );
	Cudd_RecursiveDeref( manager, sinit );
	free( cube );

	if (realizable) {
		return W;
	} else {
		Cudd_RecursiveDeref( manager, W );
		return NULL;
	}
}
