/* interactive.c -- Functions to interact with gr1c (sub)level sets.
 *                  Also see solve.c and solve_operators.c
 *
 *
 * SCL; Mar 2012.
 */


#include <string.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "ptree.h"
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


int command_loop( FILE *infp, FILE *outfp )
{
	int num_env, num_sys;

	ptree_t *tmppt;
	int var_index;
	char *input;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	while (input = readline( "" )) {
		if (!strncmp( input, "quit", strlen( "quit" ) )) {
			break;
		} else if (!strncmp( input, "help", strlen( "help" ) )) {
			fprintf( outfp, "(not implemented yet.)" );
		} else if (!strncmp( input, "envvar", strlen( "envvar" ) )) {
			var_index = 0;
			if (evar_list == NULL) {
				fprintf( outfp, "(none)" );
			} else {
				tmppt = evar_list;
				while (tmppt) {
					if (tmppt->left == NULL) {
						fprintf( outfp, "%s (%d)", tmppt->name, var_index );
					} else {
						fprintf( outfp, "%s (%d), ", tmppt->name, var_index );
					}
					tmppt = tmppt->left;
					var_index++;
				}
			}
		} else if (!strncmp( input, "sysvar", strlen( "sysvar" ) )) {
			var_index = num_env;
			if (svar_list == NULL) {
				fprintf( outfp, "(none)" );
			} else {
				tmppt = svar_list;
				while (tmppt) {
					if (tmppt->left == NULL) {
						fprintf( outfp, "%s (%d)", tmppt->name, var_index );
					} else {
						fprintf( outfp, "%s (%d), ", tmppt->name, var_index );
					}
					tmppt = tmppt->left;
					var_index++;
				}
			}
		} else if (!strncmp( input, "var", strlen( "var" ) )) {
			var_index = 0;
			if (evar_list != NULL) {
				tmppt = evar_list;
				while (tmppt) {
					fprintf( outfp, "%s (%d), ", tmppt->name, var_index );
					tmppt = tmppt->left;
					var_index++;
				}
			}
			if (svar_list != NULL) {
				tmppt = svar_list;
				while (tmppt) {
					if (tmppt->left == NULL) {
						fprintf( outfp, "%s (%d)", tmppt->name, var_index );
					} else {
						fprintf( outfp, "%s (%d), ", tmppt->name, var_index );
					}
					tmppt = tmppt->left;
					var_index++;
				}
			}
		}

		fprintf( outfp, "\n" );
		free( input );
		fflush( outfp );
	}
	if (input != NULL)
		free( input );

	return 0;
}


int levelset_interactive( DdManager *manager, unsigned char init_flags,
						  FILE *infp, FILE *outfp,
						  unsigned char verbose )
{
	command_loop( infp, outfp );

	return 1;
}
