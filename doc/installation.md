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
is the number of jobs. E.g., if there are 8 CPUs available, `make -j 8 all`.


Building
--------

For the latest development snapshot, either [clone the
repo](https://github.com/tulip-control/gr1c) (at
https://github.com/tulip-control/gr1c.git) or [download a
tarball](https://github.com/tulip-control/gr1c/tarball/master).  For the latter,
untar the file (name may vary) and change into the source directory.

    $ tar -xzf tulip-control-gr1c-658f32b.tar.gz
    $ cd tulip-control-gr1c-658f32b

We will first build [CUDD](http://vlsi.colorado.edu/~fabio/CUDD/).

<h3>Automatic</h3>

    $ ./get-deps.sh
    $ ./build-deps.sh

<h3>Manual</h3>

We mostly follow the steps as performed by the scripts listed above. To begin,
make a directory called `extern`. At the time of writing, the latest version is
3.0.0. Below we use [cURL](http://curl.haxx.se) to download it from the
command-line. Alternatively, [wget](http://www.gnu.org/software/wget/) can be
used similarly. You might also try directing your Web browser at
<ftp://vlsi.colorado.edu/pub/>, or read CUDD documentation for instructions.

    $ mkdir extern
    $ cd extern
    $ curl -O ftp://vlsi.colorado.edu/pub/cudd-3.0.0.tar.gz
    $ tar -xzf cudd-3.0.0.tar.gz
    $ cd cudd-3.0.0
    $ ./configure --prefix=`pwd`/..
    $ make
    $ make install

The last three commands are the usual autotools idiom; we install CUDD locally
in the extern/ directory, where the gr1c Makefile expects it. Consult the README
of CUDD about alternatives, e.g., building CUDD as a shared library.

With success building CUDD, we may now build gr1c. Change back to the gr1c root
source directory and run `make`. If no errors were reported, you should be able
to get the version with

    $ ./gr1c -V

Later parts of this document describe [testing gr1c](#testing) and [additional
build options](#extras).


<h2 id="testing">Testing</h2>

Before deploying gr1c and especially if you are building it yourself, run the
test suite from the source distribution. After following the build steps above,
do

    $ make check

Optionally, set the shell variable `VERBOSE` to 1 to receive detailed progress
notifications.  E.g., try `VERBOSE=1 make check`.  Most test code is placed
under the `tests` directory. **N.B.**, scripted interaction will only work if
you build without GNU Readline.  If a test fails, despite you following the
installation instructions, then please open an [issue on
GitHub](https://github.com/tulip-control/gr1c/issues) or contact Scott Livingston
at <slivingston@cds.caltech.edu>


Placing gr1c on your shell path
-------------------------------

To use the program `gr1c` outside of the directory you just built it in, you
will need to add it to the search path of your shell. Assuming you have
administrative privileges, one solution is

    $ sudo make install

which will copy `gr1c` and `rg` to `/usr/local/bin`, a commonly used location
for "local" installations.  This is based on a default installation prefix of
`/usr/local`.  Adjust it by invoking `make` with something like
`prefix=/your/new/path`.  The programs `grpatch` and `grjit` are *not* included
in as an effect of `make install`.  [Extra instructions](#extras) concerning
these are below.


<h2 id="extras">Extras</h2>

Running `make`, as described above, will not cause the programs `grpatch` and
`grjit` to be built.  To achieve this,

    $ make all

Alternatively, you can request a specific executable, e.g., `make grpatch`.  To
install `grpatch` in the same place as `gr1c`,

    $ make expinstall

Other experiment-related programs, such as `grjit`, will eventually be added to
the list installed by the expinstall command.  [Doxygen](http://www.doxygen.org)
must be installed to build the documentation...including the page you are now
reading.  Try

    $ make doc

and the result will be under `doc/build`.  Note that Doxygen version 1.8 or
later is required to build documentation files formatted with
[Markdown](http://daringfireball.net/projects/markdown), which have names ending
in `.md` under `doc/`.  For older versions, you can read Markdown files directly
using any plain text viewer.  E.g., the main page is `doc/api_intro.md`, and
this page is `doc/installation.md`.

You can clean the sourcetree of all executables and other temporary files by
running

    $ make clean
