Introduction        {#mainpage}
============

The citable URL is <http://scottman.net/2012/gr1c>.  The public Git repo can be
cloned from

    https://github.com/tulip-control/gr1c.git

**gr1c** is a collection of tools for GR(1) synthesis and related activities.
Its core functionality is checking realizability of and synthesizing strategies
for GR(1) specifications, though it does much more.  Relevant papers can be
found in the [bibliography](md_papers.html).

Releases are posted at https://dl.bintray.com/slivingston/generic/.  Pre-built
binaries for several platforms are available, though the preferred distribution
form is as a source bundle.  Read the [installation page](md_installation.html)
for instructions about installing gr1c from source code.  Beware the code is
still experimental. If you find a possible bug or have comments,
recommendations, or feature requests, then please open an [issue on
GitHub](https://github.com/tulip-control/gr1c/issues) or contact
Scott C. Livingston at <slivingston@cds.caltech.edu>

In the documentation below, we assume that gr1c is on the shell path. To get a
summary of possible command-line arguments,

    gr1c -h


Examples
--------

Many specification files are provided under the `examples` directory.  Begin by
reading `examples/README.txt`. Then read the [file formats](md_formats.html) and
[specification input](md_spc_format.html) pages.  As a simple first step,
consider the following specification, which is `examples/trivial.spc` after
removing comments.

    ENV: x;
    SYS: y;

    ENVINIT: !x;
    ENVTRANS: [](x <-> !x');
    ENVGOAL: []<>x;

    SYSINIT: y;
    SYSTRANS:;
    SYSGOAL: []<>(y&x) & []<>(!y);

which is represented by the LTL formula
\f[

\left( \neg x \wedge \Box \left( x \leftrightarrow \neg x' \right) \wedge \Box \rotatebox[origin=c]{45}{$\Box$} x \right) \implies \left( y \wedge \Box \rotatebox[origin=c]{45}{$\Box$} (y \wedge x) \wedge \Box \rotatebox[origin=c]{45}{$\Box$} \neg y \right).

\f]
The only environment variable is `x`, and the only system variable is `y`; both
are Boolean-valued.  We require that initially `x` is `False` and `y` is `True`.
Given the assumption that the environment will infinitely often set `x` to
`True`, we must ensure that there are infinitely many states visited such that
both `y` and `x` are `True`, and such that `y` is `False`.  Obviously there must
be at least two states in any play winning for the system because the two
guaranteed formulas (`y&x` and `!y`) cannot simultaneously be `True`.  It is
assumed that the environment will flip the variable `x` at each step, as
indicated by the transition rule `ENVTRANS`.  Finally, there is no restriction
on transitions by the system, as indicated by the empty
`SYSTRANS:;`. Equivalently, the safety component of the guarantee is `True`.  To
check whether the specification is realizable,

    gr1c -r  examples/trivial.spc

A message indicating realizability, such as "Realizable," will be printed. Now, to
synthesize a strategy for it, output the result into a
[DOT](http://www.graphviz.org/) file, and create a PNG called `temp.dot.png`
from that output, try

    gr1c -t dot examples/trivial.spc > temp.dot
    dot -Tpng -O temp.dot


Organization
------------

Besides using the API directly and linking to relevant source files (cf. [the
developer's introduction](md_start_dev.html)), several executable programs are
built, some of which are considered experimental due to inclusion of methods
involving active research.  A summary of these programs follows:

<dl>
<dt>gr1c</dt>
<dd>the basic program for synthesis of strategies for GR(1) formulae as defined
in [[KPP05]](md_papers.html#KPP05) (GR(1) is referred to as "generalized
Streett[1]" in that paper).  An original motivating algorithm was implemented by
Yaniv Sa'ar [[BJPPS12]](md_papers.html#BJPPS12), though the implementation in
gr1c differs somewhat.

When the invocation of `gr1c` is of the form `gr1c COMMAND [...]`, it acts as a
base program for using other commands, which are described below.
</dd>

<dt>gr1c&nbsp;rg</dt>
<dd>a tool for solving "reachability games," which are similar to GR(1) formulae
except with at most one system goal and where that system goal need only be
reached once (not necessarily infinitely often).  The input specifications are
[slightly different](md_spc_format.html#reachgames).
</dd>

<dt>gr1c&nbsp;patch</dt>
<dd>(experimental.)  a command-line tool for using implementations of
incremental synthesis (or "patching") algorithms, mostly from recent research
publications.  E.g., patch_localfixpoint() (cf. patching.h) concerns the method
in [[LPJM13]](md_papers.html#LPJM13).
</dd>

<dt>gr1c&nbsp;autman</dt>
<dd>(auxiliary.)  a command-line tool for manipulating automata.  Currently it
only supports [gr1c automaton format](md_formats.html#gr1cautformat).
</dd>
</dl>


Exit codes
----------

The exit code is also known as "return code" or "status code." It is the integer
returned at the end of execution.  While the particular meaning of an exit code
depends on the activity being performed, in all cases an exit code of zero is
interpreted as success. We reserve negative exit codes for indication of
internal errors and positive exit codes for activity-specific non-success. An
example of an internal error is a failed call to the standard library malloc()
because no more memory is available. (Notice that an "internal error" may be due
to an external event.) Currently all internal errors are indicated by -1, but
other negative values may be used in the future.  Below is a list of invocation
summaries and interpretations of exit codes.

Common exit codes for all activities:
- 1 : invalid command-line usage
- 2 : syntax error

`gr1c -s` (check syntax)
- 0 : correct syntax

`gr1c -r` (check realizability)
- 0 : realizable
- 3 : unrealizable

`gr1c` (perform basic GR(1) synthesis)
- 0 : success
- 3 : unrealizable

`gr1c -i` (interactive)
- 3 : unrealizable

`rg -s` (check syntax)
- 0 : correct syntax

`autman`
- 2 : syntax error in the reference specification (not necessarily given)
- 3 : syntax error in the automaton description


Further reading
---------------

- [format of input specifications](md_spc_format.html)
- [interactive sessions](md_interaction.html)
- [file formats](md_formats.html)
- [verifying output](md_verification.html)
- [the developer's introduction](md_start_dev.html)


License
-------

This is free software released under the terms of [the BSD 3-Clause
License](http://opensource.org/licenses/BSD-3-Clause).  There is no
warranty; not even for merchantability or fitness for a particular
purpose.  Consult `LICENSE.txt` for copying conditions.
