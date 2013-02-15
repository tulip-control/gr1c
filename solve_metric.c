/* solve_metric.c -- synthesis routines that use a distance between states.
 *
 *
 * SCL; 2012, 2013.
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


/** Return an array of length 2n, where n is the number of entries in
   metric_vars.  The values at indices 2i and 2i+1 are the offset and
   width of the i-th variable's binary representation.  Return NULL if
   error.  metric_vars is a space-separated list of variables to use
   in computing distance. The caller is expected to free the array.

   This function assumes that evar_list and svar_list have not been linked. */
int *get_offsets( char *metric_vars, int *num_vars )
{
	char *var_str, *tok = NULL;
	int *offw = NULL;
	ptree_t *var, *var_tail;
	int start_index, stop_index;

	if (metric_vars == NULL)
		return NULL;
	var_str = strdup( metric_vars );
	if (var_str == NULL) {
		perror( "get_offsets, strdup" );
		return NULL;
	}

	*num_vars = 0;
	tok = strtok( var_str, " " );
	if (tok == NULL) {
		free( var_str );
		return NULL;
	}
	do {
		(*num_vars)++;
		offw = realloc( offw, 2*(*num_vars)*sizeof(int) );
		if (offw == NULL) {
			perror( "get_offsets, realloc" );
			return NULL;
		}

		var = evar_list;
		start_index = 0;
		while (var) {
			if (strstr( var->name, tok ) == var->name)
				break;
			var = var->left;
			start_index++;
		}
		if (var == NULL) {
			var = svar_list;
			while (var) {
				if (strstr( var->name, tok ) == var->name)
					break;
				var = var->left;
				start_index++;
			}
			if (var == NULL) {
				fprintf( stderr, "Could not find match for \"%s\"\n", tok );
				free( offw );
				free( var_str );
				return NULL;
			}
		}

		var_tail = var;
		stop_index = start_index;
		while (var_tail->left) {
			if (strstr( var_tail->left->name, tok ) != var_tail->left->name )
				break;
			var_tail = var_tail->left;
			stop_index++;
		}

		*(offw+2*((*num_vars)-1)) = start_index;
		*(offw+2*((*num_vars)-1)+1) = stop_index-start_index+1;
	} while (tok = strtok( NULL, " " ));

	free( var_str );
	return offw;
}


int bounds_state( DdManager *manager, DdNode *T, bool *ref_state,
				  int *offw, int num_metric_vars,
				  double *Min, double *Max, unsigned char verbose )
{
	bool *state;
	double dist;
	int num_env, num_sys;
	int i;
	int *ref_mapped, *this_mapped;

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

	/* Reference and particular integral vectors */
	ref_mapped = malloc( num_metric_vars*sizeof(int) );
	this_mapped = malloc( num_metric_vars*sizeof(int) );
	if (ref_mapped == NULL || this_mapped == NULL) {
		perror( "bounds_state, malloc" );
		return -1;
	}

	for (i = 0; i < num_metric_vars; i++)
		*(ref_mapped+i) = bitvec_to_int( ref_state+(*(offw+2*i)),
										 *(offw+2*i+1) );
	*Min = *Max = -1.;  /* Distance is non-negative; thus use -1 as "unset". */

	Cudd_AutodynDisable( manager );
	Cudd_ForeachCube( manager, T, gen, gcube, gvalue ) {
		initialize_cube( state, gcube, num_env+num_sys );
		while (!saturated_cube( state, gcube, num_env+num_sys )) {

			for (i = 0; i < num_metric_vars; i++)
				*(this_mapped+i) = bitvec_to_int( state+(*(offw+2*i)),
												  *(offw+2*i+1) );

			/* 2-norm derived metric */
			/* dist = 0.; */
			/* for (i = 0; i < num_metric_vars; i++) */
			/* 	dist += pow(*(this_mapped+i) - *(ref_mapped+i), 2); */
			/* dist = sqrt( dist ); */

			/* 1-norm derived metric */
			dist = 0.;
			for (i = 0; i < num_metric_vars; i++)
				dist += fabs(*(this_mapped+i) - *(ref_mapped+i));
			if (*Min == -1. || dist < *Min)
				*Min = dist;
			if (*Max == -1. || dist > *Max)
				*Max = dist;

			increment_cube( state, gcube, num_env+num_sys );
		}

		for (i = 0; i < num_metric_vars; i++)
			*(this_mapped+i) = bitvec_to_int( state+(*(offw+2*i)),
											  *(offw+2*i+1) );

		/* 2-norm derived metric */
		/* dist = 0.; */
		/* for (i = 0; i < num_metric_vars; i++) */
		/* 	dist += pow(*(this_mapped+i) - *(ref_mapped+i), 2); */
		/* dist = sqrt( dist ); */

		/* 1-norm derived metric */
		dist = 0.;
		for (i = 0; i < num_metric_vars; i++)
			dist += fabs(*(this_mapped+i) - *(ref_mapped+i));
		if (*Min == -1. || dist < *Min)
			*Min = dist;
		if (*Max == -1. || dist > *Max)
			*Max = dist;

	}
	Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
   
	free( state );
	free( ref_mapped );
	free( this_mapped );
	return 0;
}

/* G is the goal set against which to measure distance.
   Result is written into given integer variables Min and Max;
   return 0 on success, -1 error. */
