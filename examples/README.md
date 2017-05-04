Examples
========

Example specifications at the root (i.e., the `examples` directory, in which
this README is located) are internally documented with comments.  Various
categories of examples are described in this document.

Patching
--------

Examples in the `patching` directory are intended to demonstrate routines for
incremental synthesis and relevant activities.  Informally, these concern
situations in which a given strategy must be modified.  Files with the same base
name go together, where `*.spc` (name ending in "spc") gives the nominal
specification and `*.edc` is the game edge change set.  Comments in these files
provide example descriptions.  Subsections below are organized by function of
the gr1c API, and when applicable, each begins with a description of how to
access the functionality from the command-line tools.

The common final step for visualization of results from the below code is

    dot -Tpng -O patched-image.dot

which invokes the dot program and creates a PNG image from the DOT output; the
image file name is patched-image.dot.png or similar.  Graphviz, which includes
dot, is available from http://www.graphviz.org

### patch_localfixpoint()

This function is invoked when the `-e` switch is passed to `gr1c patch`.

To run the example `dgridworld_2x10` (no environment, 2 by 10 gridworld using
integer-valued variables), try

    gr1c -t aut examples/patching/dgridworld_2x10.spc > nominal.aut
    gr1c patch -t dot -a nominal.aut -e examples/patching/dgridworld_2x10.edc examples/patching/dgridworld_2x10.spc > patched-image.dot

The first command synthesizes a nominal strategy and outputs it in the "gr1c
automaton" format and redirects it to a file named "nominal.aut".  The second
command executes the patching algorithm on nominal.aut given the edge change set
dgridworld_2x10.edc, and outputs the result in DOT format.  For a similar
deterministic gridworld example that uses exclusively variables with Boolean
domain, try `dgridworld_2x10_bool`.

### add_metric_sysgoal()

This function is invoked when the `-f` switch is passed to `gr1c patch`.

We begin with the same deterministic gridworld from the previous subsection:

    gr1c -t aut examples/patching/dgridworld_2x10.spc > nominal.aut

To add a new system goal of the grid cell (0,6), try

    gr1c patch -t dot -a nominal.aut -f 'Y_r=0 & Y_c=6' examples/patching/dgridworld_2x10.spc > patched-image.dot

where intuitively the variable `Y_r` represents the current row and `Y_c`
represents the current column.  An informal depiction of the setting is provided
in the comments near the top of `dgridworld_2x10.spc`.


Online or on-the-fly
--------------------

gr1c can be run interactively.  Among other things, this allows probing the
intermediate values of the GR(1) fixpoint computation and querying the winning
set.  Consult the "Interaction" page of the documentation.  It is distributed in
the sources at `doc/interaction.md`; a current build release is available at
https://tulip-control.github.io/gr1c/md_interaction.html

* jit:
  Examples for "just-in-time synthesis."  E.g., compare the horizon lengths of
  2trolls.spc and 1trolls.spc by

      ./gr1c -l -m -1,"x y" examples/jit/2trolls.spc
      ./gr1c -l -m -1,"x y" examples/jit/1troll.spc


API
---

Examples in the `api` directory demonstrate how to call functions and use data
structures of gr1c inside other programs. E.g.,

    make
    ./printwin ../trivial_partwin.spc


Of external origin
------------------

* PitPnuSaa2006:
  Examples from N. Piterman, A. Pnueli, and Y. Sa'ar (2006). Synthesis of
  Reactive(1) Designs. *In Proc. 7th International Conference on Verification,
  Model Checking and Abstract Interpretation*.  Run gen_arbspec.py (a Python
  script) to generate specification for case of N input lines (i.e., N requests
  and N possible grants).  gen_liftspec.py is used similarly but for case of N
  floors in "lift controller" specification.
