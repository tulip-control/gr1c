Introduction        {#mainpage}
============

The citable URL is <http://scottman.net/2012/gr1c>.  The public Git repo can be
cloned from

    https://github.com/slivingston/gr1c.git

**gr1c** is a collection of tools for GR(1) synthesis and related activities.
Its core functionality is checking realizability of and synthesizing strategies
for GR(1) specifications, though it does much more.  Relevant papers can be
found in the [bibliography](md_papers.html).

Releases are posted at http://vehicles.caltech.edu/snapshots/.  See the
[installation page](md_installation.html) for instructions about installing gr1c
from source code.  Beware the code is still experimental. If you find a possible
bug or have comments, suggestions, or feature requests, then please open an
[issue on GitHub](https://github.com/slivingston/gr1c/issues) or contact Scott
Livingston at <slivingston@cds.caltech.edu>

In the documentation below, we assume that gr1c is on the shell path. To see a
summary of possible command-line arguments,

    $ gr1c -h


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

    $ gr1c -r  examples/trivial.spc

You should see a message indicating realizability, such as "Realizable." Now, to
synthesize a strategy for it, output the result into a
[DOT](http://www.graphviz.org/) file, and create a PNG called `temp.dot.png`
from that output, try

    $ gr1c -t dot examples/trivial.spc > temp.dot
    $ dot -Tpng -O temp.dot


Components
----------

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
</dd>

<dt>rg</dt>
<dd>a tool for solving "reachability games," which are similar to GR(1) formulae
except with at most one system goal and where that system goal need only be
reached once (not necessarily infinitely often).  The input specifications are
[slightly different](md_spc_format.html#reachgames).
</dd>

<dt>grpatch</dt>
<dd>(experimental.)  a command-line tool for using implementations of
incremental synthesis (or "patching") algorithms, mostly from recent research
publications.  E.g., patch_localfixpoint() (cf. patching.h) concerns the method
in [[LPJM13]](md_papers.html#LPJM13).
</dd>

<dt>autman</dt>
<dd>(auxiliary.)  a command-line tool for manipulating automata.  Currently it
only supports [gr1c automaton format](md_formats.html#gr1cautformat).
</dd>
</dl>


Further reading
---------------

- [format of input specifications](md_spc_format.html)
- [interactive sessions](md_interaction.html)
- [file formats](md_formats.html)
- [the developer's introduction](md_start_dev.html)


License
-------

This is free software released under the terms of [the BSD 3-Clause
License](http://opensource.org/licenses/BSD-3-Clause).  There is no
warranty; not even for merchantability or fitness for a particular
purpose.  See `LICENSE.txt` for copying conditions.
