/** \file common.h
 * \brief Project-wide definitions and macros.
 *
 *
 * SCL; 2012.
 */


#ifndef COMMON_H
#define COMMON_H


#define GR1C_VERSION "0.4e"
#define GR1C_COPYRIGHT "Copyright (c) 2012 by Scott C. Livingston, California Institute of Technology\nThis is free software, released under the GNU GPLv3 and without warranty."


#define GR1C_INTERACTIVE_PROMPT ">>> "


typedef char bool;
#define True 1
#define False 0

typedef unsigned char byte;


#include "util.h"
#include "cudd.h"

#endif
