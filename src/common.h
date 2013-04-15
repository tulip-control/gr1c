/** \file common.h
 * \brief Project-wide definitions and macros.
 *
 *
 * SCL; 2012, 2013.
 */


#ifndef COMMON_H
#define COMMON_H


#define GR1C_VERSION "0.5.13"
#define GR1C_COPYRIGHT "Copyright (c) 2012, 2013 by Scott C. Livingston,\nCalifornia Institute of Technology\n\nThis is free, open source software, released under a BSD license\nand without warranty."


#define GR1C_INTERACTIVE_PROMPT ">>> "


typedef int vartype;
typedef char bool;
#define True 1
#define False 0

typedef unsigned char byte;


#include "util.h"
#include "cudd.h"

#endif
