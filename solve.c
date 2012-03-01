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
DdNode *check_realizable( DdManager *manager, unsigned char init_flags,
						  unsigned char verbose )
{
	bool realizable;
	ptree_t *var_separator;
	DdNode *einit, *sinit;
	DdNode *etrans, *strans, **egoals, **sgoals;
	DdNode *X = NULL, *X_prev = NULL;
	DdNode *Y = NULL, *Y_exmod = NULL, *Y_prev = NULL;
	DdNode **Z = NULL, **Z_prev = NULL;
	bool Z_changed;  /* Use to detect occurrence of fixpoint for all Z_i */

	/* Fixpoint iteration counters */
	int num_it_Z, num_it_Y, num_it_X;

	DdNode *tmp, *tmp2;
	int i, j;  /* Generic counters */

	DdNode **vars, **pvars;
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
		Z = malloc( num_sgoals*sizeof(DdNode *) );
		Z_prev = malloc( num_sgoals*sizeof(DdNode *) );
		sgoals = malloc( num_sgoals*sizeof(DdNode *) );
		for (i = 0; i < num_sgoals; i++) {
			*(Z+i) = NULL;
			*(Z_prev+i) = NULL;
			*(sgoals+i) = ptree_BDD( *(sys_goals+i), evar_list, manager );
		}
	} else {
		sgoals = NULL;
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

	/* Initialize */
	for (i = 0; i < num_sgoals; i++) {
		*(Z+i) = Cudd_ReadOne( manager );
		Cudd_Ref( *(Z+i) );
	}

	num_it_Z = 0;
	do {
		num_it_Z++;
		if (verbose) {
			printf( "Z iteration %d\n", num_it_Z );
			fflush( stdout );
		}

		for (i = 0; i < num_sgoals; i++) {
			if (*(Z_prev+i) != NULL)
				Cudd_RecursiveDeref( manager, *(Z_prev+i) );
			*(Z_prev+i) = *(Z+i);
		}
			
		for (i = 0; i < num_sgoals; i++) {
			if (i == num_sgoals-1) {
				*(Z+i) = compute_existsmodal( manager, *Z_prev, etrans, strans,
											  num_env, num_sys, cube );
			} else {
				*(Z+i) = compute_existsmodal( manager, *(Z_prev+i+1), etrans, strans,
											  num_env, num_sys, cube );
			}
			if (*(Z+i) == NULL) {
				/* fatal error */
				return NULL;
			}

			/* (Re)initialize Y */
			if (Y != NULL)
				Cudd_RecursiveDeref( manager, Y );
			Y = Cudd_Not( Cudd_ReadOne( manager ) );
			Cudd_Ref( Y );

			num_it_Y = 0;
			do {
				num_it_Y++;
				if (verbose) {
					printf( "\tY iteration %d\n", num_it_Y );
					fflush( stdout );
				}

				if (Y_prev != NULL)
					Cudd_RecursiveDeref( manager, Y_prev );
				Y_prev = Y;
				if (Y_exmod != NULL)
					Cudd_RecursiveDeref( manager, Y_exmod );
				Y_exmod = compute_existsmodal( manager, Y, etrans, strans,
											   num_env, num_sys, cube );
				if (Y_exmod == NULL) {
					/* fatal error */
					return NULL;
				}
				
				if (num_egoals > 0) {
					Y = Cudd_Not( Cudd_ReadOne( manager ) );
					Cudd_Ref( Y );
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
						num_it_X = 0;
						do {
							num_it_X++;
							if (verbose) {
								printf( "\t\tX iteration %d\n", num_it_X );
								fflush( stdout );
							}

							X_prev = X;
							X = compute_existsmodal( manager, X, etrans, strans,
													 num_env, num_sys, cube );
							if (X == NULL) {
								/* fatal error */
								return NULL;
							}
								
							tmp = Cudd_bddAnd( manager, *(sgoals+i), *(Z+i) );
							Cudd_Ref( tmp );
							tmp2 = Cudd_bddOr( manager, tmp, Y_exmod );
							Cudd_Ref( tmp2 );
							Cudd_RecursiveDeref( manager, tmp );

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
								
						} while (Cudd_bddCorrelation( manager, X, X_prev ) < 1.);

						tmp = Y;
						Y = Cudd_bddOr( manager, Y, X );
						Cudd_Ref( Y );
						Cudd_RecursiveDeref( manager, tmp );

					}
				} else {

					tmp = Cudd_bddAnd( manager, *(sgoals+i), *(Z+i) );
					Cudd_Ref( tmp );
					Y = Cudd_bddOr( manager, tmp, Y_exmod );
					Cudd_Ref( Y );
					Cudd_RecursiveDeref( manager, tmp );

				}

				tmp2 = Y;
				Y = Cudd_bddOr( manager, Y, Y_prev );
				Cudd_Ref( Y );
				Cudd_RecursiveDeref( manager, tmp2 );

			} while (Cudd_bddCorrelation( manager, Y, Y_prev ) < 1.);

			*(Z+i) = Cudd_bddAnd( manager, Y, *(Z_prev+i) );
			Cudd_Ref( *(Z+i) );

		}

		Z_changed = False;
		for (i = 0; i < num_sgoals; i++) {
			if (Cudd_bddCorrelation( manager, *(Z+i), *(Z_prev+i) ) < 1.) {
				Z_changed = True;
				break;
			}
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
	if (init_flags == ALL_SYS_INIT) {
		tmp = Cudd_bddAnd( manager, einit, sinit );
		Cudd_Ref( tmp );
		tmp2 = Cudd_bddAnd( manager, tmp, *Z );
		Cudd_Ref( tmp2 );
		if (Cudd_bddCorrelation( manager, tmp, tmp2 ) < 1.) {
			realizable = False;
		} else {
			realizable = True;
		}
		Cudd_RecursiveDeref( manager, tmp );
		Cudd_RecursiveDeref( manager, tmp2 );
	} else { /* EXIST_SYS_INIT */
		tmp = Cudd_bddAnd( manager, sinit, *Z );
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

		if (Cudd_bddCorrelation( manager, tmp2, Cudd_ReadOne( manager ) ) < 1.) {
			realizable = False;
		} else {
			realizable = True;
		}
		Cudd_RecursiveDeref( manager, tmp2 );
	}

	/* Pre-exit clean-up */
	tmp = *Z;
	Cudd_RecursiveDeref( manager, *Z_prev );
	for (i = 1; i < num_sgoals; i++) {
		Cudd_RecursiveDeref( manager, *(Z+i) );
		Cudd_RecursiveDeref( manager, *(Z_prev+i) );
	}
	free( Z );
	free( Z_prev );
	Cudd_RecursiveDeref( manager, einit );
	Cudd_RecursiveDeref( manager, sinit );
	Cudd_RecursiveDeref( manager, etrans );
	Cudd_RecursiveDeref( manager, strans );
	for (i = 0; i < num_egoals; i++)
		Cudd_RecursiveDeref( manager, *(egoals+i) );
	for (i = 0; i < num_sgoals; i++)
		Cudd_RecursiveDeref( manager, *(sgoals+i) );
	if (egoals != NULL)
		free( egoals );
	if (sgoals != NULL)
		free( sgoals );
	free( cube );

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
	DdNode *tmp, *tmp2;
	DdNode *ddcube;

	C = Cudd_bddVarMap( manager, C );
	if (C == NULL) {
		fprintf( stderr, "compute_existsmodal: Error in swapping variables with primed forms." );
		return NULL;
	}
	Cudd_Ref( C );

	tmp = Cudd_bddAnd( manager, strans, C );
	Cudd_Ref( tmp );
	Cudd_RecursiveDeref( manager, C );
	cube_prime_sys( cube, num_env, num_sys );
	ddcube = Cudd_CubeArrayToBdd( manager, cube );
	if (ddcube == NULL) {
		fprintf( stderr, "compute_existsmodal: Error in generating cube for quantification." );
		return NULL;
	}
	Cudd_Ref( ddcube );
	tmp2 = Cudd_bddExistAbstract( manager, tmp, ddcube );
	if (tmp2 == NULL) {
		fprintf( stderr, "compute_existsmodal: Error in performing quantification." );
		return NULL;
	}
	Cudd_Ref( tmp2 );
	Cudd_RecursiveDeref( manager, ddcube );
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
	Cudd_Ref( ddcube );
	tmp2 = Cudd_bddUnivAbstract( manager, tmp, ddcube );
	if (tmp2 == NULL) {
		fprintf( stderr, "compute_existsmodal: Error in performing quantification." );
		return NULL;
	}
	Cudd_Ref( tmp2 );
	Cudd_RecursiveDeref( manager, ddcube );
	Cudd_RecursiveDeref( manager, tmp );
	return tmp2;
}
