/** \file patching.h
 * \brief Implementation of patching and incremental synthesis algorithms.
 *
 *
 * SCL; 2012.
 */


#ifndef PATCHING_H
#define PATCHING_H

#include "solve.h"
#include "automaton.h"


/** Read given strategy in "gr1c automaton" format from stream
   strategy_fp, using edge set changes to the game graph listed in
   stream change_fp.  See [external_notes](md_formats.html) for format
   details.  If strategy_fp = NULL, then read from stdin.

   Return the head pointer of the patched strategy, or NULL if error.

   As usual, this function will not try to close given streams when
   finished. */
anode_t *patch_localfixpoint( DdManager *manager, FILE *strategy_fp, FILE *change_fp, unsigned char verbose );


#endif
