Verification
============

While the primary motivation of the **gr1c** suite is synthesis, it can
facilitate verification by expressing strategy automata as
[Spin](http://spinroot.com) [Promela](http://spinroot.com/spin/Man/promela.html)
models using spin_aut_dump(), which is available from the command-line through
the `-P` switch of `autman` and `gr1c`.


Tutorial
--------

In this tutorial, we will synthesize a strategy and then verify it using
[Spin](http://spinroot.com).  We assume that `spin` is installed and that there
is a C compiler available as `cc`.  If you are running this from a local build
of **gr1c**, then you may need to first add the directory containing `gr1c` and
other programs to `PATH`, as described in [the developer's introduction](md_start_dev.html).

Consider the small specification

    ENV: x;
    SYS: y;

    ENVINIT: !x;
    ENVTRANS: [](x <-> !x');
    ENVGOAL: []<>x;

    SYSINIT: y;
    SYSTRANS:;
    SYSGOAL: []<>(y&x) & []<>(!y);

which is `examples/trivial.spc` from the source distribution of gr1c after
removing comments.  We already know that it is realizable, so synthesize a
strategy using the [gr1c automaton format](md_formats.html#gr1cautformat)

    gr1c -t aut examples/trivial.spc > trivstrategy.aut

where shell redirection is used to save the output to a file named
"trivstrategy.aut".  Note that the `-o` switch could have been used instead.
The next step is to create a [Spin](http://spinroot.com)
[Promela](http://spinroot.com/spin/Man/promela.html) file using

    gr1c autman -i examples/trivial.spc trivstrategy.aut -P > trivaut.pml

While we could have created this file in the first step using the `-P` switch of
`gr1c`, first storing the aut file allows later generation of a Promela file (as
above) or conversion to json or dot (cf. the section on [file
formats](md_formats.html)) without repeating synthesis.  The aut file is
appropriate for the purposes of archiving.

Near the top of `trivaut.pml` is a comment block beginning with the text "LTL
formula" that contains a formula in Spin LTL syntax against which the model
should be checked.  Observe that the formula contains macros like `envinit` that
are defined later in the Promela file.  The formula can be regarded as an
instrumented version of the original specification that includes special flags
like `pmlfault` to indicate, e.g., moves by the environment that the strategy
does not address.  You should be able to reach the comment block by printing the
first few lines of the file, e.g.,

    head trivaut.pml

Because we want to verify that the strategy realizes the specification, we
generate a [never claim](http://spinroot.com/spin/Man/never.html) from the
negation of the formula that we just located in `trivaut.pml`.  Continuing our
example, we use the routine available by the `-f` switch of Spin.  Note that
there are other tools capable of generating a never-claim from an LTL formula,
e.g., [LTL2BA](http://www.lsv.ens-cachan.fr/~gastin/ltl2ba/). Placing the
formula found into <code>spin -f '!(...)'</code>, we have

    spin -f '!((X envinit && [] (!checketrans || envtrans) && []<> envgoal0000) -> (X sysinit && [] (!checkstrans || systrans) && []<> sysgoal0000 && []<> sysgoal0001 && [] !pmlfault))' >> trivaut.pml

where the `>>` operator causes the resulting never claim to be appended to the
end of the Promela file from `autman`.  (Like `>` used earlier, `>>` is another
example of shell redirection.)  Finally, the verifier is built and run from

    spin -a trivaut.pml && cc -o pan pan.c && ./pan -a

If an acceptance cycle is found, then the strategy is not winning, i.e., it does
not realize the specification.  You can find a counterexample using

    spin -p -g -l -t trivaut.pml

Note that `trivaut.pml.trail` would have been created by `./pan` after
discovering an acceptance cycle.
