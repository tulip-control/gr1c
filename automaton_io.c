/* automaton_io.c -- Definitions for input and output routines on automaton objects.
 *
 *
 * SCL; 2012.
 */


#include <stdlib.h>
#include <stdio.h>

#include "automaton.h"


#define INPUT_STRING_LEN 256

/* Memory recovery in the case of error, whether from a system call or
   while parsing, is almost nil.  Correct this behavior before
   release.  Also note that sorting of nodes could be made faster. */
anode_t *aut_aut_load( int state_len, FILE *fp )
{
	anode_t *head = NULL, *node;
	bool *state;
	int this_trans;
	int i, j, k;  /* Generic counters */
	int ia_len;  /* length of ID_array, trans_array, and node_array */
	int *ID_array;
	int **trans_array;
	anode_t **node_array;
	char line[INPUT_STRING_LEN];
	char *start, *end;
	int line_num;
	
	if (fp == NULL)
		fp = stdin;

	if (state_len < 1)
		return NULL;
	state = malloc( sizeof(bool)*state_len );
	if (state == NULL) {
		perror( "aut_aut_load, malloc" );
		return NULL;
	}

	ia_len = 1;
	ID_array = malloc( sizeof(int)*ia_len );
	trans_array = malloc( sizeof(int *)*ia_len );
	node_array = malloc( sizeof(anode_t *)*ia_len );
	if (ID_array == NULL || trans_array == NULL || node_array == NULL) {
		free( state );
		perror( "aut_aut_load, malloc" );
		return NULL;
	}

	line_num = 0;
	while (fgets( line, INPUT_STRING_LEN, fp )) {
		line_num++;
		if (strlen( line ) < 1 || *line == '\n')
			continue;

		*(ID_array+ia_len-1) = strtol( line, &end, 10 );
		if (line == end || *end == '\0')
			continue;

		start = end;
		for (i = 0; i < state_len && *end != '\0'; i++) {
			*(state+i) = strtol( start, &end, 10 );
			if (start == end)
				break;
			start = end;
		}
		if (i != state_len) {
			fprintf( stderr, "Error parsing gr1c automaton line %d.\n", line_num );
			return NULL;
		}

		*(node_array+ia_len-1) = malloc( sizeof(anode_t) );
		if (*(node_array+ia_len-1) == NULL) {
			perror( "aut_aut_load, malloc" );
			return NULL;
		}

		(*(node_array+ia_len-1))->mode = strtol( start, &end, 10 );
		if (start == end || *end == '\0') {
			fprintf( stderr, "Error parsing gr1c automaton line %d.\n", line_num );
			return NULL;
		}
		start = end;

		(*(node_array+ia_len-1))->rgrad = strtol( start, &end, 10 );
		if (start == end) {
			fprintf( stderr, "Error parsing gr1c automaton line %d.\n", line_num );
			return NULL;
		}
		start = end;

		(*(node_array+ia_len-1))->state = state;
		(*(node_array+ia_len-1))->trans_len = 0;
		(*(node_array+ia_len-1))->next = NULL;
		*(trans_array+ia_len-1) = NULL;

		this_trans = strtol( start, &end, 10 );
		while (start != end) {
			((*(node_array+ia_len-1))->trans_len)++;
			*(trans_array+ia_len-1) = realloc( *(trans_array+ia_len-1),
											   sizeof(int)*((*(node_array+ia_len-1))->trans_len) );
			if (*(trans_array+ia_len-1) == NULL) {
				perror( "aut_aut_load, realloc" );
				return NULL;
			}
			*(*(trans_array+ia_len-1) + (*(node_array+ia_len-1))->trans_len - 1) = this_trans;

			start = end;
			this_trans = strtol( start, &end, 10 );
		}

		state = malloc( sizeof(bool)*state_len );
		if (state == NULL) {
			perror( "aut_aut_load, malloc" );
			return NULL;
		}
		ia_len++;
		ID_array = realloc( ID_array, sizeof(int)*ia_len );
		trans_array = realloc( trans_array, sizeof(int *)*ia_len );
		node_array = realloc( node_array, sizeof(anode_t *)*ia_len );
		if (ID_array == NULL || trans_array == NULL || node_array == NULL) {
			perror( "aut_aut_load, realloc" );
			return NULL;
		}
	}
	free( state );
	ia_len--;
	ID_array = realloc( ID_array, sizeof(int)*ia_len );
	trans_array = realloc( trans_array, sizeof(int *)*ia_len );
	node_array = realloc( node_array, sizeof(anode_t *)*ia_len );
	if (ID_array == NULL || trans_array == NULL || node_array == NULL) {
		perror( "aut_aut_load, realloc" );
		return NULL;
	}

	for (i = 0; i < ia_len; i++) {
		for (j = 0; j < ia_len && *(ID_array+j) != i; j++) ;
		if (j == ia_len) {
			fprintf( stderr, "Error parsing gr1c automaton data; missing indices.\n" );
			return NULL;
		}

		if (i == 0) {
			head = *(node_array+j);
			node = head;
		} else {
			node->next = *(node_array+j);
			node = node->next;
		}

		if (node->trans_len > 0) {
			node->trans = malloc( sizeof(anode_t *)*(node->trans_len) );
			if (node->trans == NULL) {
				perror( "aut_aut_load, malloc" );
				return NULL;
			}
			for (j = 0; j < node->trans_len; j++) {
				for (k = 0; k < ia_len && *(ID_array+k) != *(*(trans_array+i)+j); k++) ;
				if (k == ia_len) {
					fprintf( stderr, "Error parsing gr1c automaton data; missing indices.\n" );
					return NULL;
				}
				*(node->trans+j) = *(node_array+k);
			}
		}
	}

	free( ID_array );
	free( node_array );
	for (i = 0; i < ia_len; i++)
		free( *(trans_array+i) );
	free( trans_array );
	
	return head;
}


void aut_aut_dump( anode_t *head, int state_len, FILE *fp )
{
	anode_t *node = head;
	int node_counter = 0;
	int i;
	if (fp == NULL)
		fp = stdout;
	while (node) {
		fprintf( fp, "%d", node_counter );
		for (i = 0; i < state_len; i++)
			fprintf( fp, " %d", *(node->state+i) );
		fprintf( fp, " %d %d", node->mode, node->rgrad );
		for (i = 0; i < node->trans_len; i++)
			fprintf( fp, " %d",
					 find_anode_index( head,
									   (*(node->trans+i))->mode,
									   (*(node->trans+i))->state, state_len ) );
		fprintf( fp, "\n" );
		node = node->next;
		node_counter++;
	}
}


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
			fprintf( fp, "    \"%d;\\n(%d, %d)\\n", node_counter, node->mode, node->rgrad );
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
