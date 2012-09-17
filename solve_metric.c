/* solve_metric.c -- synthesis routines that use a distance between states.
 *
 *
 * SCL; 2012.
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "common.h"
#include "logging.h"
#include "ptree.h"
#include "solve.h"
#include "solve_support.h"


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


extern int bitvec_to_int( bool *vec, int vec_len );  /* See util.c */

int bounds_state( DdManager *manager, DdNode *T, bool *ref_state, char *name,
				  double *Min, double *Max, unsigned char verbose )
{
	bool *state;
	int ref_mapped_x, this_mapped_x, ref_mapped_y, this_mapped_y;
	double dist;
	int num_env, num_sys;
	ptree_t *var = evar_list, *var_tail;
	int start_index, stop_index;
	int i;

	/* Variables used during CUDD generation (state enumeration). */
	DdGen *gen;
	CUDD_VALUE_TYPE gvalue;
	int *gcube;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	/* State vector (i.e., valuation of the variables) */
	state = malloc( sizeof(bool)*(num_env+num_sys) );
	if (state == NULL) {
		perror( "bounds_state, malloc" );
		return -1;
	}

	start_index = 0;
	while (var) {
		if (strstr( var->name, name ) == var->name)
			break;
		var = var->left;
		start_index++;
	}
	if (var == NULL) {
		var = svar_list;
		while (var) {
			if (strstr( var->name, name ) == var->name)
				break;
			var = var->left;
			start_index++;
		}
		if (var == NULL)
			return -1;  /* Could not find match. */
	}

	var_tail = var;
	stop_index = start_index;
	while (var_tail->left) {
		if (strstr( var_tail->left->name, name ) != var_tail->left->name )
			break;
		var_tail = var_tail->left;
		stop_index++;
	}

	/* ref_mapped = bitvec_to_int( ref_state+start_index, stop_index-start_index+1 ); */
	ref_mapped_y = bitvec_to_int( ref_state+8, 3 );
	ref_mapped_x = bitvec_to_int( ref_state+4, 4 );
	/* ref_mapped_x = bitvec_to_int( ref_state+7, 4 ); */
	/* ref_mapped_y = bitvec_to_int( ref_state+11, 3 ); */
	*Min = *Max = -1.;  /* Distance is non-negative; thus use -1 as "unset". */

	Cudd_AutodynDisable( manager );
	Cudd_ForeachCube( manager, T, gen, gcube, gvalue ) {
		initialize_cube( state, gcube, num_env+num_sys );
		while (!saturated_cube( state, gcube, num_env+num_sys )) {

			/* this_mapped = bitvec_to_int( state+start_index, stop_index-start_index+1 ); */
			this_mapped_x = bitvec_to_int( state+4, 4 );
			this_mapped_y = bitvec_to_int( state+8, 3 );
			/* this_mapped_x = bitvec_to_int( state+7, 4 ); */
			/* this_mapped_y = bitvec_to_int( state+11, 3 ); */
			dist = sqrt( pow((this_mapped_x-ref_mapped_x), 2) + pow((this_mapped_y-ref_mapped_y), 2) );
			if (*Min == -1. || dist < *Min)
				*Min = dist;
			if (*Max == -1. || dist > *Max)
				*Max = dist;

			/* logprint_startline(); */
			/* for (i = 0; i < num_env; i++) */
			/* 	logprint_raw( "%d", *(state+i) ); */
			/* logprint_raw( " " ); */
			/* for (i = num_env; i < num_env+num_sys; i++) */
			/* 	logprint_raw( "%d", *(state+i) ); */
			/* logprint_raw( "; %d -> %d", ref_mapped, this_mapped ); */
			/* logprint_endline(); */

			increment_cube( state, gcube, num_env+num_sys );
		}

		this_mapped_x = bitvec_to_int( state+4, 4 );
		this_mapped_y = bitvec_to_int( state+8, 3 );
		/* this_mapped_x = bitvec_to_int( state+7, 4 ); */
		/* this_mapped_y = bitvec_to_int( state+11, 3 ); */
		dist = sqrt( pow((this_mapped_x-ref_mapped_x), 2) + pow((this_mapped_y-ref_mapped_y), 2) );
		if (*Min == -1. || dist < *Min)
			*Min = dist;
		if (*Max == -1. || dist > *Max)
			*Max = dist;

	}
	Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
   
	free( state );
	return 0;
}

/* G is the goal set against which to measure distance.
   Result is written into given integer variables Min and Max;
   return 0 on success, -1 error. */
