/* util.c -- Small handy routines; declarations appear in gr1c_util.h
 *
 *
 * SCL; 2012, 2013.
 */


#define _ISOC99_SOURCE
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "gr1c_util.h"
#include "logging.h"


int bitvec_to_int( bool *vec, int vec_len )
{
	int i;
	int result = 0;
	for (i = 0; i < vec_len; i++) {
		if (*(vec+i))
			result += (1 << i);
	}
	return result;
}


bool *int_to_bitvec( int x, int vec_len )
{
	int i;
	bool *vec;
	if (vec_len < 1)
		return NULL;
	vec = malloc( vec_len*sizeof(bool) );
	if (vec == NULL) {
		perror( "int_to_bitvec, malloc" );
		return NULL;
	}
	for (i = 0; i < vec_len; i++) {
		if (x & (1 << i)) {
			*(vec+i) = 1;
		} else {
			*(vec+i) = 0;
		}
	}
	return vec;
}


ptree_t *expand_nonbool_variables( ptree_t **evar_list, ptree_t **svar_list,
								   unsigned char verbose )
{
	ptree_t *nonbool_var_list = NULL;
	ptree_t *tmppt, *expt, *prevpt, *var_separator;

	/* Make all variables boolean */
	if ((*evar_list) == NULL) {
		var_separator = NULL;
		(*evar_list) = (*svar_list);  /* that this is the deterministic case
								   is indicated by var_separator = NULL. */
	} else {
		var_separator = get_list_item( (*evar_list), -1 );
		if (var_separator == NULL) {
			fprintf( stderr, "Error: get_list_item failed on environment variables list.\n" );
			return NULL;
		}
		var_separator->left = (*svar_list);
	}
	tmppt = (*evar_list);
	while (tmppt) {
		if (tmppt->value > 0) {
			if (nonbool_var_list == NULL) {
				nonbool_var_list = init_ptree( PT_VARIABLE, tmppt->name, tmppt->value );
			} else {
				append_list_item( nonbool_var_list, PT_VARIABLE, tmppt->name, tmppt->value );
			}

		}
		tmppt = tmppt->left;
	}
	if (var_separator == NULL) {
		(*evar_list) = NULL;
	} else {
		var_separator->left = NULL;
	}

	/* Expand the variable list */
	if ((*evar_list) != NULL) {
		tmppt = (*evar_list);
		if (tmppt->value > 0) {  /* Handle special case of head node */
			expt = var_to_bool( tmppt->name, tmppt->value );
			(*evar_list) = expt;
			prevpt = get_list_item( expt, -1 );
			prevpt->left = tmppt->left;
		}
		tmppt = tmppt->left;
		while (tmppt) {
			if (tmppt->value > 0) {
				expt = var_to_bool( tmppt->name, tmppt->value );
				prevpt->left = expt;
				prevpt = get_list_item( expt, -1 );
				prevpt->left = tmppt->left;
			} else {
				prevpt = tmppt;
			}
			tmppt = tmppt->left;
		}
	}

	if ((*svar_list) != NULL) {
		tmppt = (*svar_list);
		if (tmppt->value > 0) {  /* Handle special case of head node */
			expt = var_to_bool( tmppt->name, tmppt->value );
			(*svar_list) = expt;
			prevpt = get_list_item( expt, -1 );
			prevpt->left = tmppt->left;
		}
		tmppt = tmppt->left;
		while (tmppt) {
			if (tmppt->value > 0) {
				expt = var_to_bool( tmppt->name, tmppt->value );
				prevpt->left = expt;
				prevpt = get_list_item( expt, -1 );
				prevpt->left = tmppt->left;
			} else {
				prevpt = tmppt;
			}
			tmppt = tmppt->left;
		}
	}

	return nonbool_var_list;
}


