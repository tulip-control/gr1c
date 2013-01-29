Introduction        {#mainpage}
============

The citable URL is <http://scottman.net/2012/gr1c>.  The public Git
repo can be cloned from

    https://github.com/slivingston/gr1c.git

**gr1c** is a tool for GR(1) synthesis and related activities.
Its core functionality is checking realizability of and synthesizing
strategies for GR(1) specifications.  The fixpoint formula defining
the winning set is given (and its correctness proven) in

    Y. Kesten, N. Piterman, and A. Pnueli (2005). Bridging the gap between
    fair simulation and trace inclusion. Information and Computation,
    200: 35--61. DOI:10.1016/j.ic.2005.01.006

**rg** is a tool for solving "reachability games," which are similar
to GR(1) formulae except with at most one system goal and where that
system goal must be reached once (not necessarily infinitely often).
The accepted input specifications are slightly different; the single
system goal is preceded by "<>" (as the "eventually" operator).


See the [installation page](md_installation.html) for instructions
about installing gr1c from source code.  Beware the code is still
experimental. If you find a possible bug or have comments,
suggestions, or feature requests, then please open an [issue on
GitHub](https://github.com/slivingston/gr1c/issues) or contact Scott
Livingston at <slivingston@caltech.edu>


Basic usage
-----------

In the documentation below, we assume that gr1c is on the shell
path. To see a summary of possible command-line arguments,

    $ gr1c -h

Examples
--------

A very simple and easy to interpret specification is provided in
`examples/trivial.spc`.  Stripping away comments, the spec is

    ENV: x;
    SYS: y;

    ENVINIT: x;
    ENVTRANS:;
    ENVGOAL: []<>x;

    SYSINIT: y;
    SYSTRANS:;
    SYSGOAL: []<>y&x & []<>!y;

The only environment variable is ``x``, and the only system variable
is ``y``; both are Boolean-valued. We initialize both of these to
``True`` and, given the assumption that the environment will
infinitely often set ``x`` to ``True``, we must ensure that there are
infinitely many states visited such that both ``y`` and ``x`` are
``True``, and such that ``y`` is ``False``.  Obviously there must be
at least two states in any play winning for the system because the two
guaranteed formulas (``y&x`` and ``!y``) cannot simultaneously be
``True``.  Finally, there are no restrictions on transitions by the
environment or the system. Equivalently, the safety components of the
assumption and guarantee are ``True``.  This is indicated in the
specification by ``ENVTRANS`` and ``SYSTRANS`` being empty.  To check
whether the spec is realizable,

    $ gr1c -r  examples/trivial.spc

You should see a message indicating realizability, such as
"Realizable." Now, to synthesize a strategy for it, output the result
into a [DOT](http://www.graphviz.org/) file, and create a PNG called
``temp.dot.png`` from that output, try

    $ gr1c -t dot examples/trivial.spc > temp.dot
    $ dot -Tpng -O temp.dot