int bounds_DDset( DdManager *manager, DdNode *T, DdNode *G,
				  int *offw, int num_metric_vars,
				  double *Min, double *Max, unsigned char verbose )
{
	bool **states = NULL;
	int num_states = 0;
	bool *state;
	double tMin, tMax;  /* Particular distance to goal set */
	int num_env, num_sys;
	int i, k;
	int *mapped_state;

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

	mapped_state = malloc( num_metric_vars*sizeof(int) );
	if (mapped_state == NULL) {
		perror( "bounds_DDset, malloc" );
		return -1;
	}
	for (k = 0; k < num_states; k++) {
		bounds_state( manager, G, *(states+k), offw, num_metric_vars, &tMin, &tMax, verbose );
		if (*Min == -1. || tMin < *Min)
			*Min = tMin;
		if (*Max == -1. || tMin > *Max)
			*Max = tMin;

		if (verbose > 1) {
			for (i = 0; i < num_metric_vars; i++)
				*(mapped_state+i) = bitvec_to_int( *(states+k)+(*(offw+2*i)),
												   *(offw+2*i+1) );
			logprint_startline();
			logprint_raw( "\t" );
			for (i = 0; i < num_metric_vars; i++)
				logprint_raw( "%d, ", *(mapped_state+i) );
			logprint_raw( "mi = %f", tMin );
			logprint_endline();
		}
	}

	free( mapped_state );
	for (i = 0; i < num_states; i++)
		free( *(states+i) );
	free( states );
	free( state );
	return 0;
}


/* Construct BDDs (characteristic functions of) etrans, strans,
   egoals, and sgoals as required by compute_winning_set_BDD() but
   save the result.  The motivating use-case is to compute these once
   and then provide them to later functions as needed. */
DdNode *compute_winning_set_saveBDDs( DdManager *manager, DdNode **etrans, DdNode **strans,
									  DdNode ***egoals, DdNode ***sgoals,
									  unsigned char verbose )
{
	int i;
	ptree_t *var_separator;
	DdNode *W;

	if (num_egoals == 0) {
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
		(*egoals) = malloc( num_egoals*sizeof(DdNode *) );
		for (i = 0; i < num_egoals; i++)
			*((*egoals)+i) = ptree_BDD( *(env_goals+i), evar_list, manager );
	} else {
		(*egoals) = NULL;
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

	W = compute_winning_set_BDD( manager, (*etrans), (*strans), (*egoals), (*sgoals), verbose );
	if (W == NULL) {
		fprintf( stderr, "Error compute_winning_set_saveBDDs: failed to construct winning set.\n" );
		return NULL;
	}

	return W;
}


int compute_minmax( DdManager *manager, DdNode **W, DdNode **etrans, DdNode **strans, DdNode ***sgoals,
					int **num_sublevels, double ***Min, double ***Max,
					int *offw, int num_metric_vars,
					unsigned char verbose )
{
	DdNode **egoals;
	ptree_t *var_separator;
	DdNode ***Y = NULL;
	DdNode ****X_ijr = NULL;
	bool env_nogoal_flag = False;
	int i, j, r;
	DdNode *tmp, *tmp2;

	if (num_egoals == 0)
		env_nogoal_flag = True;

	*W = compute_winning_set_saveBDDs( manager, etrans, strans, &egoals, sgoals, verbose );
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

		*(*(*Min+i)) = *(*(*Max+i)) = 0;
		for (j = 1; j < *(*num_sublevels+i)-1; j++) {
			if (verbose > 1)
				logprint( "goal %d, level %d...", i, j );
			tmp = Cudd_bddAnd( manager, *(*(Y+i)+j+1), Cudd_Not( *(*(Y+i)+j) ) );
			Cudd_Ref( tmp );
			tmp2 = Cudd_bddAnd( manager, *((*sgoals)+i), *W );
			Cudd_Ref( tmp2 );
			if (bounds_DDset( manager, tmp, tmp2, offw, num_metric_vars, *(*Min+i)+j, *(*Max+i)+j,
							  verbose )) {
				*(*(*Min+i)+j) = *(*(*Max+i)+j) = -1.;
			}
			Cudd_RecursiveDeref( manager, tmp );
			Cudd_RecursiveDeref( manager, tmp2 );
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


int compute_horizon( DdManager *manager, DdNode **W,
					 DdNode **etrans, DdNode **strans, DdNode ***sgoals,
					 char *metric_vars, unsigned char verbose )
{
	int horizon = -1, horiz_j;
	int *num_sublevels;
	double **Min, **Max;
	int i, j, k;
	int *offw, num_metric_vars;

	offw = get_offsets( metric_vars, &num_metric_vars );
	if (offw == NULL)
		return -1;

	if (compute_minmax( manager, W, etrans, strans, sgoals,
						&num_sublevels, &Min, &Max, offw, num_metric_vars,
						verbose ))
		return -1;  /* Error in compute_minmax() */

	if (verbose) {
		for (i = 0; i < num_sgoals; i++) {
			for (j = 0; j < *(num_sublevels+i)-1; j++) {
				logprint_startline();
				logprint_raw( "goal %d, level %d: ", i, j );
				logprint_raw( "%f, %f", *(*(Min+i)+j), *(*(Max+i)+j) );
				logprint_endline();
			}
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

	free( offw );
	return horizon;
}
