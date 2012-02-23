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


/* Currently only can handle case of single system goal and no
   environment goals.  Let f be the system goal. Thus the mu-calculus
   formula to find the winning set is

     \nu Z ( \mu Y ( (E)Y | f & (E)Z ))

   where (E) is the exists modal operator (for all environment
   actions, there exists a system action). */
DdNode *check_feasible( DdManager *manager )
{
	bool feasible;
	ptree_t *var_separator;
	DdNode *einit, *sinit;
	DdNode *etrans, *strans, *sgoal;
	DdNode *Y = NULL, *Z = NULL, *Y_prev = NULL, *Z_prev = NULL;
	DdNode *tmp, *tmp2;

	int num_env, num_sys;
	int *cube;  /* length will be twice total number of variables (to
				   account for both variables and their primes). */
	DdNode *ddcube;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	/* Allocate cube array, used later for quantifying over variables. */
	cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
	if (cube == NULL) {
		perror( "check_feasible, malloc" );
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
	sgoal = ptree_BDD( *sys_goals, evar_list, manager );

	/* Initialize */
	Z = Cudd_ReadOne( manager );
	Cudd_Ref( Z );

	/* To detect fix point. */
	Z_prev = Cudd_ReadOne( manager );
	Cudd_Ref( Z_prev );
	
	do {

		Z_prev = Z;

		/* (Re)initialize Y */
		if (Y != NULL)
			Cudd_RecursiveDeref( manager, Y );
		if (Y_prev != NULL)
			Cudd_RecursiveDeref( manager, Y_prev );
		Y = Cudd_Not( Cudd_ReadOne( manager ) );
		Y_prev = Cudd_Not( Cudd_ReadOne( manager ) );
		Cudd_Ref( Y );
		Cudd_Ref( Y_prev );

		Z = compute_existsmodal( manager, Z, etrans, strans,
								 num_env, num_sys, cube );
		if (Z == NULL) {
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

			tmp = Cudd_bddOr( manager, Y, sgoal );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, Y );
			Y = Cudd_bddAnd( manager, tmp, Z );
			Cudd_Ref( Y );
			Cudd_RecursiveDeref( manager, tmp );

			Y = Cudd_bddOr( manager, Y, Y_prev );
			Cudd_Ref( Y );

		} while (Cudd_bddCorrelation( manager, Y, Y_prev ) < 1.);

		Z = Cudd_bddAnd( manager, Y, Z_prev );
		Cudd_Ref( Z );

	} while (Cudd_bddCorrelation( manager, Z, Z_prev ) < 1.);
	Cudd_RecursiveDeref( manager, Z_prev );

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
	tmp2 = Cudd_bddAnd( manager, tmp, Z );
	Cudd_Ref( tmp2 );
	if (Cudd_bddCorrelation( manager, tmp, tmp2 ) < 1.) {  /* Not feasible */
		feasible = False;
	} else {  /* Feasible */
		feasible = True;
	}
	Cudd_RecursiveDeref( manager, tmp );
	Cudd_RecursiveDeref( manager, tmp2 );

	/* Pre-exit clean-up */
	free( cube );

	if (feasible) {
		return Z;
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
