/* sim.c -- Definitions for signatures appearing in sim.h.
 *
 *
 * SCL; 2012.
 */


#include <stdlib.h>

#include "sim.h"
#include "common.h"
#include "automaton.h"
#include "solve_support.h"


extern ptree_t *evar_list;
extern ptree_t *svar_list;
extern ptree_t *env_trans;
extern ptree_t *sys_trans;
extern ptree_t **env_goals;
extern ptree_t **sys_goals;
extern int num_egoals;
extern int num_sgoals;


anode_t *sim_rhc( DdManager *manager, DdNode *W, DdNode *etrans, DdNode *strans, int horizon, bool *init_state, int num_it )
{
	anode_t *play;
	int num_env, num_sys;
	bool *next_state;
	int current_it = 0, i;
	bool **env_moves;
	int emoves_len, emove_index;
	DdNode *strans_into_W;

	int *cube;  /* length will be twice total number of variables (to
				   account for both variables and their primes). */
	DdNode *tmp, *tmp2;

	/* Variables used during CUDD generation (state enumeration). */
	DdGen *gen;
	CUDD_VALUE_TYPE gvalue;
	int *gcube;
	DdNode **vars, **pvars;

	if (init_state == NULL)
		return NULL;

	srand( 0 );
	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

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

	next_state = malloc( (num_env+num_sys)*sizeof(bool) );
	if (next_state == NULL) {
		perror( "sim_rhc, malloc" );
		return NULL;
	}

	cube = malloc( 2*(num_env+num_sys)*sizeof(int) );
	if (cube == NULL) {
		perror( "sim_rhc, malloc" );
		return NULL;
	}

	tmp = Cudd_bddVarMap( manager, W );
	if (tmp == NULL) {
		fprintf( stderr, "Error synthesize: Error in swapping variables with primed forms.\n" );
		return NULL;
	}
	Cudd_Ref( tmp );
	strans_into_W = Cudd_bddAnd( manager, strans, tmp );
	Cudd_Ref( strans_into_W );
	Cudd_RecursiveDeref( manager, tmp );

	play = insert_anode( NULL, current_it, -1, init_state, num_env+num_sys );
	while (current_it < num_it) {
		current_it++;

		env_moves = get_env_moves( manager, cube,
								   init_state, etrans,
								   num_env, num_sys,
								   &emoves_len );
		emove_index = rand() % emoves_len;

		tmp = state2cof( manager, cube, 2*(num_env+num_sys), init_state, strans_into_W, 0, num_env+num_sys );
		tmp2 = state2cof( manager, cube, 2*(num_env+num_sys), *(env_moves+emove_index), tmp, num_env+num_sys, num_env );
		Cudd_RecursiveDeref( manager, tmp );

		Cudd_AutodynDisable( manager );
		gen = Cudd_FirstCube( manager, tmp2, &gcube, &gvalue );
		if (gen == NULL) {
			fprintf( stderr, "Error sim_rhc: failed to find cube.\n" );
			return NULL;
		}
		for (i = 0; i < 2*(num_env+num_sys); i++)
			*(cube+i) = *(gcube+i);
		Cudd_GenFree( gen );
		Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );
		Cudd_RecursiveDeref( manager, tmp2 );
		initialize_cube( next_state, cube+num_env+num_sys, num_env+num_sys );
		for (i = 0; i < num_env; i++)
			*(next_state+i) = *(*(env_moves+emove_index)+i);

		play = insert_anode( play, current_it, -1, next_state, num_env+num_sys );
		play = append_anode_trans( play, current_it-1, init_state, num_env+num_sys, current_it, next_state );

		for (i = 0; i < num_env+num_sys; i++)
			*(init_state+i) = *(next_state+i);
		for (i = 0; i < emoves_len; i++)
			free( *(env_moves+i) );
		free( env_moves );
	}
	

	Cudd_RecursiveDeref( manager, strans_into_W );
	free( next_state );
	return play;
}
