.. Emacs, this is -*-rst-*-

Example specifications in the root (i.e., the ``examples`` directory)
are internally documented with comments.  Other examples appear in
subdirectories:

PitPnuSaa2006
  Examples from N. Piterman, A. Pnueli, and Y. Sa'ar (2006). Synthesis
  of Reactive(1) Designs. *In Proc. 7th International Conference on
  Verification, Model Checking and Abstract Interpretation*.  Run
  gen_arbspec.py (a Python script) to generate specification for case
  of N input lines (i.e., N requests and N possible grants).
  gen_liftspec.py is used similarly but for case of N floors in "lift
  controller" specification.

patching
  Examples for patching using the method implemented through the function
  patch_localfixpoint() (see patching.h).  Files with the same base name go
  together, where *.spc (name ending in "spc") gives the nominal specification
  and *.edc is the game edge change set.  See comments in these files for
  example descriptions.

  To run the example ``dgridworld_2x10`` (no environment, 2 by 10 gridworld
  using variables with Boolean domain), try ::

    $ gr1c -t aut examples/patching/dgridworld_2x10.spc > nominal.aut
    $ grpatch -t dot -a nominal.aut -e examples/patching/dgridworld_2x10.edc examples/patching/dgridworld_2x10.spc > patched-image.dot
    $ dot -Tpng -O patched-image.dot

  The first line synthesizes a nominal strategy and outputs it in the "gr1c
  automaton" format, and redirects it to a file named "nominal.aut".  The second
  line executes the patching algorithm on nominal.aut given the edge change set
  dgridworld_2x10.edc, and outputs the result in DOT format.  The final line
  invokes the dot program, which must be installed, to create a PNG image from
  the DOT output; the image file name is patched-image.dot.png or similar.  For
  a similar deterministic gridworld example that uses exclusively variables with
  Boolean domain, try ``dgridworld_2x10_bool``.

jit
  Examples for "just-in-time synthesis."  E.g., compare the horizon lengths of
  2trolls.spc and 1trolls.spc by ::

    $ ./gr1c -l -m -1,"x y" examples/jit/2trolls.spc
    $ ./gr1c -l -m -1,"x y" examples/jit/1troll.spc
