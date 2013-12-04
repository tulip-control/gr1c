Introduction
============

Scott C. Livingston  <slivingston@cds.caltech.edu>

**gr1c** is a collection of tools for GR(1) synthesis and related activities.
Its core functionality is checking realizability of and synthesizing strategies
for GR(1) specifications, though it does much more.

**gr1c** depends on `CUDD <http://vlsi.colorado.edu/~fabio/CUDD/>`_,
the CU Decision Diagram package by Fabio Somenzi and others.


Installation
============

Releases
--------

Releases are posted at http://vehicles.caltech.edu/snapshots/

A different location may eventually be chosen, but that URL will be valid at
least until the year 2015.  Pre-built binaries for several platforms are
available, though the preferred distribution form is as a source bundle.


Building from Source
--------------------

Detailed installation instructions are available in the documentation (see
below).  This section provides an outline.

For building entirely from source code, CUDD (see above) should be placed in a
directory called "extern".  The variable CUDD_ROOT in ``Makefile`` should
indicate this directory and may need to be adjusted.  Be sure that similar flags
are used for compiling both gr1c and CUDD.  In particular, SIZEOF_VOID_P and
SIZEOF_LONG need to be set to the sizes (in bytes) of void pointer (``void *``)
and ``long int`` on your system.  gr1c ships with a Makefile that has these set
for Intel x86 64-bit, which will likely succeed for Linux x86_64 and Mac OS X.
More generally, it should suffice to copy your setting of XCFLAGS in the
``Makefile`` of CUDD to the variable CUDD_XCFLAGS in the ``Makefile`` of gr1c.

Once CUDD is built, all gr1c programs may be built and installed ::

  $ make all
  $ make install expinstall

The latter ``expinstall`` installs experimental tools, like ``grpatch``.  The
default installation prefix is /usr/local.  Adjust it by invoking ``make`` with
something like ``prefix=/your/new/path``.

A test suite is available.  To run it, after building gr1c, ::

  $ make check

Each testing step is reported if the environment variable VERBOSE is set to 1.
E.g., try ``VERBOSE=1 make check``.


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
