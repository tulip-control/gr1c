Installation
============

This building and installation guide should work for anything Unix-like, but in
particular it has been tested on, in terms of architecture for which the
executable is built,
* GNU/Linux: i386, x86_64
* Mac OS X: x86_64

Aside from standard C libraries and a basic development environment (gcc, etc.),
**gr1c** depends on [CUDD](http://vlsi.colorado.edu/~fabio/CUDD/), the CU
Decision Diagram package by Fabio Somenzi and others.  Also, gr1c interactive
mode optionally uses [GNU Readline](http://www.gnu.org/software/readline)
(disabled by default; selected via `USE_READLINE` definition in Makefile).
The parser generator [Bison](http://www.gnu.org/software/bison/) and the lexical
analyzer [flex](http://flex.sourceforge.net/) are required to build gr1c.  Other
Yacc and lex compatible tools may suffice, but this has not been tested.

If the machine on which you are building has more than one CPU, then building
time may be reduced by running more than one Make job simultaneously. To do
this, use the switch `-j N` in `make` commands in these instructions, where `N`
is the number of jobs. E.g., if there are 8 CPUs available, `make -j 8`.


Building
--------

For the latest development snapshot, either [clone the
repo](https://github.com/tulip-control/gr1c) (at
https://github.com/tulip-control/gr1c.git) or [download a
tarball](https://github.com/tulip-control/gr1c/tarball/master).  For the latter,
untar the file (name may vary) and change into the source directory.

    tar -xzf tulip-control-gr1c-658f32b.tar.gz
    cd tulip-control-gr1c-658f32b

We will first build [CUDD](http://vlsi.colorado.edu/~fabio/CUDD/).

<h3>Automatic</h3>

    ./get-deps.sh
    ./build-deps.sh

<h3>Manual</h3>

We mostly follow the steps as performed by the scripts listed above. To begin,
make a directory called `extern`. At the time of writing, the latest version is
3.0.0. Below we use [cURL](http://curl.haxx.se) to download it from the
command-line. Alternatively, [wget](http://www.gnu.org/software/wget/) can be
used similarly. You might also try directing your Web browser at
<ftp://vlsi.colorado.edu/pub/>, or read CUDD documentation for instructions.

    mkdir extern
    cd extern
    curl -O ftp://vlsi.colorado.edu/pub/cudd-3.0.0.tar.gz
    tar -xzf cudd-3.0.0.tar.gz
    cd cudd-3.0.0
    ./configure --prefix=`pwd`/..
    make
    make install

The last three commands are the usual autotools idiom; we install CUDD locally
in the extern/ directory, where the gr1c Makefile expects it. Consult the README
of CUDD about alternatives, e.g., building CUDD as a shared library.

With success building CUDD, we may now build gr1c. Change back to the gr1c root
source directory and run `make`. If no errors were reported, you should be able
to get the version with

    ./gr1c -V

Later parts of this document describe [testing gr1c](#testing) and [additional
build options](#extras).


<h2 id="testing">Testing</h2>

Before deploying gr1c and especially if you are building it yourself, run the
test suite from the source distribution. Testing covers optional features of
gr1c, so accordingly, optional dependencies become required for the purposes of
testing. If you were able to build `gr1c` as described above, then it suffices
to additionally install [Graphviz dot](http://www.graphviz.org/) and
[Spin](http://spinroot.com). Try

    ./get-extra-deps.sh
    ./build-deps.sh

which will download source code for Spin and build it. If the building process
raises an error about missing `yacc` or `y?tab.c` files, then check for `yacc`
on your shell path. To do so, try

    which yacc

If it is not found but `bison` is (compare with `which bison`), then create a
shell script that wraps access to bison emulating yacc. This can be achieved by
creating a file named `yacc`, placing the following in it

    #!/bin/sh
    bison -y $@

and marking it as executable (`chmod +x yacc`). Finally, place it on your system
path, e.g., in /usr/bin or /usr/local/bin.

Tests that use Spin for model checking are configured to search in the path used
by ./build-deps.sh.  To run a different `spin`, modify `SPINEXE` in
tests/test-verification.sh

After following the build steps above, do

    make check

Optionally, set the shell variable `VERBOSE` to 1 to receive detailed progress
notifications.  E.g., try `VERBOSE=1 make check`.  Most test code is placed
under the `tests` directory. **N.B.**, scripted interaction will only work if
you build without GNU Readline.  If a test fails, despite you following the
installation instructions, then please open an [issue on
GitHub](https://github.com/tulip-control/gr1c/issues) or contact Scott Livingston
at <slivingston@cds.caltech.edu>


Placing gr1c on your shell path
-------------------------------

To be fully operational, gr1c must be on the search path of your shell.
Assuming that you have administrative privileges, one solution is

    sudo make install

which will copy `gr1c`, `gr1c-rg`, and other programs to `/usr/local/bin`, a
commonly used location for "local" installations.  This is based on a default
installation prefix of `/usr/local`.  Adjust it by invoking `make` with
something like `prefix=/your/new/path`.

If you want to work with `gr1c` directly from the building directory, then from
the same directory where `make` was invoked,

    export PATH=`pwd`:$PATH


<h2 id="extras">Extras</h2>

The gr1c repository includes experimental programs that are only of special
interest, are not (yet) reliably maintained, or otherwise are not ready for
applications in production. These are found in the directories contrib/ and
exp/, and some parts of the implementation are available through the gr1c API.
In some cases these programs can be built by providing the executable name to
`make`, e.g., `make grjit`.

[Doxygen](http://www.doxygen.org) must be installed to build the
documentation...including the page you are now reading.  Try

    make doc

and the result will be under `doc/build`.  Note that Doxygen version 1.8 or
later is required to build documentation files formatted with
[Markdown](http://daringfireball.net/projects/markdown), which have names ending
in `.md` under `doc/`.  For older versions, you can read Markdown files directly
using any plain text viewer.  E.g., the main page is `doc/api_intro.md`, and
this page is `doc/installation.md`.

You can clean the sourcetree of all executables and other temporary files by
running

    make clean
