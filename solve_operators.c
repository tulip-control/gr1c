/* solve_operators.c -- Functions for several mu-calculus operations.
 *                      Also see solve.c
 *
 *
 * SCL; Feb, Mar 2012.
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "solve.h"


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


/* Internal routines for setting typical cube arrays. */
void cube_env( int *cube, int num_env, int num_sys );
void cube_sys( int *cube, int num_env, int num_sys );
void cube_prime_env( int *cube, int num_env, int num_sys );
void cube_prime_sys( int *cube, int num_env, int num_sys );


/* Compute exists modal operator applied to set C. */
DdNode *compute_existsmodal( DdManager *manager, DdNode *C,
							 DdNode *etrans, DdNode *strans,
							 int num_env, int num_sys, int *cube );


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


/* N.B., we assume there is at least one system goal.  This assumption
   will be removed in a future version (soon). */
DdNode *compute_winning_set( DdManager *manager, unsigned char verbose )
{
	int i;
	ptree_t *var_separator;
	DdNode *W;  /* Characteristic function of winning set */
	DdNode *etrans, *strans, **egoals, **sgoals;
	bool env_nogoal_flag = False;  /* Indicate environment has no goals */
	
	/* Set environment goal to True (i.e., any state) if none was
	   given. This simplifies the implementation below. */
	if (num_egoals == 0) {
		env_nogoal_flag = True;
		num_egoals = 1;
		env_goals = malloc( sizeof(ptree_t *) );
		*env_goals = init_ptree( PT_CONSTANT, NULL, 1 );
	}

	if (verbose) {
		printf( "== Cudd_PrintInfo(), called from compute_winning_set =================\n" );
		Cudd_PrintInfo( manager, stdout );
		printf( "======================================================================\n" );
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

	/* Break the link that appended the system variables list to the
	   environment variables list. */
	if (var_separator == NULL) {
		evar_list = NULL;
	} else {
		var_separator->left = NULL;
	}

	W = compute_winning_set_BDD( manager, etrans, strans, egoals, sgoals, verbose );

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

	return W;
}


DdNode *compute_winning_set_BDD( DdManager *manager,
								 DdNode *etrans, DdNode *strans,
								 DdNode **egoals, DdNode **sgoals,
								 unsigned char verbose )
{
	ptree_t *var_separator;
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

	if (verbose) {
		printf( "== Cudd_PrintInfo(), called from compute_winning_set_BDD =============\n" );
		Cudd_PrintInfo( manager, stdout );
		printf( "======================================================================\n" );
	}

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	/* Allocate cube array, used later for quantifying over variables. */
	cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
	if (cube == NULL) {
		perror( "check_realizable_internal, malloc" );
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

	if (num_sgoals > 0) {
		Z = malloc( num_sgoals*sizeof(DdNode *) );
		Z_prev = malloc( num_sgoals*sizeof(DdNode *) );
		for (i = 0; i < num_sgoals; i++) {
			*(Z+i) = NULL;
			*(Z_prev+i) = NULL;
		}
	}

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
				Y_exmod = compute_existsmodal( manager, Y_prev, etrans, strans,
											   num_env, num_sys, cube );
				if (Y_exmod == NULL) {
					/* fatal error */
					return NULL;
				}
				
				Y = Cudd_Not( Cudd_ReadOne( manager ) );
				Cudd_Ref( Y );
				for (j = 0; j < num_egoals; j++) {
					
					/* (Re)initialize X */
					if (X != NULL)
						Cudd_RecursiveDeref( manager, X );
					X = Cudd_ReadOne( manager );
					Cudd_Ref( X );

					/* Greatest fixpoint for X, for this env goal */
					num_it_X = 0;
					do {
						num_it_X++;
						if (verbose) {
							printf( "\t\tX iteration %d\n", num_it_X );
							fflush( stdout );
						}
						
						if (X_prev != NULL)
							Cudd_RecursiveDeref( manager, X_prev );
						X_prev = X;
						X = compute_existsmodal( manager, X_prev, etrans, strans,
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

					} while (!(Cudd_bddLeq( manager, X, X_prev )*Cudd_bddLeq( manager, X_prev, X )));

					tmp = Y;
					Y = Cudd_bddOr( manager, Y, X );
					Cudd_Ref( Y );
					Cudd_RecursiveDeref( manager, tmp );

					Cudd_RecursiveDeref( manager, X );
					X = NULL;
					Cudd_RecursiveDeref( manager, X_prev );
					X_prev = NULL;
				}

				tmp2 = Y;
				Y = Cudd_bddOr( manager, Y, Y_prev );
				Cudd_Ref( Y );
				Cudd_RecursiveDeref( manager, tmp2 );

			} while (!(Cudd_bddLeq( manager, Y, Y_prev )*Cudd_bddLeq( manager, Y_prev, Y )));

			Cudd_RecursiveDeref( manager, *(Z+i) );
			*(Z+i) = Cudd_bddAnd( manager, Y, *(Z_prev+i) );
			Cudd_Ref( *(Z+i) );

			Cudd_RecursiveDeref( manager, Y );
			Y = NULL;
			Cudd_RecursiveDeref( manager, Y_prev );
			Y_prev = NULL;
			Cudd_RecursiveDeref( manager, Y_exmod );
			Y_exmod = NULL;

		}

		Z_changed = False;
		for (i = 0; i < num_sgoals; i++) {
			if (!(Cudd_bddLeq( manager, *(Z+i), *(Z_prev+i) )*Cudd_bddLeq( manager, *(Z_prev+i), *(Z+i) ))) {
				Z_changed = True;
				break;
			}
		}
	} while (Z_changed);

	if (verbose) {
		printf( "== Cudd_PrintInfo(), called from compute_winning_set_BDD =============\n" );
		Cudd_PrintInfo( manager, stdout );
		printf( "======================================================================\n" );
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
	free( cube );

	return tmp;
}


DdNode ***compute_sublevel_sets( DdManager *manager,
								 DdNode *W,
								 DdNode *etrans, DdNode *strans,
								 DdNode **egoals, DdNode **sgoals,
								 int **num_sublevels,
								 unsigned char verbose )
{
	DdNode ***Y = NULL, *Y_exmod;
	DdNode *X = NULL, *X_prev = NULL;

	DdNode **vars, **pvars;
	int num_env, num_sys;
	int *cube;

	DdNode *tmp, *tmp2;
	int i, j;

	if (verbose) {
		printf( "== Cudd_PrintInfo(), called from compute_sublevel_sets ===============\n" );
		Cudd_PrintInfo( manager, stdout );
		printf( "======================================================================\n" );
	}

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	cube = (int *)malloc( sizeof(int)*2*(num_env+num_sys) );
	if (cube == NULL) {
		perror( "compute_sublevel_sets, malloc" );
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

	if (num_sgoals > 0) {
		Y = malloc( num_sgoals*sizeof(DdNode **) );
		*num_sublevels = malloc( num_sgoals*sizeof(int) );
		if (Y == NULL || *num_sublevels == NULL) {
			perror( "compute_sublevel_sets, malloc" );
			return NULL;
		}
		for (i = 0; i < num_sgoals; i++) {
			*(*num_sublevels+i) = 1;
			*(Y+i) = malloc( *(*num_sublevels+i)*sizeof(DdNode *) );
			if (*(Y+i) == NULL) {
				perror( "compute_sublevel_sets, malloc" );
				return NULL;
			}
			**(Y+i) = Cudd_bddAnd( manager, *(sgoals+i), W );
			Cudd_Ref( **(Y+i) );
		}
	} else {
		return NULL;
	}

	/* Build list of Y_i sets from iterations of the fixpoint formula. */
	for (i = 0; i < num_sgoals; i++) {
		while (True) {
			(*(*num_sublevels+i))++;
			*(Y+i) = realloc( *(Y+i), *(*num_sublevels+i)*sizeof(DdNode *) );
			if (*(Y+i) == NULL) {
				perror( "synthesize, realloc" );
				return NULL;
			}

			Y_exmod = compute_existsmodal( manager, *(*(Y+i)+*(*num_sublevels+i)-2),
										   etrans, strans,
										   num_env, num_sys, cube );

			*(*(Y+i)+*(*num_sublevels+i)-1) = Cudd_Not( Cudd_ReadOne( manager ) );
			Cudd_Ref( *(*(Y+i)+*(*num_sublevels+i)-1) );
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
								
					tmp = Cudd_bddAnd( manager, *(sgoals+i), W );
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

				} while (!(Cudd_bddLeq( manager, X, X_prev )*Cudd_bddLeq( manager, X_prev, X )));

				tmp = *(*(Y+i)+*(*num_sublevels+i)-1);
				*(*(Y+i)+*(*num_sublevels+i)-1) = Cudd_bddOr( manager, *(*(Y+i)+*(*num_sublevels+i)-1), X );
				Cudd_Ref( *(*(Y+i)+*(*num_sublevels+i)-1) );
				Cudd_RecursiveDeref( manager, tmp );
			
				Cudd_RecursiveDeref( manager, X );
				X = NULL;
				Cudd_RecursiveDeref( manager, X_prev );
				X_prev = NULL;
			}

			if (*(*num_sublevels+i) > 1) {
				tmp = *(*(Y+i)+*(*num_sublevels+i)-1);
				*(*(Y+i)+*(*num_sublevels+i)-1) = Cudd_bddOr( manager, *(*(Y+i)+*(*num_sublevels+i)-1), *(*(Y+i)+*(*num_sublevels+i)-2) );
				Cudd_Ref( *(*(Y+i)+*(*num_sublevels+i)-1) );
				Cudd_RecursiveDeref( manager, tmp );
			}

			if (*(*num_sublevels+i) > 1
				&& Cudd_bddLeq( manager, *(*(Y+i)+*(*num_sublevels+i)-1), *(*(Y+i)+*(*num_sublevels+i)-2))*Cudd_bddLeq( manager, *(*(Y+i)+*(*num_sublevels+i)-2), *(*(Y+i)+*(*num_sublevels+i)-1) )) {
				Cudd_RecursiveDeref( manager, *(*(Y+i)+*(*num_sublevels+i)-1) );
				(*(*num_sublevels+i))--;
				*(Y+i) = realloc( *(Y+i), *(*num_sublevels+i)*sizeof(DdNode *) );
				if (*(Y+i) == NULL) {
					perror( "synthesize, realloc" );
					return NULL;
				}
				break;
			}
		}
		Cudd_RecursiveDeref( manager, Y_exmod );
	}

	if (verbose) {
		printf( "== Cudd_PrintInfo(), called from compute_sublevel_sets ===============\n" );
		Cudd_PrintInfo( manager, stdout );
		printf( "======================================================================\n" );
	}

	return Y;
}
