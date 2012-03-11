Introduction
============

Scott C. Livingston  <slivingston@caltech.edu>

**gr1c** is a tool for checking realizability of and synthesizing
strategies for GR(1) specifications.  The fixpoint formula defining
the winning set is given (and its correctness proven) in

Y. Kesten, N. Piterman, and A. Pnueli (2005). Bridging the gap between
fair simulation and trace inclusion. *Information and Computation*,
200: 35--61. DOI: 10.1016/j.ic.2005.01.006

gr1c depends on CUDD, the CU Decision Diagram package by Fabio Somenzi
and others.  See http://vlsi.colorado.edu/~fabio/CUDD/


Installation
============

CUDD (see above) should be placed in a directory called "extern".  Be
sure that similar flags are used for compiling both gr1c and CUDD.  In
particular, SIZEOF_VOID_P and SIZEOF_LONG need to be set to the sizes
(in bytes) of void pointer (void *) and long int on your system. gr1c
ships with a Makefile that has these set for Intel x86 64-bit.

If you do not know the appropriate sizes for your system, then write a
C program that prints sizeof(void *) and sizeof(long) to find them.
E.g., ::

  #include <stdio.h>
  int main()
  {
      printf( "void *: %lu\nlong: %lu\n", sizeof(void *), sizeof(long) );
      return 0;
  }

Once CUDD is built, it suffices to run ``make``.  If this fails, first
look in the Makefile to verify the expected relative location of CUDD
(named "CUDD_ROOT").


Grammar for GR(1)
=================

*Summary:* C-like, infix syntax.

/* Define which variables are controlled and uncontrolled. */
envvarlist ::= envvarlist VARIABLE | 'ENV:' VARIABLE
sysvarlist ::= sysvarlist VARIABLE | 'SYS:' VARIABLE

propformula ::= "False" | "True" | VARIABLE | '!' propformula | propformula '&' propformula | propformula '|' propformula | '(' propformula ')' | propformula "->" propformula | VARIABLE '=' NUMBER

/* The only difference between propformula and tpropformula is variables can be primed (next operator) in the latter. */
tpropformula ::= "False" | "True" | VARIABLE | VARIABLE '\'' | '!' propformula | propformula '&' propformula | propformula '|' propformula | '(' propformula ')' | propformula "->" propformula | VARIABLE '=' NUMBER

transformula ::= "[]" tpropformula | transformula '&' transformula
goalformula ::= "[]<>" propformula | goalformua '&' goalformula


License
=======

This is free software released under the terms of the GNU General
Public License, version 3.  No warranty.
