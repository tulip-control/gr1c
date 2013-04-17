Developer's introduction
========================

This page describes how to (begin to) navigate the code in a way
useful to those who wish to modify it, link against, etc.


Layout of the sources
---------------------

<dl>

<dt>`.`</dt>
<dd>Root directory of the source tree.  Contains LICENSE.txt, the directories
listed below, etc.</dd>

<dt>`src/`</dt>
<dd>Source code.  The main entry points for the programs `gr1c` and
`rg` are in `src/main.c` and `src/rg_main.c`, respectively.</dd>

<dt>`exp/`</dt>
<dd>Consult `exp/README.txt`.  The main entry point for the program
`grpatch` is in `exp/grpatch.c`.</dd>

<dt>`examples/`</dt>
<dd>Example specifications, edge change files, etc.  Some files are
intended for use with the program `gr1c`, others with `rg`, and still
others with `grpatch` and `grjit`, or some combination thereof.
Documentation for examples is provided in `examples/README.txt` or in
comments at the top of the respective file.</dd>

<dt>`tests/`</dt>
<dd>A test suite, which is usually invoked by ``make check`` from the
root directory.  Tests are built and run by `tests/Makefile`.  An
environment variable `VERBOSE` may be set to 1 to indicate that
testing progress should be reported as it happens.</dd>

<dt>`doc/`</dt>
<dd>Documentation source files and configuration.</dd>

</dl>


Unsorted notes
--------------

Internally variables of non-Boolean domains are mapped into a set of
Boolean variables that use the original variable name as a prefix and
then append the corresponding bit index. E.g., `y` with domain of
{0,1,2} would translate to 2 variables, `y0`, `y1` that are used to
encode the bitvectors 00, 01, and 10. 11 (i.e., `y0` and `y1` both set
to `True`) is unreachable.


Code style
----------

My strongest taste is for the Linux kernel style, as expounded in
[Documentation/CodingStyle](http://lxr.linux.no/#linux+v3.8.8/Documentation/CodingStyle).
However, I prefer a tab width of 4 spaces.  Documentation in source
code should usually be line-wrapped at 70 characters (default in
Emacs), but more importantly, lines of code or comments should rarely
exceed 80 characters in length.  README or other plain text files
describing nearby stuff are written in
[reStructuredText](http://docutils.sourceforge.net/rst.html) and
line-wrapped at 80 characters.  Many comments in the code (e.g., if
beginning with `/**`) and `.md` files under the `doc` directory are
processed by [Doxygen](http://www.doxygen.org).  So in particular,
`.md` files under `doc` are written in
[Markdown](http://daringfireball.net/projects/markdown/).  If you use
[Emacs](http://www.gnu.org/software/emacs/), then you may be able to
achieve the correct line width and tab conventions by adding the
following to your `.emacs` configuration file.

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
       (indent-tabs-mode . t)
       (c-basic-offset . 4)))