int expand_nonbool_GR1( ptree_t *evar_list, ptree_t *svar_list,
						ptree_t **env_init, ptree_t **sys_init,
						ptree_t **env_trans, ptree_t **sys_trans,
						ptree_t ***env_goals, int num_env_goals,
						ptree_t ***sys_goals, int num_sys_goals,
						unsigned char verbose )
{
	int i;
	ptree_t *tmppt, *prevpt, *var_separator;
	int maxbitval;

	/* Handle "don't care" bits */
	tmppt = evar_list;
	while (tmppt) {
		maxbitval = (int)(pow( 2, ceil(log2( tmppt->value+1 )) ));
		if (maxbitval-1 > tmppt->value) {
			if (verbose > 1)
				logprint( "In mapping %s to a bitvector, blocking values %d-%d", tmppt->name, tmppt->value+1, maxbitval-1 );
			prevpt = (*env_trans);
			(*env_trans) = init_ptree( PT_AND, NULL, 0 );
			(*env_trans)->right = prevpt;
			(*env_trans)->left = unreach_expanded_bool( tmppt->name, tmppt->value+1, maxbitval-1 );
		}
		tmppt = tmppt->left;
	}

	tmppt = svar_list;
	while (tmppt) {
		maxbitval = (int)(pow( 2, ceil(log2( tmppt->value+1 )) ));
		if (maxbitval-1 > tmppt->value) {
			if (verbose > 1)
				logprint( "In mapping %s to a bitvector, blocking values %d-%d", tmppt->name, tmppt->value+1, maxbitval-1 );
			prevpt = (*sys_trans);
			(*sys_trans) = init_ptree( PT_AND, NULL, 0 );
			(*sys_trans)->right = prevpt;
			(*sys_trans)->left = unreach_expanded_bool( tmppt->name, tmppt->value+1, maxbitval-1 );
		}
		tmppt = tmppt->left;
	}

	if (evar_list == NULL) {
		var_separator = NULL;
		evar_list = svar_list;  /* that this is the deterministic case
								   is indicated by var_separator = NULL. */
	} else {
		var_separator = get_list_item( evar_list, -1 );
		if (var_separator == NULL) {
			fprintf( stderr, "Error: get_list_item failed on environment variables list.\n" );
			return -1;
		}
		var_separator->left = svar_list;
	}
	tmppt = evar_list;
	while (tmppt) {
		if (tmppt->value > 0) {
			if (verbose > 1)
				logprint( "Expanding nonbool variable %s in SYSINIT...", tmppt->name );
			(*sys_init) = expand_to_bool( (*sys_init), tmppt->name, tmppt->value );
			if (verbose > 1) {
				logprint( "Done." );
				logprint( "Expanding nonbool variable %s in ENVINIT...", tmppt->name );
			}
			(*env_init) = expand_to_bool( (*env_init), tmppt->name, tmppt->value );
			if (verbose > 1) {
				logprint( "Done." );
				logprint( "Expanding nonbool variable %s in SYSTRANS...", tmppt->name );
			}
			(*sys_trans) = expand_to_bool( (*sys_trans), tmppt->name, tmppt->value );
			if (verbose > 1) {
				logprint( "Done." );
				logprint( "Expanding nonbool variable %s in ENVTRANS...", tmppt->name );
			}
			(*env_trans) = expand_to_bool( (*env_trans), tmppt->name, tmppt->value );
			if (verbose > 1)
				logprint( "Done." );
			if ((*sys_init) == NULL || (*env_init) == NULL || (*sys_trans) == NULL || (*env_trans) == NULL) {
				fprintf( stderr, "Failed to convert non-Boolean variable to its Boolean equivalent." );
				return -1;
			}
			for (i = 0; i < num_env_goals; i++) {
				if (verbose > 1)
					logprint( "Expanding nonbool variable %s in ENVGOAL %d...", tmppt->name, i );
				*((*env_goals)+i) = expand_to_bool( *((*env_goals)+i), tmppt->name, tmppt->value );
				if (*((*env_goals)+i) == NULL) {
					fprintf( stderr, "Failed to convert non-Boolean variable to its Boolean equivalent." );
					return -1;
				}
				if (verbose > 1)
					logprint( "Done." );
			}
			for (i = 0; i < num_sys_goals; i++) {
				if (verbose > 1)
					logprint( "Expanding nonbool variable %s in SYSGOAL %d...", tmppt->name, i );
				*((*sys_goals)+i) = expand_to_bool( *((*sys_goals)+i), tmppt->name, tmppt->value );
				if (*((*sys_goals)+i) == NULL) {
					fprintf( stderr, "Failed to convert non-Boolean variable to its Boolean equivalent." );
					return -1;
				}
				if (verbose > 1)
					logprint( "Done." );
			}
		}
		
		tmppt = tmppt->left;
	}
	if (var_separator == NULL) {
		evar_list = NULL;
	} else {
		var_separator->left = NULL;
	}

	return 0;
}


