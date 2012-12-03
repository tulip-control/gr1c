Introduction
============

Scott C. Livingston  <slivingston@caltech.edu>

**gr1c** is a tool for GR(1) synthesis and related activities.
Its core functionality is checking realizability of and synthesizing
strategies for GR(1) specifications.  The fixpoint formula defining
the winning set is given (and its correctness proven) in

    Y. Kesten, N. Piterman, and A. Pnueli (2005). Bridging the gap between
    fair simulation and trace inclusion. *Information and Computation*,
    200: 35--61. DOI:`10.1016/j.ic.2005.01.006 <http://dx.doi.org/10.1016/j.ic.2005.01.006>`_.

Documentation is available under the ``doc`` directory. API-related
documents are built with `Doxygen <http://www.doxygen.org>`_; version
1.8.0 or later required for Markdown support.  Snapshots are
occasionally posted at http://slivingston.github.com/gr1c

**rg** is a tool for solving "reachability games," which are similar
to GR(1) formulae except with at most one system goal and where that
system goal must be reached once (not necessarily infinitely often).
The accepted input specifications are slightly different; the single
system goal is preceded by "<>" (as the "eventually" operator).

**gr1c** depends on CUDD, the CU Decision Diagram package by Fabio Somenzi
and others.  See http://vlsi.colorado.edu/~fabio/CUDD/


Installation
============

A test suite is available. To run it, after building gr1c, ::

  $ make check

For installation, CUDD (see above) should be placed in a directory
called "extern".  Be sure that similar flags are used for compiling
both gr1c and CUDD.  In particular, SIZEOF_VOID_P and SIZEOF_LONG need
to be set to the sizes (in bytes) of void pointer (``void *``) and
``long int`` on your system. gr1c ships with a Makefile that has these
set for Intel x86 64-bit.

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


License
=======

This is free software released under the terms of the GNU General
Public License, version 3.  There is no warranty; not even for
merchantability or fitness for a particular purpose.  See LICENSE.txt
for copying conditions.
