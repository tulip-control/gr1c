/* solve.c -- Definitions for signatures appearing in solve.h.
 *
 *
 * SCL; Feb 2012.
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "solve.h"

typedef unsigned char bool;
#define True 1
#define False 0

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


/* Internal routines for setting typial cube arrays. */
void cube_env( int *cube, int num_env, int num_sys );
void cube_sys( int *cube, int num_env, int num_sys );
void cube_prime_env( int *cube, int num_env, int num_sys );
void cube_prime_sys( int *cube, int num_env, int num_sys );

/* Compute exists modal operator of set C. */
DdNode *compute_existsmodal( DdManager *manager, DdNode *C,
							 DdNode *etrans, DdNode *strans,
							 int num_env, int num_sys, int *cube );


/* N.B., we assume there is at least one system goal.  This assumption
   will be removed in a future version (soon). */
DdNode *check_realizable( DdManager *manager )
{
	bool realizable;
	ptree_t *var_separator;
	DdNode *einit, *sinit;
	DdNode *etrans, *strans, **egoals, **sgoals;
	DdNode *Y = NULL, *X = NULL, *Y_prev = NULL, *X_prev = NULL;
	DdNode **Z, *Z_prev;
	/* Z_i for the system goals.  Only one "previous" is needed for
	   fixpoint detection. */
	bool Z_changed;  /* Indicator flag while looping over greatest
					    fixpoint computations of Z_i. */
	int num_it;  /* Count number iterations to arrive at fixpoint. */

	DdNode *tmp, *tmp2;
	int i, j;  /* Generic counters */

	int num_env, num_sys;
	int *cube;  /* length will be twice total number of variables (to
				   account for both variables and their primes). */
	DdNode *ddcube;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	/* Allocate cube array, used later for quantifying over variables. */
	cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
	if (cube == NULL) {
		perror( "check_realizable, malloc" );
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
	etrans = ptree_BDD( env_trans, evar_list, manager );
	strans = ptree_BDD( sys_trans, evar_list, manager );

	/* Build goal BDDs, if present. */
	if (num_egoals > 0) {
		egoals = malloc( num_egoals*sizeof(DdNode *) );
		for (i = 0; i < num_egoals; i++)
			*(egoals+i) = ptree_BDD( *(env_goals+i), evar_list, manager );
	} else {
		egoals = NULL;
	}
	if (num_sgoals > 0) {
		Z = malloc( num_sgoals*sizeof(DdNode *) );
		sgoals = malloc( num_sgoals*sizeof(DdNode *) );
		for (i = 0; i < num_sgoals; i++)
			*(sgoals+i) = ptree_BDD( *(sys_goals+i), evar_list, manager );
	} else {
		Z = malloc( sizeof(DdNode *) );
		sgoals = NULL;
	}
	

	/* Initialize */
	for (i = 0; i < num_sgoals; i++) {
		*(Z+i) = Cudd_ReadOne( manager );
		Cudd_Ref( *(Z+i) );
	}

	do {
		Z_changed = False;
		for (i = 0; i < num_sgoals; i++) {

			/* To detect fixpoint for this Z_i */
			Z_prev = Cudd_ReadOne( manager );
			Cudd_Ref( Z_prev );
			num_it = 0;
	
			do {
				num_it++;

				Z_prev = *(Z+i);

				/* (Re)initialize Y */
				if (Y != NULL)
					Cudd_RecursiveDeref( manager, Y );
				if (Y_prev != NULL)
					Cudd_RecursiveDeref( manager, Y_prev );
				Y = Cudd_Not( Cudd_ReadOne( manager ) );
				Y_prev = Cudd_Not( Cudd_ReadOne( manager ) );
				Cudd_Ref( Y );
				Cudd_Ref( Y_prev );

				if (i == num_sgoals-1) {
					*(Z+i) = compute_existsmodal( manager, *Z, etrans, strans,
												  num_env, num_sys, cube );
				} else {
					*(Z+i) = compute_existsmodal( manager, *(Z+i+1), etrans, strans,
												  num_env, num_sys, cube );
				}
				if (*(Z+i) == NULL) {
					/* fatal error */
					return NULL;
				}

				do {

					Y_prev = Y;

					Y = compute_existsmodal( manager, Y, etrans, strans,
											 num_env, num_sys, cube );
					if (Y == NULL) {
						/* fatal error */
						return NULL;
					}

					if (num_egoals > 0) {
						for (j = 0; j < num_egoals; j++) {

							/* (Re)initialize X */
							if (X != NULL)
								Cudd_RecursiveDeref( manager, X );
							if (X_prev != NULL)
								Cudd_RecursiveDeref( manager, X_prev );
							X = Cudd_ReadOne( manager );
							X_prev = Cudd_ReadOne( manager );
							Cudd_Ref( X );
							Cudd_Ref( X_prev );

							/* Greatest fixpoint for X, for this env goal */
							do {

								X_prev = X;
							   
								X = compute_existsmodal( manager, X, etrans, strans,
														 num_env, num_sys, cube );
								if (X == NULL) {
									/* fatal error */
									return NULL;
								}
								
								tmp = Cudd_bddAnd( manager, *(sgoals+i), *(Z+i) );
								Cudd_Ref( tmp );
								tmp2 = Cudd_bddOr( manager, tmp, Y );
								Cudd_Ref( tmp2 );
								Cudd_RecursiveDeref( manager, tmp );

								tmp = Cudd_bddOr( manager, tmp2, Cudd_Not( *(egoals+j) ) );
								Cudd_Ref( tmp );
								Cudd_RecursiveDeref( manager, tmp2 );

								X = Cudd_bddAnd( manager, tmp, X );
								Cudd_Ref( X );
								Cudd_RecursiveDeref( manager, tmp );

								X = Cudd_bddAnd( manager, X, X_prev );
								Cudd_Ref( X );
								
							} while (Cudd_bddCorrelation( manager, X, X_prev ) < 1.);

							Y = Cudd_bddOr( manager, Y, X );
							Cudd_Ref( Y );

						}
					} else {

						tmp = Cudd_bddAnd( manager, *(sgoals+i), *(Z+i) );
						Cudd_Ref( tmp );
						Y = Cudd_bddOr( manager, tmp, Y );
						Cudd_Ref( Y );
						Cudd_RecursiveDeref( manager, tmp );

						Y = Cudd_bddOr( manager, Y, Y_prev );
						Cudd_Ref( Y );
						
					}

				} while (Cudd_bddCorrelation( manager, Y, Y_prev ) < 1.);

				*(Z+i) = Cudd_bddAnd( manager, Y, Z_prev );
				Cudd_Ref( *(Z+i) );

			} while (Cudd_bddCorrelation( manager, *(Z+i), Z_prev ) < 1.);
			Cudd_RecursiveDeref( manager, Z_prev );

			if (num_it > 1)
				Z_changed = True;

		}
	} while (Z_changed);

	/* Break the link that appended the system variables list to the
	   environment variables list. */
	if (var_separator == NULL) {
		evar_list = NULL;
	} else {
		var_separator->left = NULL;
	}

	/* Does winning set contain all initial states? */
	tmp = Cudd_bddAnd( manager, einit, sinit );
	Cudd_Ref( tmp );
	tmp2 = Cudd_bddAnd( manager, tmp, *Z );
	Cudd_Ref( tmp2 );
	if (Cudd_bddCorrelation( manager, tmp, tmp2 ) < 1.) {  /* Not realizable */
		realizable = False;
	} else {  /* Realizable */
		realizable = True;
	}
	Cudd_RecursiveDeref( manager, tmp );
	Cudd_RecursiveDeref( manager, tmp2 );

	/* Pre-exit clean-up */
	free( cube );
	if (egoals != NULL)
		free( egoals );
	if (sgoals != NULL)
		free( sgoals );

	tmp = *Z;
	free( Z );
	if (realizable) {
		return tmp;
	} else {
		return NULL;
	}
}


void cube_env( int *cube, int num_env, int num_sys )
{
	int i;
	for (i = num_env; i < 2*(num_env+num_sys); i++)
		*(cube+i) = 2;

	for (i = 0; i < num_env; i++)
		*(cube+i) = 1;
}

void cube_sys( int *cube, int num_env, int num_sys )
{
	int i;
	for (i = 0; i < 2*(num_env+num_sys); i++)
		*(cube+i) = 2;

	for (i = num_env; i < num_env+num_sys; i++)
		*(cube+i) = 1;
}

void cube_prime_env( int *cube, int num_env, int num_sys )
{
	int i;
	for (i = 0; i < 2*(num_env+num_sys); i++)
		*(cube+i) = 2;

	for (i = num_env+num_sys; i < 2*num_env+num_sys; i++)
		*(cube+i) = 1;
}

void cube_prime_sys( int *cube, int num_env, int num_sys )
{
	int i;
	for (i = 0; i < 2*num_env+num_sys; i++)
		*(cube+i) = 2;

	for (i = 2*num_env+num_sys; i < 2*(num_env+num_sys); i++)
		*(cube+i) = 1;
}


DdNode *compute_existsmodal( DdManager *manager, DdNode *C,
							 DdNode *etrans, DdNode *strans,
							 int num_env, int num_sys, int *cube )
{
	int i;
	DdNode *tmp, *tmp2;
	DdNode *ddcube;

	/* Indicate permutation using cube array. */
	for (i = 0; i < num_env+num_sys; i++)
		*(cube+i) = num_env+num_sys+i;
	for (i = num_env+num_sys; i < 2*(num_env+num_sys); i++)
		*(cube+i) = i-(num_env+num_sys);
	C = Cudd_bddPermute( manager, C, cube );

	tmp = Cudd_bddAnd( manager, strans, C );
	Cudd_Ref( tmp );
	Cudd_RecursiveDeref( manager, C );
	cube_prime_sys( cube, num_env, num_sys );
	ddcube = Cudd_CubeArrayToBdd( manager, cube );
	if (ddcube == NULL) {
		fprintf( stderr, "compute_existsmodal: Error in generating cube for quantification." );
		return NULL;
	}
	tmp2 = Cudd_bddExistAbstract( manager, tmp, ddcube );
	if (tmp2 == NULL) {
		fprintf( stderr,
				 "compute_existsmodal: Error in performing quantification." );
		return NULL;
	}
	Cudd_Ref( tmp2 );
	Cudd_RecursiveDeref( manager, tmp );

	tmp = Cudd_bddOr( manager, Cudd_Not( etrans ), tmp2 );
	Cudd_Ref( tmp );
	Cudd_RecursiveDeref( manager, tmp2 );
	cube_prime_env( cube, num_env, num_sys );
	ddcube = Cudd_CubeArrayToBdd( manager, cube );
	if (ddcube == NULL) {
		fprintf( stderr, "compute_existsmodal: Error in generating cube for quantification." );
		return NULL;
	}
	tmp2 = Cudd_bddUnivAbstract( manager, tmp, ddcube );
	if (tmp2 == NULL) {
		fprintf( stderr,
				 "compute_existsmodal: Error in performing quantification." );
		return NULL;
	}
	Cudd_Ref( tmp2 );
	Cudd_RecursiveDeref( manager, tmp );
	return tmp2;
}
