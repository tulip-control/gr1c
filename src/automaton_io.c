/* automaton_io.c -- Definitions for input/output routines on automata.
 *
 *
 * SCL; 2012-2015
 */


#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ptree.h"
#include "automaton.h"


#define INPUT_STRING_LEN 1024

/* Future work: sorting of nodes could be made faster. */
anode_t *aut_aut_loadver( int state_len, FILE *fp, int *version )
{
    anode_t *head = NULL, *node;
    vartype *state;
    int this_trans;
    int i, j, k;  /* Generic counters */
    int ia_len;  /* length of ID_array, trans_array, and node_array */
    int *ID_array;
    int **trans_array;
    anode_t **node_array;
    char line[INPUT_STRING_LEN];
    char *start, *end;
    int line_num;
    int detected_version = -1;
    int *tmp_intp;
    int **tmp_intpp;
    anode_t **tmp_anode_tpp;

    if (fp == NULL)
        fp = stdin;

    if (state_len < 1)
        return NULL;
    state = malloc( sizeof(vartype)*state_len );
    if (state == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    ia_len = 1;
    ID_array = malloc( sizeof(int)*ia_len );
    trans_array = malloc( sizeof(int *)*ia_len );
    *(trans_array+ia_len-1) = NULL;
    node_array = malloc( sizeof(anode_t *)*ia_len );
    *(node_array+ia_len-1) = NULL;
    if (ID_array == NULL || trans_array == NULL || node_array == NULL) {
        free( state );
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    line_num = 0;
    while (fgets( line, INPUT_STRING_LEN, fp )) {
        line_num++;
        if (strlen( line ) < 1 || *line == '#' || *line == '\n' || *line == '\r')
            continue;

        *(ID_array+ia_len-1) = strtol( line, &end, 10 );
        if (line == end) {
            fprintf( stderr,
                     "Error parsing gr1c automaton line %d.\n", line_num );
            head = NULL;
            goto gc;
        } else if (detected_version < 0) {
            if (*end == '\0' || *end == '\n' || *end == '\r') {
                detected_version = *(ID_array+ia_len-1);
                if (detected_version < 0) {
                    fprintf( stderr,
                             "Invalid version number \"%d\" found while"
                             " parsing gr1c automaton line %d.\n",
                             detected_version,
                             line_num );
                    head = NULL;
                    goto gc;
                }
                continue;
            } else {
                /* If no explicit version number, then assume 0, and
                   continue parsing this line accordingly. */
                detected_version = 0;
            }

            if (detected_version != 0 && detected_version != 1) {
                fprintf( stderr,
                         "Only gr1c automaton format versions 0 and 1 are supported." );
                head = NULL;
                goto gc;
            }
        }

        start = end;
        for (i = 0; i < state_len && *end != '\0'; i++) {
            *(state+i) = strtol( start, &end, 10 );
            if (start == end)
                break;
            start = end;
        }
        if (i != state_len) {
            fprintf( stderr,
                     "Error parsing gr1c automaton line %d.\n", line_num );
            head = NULL;
            goto gc;
        }

        *(node_array+ia_len-1) = malloc( sizeof(anode_t) );
        if (*(node_array+ia_len-1) == NULL) {
            perror( __FILE__ ",  malloc" );
            exit(-1);
        }

        if (detected_version == 1) {
            (*(node_array+ia_len-1))->initial = strtol( start, &end, 10 );
            if (start == end || *end == '\0') {
                fprintf( stderr,
                         "Error parsing gr1c automaton line %d.\n", line_num );
                head = NULL;
                goto gc;
            }
            if ((*(node_array+ia_len-1))->initial != 0
                && (*(node_array+ia_len-1))->initial != 1) {
                fprintf( stderr,
                         "Invalid value for node field \"initial\" on line %d.\n", line_num );
                head = NULL;
                goto gc;
            }
            start = end;
        }

        (*(node_array+ia_len-1))->mode = strtol( start, &end, 10 );
        if (start == end || *end == '\0') {
            fprintf( stderr,
                     "Error parsing gr1c automaton line %d.\n", line_num );
            head = NULL;
            goto gc;
        }
        start = end;

        (*(node_array+ia_len-1))->rgrad = strtol( start, &end, 10 );
        if (start == end) {
            fprintf( stderr,
                     "Error parsing gr1c automaton line %d.\n", line_num );
            head = NULL;
            goto gc;
        }
        start = end;

        if (detected_version == 0)
            (*(node_array+ia_len-1))->initial = False;

        (*(node_array+ia_len-1))->state = state;
        (*(node_array+ia_len-1))->trans_len = 0;
        (*(node_array+ia_len-1))->next = NULL;
        *(trans_array+ia_len-1) = NULL;

        this_trans = strtol( start, &end, 10 );
        while (start != end) {
            ((*(node_array+ia_len-1))->trans_len)++;
            *(trans_array+ia_len-1)
                = realloc( *(trans_array+ia_len-1),
                           sizeof(int)*((*(node_array+ia_len-1))->trans_len) );
            if (*(trans_array+ia_len-1) == NULL) {
                perror( __FILE__ ",  realloc" );
                exit(-1);
            }
            *(*(trans_array+ia_len-1)
              + (*(node_array+ia_len-1))->trans_len - 1) = this_trans;

            start = end;
            this_trans = strtol( start, &end, 10 );
        }

        state = malloc( sizeof(vartype)*state_len );
        if (state == NULL) {
            perror( __FILE__ ",  malloc" );
            exit(-1);
        }
        ia_len++;

        tmp_intp = realloc( ID_array, sizeof(int)*ia_len );
        if (tmp_intp == NULL) {
            free( ID_array );
            ID_array = NULL;
        } else {
            ID_array = tmp_intp;
        }
        tmp_intpp = realloc( trans_array, sizeof(int *)*ia_len );
        if (tmp_intpp == NULL) {
            free( trans_array );
            trans_array = NULL;
        } else {
            trans_array = tmp_intpp;
        }
        tmp_anode_tpp = realloc( node_array, sizeof(anode_t *)*ia_len );
        if (tmp_anode_tpp == NULL) {
            free( node_array );
            node_array = NULL;
        } else {
            node_array = tmp_anode_tpp;
        }
        if (ID_array == NULL || trans_array == NULL || node_array == NULL) {
            perror( __FILE__ ",  realloc" );
            exit(-1);
        }
        *(trans_array+ia_len-1) = NULL;
        *(node_array+ia_len-1) = NULL;
    }
    free( state );
    state = NULL;
    ia_len--;
    if (ia_len == 0) {
        head = NULL;
        goto gc;
    }

    tmp_intp = realloc( ID_array, sizeof(int)*ia_len );
    if (tmp_intp == NULL) {
        free( ID_array );
        ID_array = NULL;
    } else {
        ID_array = tmp_intp;
    }
    tmp_intpp = realloc( trans_array, sizeof(int *)*ia_len );
    if (tmp_intpp == NULL) {
        free( trans_array );
        trans_array = NULL;
    } else {
        trans_array = tmp_intpp;
    }
    tmp_anode_tpp = realloc( node_array, sizeof(anode_t *)*ia_len );
    if (tmp_anode_tpp == NULL) {
        free( node_array );
        node_array = NULL;
    } else {
        node_array = tmp_anode_tpp;
    }
    if (ID_array == NULL || trans_array == NULL || node_array == NULL) {
        perror( __FILE__ ",  realloc" );
        exit(-1);
    }

    for (i = 0; i < ia_len; i++) {
        for (j = 0; j < ia_len && *(ID_array+j) != i; j++) ;
        if (j == ia_len) {
            fprintf( stderr,
                     "Error parsing gr1c automaton data; missing indices.\n" );
            head = NULL;
            goto gc;
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
                perror( __FILE__ ",  malloc" );
                exit(-1);
            }
            for (j = 0; j < node->trans_len; j++) {
                for (k = 0;
                     k < ia_len && *(ID_array+k) != *(*(trans_array+i)+j);
                     k++) ;
                if (k == ia_len) {
                    fprintf( stderr,
                             "Error parsing gr1c automaton data; missing"
                             " indices.\n" );
                    head = NULL;
                    goto gc;
                }
                *(node->trans+j) = *(node_array+k);
            }
        }
    }

    /* The "gc" label abbreviates "garbage collection". */
  gc:
    free( state );
    free( ID_array );
    if (head == NULL) {
        for (i = 0; i < ia_len; i++)
            delete_aut( *(node_array+i) );
    }
    free( node_array );
    for (i = 0; i < ia_len; i++)
        free( *(trans_array+i) );
    free( trans_array );

    if (version != NULL && head != NULL)
        *version = detected_version;
    return head;
}

anode_t *aut_aut_load( int state_len, FILE *fp )
{
    return aut_aut_loadver( state_len, fp, NULL );
}


int aut_aut_dumpver( anode_t *head, int state_len, FILE *fp, int version )
{
    anode_t *node = head;
    int node_counter = 0;
    int i;

    if (fp == NULL)
        fp = stdout;

    fprintf( fp, "%d\n", version );
    switch (version) {
    case 0:
        while (node) {
            fprintf( fp, "%d", node_counter );
            for (i = 0; i < state_len; i++)
                fprintf( fp, " %d", *(node->state+i) );
            fprintf( fp, " %d %d", node->mode, node->rgrad );
            for (i = 0; i < node->trans_len; i++)
                fprintf( fp, " %d", anode_index( head, *(node->trans+i) ) );
            fprintf( fp, "\n" );
            node = node->next;
            node_counter++;
        }
        break;

    case 1:
        while (node) {
            fprintf( fp, "%d", node_counter );
            for (i = 0; i < state_len; i++)
                fprintf( fp, " %d", *(node->state+i) );
            fprintf( fp, " %d %d %d", node->initial, node->mode, node->rgrad );
            for (i = 0; i < node->trans_len; i++)
                fprintf( fp, " %d", anode_index( head, *(node->trans+i) ) );
            fprintf( fp, "\n" );
            node = node->next;
            node_counter++;
        }
        break;

    default:
        return -1;  /* Unrecognized gr1c automaton format version */
    }

    return 0;
}


void aut_aut_dump( anode_t *head, int state_len, FILE *fp )
{
    aut_aut_dumpver( head, state_len, fp, 1 );
}


int dot_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
                  unsigned char format_flags, FILE *fp )
{
    int i, j, last_nonzero_env, last_nonzero_sys;
    anode_t *node;
    int node_counter = 0;
    ptree_t *var;
    int num_env, num_sys;
    char this_node_str[INPUT_STRING_LEN];
    int nb = 0;

    if (fp == NULL)
        fp = stdout;

    num_env = tree_size( evar_list );
    num_sys = tree_size( svar_list );

    fprintf( fp,
             "/* created using gr1c, version "
             GR1C_VERSION " */\n" );
    fprintf( fp, "digraph A {\n    \"\" [shape=none]\n" );
    node = head;
    while (node) {
        /* Buffer node string */
        nb = snprintf( this_node_str, INPUT_STRING_LEN,
                       "\"%d;\\n", node_counter );
        if (nb >= INPUT_STRING_LEN)
            return -1;
        if (format_flags & DOT_AUT_ATTRIB) {
            nb += snprintf( this_node_str+nb, INPUT_STRING_LEN-nb,
                            "(%d, %d)\\n", node->mode, node->rgrad );
            if (nb >= INPUT_STRING_LEN)
                return -1;
        }
        if ((format_flags & 0x1) == DOT_AUT_ALL) {
            last_nonzero_env = num_env-1;
            last_nonzero_sys = num_sys-1;
        } else {
            for (last_nonzero_env = num_env-1; last_nonzero_env >= 0
                     && *(node->state+last_nonzero_env) == 0;
                 last_nonzero_env--) ;
            for (last_nonzero_sys = num_sys-1; last_nonzero_sys >= 0
                     && *(node->state+num_env+last_nonzero_sys) == 0;
                 last_nonzero_sys--) ;
        }
        if (last_nonzero_env < 0 && last_nonzero_sys < 0) {
            nb += snprintf( this_node_str+nb, INPUT_STRING_LEN-nb,
                            "{}" );
            if (nb >= INPUT_STRING_LEN)
                return -1;
        } else {
            if (!(format_flags & DOT_AUT_EDGEINPUT)) {
                for (j = 0; j < num_env; j++) {
                    if ((format_flags & DOT_AUT_BINARY)
                        && *(node->state+j) == 0)
                        continue;
                    var = get_list_item( evar_list, j );
                    if (j == last_nonzero_env) {
                        if (format_flags & DOT_AUT_BINARY) {
                            nb += snprintf( this_node_str+nb,
                                            INPUT_STRING_LEN-nb,
                                            "%s", var->name );
                            if (nb >= INPUT_STRING_LEN)
                                return -1;
                        } else {
                            nb += snprintf( this_node_str+nb,
                                            INPUT_STRING_LEN-nb,
                                            "%s=%d",
                                            var->name, *(node->state+j) );
                            if (nb >= INPUT_STRING_LEN)
                                return -1;
                        }
                        if ((last_nonzero_sys >= 0
                             || (format_flags & DOT_AUT_ALL))
                            && !(format_flags & DOT_AUT_EDGEINPUT)) {
                            nb += snprintf( this_node_str+nb,
                                            INPUT_STRING_LEN-nb,
                                            ", " );
                            if (nb >= INPUT_STRING_LEN)
                                return -1;
                        }
                    } else {
                        if (format_flags & DOT_AUT_BINARY) {
                            nb += snprintf( this_node_str+nb,
                                            INPUT_STRING_LEN-nb,
                                            "%s, ", var->name );
                            if (nb >= INPUT_STRING_LEN)
                                return -1;
                        } else {
                            nb += snprintf( this_node_str+nb,
                                            INPUT_STRING_LEN-nb,
                                            "%s=%d, ",
                                            var->name, *(node->state+j) );
                            if (nb >= INPUT_STRING_LEN)
                                return -1;
                        }
                    }
                }
            }
            if (last_nonzero_sys < 0 && (format_flags & DOT_AUT_EDGEINPUT)) {
                nb += snprintf( this_node_str+nb,
                                INPUT_STRING_LEN-nb,
                                "{}" );
                if (nb >= INPUT_STRING_LEN)
                    return -1;
            } else {
                for (j = 0; j < num_sys; j++) {
                    if ((format_flags & DOT_AUT_BINARY)
                        && *(node->state+num_env+j) == 0)
                        continue;
                    var = get_list_item( svar_list, j );
                    if (j == last_nonzero_sys) {
                        if (format_flags & DOT_AUT_BINARY) {
                            nb += snprintf( this_node_str+nb,
                                            INPUT_STRING_LEN-nb,
                                            "%s", var->name );
                            if (nb >= INPUT_STRING_LEN)
                                return -1;
                        } else {
                            nb += snprintf( this_node_str+nb,
                                            INPUT_STRING_LEN-nb,
                                            "%s=%d",
                                            var->name,
                                            *(node->state+num_env+j) );
                            if (nb >= INPUT_STRING_LEN)
                                return -1;
                        }
                    } else {
                        if (format_flags & DOT_AUT_BINARY) {
                            nb += snprintf( this_node_str+nb,
                                            INPUT_STRING_LEN-nb,
                                            "%s, ", var->name );
                            if (nb >= INPUT_STRING_LEN)
                                return -1;
                        } else {
                            nb += snprintf( this_node_str+nb,
                                            INPUT_STRING_LEN-nb,
                                            "%s=%d, ",
                                            var->name,
                                            *(node->state+num_env+j) );
                            if (nb >= INPUT_STRING_LEN)
                                return -1;
                        }
                    }
                }
            }
        }
        nb += snprintf( this_node_str+nb, INPUT_STRING_LEN-nb, "\"" );
        if (nb >= INPUT_STRING_LEN)
            return -1;

        fprintf( fp, "    %s\n", this_node_str );

        /* Next print all outgoing edges from the current node, and a
           special incoming edge if this node is initial. */
        if (node->initial)
            fprintf( fp, "    \"\" -> %s\n", this_node_str );
        for (i = 0; i < node->trans_len; i++) {
            fprintf( fp, "    %s -> ", this_node_str );
            fprintf( fp, "\"%d;\\n", anode_index( head, *(node->trans+i) ) );
            if (format_flags & DOT_AUT_ATTRIB) {
                fprintf( fp,
                         "(%d, %d)\\n",
                         (*(node->trans+i))->mode, (*(node->trans+i))->rgrad);
            }
            if ((format_flags & 0x1) == DOT_AUT_ALL) {
                last_nonzero_env = num_env-1;
                last_nonzero_sys = num_sys-1;
            } else {
                for (last_nonzero_env = num_env-1; last_nonzero_env >= 0
                         && *((*(node->trans+i))->state+last_nonzero_env) == 0;
                     last_nonzero_env--) ;
                for (last_nonzero_sys = num_sys-1; last_nonzero_sys >= 0
                         && *((*(node->trans+i))->state
                              +num_env+last_nonzero_sys) == 0;
                     last_nonzero_sys--) ;
            }
            if (last_nonzero_env < 0 && last_nonzero_sys < 0) {
                fprintf( fp, "{}" );
            } else {
                if (!(format_flags & DOT_AUT_EDGEINPUT)) {
                    for (j = 0; j < num_env; j++) {
                        if ((format_flags & DOT_AUT_BINARY)
                            && *((*(node->trans+i))->state+j) == 0)
                            continue;
                        var = get_list_item( evar_list, j );
                        if (j == last_nonzero_env) {
                            if (format_flags & DOT_AUT_BINARY) {
                                fprintf( fp, "%s", var->name );
                            } else {
                                fprintf( fp,
                                         "%s=%d",
                                         var->name,
                                         *((*(node->trans+i))->state+j) );
                            }
                            if ((last_nonzero_sys >= 0
                                 || (format_flags & DOT_AUT_ALL))
                                && !(format_flags & DOT_AUT_EDGEINPUT))
                                fprintf( fp, ", " );
                        } else {
                            if (format_flags & DOT_AUT_BINARY) {
                                fprintf( fp, "%s, ", var->name );
                            } else {
                                fprintf( fp,
                                         "%s=%d, ",
                                         var->name,
                                         *((*(node->trans+i))->state+j) );
                            }
                        }
                    }
                }
                if (last_nonzero_sys < 0
                    && (format_flags & DOT_AUT_EDGEINPUT)) {
                    fprintf( fp, "{}" );
                } else {
                    for (j = 0; j < num_sys; j++) {
                        if ((format_flags & DOT_AUT_BINARY)
                            && *((*(node->trans+i))->state+num_env+j) == 0)
                            continue;
                        var = get_list_item( svar_list, j );
                        if (j == last_nonzero_sys) {
                            if (format_flags & DOT_AUT_BINARY) {
                                fprintf( fp, "%s", var->name );
                            } else {
                                fprintf( fp,
                                         "%s=%d",
                                         var->name,
                                         *((*(node->trans+i))->state
                                           +num_env+j) );
                            }
                        } else {
                            if (format_flags & DOT_AUT_BINARY) {
                                fprintf( fp, "%s, ", var->name );
                            } else {
                                fprintf( fp,
                                         "%s=%d, ",
                                         var->name,
                                         *((*(node->trans+i))->state
                                           +num_env+j) );
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
                        if ((format_flags & DOT_AUT_BINARY)
                            && *((*(node->trans+i))->state+j) == 0)
                            continue;
                        var = get_list_item( evar_list, j );
                        if (j == last_nonzero_env) {
                            if (format_flags & DOT_AUT_BINARY) {
                                fprintf( fp, "%s", var->name );
                            } else {
                                fprintf( fp,
                                         "%s=%d",
                                         var->name,
                                         *((*(node->trans+i))->state+j) );
                            }
                        } else {
                            if (format_flags & DOT_AUT_BINARY) {
                                fprintf( fp, "%s, ", var->name );
                            } else {
                                fprintf( fp,
                                         "%s=%d, ",
                                         var->name,
                                         *((*(node->trans+i))->state+j) );
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


int tulip_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
                    FILE *fp )
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
    fprintf( fp,
             "<tulipcon xmlns=\"http://tulip-control.sourceforge.net/ns/1\""
             " version=\"1\">\n" );
    fprintf( fp, "  <env_vars>\n" );
    for (i = 0; i < num_env; i++) {
        var = get_list_item( evar_list, i );
        if (var->value >= 0) {
            fprintf( fp,
                     "    <item key=\"%s\" value=\"[0,%d]\" />\n",
                     var->name, var->value );
        } else {
            fprintf( fp,
                     "    <item key=\"%s\" value=\"boolean\" />\n",
                     var->name );
        }
    }
    fprintf( fp, "  </env_vars>\n" );
    fprintf( fp, "  <sys_vars>\n" );
    for (i = 0; i < num_sys; i++) {
        var = get_list_item( svar_list, i );
        if (var->value >= 0) {
            fprintf( fp,
                     "    <item key=\"%s\" value=\"[0,%d]\" />\n",
                     var->name, var->value );
        } else {
            fprintf( fp,
                     "    <item key=\"%s\" value=\"boolean\" />\n",
                     var->name );
        }
    }
    fprintf( fp, "  </sys_vars>\n" );
    fprintf( fp,
             "  <spec>\n    "
             "<env_init></env_init><env_safety></env_safety>"
             "<env_prog></env_prog>"
             "<sys_init></sys_init><sys_safety></sys_safety>"
             "<sys_prog></sys_prog>\n"
             "  </spec>\n" );

    fprintf( fp, "  <aut type=\"basic\">\n" );
    node = head;
    while (node) {
        fprintf( fp, "    <node>\n      <id>%d</id><anno>", node_counter );
        if (node->mode != -1 && node->rgrad != -1)
            fprintf( fp, "%d %d", node->mode, node->rgrad );
        fprintf( fp, "</anno>\n      <child_list>" );
        for (i = 0; i < node->trans_len; i++)
            fprintf( fp, " %d", anode_index( head, *(node->trans+i) ) );
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
    fprintf( fp,
             "  <extra>created using gr1c, version "
             GR1C_VERSION "</extra>\n</tulipcon>\n" );

    return 0;
}


void list_aut_dump( anode_t *head, int state_len, FILE *fp )
{
    anode_t *node = head;
    int node_counter = 0;
    int i;
    if (fp == NULL)
        fp = stdout;
    while (node) {
        fprintf( fp, "%4d ", node_counter );
        if (node->initial)
            fprintf( fp, "(init) " );
        fprintf( fp, ": " );
        if (state_len > 0) {
            for (i = 0; i < state_len-1; i++) {
                fprintf( fp, "%d,", *(node->state+i) );
            }
            fprintf( fp, "%d", *(node->state+state_len-1) );
        } else {
            fprintf( fp, "(nil)" );
        }
        fprintf( fp, " - %2d - %2d - [", node->mode, node->rgrad );
        for (i = 0; i < node->trans_len; i++)
            fprintf( fp, " %d", anode_index( head, *(node->trans+i) ) );
        fprintf( fp, "]\n" );
        node = node->next;
        node_counter++;
    }
}


#define TIMESTAMP_LEN 32
int json_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
                   FILE *fp )
{
    int num_env, num_sys;

    struct tm *timeptr;
    time_t clock;
    char timestamp[TIMESTAMP_LEN];
    int i;
    ptree_t *var;

    if (fp == NULL)
        fp = stdout;

    num_env = tree_size( evar_list );
    num_sys = tree_size( svar_list );

    clock = time( NULL );
    timeptr = gmtime( &clock );  /* UTC */
    if (strftime( timestamp, TIMESTAMP_LEN,
                  "%Y-%m-%d %H:%M:%S", timeptr ) == 0) {
        fprintf( stderr, "ERROR: strftime() failed to create timestamp." );
        return -1;
    }

    fprintf( fp, "{\"version\": 1,\n" );  /* gr1c JSON format version */
    fprintf( fp, " \"gr1c\": \"" GR1C_VERSION "\",\n" );
    fprintf( fp, " \"date\": \"%s\",\n", timestamp );
    fprintf( fp, " \"extra\": \"\",\n\n" );

    fprintf( fp, " \"ENV\": [" );
    for (i = 0; i < num_env; i++) {
        var = get_list_item( evar_list, i );
        fprintf( fp, "{\"%s\": ", var->name );
        if (var->value >= 0) {
            fprintf( fp, "[0,%d]}", var->value );
        } else {
            fprintf( fp, "\"boolean\"}" );
        }
        if (i < num_env-1)
            fprintf( fp, ", " );
    }
    fprintf( fp, "],\n \"SYS\": [" );
    for (i = 0; i < num_sys; i++) {
        var = get_list_item( svar_list, i );
        fprintf( fp, "{\"%s\": ", var->name );
        if (var->value >= 0) {
            fprintf( fp, "[0, %d]}", var->value );
        } else {
            fprintf( fp, "\"boolean\"}" );
        }
        if (i < num_sys-1)
            fprintf( fp, ", " );
    }
    fprintf( fp, "],\n\n" );

    fprintf( fp, " \"nodes\": {\n" );
    while (head) {
        fprintf( fp, "\"0x%X\": {\n", head );
        fprintf( fp, "    \"state\": [" );
        for (i = 0; i < num_env+num_sys; i++) {
            fprintf( fp, "%d", *(head->state+i) );
            if (i < num_env+num_sys-1)
                fprintf( fp, ", " );
        }

        fprintf( fp,
                 "],\n    \"mode\": %d,\n    \"rgrad\": %d,\n",
                 head->mode, head->rgrad );
        if (head->initial) {
            fprintf( fp, "    \"initial\": true,\n" );
        } else {
            fprintf( fp, "    \"initial\": false,\n" );
        }

        fprintf( fp, "    \"trans\": [" );
        for (i = 0; i < head->trans_len; i++) {
            fprintf( fp, "\"0x%X\"", *(head->trans+i) );
            if (i < head->trans_len-1)
                fprintf( fp, ", " );
        }
        fprintf( fp, "] }" );

        head = head->next;
        if (head != NULL)
            fprintf( fp, "," );
        fprintf( fp, "\n" );
    }
    fprintf( fp, "}}\n" );
    return 0;
}


void spin_aut_ltl_formula( int num_env,
                           ptree_t *env_init, ptree_t *sys_init,
                           int num_env_goals, int num_sys_goals,
                           FILE *fp )
{
    int i;
    if (fp == NULL)
        fp = stdout;
    if (num_env > 0) {
        fprintf( fp, "(" );
        if (env_init != NULL)
            fprintf( fp, "X envinit && " );
        fprintf( fp, "[] (!checketrans || envtrans)" );
        for (i = 0; i < num_env_goals; i++)
            fprintf( fp, " && []<> envgoal%04d", i );
        fprintf( fp, ") -> (" );
    }
    if (sys_init != NULL)
        fprintf( fp, "X sysinit && " );
    fprintf( fp, "[] (!checkstrans || systrans)" );
    for (i = 0; i < num_sys_goals; i++)
        fprintf( fp, " && []<> sysgoal%04d", i );
    fprintf( fp, " && [] !pmlfault" );
    if (num_env > 0)
        fprintf( fp, ")" );
    fprintf( fp, "\n" );
}

int spin_aut_dump( anode_t *head, ptree_t *evar_list, ptree_t *svar_list,
                   ptree_t *env_init, ptree_t *sys_init,
                   ptree_t **env_trans_array, int et_array_len,
                   ptree_t **sys_trans_array, int st_array_len,
                   ptree_t **env_goals, int num_env_goals,
                   ptree_t **sys_goals, int num_sys_goals,
                   FILE *fp, FILE *formula_fp )
{
    anode_t *node, *next_node;
    int num_env, num_sys;
    int node_counter, next_node_counter;
    vartype *env_counter;
    ptree_t *tmppt, *var_separator;
    int i, j;

    if (fp == NULL)
        fp = stdout;

    if (formula_fp == NULL)
        formula_fp = stdout;

    if (formula_fp == fp)
        formula_fp = NULL;

    num_env = tree_size( evar_list );
    num_sys = tree_size( svar_list );

    env_counter = malloc( num_env*sizeof(int) );
    if (env_counter == NULL) {
        perror( __FILE__ ",  malloc" );
        exit(-1);
    }

    fprintf( fp,
             "/* Spin Promela model created by gr1c, version "
             GR1C_VERSION " */\n\n" );

    fprintf( fp, "/* LTL formula\n" );
    spin_aut_ltl_formula( num_env, env_init, sys_init,
                          num_env_goals, num_sys_goals,
                          fp );
    if (formula_fp)
        spin_aut_ltl_formula( num_env, env_init, sys_init,
                              num_env_goals, num_sys_goals,
                              formula_fp );
    fprintf( fp, "*/\n\n" );

    if (num_env > 0) {
        if (env_init != NULL) {
            fprintf( fp, "\n#define envinit " );
            print_formula( env_init, fp, FORMULA_SYNTAX_SPIN );
        }
        fprintf( fp, "\n#define envtrans " );
        if (et_array_len == 0) {
            fprintf( fp, "true" );
        } else {
            for (i = 0; i < et_array_len; i++) {
                print_formula( *(env_trans_array+i), fp, FORMULA_SYNTAX_SPIN );
                if (i < et_array_len-1)
                    fprintf( fp, " && " );
            }
        }
        if (num_env_goals > 0) {
            for (i = 0; i < num_env_goals; i++) {
                fprintf( fp, "\n#define envgoal%04d ", i );
                print_formula( *(env_goals+i), fp, FORMULA_SYNTAX_SPIN );
            }
        }
    }
    if (sys_init != NULL) {
        fprintf( fp, "\n#define sysinit " );
        print_formula( sys_init, fp, FORMULA_SYNTAX_SPIN );
    }
    fprintf( fp, "\n#define systrans " );
    if (st_array_len == 0) {
        fprintf( fp, "true" );
    } else {
        for (i = 0; i < st_array_len; i++) {
            print_formula( *(sys_trans_array+i), fp, FORMULA_SYNTAX_SPIN );
            if (i < st_array_len-1)
                fprintf( fp, " && " );
        }
    }
    if (num_sys_goals > 0) {
        for (i = 0; i < num_sys_goals; i++) {
            fprintf( fp, "\n#define sysgoal%04d ", i );
            print_formula( *(sys_goals+i), fp, FORMULA_SYNTAX_SPIN );
        }
    }
    fprintf( fp, "\n\n" );

    if (num_env == 0) {
        var_separator = NULL;
        evar_list = svar_list;
    } else {
        var_separator = get_list_item( evar_list, -1 );
        if (var_separator == NULL) {
            fprintf( stderr,
                     "Error: get_list_item failed on environment variables"
                     " list.\n" );
            free( env_counter );
            return -1;
        }
        var_separator->left = svar_list;
    }

    tmppt = evar_list;
    while (tmppt) {
        if (tmppt->value == -1) {
            fprintf( fp, "bool " );
        } else {
            fprintf( fp, "int " );
        }
        fprintf( fp, "%s;\n", tmppt->name );

        if (tmppt->value == -1) {
            fprintf( fp, "bool " );
        } else {
            fprintf( fp, "int " );
        }
        fprintf( fp, "%s_next;\n", tmppt->name );

        tmppt = tmppt->left;
    }

    if (num_env > 0)
        fprintf( fp,
                 "\nbool checketrans = false;"
                 "  /* Check env transition rule? */" );
    fprintf( fp,
             "\nbool checkstrans = false;"
             "  /* Check sys transition rule? */\n"
             "bool pmlfault = false;\n\n" );
    fprintf( fp, "init\n{\nint current_node;\n\n" );

    /* Initial nodes */
    fprintf( fp, "d_step {\nif" );
    node = head;
    node_counter = 0;
    while (node) {
        if (node->initial) {
            fprintf( fp, "\n:: (1) -> current_node=%d",
                     node_counter );

            tmppt = evar_list;
            for (i = 0; i < num_env+num_sys; i++) {
                fprintf( fp, "; %s=", tmppt->name );
                if (tmppt->value == -1) {
                    if (*(node->state+i)) {
                        fprintf( fp, "true" );
                    } else {
                        fprintf( fp, "false" );
                    }
                } else {
                    fprintf( fp, "%d", *(node->state+i) );
                }
                tmppt = tmppt->left;
            }
        }

        node = node->next;
        node_counter++;
    }
    fprintf( fp, "\nfi\n};\n\n" );

    /* Random environment move */
    if (num_env > 0) {
        for (i = 0; i < num_env; i++)
            *(env_counter+i) = 0;
        fprintf( fp, "ENV_MOVE:\nif" );
        while (True) {
            fprintf( fp, "\n:: (1) ->" );
            tmppt = evar_list;
            for (i = 0; i < num_env; i++) {
                fprintf( fp, " %s_next=", tmppt->name );
                if (tmppt->value == -1) {
                    if (*(env_counter+i)) {
                        fprintf( fp, "true" );
                    } else {
                        fprintf( fp, "false" );
                    }
                } else {
                    fprintf( fp, "%d", *(env_counter+i) );
                }
                if (i < num_env-1)
                    fprintf( fp, ";" );
                tmppt = tmppt->left;
            }

            /* Attempt to increment env_counter; break out of loop if done. */
            tmppt = evar_list;
            for (i = 0; i < num_env; i++) {
                if ((*(env_counter+i) > 0 && tmppt->value == -1)  /* Bool */
                    || (*(env_counter+i) >= tmppt->value
                        && tmppt->value >= 0)) {  /* Int */
                    *(env_counter+i) = 0;
                } else {
                    *(env_counter+i) += 1;
                    break;
                }
                tmppt = tmppt->left;
            }
            if (i == num_env)  /* Overflow, i.e., done enumerating? */
                break;
        }
        fprintf( fp, "\nfi;\nchecketrans = true; checketrans = false;\n\n" );
    }

    /* Take move according to strategy (FSM), or fault if there is no
       consistent edge. */
    fprintf( fp, "SYS_MOVE:\nif" );
    node = head;
    node_counter = 0;
    while (node) {
        for (j = 0; j < node->trans_len; j++) {
            fprintf( fp, "\n:: (current_node == %d", node_counter );

            tmppt = evar_list;
            for (i = 0; i < num_env; i++) {
                fprintf( fp, " && " );
                if (tmppt->value == -1) {
                    if (!(*((*(node->trans+j))->state+i))) {
                        fprintf( fp, "!" );
                    }
                    fprintf( fp, "%s_next", tmppt->name );
                } else {
                    fprintf( fp, "%s_next == %d",
                             tmppt->name, *((*(node->trans+j))->state+i) );
                }
                tmppt = tmppt->left;
            }

            next_node_counter = 0;
            next_node = head;
            while (next_node != *(node->trans+j)) {
                next_node = next_node->next;
                next_node_counter++;
            }
            fprintf( fp, ") -> current_node=%d", next_node_counter );
            tmppt = svar_list;
            for (i = num_env; i < num_env+num_sys; i++) {
                fprintf( fp, "; %s_next=", tmppt->name );
                if (tmppt->value == -1) {
                    if (*((*(node->trans+j))->state+i)) {
                        fprintf( fp, "true" );
                    } else {
                        fprintf( fp, "false" );
                    }
                } else {
                    fprintf( fp, "%d", *((*(node->trans+j))->state+i) );
                }
                tmppt = tmppt->left;
            }
        }

        node_counter++;
        node = node->next;
    }
    fprintf( fp, "\n:: else -> pmlfault=true" );
    fprintf( fp, "\nfi;\ncheckstrans = true; checkstrans = false;\n\n" );

    /* Actually apply the moves */
    fprintf( fp, "atomic { " );
    tmppt = evar_list;
    while (tmppt) {
        fprintf( fp, "%s = %s_next", tmppt->name, tmppt->name );
        tmppt = tmppt->left;
        if (tmppt)
            fprintf( fp, "; " );
    }
    if (num_env > 0) {
        fprintf( fp, " };\n\ngoto ENV_MOVE\n}\n\n" );
    } else {
        fprintf( fp, " };\n\ngoto SYS_MOVE\n}\n\n" );
    }

    if (var_separator != NULL)
        var_separator->left = NULL;

    free( env_counter );
    return 0;
}
