/* Enumerate states in winning set.
 *
 * An example of using the gr1c API. Provide the specification by invoking with
 * a file name. It prints states with variable ordering as in the specification.
 *
 *
 * SCL; 2017
 */


#include <stdlib.h>
#include <stdio.h>

#include "solve.h"
#include "gr1c_util.h"
extern int yyparse( void );
extern void yyrestart( FILE *new_file );


/**************************
 **** Global variables ****/

extern specification_t spc;

/**************************/


int main( int argc, char **argv )
{
    FILE *fp;
    int j;  /* Generic counter */
    unsigned char verbose = 0;
    unsigned char init_flags = ALL_ENV_EXIST_SYS_INIT;
    DdManager *manager;
    int num_env, num_sys;
    DdNode *W = NULL;

    if (argc < 2) {
        printf( "Usage: printwin FILE\n" );
        return 0;
    }

    /* Parse the specification. */
    fp = fopen( argv[1], "r" );
    if (fp == NULL) {
        perror( __FILE__ ",  fopen" );
        return -1;
    }
    yyrestart( fp );
    SPC_INIT( spc );
    if (yyparse())
        return 2;
    fclose( fp );

    if (check_gr1c_form( spc.evar_list, spc.svar_list, spc.env_init, spc.sys_init,
                         spc.env_trans_array, spc.et_array_len,
                         spc.sys_trans_array, spc.st_array_len,
                         spc.env_goals, spc.num_egoals, spc.sys_goals, spc.num_sgoals,
                         init_flags ) < 0)
        return 2;

    /* Omission implies empty. */
    if (spc.et_array_len == 0) {
        spc.et_array_len = 1;
        spc.env_trans_array = malloc( sizeof(ptree_t *) );
        if (spc.env_trans_array == NULL) {
            perror( __FILE__ ",  malloc" );
            return -1;
        }
        *spc.env_trans_array = init_ptree( PT_CONSTANT, NULL, 1 );
    }
    if (spc.st_array_len == 0) {
        spc.st_array_len = 1;
        spc.sys_trans_array = malloc( sizeof(ptree_t *) );
        if (spc.sys_trans_array == NULL) {
            perror( __FILE__ ",  malloc" );
            return -1;
        }
        *spc.sys_trans_array = init_ptree( PT_CONSTANT, NULL, 1 );
    }
    if (spc.num_sgoals == 0) {
        spc.num_sgoals = 1;
        spc.sys_goals = malloc( sizeof(ptree_t *) );
        if (spc.sys_goals == NULL) {
            perror( __FILE__ ",  malloc" );
            return -1;
        }
        *spc.sys_goals = init_ptree( PT_CONSTANT, NULL, 1 );
    }

    if (expand_nonbool_GR1( spc.evar_list, spc.svar_list, &spc.env_init, &spc.sys_init,
                            &spc.env_trans_array, &spc.et_array_len,
                            &spc.sys_trans_array, &spc.st_array_len,
                            &spc.env_goals, spc.num_egoals, &spc.sys_goals, spc.num_sgoals,
                            init_flags, verbose ) < 0) {
        return -1;
    }
    spc.nonbool_var_list = expand_nonbool_variables( &spc.evar_list, &spc.svar_list,
                                                     verbose );

    /* Merge component safety (transition) formulas */
    if (spc.et_array_len > 1) {
        spc.env_trans = merge_ptrees( spc.env_trans_array, spc.et_array_len, PT_AND );
    } else if (spc.et_array_len == 1) {
        spc.env_trans = *spc.env_trans_array;
    } else {
        fprintf( stderr,
                 "Syntax error: GR(1) specification is missing environment"
                 " transition rules.\n" );
        return 2;
    }
    if (spc.st_array_len > 1) {
        spc.sys_trans = merge_ptrees( spc.sys_trans_array, spc.st_array_len, PT_AND );
    } else if (spc.st_array_len == 1) {
        spc.sys_trans = *spc.sys_trans_array;
    } else {
        fprintf( stderr,
                 "Syntax error: GR(1) specification is missing system"
                 " transition rules.\n" );
        return 2;
    }

    num_env = tree_size( spc.evar_list );
    num_sys = tree_size( spc.svar_list );

    manager = Cudd_Init( 2*(num_env+num_sys),
                         0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, verbose );
    Cudd_SetMaxCacheHard( manager, (unsigned int)-1 );
    Cudd_AutodynEnable( manager, CUDD_REORDER_SAME );


    W = check_realizable( manager, init_flags, 0 );
    print_support( manager, num_env+num_sys, W, stdout );


    /* Clean-up */
    delete_tree( spc.evar_list );
    delete_tree( spc.svar_list );
    delete_tree( spc.nonbool_var_list );
    delete_tree( spc.env_init );
    delete_tree( spc.sys_init );
    delete_tree( spc.env_trans );
    free( spc.env_trans_array );
    delete_tree( spc.sys_trans );
    free( spc.sys_trans_array );
    for (j = 0; j < spc.num_egoals; j++)
        delete_tree( *(spc.env_goals+j) );
    if (spc.num_egoals > 0)
        free( spc.env_goals );
    for (j = 0; j < spc.num_sgoals; j++)
        delete_tree( *(spc.sys_goals+j) );
    if (spc.num_sgoals > 0)
        free( spc.sys_goals );

    if (W != NULL)
        Cudd_RecursiveDeref( manager, W );
    Cudd_Quit(manager);

    return 0;
}
