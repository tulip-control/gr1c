/* automaton_io.c -- Definitions for input and output routines on automaton objects.
 *
 *
 * SCL; 2012.
 */


#include <stdlib.h>
#include <stdio.h>

#include "automaton.h"


int dot_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
				  unsigned char format_flags, FILE *fp )
{
	int i, j, last_nonzero_env, last_nonzero_sys;
	anode_t *node;
	int node_counter = 0;
	ptree_t *var;
	int num_env, num_sys;

	if (fp == NULL)
		fp = stdout;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	fprintf( fp, "/* strategy synthesized with gr1c, version " GR1C_VERSION " */\n" );
	fprintf( fp, "digraph A {\n" );
	node = head;
	while (node) {
		for (i = 0; i < node->trans_len; i++) {
			fprintf( fp, "    \"%d;\\n", node_counter );
			if ((format_flags & 0x1) == DOT_AUT_ALL) {
				last_nonzero_env = num_env-1;
				last_nonzero_sys = num_sys-1;
			} else {
				for (last_nonzero_env = num_env-1; last_nonzero_env >= 0
						 && *(node->state+last_nonzero_env) == 0; last_nonzero_env--) ;
				for (last_nonzero_sys = num_sys-1; last_nonzero_sys >= 0
						 && *(node->state+num_env+last_nonzero_sys) == 0; last_nonzero_sys--) ;
			}
			if (last_nonzero_env < 0 && last_nonzero_sys < 0) {
				fprintf( fp, "{}" );
			} else {
				if (!(format_flags & DOT_AUT_EDGEINPUT)) {
					for (j = 0; j < num_env; j++) {
						if ((format_flags & DOT_AUT_BINARY) && *(node->state+j) == 0)
							continue;
						var = get_list_item( evar_list, j );
						if (j == last_nonzero_env) {
							if (format_flags & DOT_AUT_BINARY) {
								fprintf( fp, "%s", var->name );
							} else {
								fprintf( fp, "%s=%d", var->name, *(node->state+j) );
							}
						} else {
							if (format_flags & DOT_AUT_BINARY) {
								fprintf( fp, "%s, ", var->name );
							} else {
								fprintf( fp, "%s=%d, ", var->name, *(node->state+j) );
							}
						}
					}
				}
				if (last_nonzero_sys < 0 && (format_flags & DOT_AUT_EDGEINPUT)) {
					fprintf( fp, "{}" );
				} else {
					for (j = 0; j < num_sys; j++) {
						if ((format_flags & DOT_AUT_BINARY) && *(node->state+num_env+j) == 0)
							continue;
						var = get_list_item( svar_list, j );
						if (j == last_nonzero_sys) {
							if (format_flags & DOT_AUT_BINARY) {
								fprintf( fp, "%s", var->name );
							} else {
								fprintf( fp, "%s=%d", var->name, *(node->state+num_env+j) );
							}
						} else {
							if (format_flags & DOT_AUT_BINARY) {
								fprintf( fp, "%s, ", var->name );
							} else {
								fprintf( fp, "%s=%d, ", var->name, *(node->state+num_env+j) );
							}
						}
					}
				}
			}
			fprintf( fp, "\" -> \"%d;\\n",
					 find_anode_index( head,
									   (*(node->trans+i))->mode,
									   (*(node->trans+i))->state, num_env+num_sys ) );
			if ((format_flags & 0x1) == DOT_AUT_ALL) {
				last_nonzero_env = num_env-1;
				last_nonzero_sys = num_sys-1;
			} else {
				for (last_nonzero_env = num_env-1; last_nonzero_env >= 0
						 && *((*(node->trans+i))->state+last_nonzero_env) == 0; last_nonzero_env--) ;
				for (last_nonzero_sys = num_sys-1; last_nonzero_sys >= 0
						 && *((*(node->trans+i))->state+num_env+last_nonzero_sys) == 0; last_nonzero_sys--) ;
			}
			if (last_nonzero_env < 0 && last_nonzero_sys < 0) {
				fprintf( fp, "{}" );
			} else {
				if (!(format_flags & DOT_AUT_EDGEINPUT)) {
					for (j = 0; j < num_env; j++) {
						if ((format_flags & DOT_AUT_BINARY) && *((*(node->trans+i))->state+j) == 0)
							continue;
						var = get_list_item( evar_list, j );
						if (j == last_nonzero_env) {
							if (format_flags & DOT_AUT_BINARY) {
								fprintf( fp, "%s", var->name );
							} else {
								fprintf( fp, "%s=%d", var->name, *((*(node->trans+i))->state+j) );
							}
						} else {
							if (format_flags & DOT_AUT_BINARY) {
								fprintf( fp, "%s, ", var->name );
							} else {
								fprintf( fp, "%s=%d, ", var->name, *((*(node->trans+i))->state+j) );
							}
						}
					}
				}
				if (last_nonzero_sys < 0 && (format_flags & DOT_AUT_EDGEINPUT)) {
					fprintf( fp, "{}" );
				} else {
					for (j = 0; j < num_sys; j++) {
						if ((format_flags & DOT_AUT_BINARY) && *((*(node->trans+i))->state+num_env+j) == 0)
							continue;
						var = get_list_item( svar_list, j );
						if (j == last_nonzero_sys) {
							if (format_flags & DOT_AUT_BINARY) {
								fprintf( fp, "%s", var->name );
							} else {
								fprintf( fp, "%s=%d", var->name, *((*(node->trans+i))->state+num_env+j) );
							}
						} else {
							if (format_flags & DOT_AUT_BINARY) {
								fprintf( fp, "%s, ", var->name );
							} else {
								fprintf( fp, "%s=%d, ", var->name, *((*(node->trans+i))->state+num_env+j) );
							}
						}
					}
				}
			}
			fprintf( fp, "\"" );
			if (format_flags & DOT_AUT_EDGEINPUT) {
				fprintf( fp, "[label=\"" );
				if (last_nonzero_env < 0) {
					fprintf( fp, "{}" );
				} else {
					for (j = 0; j < num_env; j++) {
						if ((format_flags & DOT_AUT_BINARY) && *((*(node->trans+i))->state+j) == 0)
							continue;
						var = get_list_item( evar_list, j );
						if (j == last_nonzero_env) {
							if (format_flags & DOT_AUT_BINARY) {
								fprintf( fp, "%s", var->name );
							} else {
								fprintf( fp, "%s=%d", var->name, *((*(node->trans+i))->state+j) );
							}
						} else {
							if (format_flags & DOT_AUT_BINARY) {
								fprintf( fp, "%s, ", var->name );
							} else {
								fprintf( fp, "%s=%d, ", var->name, *((*(node->trans+i))->state+j) );
							}
						}
					}
				}
				fprintf( fp, "\"]" );
			}
			fprintf( fp, "\n" );
		}
		node_counter++;
		node = node->next;
	}
	fprintf( fp, "}\n" );

	return 0;
}


