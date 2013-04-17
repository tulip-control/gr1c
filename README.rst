Introduction
============

Scott C. Livingston  <slivingston@cds.caltech.edu>

**gr1c** is a tool for GR(1) synthesis and related activities.  Its core
functionality is checking realizability of and synthesizing strategies for GR(1)
specifications, though it does much more.

**rg** is a tool for solving "reachability games," which are similar to GR(1)
formulae except with at most one system goal and where that system goal must be
reached once (not necessarily infinitely often).  The accepted input
specifications are slightly different; the single system goal is preceded by
"<>" (as the "eventually" operator).

**gr1c** depends on `CUDD <http://vlsi.colorado.edu/~fabio/CUDD/>`_,
the CU Decision Diagram package by Fabio Somenzi and others.


Build and Installation
======================

For installation, CUDD (see above) should be placed in a directory called
"extern".  Be sure that similar flags are used for compiling both gr1c and CUDD.
In particular, SIZEOF_VOID_P and SIZEOF_LONG need to be set to the sizes (in
bytes) of void pointer (``void *``) and ``long int`` on your system. gr1c ships
with a Makefile that has these set for Intel x86 64-bit.

If you do not know the appropriate sizes for your system, then write a C program
that prints ``sizeof(void *)`` and ``sizeof(long)`` to find them.  E.g., ::

  #include <stdio.h>
  int main()
  {
      printf( "void *: %lu\nlong: %lu\n", sizeof(void *), sizeof(long) );
      return 0;
  }

Once CUDD is built, it suffices to run ``make``.  If this fails, first
look in the Makefile to verify the expected relative location of CUDD
(named "CUDD_ROOT").

A test suite is available.  To run it, after building gr1c, ::

  $ make check

Each testing step is reported if the environment variable VERBOSE is set to 1.
E.g., try ``VERBOSE=1 make check``.  Detailed installation instructions are
available in the documentation (see below).


Examples and Documentation
==========================

Many examples are provided.  Begin by reading ``examples/README.txt``.

Documentation is available under the ``doc`` directory and is built using
`Doxygen <http://www.doxygen.org>`_; version 1.8.0 or later required for
Markdown support.  To build it, try ::

  $ make doc

and the result will be under ``doc/build``. Snapshots are occasionally posted at
http://slivingston.github.io/gr1c


License
=======

This is free software released under the terms of `the BSD 3-Clause License
<http://opensource.org/licenses/BSD-3-Clause>`_.  There is no warranty; not even
for merchantability or fitness for a particular purpose.  See LICENSE.txt for
copying conditions.
