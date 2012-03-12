Introduction
============

Scott C. Livingston  <slivingston@caltech.edu>

**gr1c** is a tool for checking realizability of and synthesizing
strategies for GR(1) specifications.  The fixpoint formula defining
the winning set is given (and its correctness proven) in

    Y. Kesten, N. Piterman, and A. Pnueli (2005). Bridging the gap between
    fair simulation and trace inclusion. *Information and Computation*,
    200: 35--61. DOI:`10.1016/j.ic.2005.01.006 <http://dx.doi.org/10.1016/j.ic.2005.01.006>`_.

**gr1c** depends on CUDD, the CU Decision Diagram package by Fabio Somenzi
and others.  See http://vlsi.colorado.edu/~fabio/CUDD/


Installation
============

CUDD (see above) should be placed in a directory called "extern".  Be
sure that similar flags are used for compiling both gr1c and CUDD.  In
particular, SIZEOF_VOID_P and SIZEOF_LONG need to be set to the sizes
(in bytes) of void pointer (``void *``) and ``long int`` on your system. gr1c
ships with a Makefile that has these set for Intel x86 64-bit.

If you do not know the appropriate sizes for your system, then write a
C program that prints ``sizeof(void *)`` and ``sizeof(long)`` to find them.
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

*Summary:* C-like, infix syntax. Comments begin with ``#`` and
continue until the end of line. The grammar below is not complete (nor
normative), but should be enough for you to compose
specifications. Also look under ``examples`` directory.

::

  /* Define which variables are controlled and uncontrolled. */
  envvarlist ::= 'ENV:' VARIABLE | envvarlist VARIABLE
  sysvarlist ::= 'SYS:' VARIABLE | sysvarlist VARIABLE

  propformula ::= "False" | "True" | VARIABLE | '!' propformula | propformula '&' propformula | propformula '|' propformula | propformula "->" propformula | VARIABLE '=' NUMBER | '(' propformula ')'

  /* The only difference between propformula and tpropformula is variables can be primed (next operator) in the latter. */
  tpropformula ::= "False" | "True" | VARIABLE | VARIABLE '\'' | '!' tpropformula | tpropformula '&' tpropformula | tpropformula '|' tpropformula | tpropformula "->" tpropformula | VARIABLE '=' NUMBER | '(' tpropformula ')'

  transformula ::= "[]" tpropformula | transformula '&' transformula
  goalformula ::= "[]<>" propformula | goalformua '&' goalformula


Interactive gr1c
================

If started with the command-line flag "-i", **gr1c** will compute the
sublevel sets from the fixpoint computation of the winning set and
then accept commands until "quit" or end-of-file reached.  States
should be given as space separated values.  Recognized commands are:

winning STATE
  is STATE in winning set?

envnext STATE
  list of possible next valuations of environment variables, given
  current STATE; each is on a separate line, with termination
  indicated by a final line of "---".

sysnext STATE1 STATE2ENV
  list of possible next valuations of system variables, given current
  STATE1 and environment move STATE2ENV from that state; each is on a
  separate line, with termination indicated by a final line of "---".

restrict STATE1 STATE2
  remove the edge from STATE1 to STATE2, if present.

relax STATE1 STATE2
  add an edge from STATE1 to STATE2.

un{restrict,relax}
  clear all restrictions or relaxations thus far.

unreachable STATE
  make STATE unreachable, i.e., all ingoing edges are removed.

getindex STATE GOALMODE
  get reachability index of STATE for goal index GOALMODE.

setindex STATE GOALMODE
  set reachability index of STATE for goal index GOALMODE; internally
  this changes sublevel membership of STATE.

refresh winning
  compute winning set (presumably after transition (safety) rules have
  changed from restrict and relax commands); this does *not* change
  the sublevel sets.

refresh levels
  compute (sub)level sets (presumably after transition (safety) rules
  have changed from restrict and relax commands); this command also
  causes the winning set to be computed (again).

addvar {env,sys} VARIABLE
  add VARIABLE to list of environment (resp. system) variables if
  "env" (resp. "sys").

envvar
  print list of environment variables in order.

sysvar
  print list of system variables in order.

var
  print all variable names with indices; your state values must match
  this order.

numgoals
  print number of system goals.

{enable,disable} autoreorder
  enable (resp. disable) automatic BDD reordering.

quit
  terminate gr1c.


License
=======

This is free software released under the terms of the GNU General
Public License, version 3.  There is no warranty; not even for
merchantability or fitness for a particular purpose.  See LICENSE.txt
for copying conditions.
