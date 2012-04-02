/* patching.c -- Definitions for signatures appearing in patching.h.
 *
 *
 * SCL; Apr 2012.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "patching.h"


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

/* See interactive.c for definition and documentation. */
extern int read_state_str( char *input, bool **state, int max_len );


anode_t *find_anode_ID( anode_t *head, int ID )
{
	int i;
	if (head == NULL || i < 0)
		return NULL;
	for (i = 0; i < ID && head != NULL; i++)
		head = head->next;
	return head;
}


#define INPUT_STRING_LEN 256

anode_t *patch_localfixpoint( DdManager *manager, FILE *strategy_fp, FILE *change_fp, unsigned char verbose )
{
	ptree_t *var_separator;
	DdNode *einit, *sinit, *etrans, *strans, **egoals, **sgoals;
	int num_env, num_sys;

	DdNode *vertex1, *vertex2; /* ...regarding vertices of the game graph. */
	char line[INPUT_STRING_LEN];
	bool *state;

	bool env_nogoal_flag = False;  /* Indicate environment has no goals */

	int i, j;  /* Generic counters */
	DdNode *tmp, *tmp2;
	int num_read;
	char *start, *end;
	anode_t *strategy;
	anode_t *node;
	int *N = NULL;  /* "neighborhood" of nodes (a list of node IDs). */
	int N_len = 0;
	int N_mode = -1;  /* Goal mode of the automaton nodes in the neighborhood */

	anode_t *affected = NULL;  /* Array of pointers to nodes directly
								  affected by edge set change. */
	int affected_len = 0;

	if (change_fp == NULL)
		return NULL;  /* Require game changes to be listed in an open stream. */

	if (strategy_fp == NULL)
		strategy_fp = stdin;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	strategy = aut_aut_load( num_env+num_sys, strategy_fp );
	if (strategy == NULL) {
		return NULL;
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

	/* Process game graph changes */
	N_len = 1;
	N = malloc( sizeof(int)*N_len);
	if (N == NULL) {
		perror( "patch_localfixpoint, malloc" );
		return NULL;
	}
	while (fgets( line, INPUT_STRING_LEN, change_fp )) {
		if (strlen( line ) < 1 || *line == '\n')  /* Blank line? */
			continue;

		*(N+N_len-1) = strtol( line, &end, 10 );
		if (end != line) {
			do {
				N_len++;
				N = realloc( N, sizeof(int)*N_len );
				if (N == NULL) {
					perror( "patch_localfixpoint, realloc" );
					return NULL;
				}
				start = end;
				*(N+N_len-1) = strtol( start, &end, 10 );
			} while (end != start && *end != '\0');

			node = find_anode_ID( strategy, *N );
			if (node == NULL) {
				fprintf( stderr, "Error: invalid node ID in neighborhood.\n" );
				return NULL;
			}
			N_mode = node->mode;

			if (verbose) {
				printf( "Neighborhood goal mode of %d with node IDs:", N_mode );
				for (i = 0; i < N_len-1; i++)
					printf( " %d", *(N+i) );
				printf( "\n" );
			}
		} else if (!strncmp( line, "restrict ", strlen( "restrict " ) ) || !strncmp( line, "relax ", strlen( "relax " ) )) {
			if (N_mode < 0) {
				fprintf( stderr, "Error: command encountered before goal mode set.\n" );
				return NULL;
			}

			if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
				num_read = read_state_str( line+strlen( "restrict" )+1, &state, 2*(num_env+num_sys) );
			} else { /* "relax " */
				num_read = read_state_str( line+strlen( "relax" )+1, &state, 2*(num_env+num_sys) );
			}
			if (num_read != 2*(num_env+num_sys) && num_read != 2*num_env+num_sys) {
				if (num_read > 0)
					free( state );
				fprintf( stderr, "Error: invalid arguments to restrict or relax command.\n" );
				return NULL;
			}
			if (verbose) {
				if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
					if (num_read == 2*(num_env+num_sys)) {
						printf( "Removing controlled edge from" );
					} else {
						printf( "Removing uncontrolled edge from" );
					}
				} else { /* "relax " */
					if (num_read == 2*(num_env+num_sys)) {
						printf( "Adding controlled edge from" );
					} else {
						printf( "Adding uncontrolled edge from" );
					}
				}
				for (i = 0; i < num_env+num_sys; i++)
					printf( " %d", *(state+i) );
				printf( " to" );
				for (i = num_env+num_sys; i < num_read; i++)
					printf( " %d", *(state+i) );
				printf( "\n" );
			}

			vertex1 = Cudd_ReadOne( manager );
			Cudd_Ref( vertex1 );
			for (i = 0; i < num_env+num_sys; i++) {
				if (*(state+i)) {
					tmp = Cudd_bddAnd( manager, vertex1,
									   Cudd_bddIthVar( manager, i ) );
				} else {
					tmp = Cudd_bddAnd( manager, vertex1,
									   Cudd_Not( Cudd_bddIthVar( manager, i ) ) );
				}
				Cudd_Ref( tmp );
				Cudd_RecursiveDeref( manager, vertex1 );
				vertex1 = tmp;
			}
			vertex2 = Cudd_ReadOne( manager );
			Cudd_Ref( vertex2 );
			for (i = num_env+num_sys; i < num_read; i++) {
				if (*(state+i)) {
					tmp = Cudd_bddAnd( manager, vertex2,
									   Cudd_bddIthVar( manager, i ) );
				} else {
					tmp = Cudd_bddAnd( manager, vertex2,
									   Cudd_Not( Cudd_bddIthVar( manager, i ) ) );
				}
				Cudd_Ref( tmp );
				Cudd_RecursiveDeref( manager, vertex2 );
				vertex2 = tmp;
			}
			tmp = Cudd_Not( vertex1 );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, vertex1 );
			vertex1 = tmp;
			if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
				tmp = Cudd_Not( vertex2 );
				Cudd_Ref( tmp );
				Cudd_RecursiveDeref( manager, vertex2 );
				vertex2 = tmp;
			}
			tmp = Cudd_bddOr( manager, vertex1, vertex2 );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, vertex1 );
			Cudd_RecursiveDeref( manager, vertex2 );
			if (num_read == 2*num_env+num_sys) {
				if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
					tmp2 = Cudd_bddAnd( manager, tmp, etrans );
				} else { /* "relax " */
					tmp2 = Cudd_bddOr( manager, tmp, etrans );
				}
				Cudd_Ref( tmp2 );
				Cudd_RecursiveDeref( manager, etrans );
				etrans = tmp2;
			} else { /* num_read == 2*(num_env+num_sys) */
				if (!strncmp( line, "restrict ", strlen( "restrict " ) )) {
					tmp2 = Cudd_bddAnd( manager, tmp, strans );
				} else { /* "relax " */
					tmp2 = Cudd_bddOr( manager, tmp, strans );
				}
				Cudd_Ref( tmp2 );
				Cudd_RecursiveDeref( manager, strans );
				strans = tmp2;
			}
			Cudd_RecursiveDeref( manager, tmp );
			free( state );

		} else if (!strncmp( line, "blocksys ", strlen( "blocksys " ) )) {
			if (N_mode < 0) {
				fprintf( stderr, "Error: command encountered before goal mode set.\n" );
				return NULL;
			}

			num_read = read_state_str( line+strlen( "blocksys " )+1, &state, num_sys );
			if (num_read != num_sys) {
				if (num_read > 0)
					free( state );
				fprintf( stderr, "Error: invalid arguments to blocksys command.\n" );
				return NULL;
			}
			if (verbose) {
				printf( "Removing system moves into" );
				for (i = 0; i < num_sys; i++)
					printf( " %d", *(state+i) );
				printf( "\n" );
			}

			vertex2 = Cudd_ReadOne( manager );
			Cudd_Ref( vertex2 );
			for (i = 0; i < num_sys; i++) {
				if (*(state+i)) {
					tmp = Cudd_bddAnd( manager, vertex2,
									   Cudd_bddIthVar( manager, 2*num_env+num_sys+i ) );
				} else {
					tmp = Cudd_bddAnd( manager, vertex2,
									   Cudd_Not( Cudd_bddIthVar( manager, 2*num_env+num_sys+i ) ) );
				}
				Cudd_Ref( tmp );
				Cudd_RecursiveDeref( manager, vertex2 );
				vertex2 = tmp;
			}
			tmp = Cudd_Not( vertex2 );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, vertex2 );
			vertex2 = tmp;

			tmp = Cudd_bddAnd( manager, strans, vertex2 );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, strans );
			strans = tmp;

			Cudd_RecursiveDeref( manager, vertex2 );
			free( state );

		} else if (!strncmp( line, "blockenv ", strlen( "blockenv " ) )) {
			if (N_mode < 0) {
				fprintf( stderr, "Error: command encountered before goal mode set.\n" );
				return NULL;
			}

			num_read = read_state_str( line+strlen( "blockenv " )+1, &state, num_env );
			if (num_read != num_env) {
				if (num_read > 0)
					free( state );
				fprintf( stderr, "Error: invalid arguments to blockenv command.\n" );
				return NULL;
			}
			if (verbose) {
				printf( "Removing environment moves into" );
				for (i = 0; i < num_env; i++)
					printf( " %d", *(state+i) );
				printf( "\n" );
			}
			vertex2 = Cudd_ReadOne( manager );
			Cudd_Ref( vertex2 );
			for (i = 0; i < num_env; i++) {
				if (*(state+i)) {
					tmp = Cudd_bddAnd( manager, vertex2,
									   Cudd_bddIthVar( manager, num_env+num_sys+i ) );
				} else {
					tmp = Cudd_bddAnd( manager, vertex2,
									   Cudd_Not( Cudd_bddIthVar( manager, num_env+num_sys+i ) ) );
				}
				Cudd_Ref( tmp );
				Cudd_RecursiveDeref( manager, vertex2 );
				vertex2 = tmp;
			}
			tmp = Cudd_Not( vertex2 );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, vertex2 );
			vertex2 = tmp;

			tmp = Cudd_bddAnd( manager, etrans, vertex2 );
			Cudd_Ref( tmp );
			Cudd_RecursiveDeref( manager, etrans );
			etrans = tmp;

			Cudd_RecursiveDeref( manager, vertex2 );
			free( state );

		} else {
			fprintf( stderr, "WARNING: unrecognized line in given edge change file.\n" );
			continue;
		}

		if (verbose)
			fflush( stdout );
	}
	N_len--;  /* Remove unused tail */
	N = realloc( N, sizeof(int)*N_len );
	if (N == NULL) {
		perror( "patch_localfixpoint, realloc" );
		return NULL;
	}

	

	/* Pre-exit clean-up */
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
	if (env_nogoal_flag) {
		num_egoals = 0;
		delete_tree( *env_goals );
		free( env_goals );
	}

	return strategy;
}