int bounds_DDset( DdManager *manager, DdNode *T, DdNode *G, char *name,
				  double *Min, double *Max, unsigned char verbose )
{
	bool **states = NULL;
	int num_states = 0;
	bool *state;
	double tMin, tMax;  /* Particular distance to goal set */
	int num_env, num_sys;
	ptree_t *var = evar_list, *var_tail;
	int start_index, stop_index;
	int i, k;

	/* Variables used during CUDD generation (state enumeration). */
	DdGen *gen;
	CUDD_VALUE_TYPE gvalue;
	int *gcube;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	state = malloc( (num_env+num_sys)*sizeof(bool) );
	if (state == NULL) {
		perror( "bounds_DDset, malloc" );
		return -1;
	}

	start_index = 0;
	while (var) {
		if (strstr( var->name, name ) == var->name)
			break;
		var = var->left;
		start_index++;
	}
	if (var == NULL) {
		var = svar_list;
		while (var) {
			if (strstr( var->name, name ) == var->name)
				break;
			var = var->left;
			start_index++;
		}
		if (var == NULL)
			return -1;  /* Could not find match. */
	}

	var_tail = var;
	stop_index = start_index;
	while (var_tail->left) {
		if (strstr( var_tail->left->name, name ) != var_tail->left->name )
			break;
		var_tail = var_tail->left;
		stop_index++;
	}

	*Min = *Max = -1.;  /* Distance is non-negative; thus use -1 as "unset". */

	Cudd_AutodynDisable( manager );
	Cudd_ForeachCube( manager, T, gen, gcube, gvalue ) {
		initialize_cube( state, gcube, num_env+num_sys );
		while (!saturated_cube( state, gcube, num_env+num_sys )) {
			
			num_states++;
			states = realloc( states, num_states*sizeof(bool *) );
			if (states == NULL) {
				perror( "bounds_DDset, realloc" );
				return -1;
			}
			*(states+num_states-1) = malloc( (num_env+num_sys)*sizeof(bool) );
			if (*(states+num_states-1) == NULL) {
				perror( "bounds_DDset, malloc" );
				return -1;
			}
			for (i = 0; i < num_env+num_sys; i++)
				*(*(states+num_states-1)+i) = *(state+i);

			increment_cube( state, gcube, num_env+num_sys );
		}

		num_states++;
		states = realloc( states, num_states*sizeof(bool *) );
		if (states == NULL) {
			perror( "bounds_DDset, realloc" );
			return -1;
		}
		
		*(states+num_states-1) = malloc( (num_env+num_sys)*sizeof(bool) );
		if (*(states+num_states-1) == NULL) {
			perror( "bounds_DDset, malloc" );
			return -1;
		}
		for (i = 0; i < num_env+num_sys; i++)
			*(*(states+num_states-1)+i) = *(state+i);

	}
	Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );

	for (k = 0; k < num_states; k++) {
		bounds_state( manager, G, *(states+k), name, &tMin, &tMax, verbose );
		if (*Min == -1. || tMin < *Min)
			*Min = tMin;
		if (*Max == -1. || tMin > *Max)
			*Max = tMin;

		/* logprint_startline(); */
		/* for (i = 0; i < num_env; i++) */
		/* 	logprint_raw( "%d", *(*(states+k)+i) ); */
		/* logprint_raw( " " ); */
		/* for (i = num_env; i < num_env+num_sys; i++) */
		/* 	logprint_raw( "%d", *(*(states+k)+i) ); */
		/* logprint_raw( "; mi = %f", tMin ); */
		/* logprint_endline(); */

		logprint( "%d,%d; %d,%d; mi = %f",
				  bitvec_to_int( *(states+k), 2 ),
				  bitvec_to_int( *(states+k)+2, 2 ),
				  bitvec_to_int( *(states+k)+4, 4 ),
				  bitvec_to_int( *(states+k)+8, 3 ),
				  /* bitvec_to_int( *(states+k), 4 ), */
				  /* bitvec_to_int( *(states+k)+4, 3 ), */
				  /* bitvec_to_int( *(states+k)+7, 4 ), */
				  /* bitvec_to_int( *(states+k)+11, 3 ), */
				  tMin );
	}
   
	for (i = 0; i < num_states; i++)
		free( *(states+i) );
	free( states );
	free( state );
	return 0;
}


