/** \file gr1c_util.h
 * \brief Small handy routines not otherwise sorted.
 *
 *
 * SCL; 2013, 2014.
 */


#ifndef GR1C_UTIL_H
#define GR1C_UTIL_H

#include "common.h"
#include "ptree.h"


/** Convert unsigned bitvector into integer.

   \param vec array that represents the bitvector, where the least significant
   bit is first.

   \param vec_len length of \p vec.

   \return integer obtained by unsigned interpretation of given bitvector. */
int bitvec_to_int( vartype *vec, int vec_len );

/** Convert integer to unsigned bitvector.

   \param x integer that is to be converted.

   \param vec_len length of the allocated array (bitvector).
   If \p vec_len is not sufficiently large, then the result is effectively a
   truncated form of the full bitvector for the given integer.

   \return pointer to array that represents the bitvector, where the least
   significant bit is first. The caller is assumed to free the allocated
   array. E.g., call free() on the return value.
   If vec_len < 1, then return value is NULL. */
vartype *int_to_bitvec( int x, int vec_len );


/** Expand any variables with integral domains.

   Expand any variables with integral domains in given lists of variables, where
   the expansion is as performed by var_to_bool() (cf. ptree.h). Variables that
   have type boolean are ignored.

   \param evar_list linked list of environment variables.

   \param svar_list linked list of system variables.

   \param verbose level of detail in logging; larger implies more detail. 0
   (zero) to be quiet.

   \return list of names of replaced variables. The caller is assumed to free
   this, e.g., using delete_tree(). */
ptree_t *expand_nonbool_variables( ptree_t **evar_list, ptree_t **svar_list,
                                   unsigned char verbose );

/** Return an array of length mapped_len where the nonboolean
   variables in given state array have been expanded according to
   their offsets and widths in offw.  Return NULL if error. */
vartype *expand_nonbool_state( vartype *state, int *offw, int num_nonbool,
                               int mapped_len );

/** Similar to get_offsets() except that evar_list and svar_list are
   arguments (rather than global) and the variables with nonboolean
   domains are given as a list. */
int *get_offsets_list( ptree_t *evar_list, ptree_t *svar_list,
                       ptree_t *nonbool_var_list );

/** Expand any variables with integral domains in parse trees of the
   GR(1) specification components.

   \p init_flags as used with check_realizable() and synthesize()
   (cf. solve.h) is necessary to decide how restrictions of
   unreachable values that result from nonbool expansion should be
   incorporated into initial conditions.  The _init trees are assumed
   to be well-formed given init_flags, as verified by check_gr1c_form().

   \param verbose level of detail in logging; larger implies more detail. 0
   (zero) to be quiet. */
int expand_nonbool_GR1( ptree_t *evar_list, ptree_t *svar_list,
                        ptree_t **env_init, ptree_t **sys_init,
                        ptree_t ***env_trans_array, int *et_array_len,
                        ptree_t ***sys_trans_array, int *st_array_len,
                        ptree_t ***env_goals, int num_env_goals,
                        ptree_t ***sys_goals, int num_sys_goals,
                        unsigned char init_flags,
                        unsigned char verbose );

/** Print to outf if it is not NULL.  Otherwise, dump to the log. */
void print_GR1_spec( ptree_t *evar_list, ptree_t *svar_list,
                     ptree_t *env_init, ptree_t *sys_init,
                     ptree_t **env_trans_array, int et_array_len,
                     ptree_t **sys_trans_array, int st_array_len,
                     ptree_t **env_goals, int num_env_goals,
                     ptree_t **sys_goals, int num_sys_goals,
                     FILE *outf );

/** Verify well-formedness (as much as gr1c demands) of GR(1)
   specification.  Whether the initial conditions (i.e., ENVINIT and
   SYSINIT) are in acceptable form depends on init_flags, which
   corresponds to the argument by the same name used with
   check_realizable() and synthesize() (cf. solve.h).  Use
   UNDEFINED_INIT in init_flags to avoid checking particular
   interpretations of initial conditions. */
int check_gr1c_form( ptree_t *evar_list, ptree_t *svar_list,
                     ptree_t *env_init, ptree_t *sys_init,
                     ptree_t **env_trans_array, int et_array_len,
                     ptree_t **sys_trans_array, int st_array_len,
                     ptree_t **env_goals, int num_env_goals,
                     ptree_t **sys_goals, int num_sys_goals,
                     unsigned char init_flags );

/** Print the set of bitvectors (states) on which the given BDD X
   evaluates to true.  Formatting is one state per line and on each
   line, space between every four bits.  Write to outf if it is not
   NULL.  Otherwise, dump to the log. */
void print_support( DdManager *manager, int state_len, DdNode *X, FILE *outf );


#endif
