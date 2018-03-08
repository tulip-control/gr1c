/** \file common.h
 * \brief Project-wide definitions and macros.
 *
 *
 * SCL; 2012-2015
 */


#ifndef COMMON_H
#define COMMON_H

#include "ptree.h"


#ifndef GR1C_DEVEL
#define GR1C_DEVEL ""
#endif
#define GR1C_VERSION "0.13.0" GR1C_DEVEL
#define GR1C_COPYRIGHT "Copyright (c) 2012-2018 by California Institute of Technology and contributors,\n" \
                       "This is free, open source software, released under a BSD license\n" \
                       "and without warranty."


#define GR1C_INTERACTIVE_PROMPT ">>> "


typedef int vartype;
typedef char bool;
#define True 1
#define False 0

typedef unsigned char byte;

typedef struct {
    ptree_t *nonbool_var_list;
    int original_num_env;
    int original_num_sys;
    int *offw;

    ptree_t *evar_list;
    ptree_t *svar_list;
    ptree_t *env_init;
    ptree_t *sys_init;
    ptree_t *env_trans;  /* Built from components in env_trans_array. */
    ptree_t *sys_trans;  /* Built from components in sys_trans_array. */
    ptree_t **env_goals;
    ptree_t **sys_goals;
    int num_egoals;
    int num_sgoals;

    ptree_t **env_trans_array;
    ptree_t **sys_trans_array;
    int et_array_len;
    int st_array_len;
} specification_t;

#define SPC_INIT(X) \
    X.nonbool_var_list = NULL; \
    X.original_num_env = 0; \
    X.original_num_sys = 0; \
    X.offw = NULL; \
    X.evar_list = NULL; \
    X.svar_list = NULL; \
    X.sys_init = NULL; \
    X.env_init = NULL; \
    X.env_goals = NULL; \
    X.sys_goals = NULL; \
    X.num_egoals = 0; \
    X.num_sgoals = 0; \
    X.env_trans_array = NULL; \
    X.sys_trans_array = NULL; \
    X.et_array_len = 0; \
    X.st_array_len = 0


#include "cudd.h"


#ifdef USE_READLINE
#include <readline/readline.h>
#define READLINE_PRINT_VERSION() \
    printf( "readline %d.%d\n", RL_VERSION_MAJOR, RL_VERSION_MAJOR )
#else
#define READLINE_PRINT_VERSION()
#endif
#define PRINT_LINKED_VERSIONS() \
    printf( "\nLinked with the following externals:\n" ); \
    READLINE_PRINT_VERSION(); \
    printf( "CUDD " ); \
    Cudd_PrintVersion( stdout )


#endif
