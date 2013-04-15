/** \file gr1c_util.h
 * \brief Small handy routines not otherwise sorted.
 *
 *
 * SCL; 2013.
 */


#ifndef GR1C_UTIL_H
#define GR1C_UTIL_H

#include "common.h"
#include "ptree.h"


/** least significant bit first; unsigned. */
int bitvec_to_int( vartype *vec, int vec_len );

/** least significant bit first; unsigned.  Up to given length vec_len;
   if vec_len is not sufficiently large, then the result is
   effectively a truncated form of the full bitvector for the given
   integer.  The caller is assumed to free the bitvector. */
vartype *int_to_bitvec( int x, int vec_len );


/* Expand any variables with integral domains in given lists of
   environment and system variables (evar_list and svar_list, respectively). */
ptree_t *expand_nonbool_variables( ptree_t **evar_list, ptree_t **svar_list,
								   unsigned char verbose );

/* Expand any variables with integral domains in parse trees of the
   GR(1) specification components */
int expand_nonbool_GR1( ptree_t *evar_list, ptree_t *svar_list,
						ptree_t **env_init, ptree_t **sys_init,
						ptree_t **env_trans, ptree_t **sys_trans,
						ptree_t ***env_goals, int num_env_goals,
						ptree_t ***sys_goals, int num_sys_goals,
						unsigned char verbose );

/* Print to outf if it is not NULL.  Otherwise, dump to the log. */
void print_GR1_spec( ptree_t *evar_list, ptree_t *svar_list,
					 ptree_t *env_init, ptree_t *sys_init,
					 ptree_t *env_trans, ptree_t *sys_trans,
					 ptree_t **env_goals, int num_env_goals,
					 ptree_t **sys_goals, int num_sys_goals,
					 FILE *outf );

/* Verify well-formedness (as much as gr1c demands) of GR(1) specification. */
int check_gr1c_form( ptree_t *evar_list, ptree_t *svar_list,
					 ptree_t *env_init, ptree_t *sys_init,
					 ptree_t **env_trans_array, int et_array_len,
					 ptree_t **sys_trans_array, int st_array_len,
					 ptree_t **env_goals, int num_env_goals,
					 ptree_t **sys_goals, int num_sys_goals );

#endif
