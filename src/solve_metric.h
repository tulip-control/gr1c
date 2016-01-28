/** \file solve_metric.h
 * \brief Synthesis routines that use a distance between states.
 *
 *
 * SCL; 2013-2014
 */


#ifndef SOLVE_METRIC_H
#define SOLVE_METRIC_H

#include "common.h"


/** Return an array of length 2n, where n is the number of entries in
   metric_vars.  The values at indices 2i and 2i+1 are the offset and
   width of the i-th variable's binary representation.  Return NULL if
   error.  metric_vars is a space-separated list of variables to use
   in computing distance. The caller is expected to free the array.

   This function assumes that evar_list and svar_list have not been
   linked.  get_offsets_list() is a more general version. */
int *get_offsets( char *metric_vars, int *num_vars );

/** G is the goal set against which to measure distance.
   Result is written into given variables Min and Max;
   return 0 on success, -1 error. */
int bounds_DDset( DdManager *manager, DdNode *T, DdNode *G,
                  int *offw, int num_metric_vars,
                  double *Min, double *Max, unsigned char verbose );

int bounds_state( DdManager *manager, DdNode *T, vartype *ref_state,
                  int *offw, int num_metric_vars,
                  double *Min, double *Max, unsigned char verbose );

int compute_horizon( DdManager *manager, DdNode **W,
                     DdNode **etrans, DdNode **strans, DdNode ***sgoals,
                     char *metric_vars, unsigned char verbose );


#endif