int compute_minmax( DdManager *manager, DdNode **W, DdNode **etrans, DdNode **strans, DdNode ***sgoals,
					int **num_sublevels, double ***Min, double ***Max,
					unsigned char verbose )
{
	DdNode **egoals;
	ptree_t *var_separator;
	DdNode ***Y = NULL;
	DdNode ****X_ijr = NULL;
	bool env_nogoal_flag = False;
	int i, j, r;
	DdNode *tmp, *tmp2;

	if (num_egoals == 0) {
		env_nogoal_flag = True;
		num_egoals = 1;
		env_goals = malloc( sizeof(ptree_t *) );
		*env_goals = init_ptree( PT_CONSTANT, NULL, 1 );
	}

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

	if (verbose)
		logprint( "Building environment transition BDD..." );
	(*etrans) = ptree_BDD( env_trans, evar_list, manager );
	if (verbose) {
		logprint( "Done." );
		logprint( "Building system transition BDD..." );
	}
	(*strans) = ptree_BDD( sys_trans, evar_list, manager );
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
		(*sgoals) = malloc( num_sgoals*sizeof(DdNode *) );
		for (i = 0; i < num_sgoals; i++)
			*((*sgoals)+i) = ptree_BDD( *(sys_goals+i), evar_list, manager );
	} else {
		(*sgoals) = NULL;
	}

	if (var_separator == NULL) {
		evar_list = NULL;
	} else {
		var_separator->left = NULL;
	}

	*W = compute_winning_set_BDD( manager, (*etrans), (*strans), egoals, (*sgoals), verbose );
	if (*W == NULL) {
		fprintf( stderr, "Error compute_minmax: failed to construct winning set.\n" );
		return -1;
	}
	Y = compute_sublevel_sets( manager, *W, (*etrans), (*strans),
							   egoals, num_egoals,
							   (*sgoals), num_sgoals,
							   num_sublevels, &X_ijr, verbose );
	if (Y == NULL) {
		fprintf( stderr, "Error compute_minmax: failed to construct sublevel sets.\n" );
		return -1;
	}

	*Min = malloc( num_sgoals*sizeof(double *) );
	*Max = malloc( num_sgoals*sizeof(double *) );
	if (*Min == NULL || *Max == NULL) {
		perror( "compute_minmax, malloc" );
		return -1;
	}

	for (i = 0; i < num_sgoals; i++) {
		*(*Min + i) = malloc( (*(*num_sublevels+i)-1)*sizeof(double) );
		*(*Max + i) = malloc( (*(*num_sublevels+i)-1)*sizeof(double) );
		if (*(*Min + i) == NULL || *(*Max + i) == NULL) {
			perror( "compute_minmax, malloc" );
			return -1;
		}
		for (j = 1; j < *(*num_sublevels+i); j++) {
			logprint( "goal %d, level %d...", i, j );
			tmp = Cudd_bddAnd( manager, *(*(Y+i)+j), Cudd_Not( *(*(Y+i)+j-1) ) );
			Cudd_Ref( tmp );
			tmp2 = Cudd_bddAnd( manager, tmp, *etrans );
			Cudd_Ref( tmp2 );
			Cudd_RecursiveDeref( manager, tmp );
			tmp = Cudd_bddAnd( manager, tmp2, *strans );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, tmp2 );
			if (bounds_DDset( manager, tmp, **(Y+i), "x", *(*Min+i)+j-1, *(*Max+i)+j-1,
							  verbose )) {
				*(*(*Min+i)+j-1) = *(*(*Max+i)+j-1) = -1.;
			}
			Cudd_RecursiveDeref( manager, tmp );
		}
	}


	/* Pre-exit clean-up */
	for (i = 0; i < num_egoals; i++)
		Cudd_RecursiveDeref( manager, *(egoals+i) );
	if (num_egoals > 0)
		free( egoals );
	if (env_nogoal_flag) {
		num_egoals = 0;
		delete_tree( *env_goals );
		free( env_goals );
	}
	for (i = 0; i < num_sgoals; i++) {
		for (j = 0; j < *(*num_sublevels+i); j++) {
			Cudd_RecursiveDeref( manager, *(*(Y+i)+j) );
			for (r = 0; r < num_egoals; r++) {
				Cudd_RecursiveDeref( manager, *(*(*(X_ijr+i)+j)+r) );
			}
			free( *(*(X_ijr+i)+j) );
		}
		if (*(*num_sublevels+i) > 0) {
			free( *(Y+i) );
			free( *(X_ijr+i) );
		}
	}
	if (num_sgoals > 0) {
		free( Y );
		free( X_ijr );
	}

	return 0;
}


int compute_horizon( DdManager *manager, DdNode **W, DdNode **etrans, DdNode **strans, DdNode ***sgoals, unsigned char verbose )
{
	int horizon = -1, horiz_j;
	int *num_sublevels;
	double **Min, **Max;
	int i, j, k;

	if (compute_minmax( manager, W, etrans, strans, sgoals, &num_sublevels, &Min, &Max, verbose ))
		return -1;  /* Error in compute_minmax() */

	for (i = 0; i < num_sgoals; i++) {
		for (j = 1; j < *(num_sublevels+i); j++) {
			logprint_startline();
			logprint_raw( "goal %d, level %d: ", i, j );
			logprint_raw( "%f, %f", *(*(Min+i)+j-1), *(*(Max+i)+j-1) );
			logprint_endline();
		}
	}

	for (i = 0; i < num_sgoals; i++) {
		for (j = 3; j < *(num_sublevels+i); j++) {
			horiz_j = 1;
			for (k = j-2; k >= 1; k--) {
				if (*(*(Max+i)+k-1) >= *(*(Min+i)+j-1))
					horiz_j = j-k;
			}
			if (horiz_j > horizon)
				horizon = horiz_j;
		}
	}

	if (num_sgoals > 0) {
		free( num_sublevels );
		for (i = 0; i < num_sgoals; i++) {
			free( *(Min+i) );
			free( *(Max+i) );
		}
		free( Min );
		free( Max );
	}
	return horizon;
}
