/* patching_hotswap.c -- More definitions for signatures in patching.h.
 *
 *
 * SCL; 2013.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "logging.h"
#include "automaton.h"
#include "ptree.h"
#include "patching.h"
#include "solve.h"
#include "solve_support.h"
#include "solve_metric.h"


extern ptree_t *nonbool_var_list;
extern ptree_t *evar_list;
extern ptree_t *svar_list;
extern ptree_t **env_goals;
extern ptree_t **sys_goals;
extern int num_egoals;
extern int num_sgoals;
extern ptree_t *env_trans;
extern ptree_t *sys_trans;


extern void pprint_state( bool *state, int num_env, int num_sys, FILE *fp );  /* Defined in patching.c */

extern anode_t *synthesize_reachgame_BDD( DdManager *manager, int num_env, int num_sys,
										  DdNode *Entry, DdNode *Exit,
										  DdNode *etrans, DdNode *strans,
										  DdNode **egoals, DdNode *N_BDD,
										  unsigned char verbose );  /* Defined in patching_support.c */


anode_t *add_metric_sysgoal( DdManager *manager, FILE *strategy_fp, int original_num_env, int original_num_sys, int *offw, int num_metric_vars, ptree_t *new_sysgoal, unsigned char verbose )
{
	ptree_t *var_separator;
	DdNode *etrans, *strans, **egoals, **sgoals;
	bool env_nogoal_flag = False;  /* Indicate environment has no goals */

	anode_t *strategy = NULL, *component_strategy;
	int num_env, num_sys;
	anode_t *node1, *node2;

	double *Min, *Max;  /* One (Min, Max) pair per original system goal */

	int Gi_len[2];
	anode_t **Gi[2];  /* Array of two pointers to arrays of nodes in
						 the given strategy between which we wish to
						 insert the new system goal. */
	int istar[2];  /* Indices of system goals between which to insert. */
	DdNode *Gi_BDD = NULL;
	DdNode *new_sgoal = NULL;
	anode_t **new_reached = NULL;
	int new_reached_len = 0;

	anode_t **Gi_succ = NULL;
	int Gi_succ_len = 0;

	int i, j;  /* Generic counters */
	bool found_flag;
	int node_counter;
	DdNode *tmp, *tmp2;
	DdNode **vars, **pvars;

	if (strategy_fp == NULL)
		strategy_fp = stdin;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	strategy = aut_aut_load( original_num_env+original_num_sys, strategy_fp );
	if (strategy == NULL) {
		return NULL;
	}
	if (verbose)
		logprint( "Read in strategy of size %d", aut_size( strategy ) );

	if (verbose > 1)
		logprint( "Expanding nonbool variables in the given strategy automaton..." );
	if (aut_expand_bool( strategy, evar_list, svar_list, nonbool_var_list )) {
		fprintf( stderr, "Error add_metric_sysgoal: Failed to expand nonboolean variables in given automaton." );
		return NULL;
	}
	if (verbose > 1) {
		logprint( "Given strategy after variable expansion:" );
		logprint_startline();
		/* list_aut_dump( strategy, num_env+num_sys, getlogstream() ); */
		dot_aut_dump( strategy, evar_list, svar_list, DOT_AUT_ATTRIB, getlogstream() );
		logprint_endline();
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

	new_sgoal = ptree_BDD( new_sysgoal, evar_list, manager );

	if (var_separator == NULL) {
		evar_list = NULL;
	} else {
		var_separator->left = NULL;
	}


	/* Find which original system goal is closest to the new one */
	if (offw != NULL && num_metric_vars > 0) {
		Min = malloc( num_sgoals*sizeof(double) );
		Max = malloc( num_sgoals*sizeof(double) );
		if (Min == NULL || Max == NULL) {
			perror( "add_metric_sysgoal, malloc" );
			return NULL;
		}
		for (i = 0; i < num_sgoals; i++) {
			if (bounds_DDset( manager, *(sgoals+i), new_sgoal, offw, num_metric_vars, Min+i, Max+i, verbose )) {
				fprintf( stderr, "Error add_metric_sysgoal: bounds_DDset() failed to compute distance from original goal %d", i );
				return NULL;
			}
		}
		istar[0] = 0;
		for (i = 1; i < num_sgoals; i++) {
			if (*(Min+i) < *(Min+istar[0]))
				istar[0] = i;
		}
		if (verbose)
			logprint( "add_metric_sysgoal: The original system goal with index %d has minimum distance", istar[0] );
	} else {
		Min = Max = NULL;
		istar[0] = num_sgoals-1;
	}

	if (istar[0] == num_sgoals-1) {
		istar[1] = 0;
	} else {
		istar[1] = istar[0]+1;
	}
	Gi[0] = Gi[1] = NULL;
	Gi_len[0] = Gi_len[1] = 0;

	/* Find i* and i*+1 nodes */
	for (i = 0; i < 2; i++) {
		if (verbose > 1)
			logprint( "Adding nodes to set G_{%d}:", istar[i] );
		node_counter = 0;
		node1 = strategy;
		while (node1) {
			node2 = strategy;
			found_flag = False;
			while (node2 && !found_flag) {
				for (j = 0; j < node2->trans_len; j++) {
					if (*(node2->trans+j) == node1) {
						if ((node2->mode < node1->mode && node2->mode <= istar[i] && node1->mode > istar[i])
							|| (node2->mode > node1->mode && (node1->mode > istar[i] || node2->mode <= istar[i]))) {

							Gi_len[i]++;
							Gi[i] = realloc( Gi[i], Gi_len[i]*sizeof(anode_t *) );
							if (Gi[i] == NULL) {
								perror( "add_metric_sysgoal, realloc" );
								return NULL;
							}

							if (verbose > 1) {
								logprint_startline();
								logprint_raw( "\t%d : ", node_counter );
								pprint_state( node1->state, num_env, num_sys, getlogstream() );
								logprint_raw( " - %d", node1->mode );
								logprint_endline();
							}
							*(Gi[i]+Gi_len[i]-1) = node1;

							found_flag = True;
						}
						break;
					}
				}

				node2 = node2->next;
			}

			node_counter++;
			node1 = node1->next;
		}
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


	/* Build characteristic function for G_{i*} set. */
	Gi_BDD = Cudd_Not( Cudd_ReadOne( manager ) );
	Cudd_Ref( Gi_BDD );
	for (i = 0; i < Gi_len[0]; i++) {
		tmp2 = state2BDD( manager, (*(Gi[0]+i))->state, 0, num_env+num_sys );
		tmp = Cudd_bddOr( manager, Gi_BDD, tmp2 );
		Cudd_Ref( tmp );
		Cudd_RecursiveDeref( manager, Gi_BDD );
		Cudd_RecursiveDeref( manager, tmp2 );
		Gi_BDD = tmp;
	}
	tmp2 = NULL;

	tmp = Cudd_ReadOne( manager );
	Cudd_Ref( tmp );
	component_strategy = synthesize_reachgame_BDD( manager, num_env, num_sys,
												   Gi_BDD, new_sgoal,
												   etrans, strans, egoals, tmp, verbose );
	Cudd_RecursiveDeref( manager, tmp );
	if (component_strategy == NULL) {
		delete_aut( strategy );
		return NULL;  /* Failure */
	}
	if (verbose > 1) {
		logprint( "Component strategy to reach the new system goal from  G_{i*}:" );
		logprint_startline();
		/* list_aut_dump( component_strategy, num_env+num_sys, getlogstream() ); */
		dot_aut_dump( component_strategy, evar_list, svar_list, DOT_AUT_BINARY | DOT_AUT_ATTRIB, getlogstream() );
		logprint_endline();
	}
		
	Cudd_RecursiveDeref( manager, Gi_BDD );
	Gi_BDD = NULL;


	/* Find which states were actually reached before building
	   component strategy back into the given (original) strategy. */
	node1 = component_strategy;
	while (node1) {
		if (node1->trans_len == 0) {
			new_reached_len++;
			new_reached = realloc(new_reached, new_reached_len*sizeof(anode_t *) );
			if (new_reached == NULL) {
				perror( "add_metric_sysgoal, realloc" );
				return NULL;
			}
			*(new_reached+new_reached_len-1) = node1;
		}

		node1 = node1->next;
	}

	
	node1 = strategy;
	while (node1->next)
		node1 = node1->next;
	node1->next = component_strategy;

	node1 = component_strategy;
	while (node1) {
		node1->mode = num_sgoals+1;  /* Temporary mode label for this component. */
		node1 = node1->next;
	}

	/* From original to first component... */
	for (i = 0; i < Gi_len[0]; i++) {
		if ((*(Gi[0]+i))->trans_len > 0) {
			Gi_succ = realloc( Gi_succ, (Gi_succ_len + (*(Gi[0]+i))->trans_len)*sizeof(anode_t *) );
			if (Gi_succ == NULL) {
				perror( "add_metric_sysgoal, realloc" );
				return NULL;
			}
			for (j = 0; j < (*(Gi[0]+i))->trans_len; j++)
				*(Gi_succ+Gi_succ_len+j) = *((*(Gi[0]+i))->trans+j);
			Gi_succ_len += (*(Gi[0]+i))->trans_len;
		}

		(*(Gi[0]+i))->trans_len = 0;
		free( (*(Gi[0]+i))->trans );
		(*(Gi[0]+i))->trans = NULL;

		(*(Gi[0]+i))->mode = num_sgoals+1;  /* Temporary mode label for this component. */

		node1 = component_strategy;
		while (node1) {
			if (statecmp( node1->state, (*(Gi[0]+i))->state, num_env+num_sys )) {
				(*(Gi[0]+i))->trans_len = node1->trans_len;
				(*(Gi[0]+i))->trans = malloc( (node1->trans_len)*sizeof(anode_t *) );
				if ((*(Gi[0]+i))->trans == NULL) {
					perror( "add_metric_sysgoal, malloc" );
					return NULL;
				}
				for (j = 0; j < node1->trans_len; j++)
					*((*(Gi[0]+i))->trans+j) = *(node1->trans+j);
				break;
			}
			node1 = node1->next;
		}
		if (node1 == NULL) {
			fprintf( stderr, "Error add_metric_sysgoal: component automaton is not compatible with original." );
			delete_aut( strategy );
			return NULL;
		}

		/* Delete the obviated node */
		replace_anode_trans( strategy, node1, *(Gi[0]+i) );
		strategy = delete_anode( strategy, node1 );
	}

	if (verbose > 1) {
		logprint( "Partially patched strategy before de-expanding variables:" );
		logprint_startline();
		/* list_aut_dump( strategy, num_env+num_sys, getlogstream() ); */
		dot_aut_dump( strategy, evar_list, svar_list, DOT_AUT_BINARY | DOT_AUT_ATTRIB, getlogstream() );
		logprint_endline();
	}


	tmp = Cudd_ReadOne( manager );
	Cudd_Ref( tmp );
	component_strategy = synthesize_reachgame( manager, num_env, num_sys,
											   new_reached, new_reached_len,
											   Gi[1], Gi_len[1],
											   etrans, strans, egoals, tmp, verbose );
	Cudd_RecursiveDeref( manager, tmp );
	if (component_strategy == NULL) {
		delete_aut( strategy );
		return NULL;  /* Failure */
	}
	if (verbose > 1) {
		logprint( "Component strategy to reach G_{i*+1} from the new system goal:" );
		logprint_startline();
		/* list_aut_dump( component_strategy, num_env+num_sys, getlogstream() ); */
		dot_aut_dump( component_strategy, evar_list, svar_list, DOT_AUT_BINARY | DOT_AUT_ATTRIB, getlogstream() );
		logprint_endline();
	}

	/* Attach remaining pieces */
	node1 = strategy;
	while (node1->next)
		node1 = node1->next;
	node1->next = component_strategy;

	/* From first component to second component... */
	for (i = 0; i < new_reached_len; i++) {
		(*(new_reached+i))->mode = -1;  /* Temporary mode label for this component. */
		node1 = component_strategy;
		while (node1) {
			if (statecmp( node1->state, (*(new_reached+i))->state, num_env+num_sys )) {
				(*(new_reached+i))->trans_len = node1->trans_len;
				(*(new_reached+i))->trans = malloc( (node1->trans_len)*sizeof(anode_t *) );
				if ((*(new_reached+i))->trans == NULL) {
					perror( "add_metric_sysgoal, malloc" );
					return NULL;
				}
				for (j = 0; j < node1->trans_len; j++)
					*((*(new_reached+i))->trans+j) = *(node1->trans+j);
				(*(new_reached+i))->rgrad = node1->rgrad;
				break;
			}
			node1 = node1->next;
		}
		if (node1 == NULL) {
			fprintf( stderr, "Error add_metric_sysgoal: component automata are not compatible." );
			delete_aut( strategy );
			return NULL;
		}

		replace_anode_trans( strategy, node1, *(new_reached+i) );
		strategy = delete_anode( strategy, node1 );
	}

	/* From second component into original... */
	node1 = component_strategy;
	while (node1) {
		if (node1->trans_len == 0) {

			for (i = 0; i < Gi_len[1]; i++) {
				if (statecmp( (*(Gi[1]+i))->state, node1->state, num_env+num_sys )) {
					node1->trans_len = (*(Gi[1]+i))->trans_len;
					node1->trans = malloc( (node1->trans_len)*sizeof(anode_t *) );
					if (node1->trans == NULL) {
						perror( "add_metric_sysgoal, malloc" );
						return NULL;
					}
					for (j = 0; j < node1->trans_len; j++)
						*(node1->trans+j) = *((*(Gi[1]+i))->trans+j);
					node1->rgrad = (*(Gi[1]+i))->rgrad;
					node1->mode = (*(Gi[1]+i))->mode;
					break;
				}
			}
			if (i == Gi_len[1]) {
				fprintf( stderr, "Error add_metric_sysgoal: component automata are not compatible." );
				delete_aut( strategy );
				return NULL;
			}

		}
		node1 = node1->next;
	}

	strategy = forward_prune( strategy, Gi_succ, Gi_succ_len );
	if (strategy == NULL) {
		fprintf( stderr, "Error add_metric_sysgoal: pruning failed." );
		return NULL;
	}
	Gi_succ = NULL; Gi_succ_len = 0;

	/* Update labels, thus completing the insertion process */
	if (istar[0] == num_sgoals-1) {
		node1 = strategy;
		while (node1) {
			if (node1->mode == num_sgoals+1) {
				node1->mode = num_sgoals;
			} else if (node1->mode == -1) {
				node1->mode = 0;
			}
			node1 = node1->next;
		}
	} else {  /* istar[0] < istar[1] */
		for (i = num_sgoals-1; i >= istar[1]; i--) {
			node1 = strategy;
			while (node1) {
				if (node1->mode == i)
					(node1->mode)++;
				node1 = node1->next;
			}
		}
		node1 = strategy;
		while (node1) {
			if (node1->mode == num_sgoals+1) {
				node1->mode = istar[1];
			} else if (node1->mode == -1) {
				node1->mode = istar[1]+1;
			}
			node1 = node1->next;
		}
	}


	/* Pre-exit clean-up */
	free( Min );
	free( Max );
	free( Gi[0] );
	free( Gi[1] );
	free( new_reached );
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

	return strategy;
}
