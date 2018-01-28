Developer's introduction
========================

This page describes how to navigate the code in a way useful to those who wish
to modify it, link against, etc.

The main program for users is `gr1c`. Besides access to the basic GR(1)
synthesis routine, `gr1c` handles commands like `gr1c rg` that cause separate
programs to be invoked, e.g., `gr1c-rg`. These programs must be on the system
path, so `./gr1c rg` may fail if the local directory is not on the path. To add
it, try

    export PATH=`pwd`:$PATH


Layout of the sources
---------------------

<dl>

<dt>`.`</dt>
<dd>Root directory of the source tree.  Contains LICENSE.txt, the directories
listed below, etc.</dd>

<dt>`src/`</dt>
<dd>Source code.  Noteworthy files for beginning to learn the internals of
gr1c include:
<ul>
  <li>`common.h`, which defines the version number.</li>
  <li>`main.c`, main entry point for the program `gr1c`.</li>
  <li>`rg_main.c`, main entry point for the program `gr1c-rg`.</li>
</ul></dd>

<dt>`exp/`</dt>
<dd>Consult `exp/README.txt`.  The main entry point for the program `gr1c-patch` is
in `exp/grpatch.c`.</dd>

<dt>`aux/`</dt>
<dd>Consult `aux/README.txt`.  The main entry point for the program `gr1c-autman` is
in `aux/autman.c`.</dd></dd>

<dt>`examples/`</dt>
<dd>Example specifications, edge change files, etc.  Some files are intended for
use with the program `gr1c`, others with `rg`, and still others with `grpatch`
and `grjit`, or some combination thereof.  Documentation for examples is
provided in `examples/README.md` or in comments at the top of the respective
file.</dd>

<dt>`tests/`</dt>
<dd>A test suite, which is usually invoked by ``make check`` from the root
directory.  Tests are built and run by `tests/Makefile`.  An environment
variable `VERBOSE` may be set to 1 to indicate that testing progress should be
reported as it happens.</dd>

<dt>`doc/`</dt>
<dd>Documentation source files and configuration.</dd>

<dt>`packaging/`</dt>
<dd>Consult `packaging/README.txt`.</dd>

</dl>


Design notes
------------

### Non-boolean domains

Internally, variables of non-Boolean domains are mapped into a set of Boolean
variables that use the original variable name as a prefix and then append the
corresponding bit index. E.g., `y` with domain of {0,1,2} would translate to 2
variables, `y0`, `y1` that are used to encode the bitvectors 00, 01, and 10. 11
(i.e., `y0` and `y1` both set to `True`) is unreachable.

A significant consequence is that the number of states generated internally
happens at boundaries of powers of two, e.g., so that an integer variable domain
of `[0,513]` incurs the same number of states as `[0,1023]`.


Quality assurance
-----------------

### Test coverage

Test coverage can be measured by
[gcov](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html), which is distributed with
[GCC](https://gcc.gnu.org). To use it, add `-fprofile-arcs -ftest-coverage` to
CFLAGS and `-lgcov` to LDFLAGS in the root Makefile and tests/Makefile. Examples
of how to do so are in the comments of these Makefiles. Then, `make check` to
run all tests. (Consider doing `make clean && make all` before to ensure that
all built files are up-to-date.) The coverage report for a source file can be
obtained from `gcov` by calling it with the file name. E.g.,

    gcov automaton.c

will produce automaton.c.gcov, which describes coverage of src/automaton.c.
To obtain coverage-annotated files for all C source code, try

    for k in `find src exp aux -name \*.c`; do gcov `basename ${k}`; done

### Static analysis

Usage on gr1c of several tools for static analysis, i.e., tools that examine the
source code without running the programs, are given below. The treatment is not
exhaustive. For Clang static analyzer <http://clang-analyzer.llvm.org>, try

    scan-build make all

For cppcheck <http://cppcheck.sourceforge.net>, try

    cppcheck --enable=all src 2> cppcheck-issues.txt

Other websites about cppcheck are <https://github.com/danmar/cppcheck/> and
<https://sourceforge.net/projects/cppcheck/>.

### Dynamic analysis

Several tools for dynamic analysis, i.e., tools that examine the behavior of
live processes, are listed below. The treatment is not exhaustive.

* Tools that use the Valgrind instrumentation framework, which are available at
  http://www.valgrind.org


Making releases
---------------

Scripts and other files useful for creating release distributions are under
`packaging/`. Currently these are Git-agnostic, so the version must be
explicitly declared by setting the environment variable `GR1C_VERSION`. If Git
is installed and a release commit is checked-out, then try

    export GR1C_VERSION=`git describe --tags --candidates=0 HEAD|cut -c 2-`

### The checklist

- Assign `RELEASE=1` at the top of the root Makefile.
- Update the CHANGELOG.
- Add copyright year where appropriate.
- Tag and sign.
- Create release note on GitHub.
- Build and post API manual of this release.
- Re-assign `RELEASE=0` at the top of the root Makefile to indicate that commits
  after the tag are under development.


Code style
----------

Version numbers are of the form MAJOR.MINOR.MICRO; when a tag is made for a
release, it must be named as the version number preceded by "v", e.g., "v0.6.1"
for version 0.6.1.  The next commit following a release tag should use a version
number greater than that of the tag.  For example, the first commit after that
tagged "v0.6.1" has version 0.6.2.

For C, I mostly prefer the [Linux](https://www.kernel.org/) kernel style (as
expounded in Documentation/CodingStyle in the Linux source tree), except that
indentation should use spaces (not tab charactors) and be in units of 4 spaces.
Documentation in source code should usually be
line-wrapped at 70 characters (default in Emacs), but more importantly, lines of
code or comments should rarely exceed 80 characters in length.  README or other
plain text files describing nearby stuff are written in
[Markdown](http://daringfireball.net/projects/markdown/) and line-wrapped at 80
characters.  Many comments in the code (e.g., if beginning with `/**`) and `.md`
files under the `doc` directory are processed by [Doxygen]
(http://www.doxygen.org).  With [Emacs](http://www.gnu.org/software/emacs/) it
may be possible to achieve the correct line width and tab conventions by adding
the following to your `.emacs` configuration file.

    (add-hook 'rst-mode-hook
      '(lambda ()
	 (setq fill-column 80)))

    (add-hook 'c-mode-common-hook
	      '(lambda ()
		(c-add-style "scott-c-style" scott-c-style t)))

    (add-hook 'c++-mode-common-hook
      '(lambda ()
	(c-add-style "scott-c-style" scott-c-style t)))

    (setq scott-c-style
      '((tab-width . 4)
       (indent-tabs-mode . nil)
       (c-basic-offset . 4)))
