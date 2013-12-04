Installation
============

This building and installation guide should work for anything Unix-like, but in particular it has been tested on, in terms of architecture for which the executable is built,
* GNU/Linux: i386, x86_64
* Mac OS X: x86_64

Aside from standard C libraries and a basic development environment (gcc, etc.), **gr1c** depends on [CUDD](http://vlsi.colorado.edu/~fabio/CUDD/), the CU Decision Diagram package by Fabio Somenzi and others.  Also, gr1c interactive mode optionally uses GNU Readline (disabled by default; see `USE_READLINE` definition in Makefile).  The parser generator [Bison](http://www.gnu.org/software/bison/) and the lexical analyzer [flex](http://flex.sourceforge.net/) are required to build gr1c.  Other Yacc and lex compatible tools may suffice, but this has not been tested.


Building
--------

For the latest development snapshot, either [clone the repo](https://github.com/slivingston/gr1c) (at https://github.com/slivingston/gr1c.git) or [download a tarball](https://github.com/slivingston/gr1c/tarball/master).  For the latter, untar the file (name may vary) and change into the source directory.

    $ tar -xzf slivingston-gr1c-658f32b.tar.gz
    $ cd slivingston-gr1c-658f32b

We will first build [CUDD](http://vlsi.colorado.edu/~fabio/CUDD/). Let's make a directory called `extern` for this purpose. At the time of writing, the latest version is 2.5.0. Below we use `wget` to download it from the command-line. You might also try directing your Web browser at <ftp://vlsi.colorado.edu/pub/>, or see CUDD documentation for instructions.

    $ mkdir extern
    $ cd extern
    $ wget ftp://vlsi.colorado.edu/pub/cudd-2.5.0.tar.gz
    $ tar -xzf cudd-2.5.0.tar.gz
    $ cd cudd-2.5.0

Now you should be in the root of the CUDD source tree. Open the `Makefile` in your favorite editor (e.g., [Emacs](http://www.gnu.org/software/emacs/)) and make sure the settings look reasonable for your setup.  The comments therein should be enough to guide you.  If in doubt, consult the `Makefile` of gr1c, in particular CUDD_XCFLAGS, which is known to succeed for x86_64 GNU/Linux and Mac OS (64-bit) and thus could be copied directly into the setting of the XCFLAGS variable in the `Makefile` of CUDD.  More generally, try

    XCFLAGS = -mtune=native -DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=N -DSIZEOF_LONG=M

where `SIZEOF_VOID_P` and `SIZEOF_LONG` refer to the number of bytes used for `void` pointers and `long int` on your computer.  One method for finding appropriate `N` and `M` is [described below](#determinewsize).  Finally,

    $ make

With success building CUDD, we may now build gr1c. Change back to the gr1c root source directory and open the `Makefile`. There are two items to check. First, be sure that CUDD_ROOT matches where you just built CUDD; for the version used above, that would be `extern/cudd-2.5.0`. Second, be sure that CUDD_XCFLAGS matches XCFLAGS that you used to build CUDD.  Finally, run `make`. If no errors were reported, you should be able to see the version with

    $ ./gr1c -V

Later parts of this document describe [testing gr1c](#testing) and [additional build options](#extras).

<h3 id="determinewsize">Determining word size on your computer</h3>

If you want to select `N` and `M` for `-DSIZEOF_VOID_P=N -DSIZEOF_LONG=M` as suggested above, then you must determine the number of bytes used for `void` pointers and `long int` on your computer. These will be provided as compile flags when building CUDD and gr1c. An easy way to discover these is to place

    #include <stdio.h>
    int main()
    {
        printf( "void *: %lu\nlong: %lu\n", sizeof(void *), sizeof(long) );
        return 0;
    }

in a plain text file, say called `testsize.c`, and then

    $ gcc -o testsize testsize.c
    $ ./testsize

The output will give the number of bytes for each type.  For example, in the case of 64-bit, you may see

    void *: 8
    long: 8


<h2 id="testing">Testing</h2>

Before deploying gr1c and especially if you are building it yourself, run the test suite from the source distribution. After following the build steps above, do

    $ make check

Optionally, set the shell variable `VERBOSE` to 1 to receive detailed progress notifications.  E.g., try `VERBOSE=1 make check`.  Most test code is placed under the `tests` directory. **N.B.**, scripted interaction will only work if you build without GNU Readline.  If a test fails, despite you following the installation instructions, then please open an [issue on GitHub](https://github.com/slivingston/gr1c/issues) or contact Scott Livingston at <slivingston@cds.caltech.edu>


Placing gr1c on your shell path
-------------------------------

To use the program `gr1c` outside of the directory you just built it in, you will need to add it to the search path of your shell. Assuming you have administrative privileges, one solution is

    $ sudo make install

which will copy `gr1c` and `rg` to `/usr/local/bin`, a commonly used location for "local" installations.  This is based on a default installation prefix of `/usr/local`.  Adjust it by invoking `make` with something like `prefix=/your/new/path`.  The programs `grpatch` and `grjit` are *not* included in as an effect of `make install`.  [Extra instructions](#extras) concerning these are below.


<h2 id="extras">Extras</h2>

Running `make`, as described above, will not cause the programs
`grpatch` and `grjit` to be built.  To achieve this,

    $ make exp

Alternatively, you can request a specific executable, e.g., `make
grpatch`.  To install `grpatch` in the same place as `gr1c`,

    $ make expinstall

Other experiment-related programs, such as `grjit`, will eventually be
added to the list installed by the expinstall command.
[Doxygen](http://www.doxygen.org) (version 1.8.0 or later) must be
installed to build the documentation...including the page you are now
reading.  Try

    $ make doc

and the result will be under `doc/build`.  You can clean the
sourcetree of all executables and other temporary files by running
`make clean`.