int tulip_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list, FILE *fp )
{
	int i;
	anode_t *node;
	int node_counter = 0;
	ptree_t *var;
	int num_env, num_sys;

	if (fp == NULL)
		fp = stdout;

	num_env = tree_size( evar_list );
	num_sys = tree_size( svar_list );

	fprintf( fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
	fprintf( fp, "<tulipcon xmlns=\"http://tulip-control.sourceforge.net/ns/0\" version=\"0\">\n" );
	fprintf( fp, "  <env_vars>\n" );
	for (i = 0; i < num_env; i++) {
		var = get_list_item( evar_list, i );
		fprintf( fp, "    <item key=\"%s\" value=\"boolean\" />\n", var->name );
	}
	fprintf( fp, "  </env_vars>\n" );
	fprintf( fp, "  <sys_vars>\n" );
	for (i = 0; i < num_sys; i++) {
		var = get_list_item( svar_list, i );
		fprintf( fp, "    <item key=\"%s\" value=\"boolean\" />\n", var->name );
	}
	fprintf( fp, "  </sys_vars>\n" );
	fprintf( fp, "  <spec>\n    <env_init></env_init><env_safety></env_safety><env_prog></env_prog><sys_init></sys_init><sys_safety></sys_safety><sys_prog></sys_prog>\n  </spec>\n" );
	
	fprintf( fp, "  <aut>\n" );
	node = head;
	while (node) {
		fprintf( fp, "    <node>\n      <id>%d</id><name></name>\n      <child_list>", node_counter );
		for (i = 0; i < node->trans_len; i++)
			fprintf( fp, " %d",
					 find_anode_index( head,
									   (*(node->trans+i))->mode,
									   (*(node->trans+i))->state, num_env+num_sys ) );
		fprintf( fp, "</child_list>\n      <state>\n" );
		for (i = 0; i < num_env; i++) {
			var = get_list_item( evar_list, i );
			fprintf( fp, "        <item key=\"%s\" value=\"%d\" />\n",
					 var->name, *(node->state+i) );
		}
		for (i = 0; i < num_sys; i++) {
			var = get_list_item( svar_list, i );
			fprintf( fp, "        <item key=\"%s\" value=\"%d\" />\n",
					 var->name, *(node->state+num_env+i) );
		}
		fprintf( fp, "      </state>\n    </node>\n" );
		node_counter++;
		node = node->next;
	}

	fprintf( fp, "  </aut>\n" );
	fprintf( fp, "  <extra>strategy synthesized with gr1c, version " GR1C_VERSION "</extra>\n</tulipcon>\n" );

	return 0;
}


void list_aut_dump( anode_t *head, int state_len, FILE *fp )
{
	anode_t *node = head;
	int node_counter = 0;
	int i;
	int width_count;
	if (fp == NULL)
		fp = stdout;
	while (node) {
		width_count = 0;
		fprintf( fp, "%4d : ", node_counter );
		for (i = 0; i < state_len; i++) {
			fprintf( fp, "%d", *(node->state+i) );
			width_count++;
			if (width_count == 4) {
				fprintf( fp, " " );
				width_count = 0;
			}
		}
		fprintf( fp, " - %2d - [", node->mode );
		for (i = 0; i < node->trans_len; i++)
			fprintf( fp, " %d",
					 find_anode_index( head,
									   (*(node->trans+i))->mode,
									   (*(node->trans+i))->state, state_len ) );
		fprintf( fp, "]\n" );
		node = node->next;
		node_counter++;
	}
}
