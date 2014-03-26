/** \file common.h
 * \brief Project-wide definitions and macros.
 *
 *
 * SCL; 2012-2014.
 */


#ifndef COMMON_H
#define COMMON_H


#define GR1C_VERSION "0.8.1"
#define GR1C_COPYRIGHT "Copyright (c) 2012-2014 by Scott C. Livingston,\n" \
	"California Institute of Technology\n\n" \
	"This is free, open source software, released under a BSD license\n" \
	"and without warranty."


#define GR1C_INTERACTIVE_PROMPT ">>> "


typedef int vartype;
typedef char bool;
#define True 1
#define False 0

typedef unsigned char byte;


#include "util.h"
#include "cudd.h"

#endif