void print_GR1_spec( ptree_t *evar_list, ptree_t *svar_list,
					 ptree_t *env_init, ptree_t *sys_init,
					 ptree_t *env_trans, ptree_t *sys_trans,
					 ptree_t **env_goals, int num_env_goals,
					 ptree_t **sys_goals, int num_sys_goals,
					 FILE *outf )
{
	int i;
	ptree_t *tmppt;
	FILE *prev_logf;
	int prev_logoptions;

	if (outf != NULL) {
		prev_logf = getlogstream();
		prev_logoptions = getlogopt();
		setlogstream( outf );
		setlogopt( LOGOPT_NOTIME );
	}

	logprint_startline(); logprint_raw( "ENV:" );
	tmppt = evar_list;
	while (tmppt) {
		logprint_raw( " %s", tmppt->name );
		tmppt = tmppt->left;
	}
	logprint_raw( ";" ); logprint_endline();

	logprint_startline(); logprint_raw( "SYS:" );
	tmppt = svar_list;
	while (tmppt) {
		logprint_raw( " %s", tmppt->name );
		tmppt = tmppt->left;
	}
	logprint_raw( ";" ); logprint_endline();

	logprint_startline(); logprint_raw( "ENV INIT:  " );
	print_formula( env_init, getlogstream() );
	logprint_raw( ";" ); logprint_endline();

	logprint_startline(); logprint_raw( "SYS INIT:  " );
	print_formula( sys_init, getlogstream() );
	logprint_raw( ";" ); logprint_endline();

	logprint_startline(); logprint_raw( "ENV TRANS:  [] " );
	print_formula( env_trans, getlogstream() );
	logprint_raw( ";" ); logprint_endline();

	logprint_startline(); logprint_raw( "SYS TRANS:  [] " );
	print_formula( sys_trans, getlogstream() );
	logprint_raw( ";" ); logprint_endline();

	logprint_startline(); logprint_raw( "ENV GOALS:  " );
	if (num_env_goals == 0) {
		logprint_raw( "(none)" );
	} else {
		logprint_raw( "[]<> " );
		print_formula( *env_goals, getlogstream() );
		for (i = 1; i < num_env_goals; i++) {
			logprint_raw( " & []<> " );
			print_formula( *(env_goals+i), getlogstream() );
		}
	}
	logprint_raw( ";" ); logprint_endline();

	logprint_startline(); logprint_raw( "SYS GOALS:  " );
	if (num_sys_goals == 0) {
		logprint_raw( "(none)" );
	} else {
		logprint_raw( "[]<> " );
		print_formula( *sys_goals, getlogstream() );
		for (i = 1; i < num_sys_goals; i++) {
			logprint_raw( " & []<> " );
			print_formula( *(sys_goals+i), getlogstream() );
		}
	}
	logprint_raw( ";" ); logprint_endline();

	if (outf != NULL) {
		setlogstream( prev_logf );
		setlogopt( prev_logoptions );
	}
}
