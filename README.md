Introduction
============

**gr1c** is a collection of tools for GR(1) synthesis and related activities.
Its core functionality is checking realizability of and synthesizing strategies
for GR(1) specifications, though it does much more.

The citable URL is http://scottman.net/2012/gr1c

The public Git repo can be cloned from https://github.com/tulip-control/gr1c.git
Documentation for the latest release is at https://tulip-control.github.io/gr1c
Bug reports, feature requests, and substantial comments can be submitted via the
[project issue tracker](https://github.com/tulip-control/gr1c/issues) or via
email to the authors.


Examples and Documentation
==========================

Many examples are provided.  Begin by reading `examples/README.md`.

The main documentation is built from `.md` files under the `doc` directory and
API comments in the source code. It is possible to read these files directly,
i.e., without building and browsing HTML files.

    make doc

will run [Doxygen](http://www.doxygen.org) and place the result in `doc/build`.


Building and installation
=========================

We use Travis CI to build and test gr1c from the current source code in the
repository. [![Build Status](https://travis-ci.org/tulip-control/gr1c.svg?branch=master)](https://travis-ci.org/tulip-control/gr1c)

Dependencies
------------

- [CUDD](http://vlsi.colorado.edu/~fabio/CUDD/), the CU Decision Diagram package
  by Fabio Somenzi and others.


### Optional

The following are optional dependencies. Each item is followed by a summary of
what is to be gained by building gr1c with it.

- [GNU Readline](http://www.gnu.org/software/readline/), for an enhanced command
  prompt during interactive sessions. Note that there is a built-in prompt.


Building from Source
--------------------

Detailed installation instructions are in the repository at doc/installation.md
For the most recent release, a copy of these instructions is on the Web at
https://tulip-control.github.io/gr1c/md_installation.html

For Linux x86_64 and Mac OSX, try the following.

    ./get-deps.sh
    ./build-deps.sh
    make
    make check

Consider using `make -j N` where N is the number of jobs to run simultaneously.
The last command runs a test suite. Each testing step is reported if the
environment variable VERBOSE is set to 1.  E.g., try `VERBOSE=1 make check`.
Finally,

    make install

The default installation prefix is /usr/local.  Adjust it by invoking `make`
with something like `prefix=/your/new/path`.


License
=======

This is free software released under the terms of [the BSD 3-Clause License](
https://opensource.org/licenses/BSD-3-Clause).  There is no warranty; not even
for merchantability or fitness for a particular purpose.  Consult LICENSE.txt
for copying conditions.
