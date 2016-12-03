Specification input format
==========================

In its most basic usage, the program `gr1c` will synthesize a winning strategy
for a given specification or declare that it is impossible (i.e., the GR(1)
specification is *unrealizable*).  Specifications (informally abbreviated
"specs") are provided as input files at invocation.  If no file is provided,
then `gr1c` will attempt to read the specification from `stdin`.  For instance,
the following two commands will lead to the same output.  Note that you may need
to use `./gr1c` instead if you have built but not installed the `gr1c`
executable.

    gr1c examples/trivial.spc
    cat examples/trivial.spc | gr1c

Formatting possibilities for specification files are diverse.  The main cases
are covered here.  Each file consists of several sections each of which may span
multiple lines.  Each section begins with "ENV" or "SYS" and ends with ";".
Comments begin with "#", span the remainder of the line (after "#"), and may be
inserted anywhere.  Any omitted sections are considered empty.  E.g., omission
of the `ENVINIT` section is equivalent to including `ENVINIT:;`.

System (resp., environment) variables are declared by a section beginning with
`SYS:` (resp., `ENV:`) and consisting of a space-separated list of variable
names and domains if not Boolean.  The only supported nonboolean domains are of
the form `[0,n]` where `n` is a nonnegative integer.  E.g.,

    SYS: a b [0,5] c;

declares three variables: `a`, `b`, and `c`.  `a` and `c` have Boolean domains
(i.e., can only be `True` or `False`), while `b` can take any integer value in
the set `{0,1,2,3,4,5}`.  There must be at least one variable; i.e., at least
one of `ENV` or `SYS` must be nonempty.

For both the environment and system sections, an empty transition rule section,
e.g., `SYSTRANS:;`, is equivalent to `SYSTRANS:[]True;`, thus imposing no
restrictions.  Depending on the desired [interpretation of initial
conditions](#initconditions), an empty initial values section, e.g., `SYSINIT:;`
is equivalent to `SYSINIT:True;`.  An empty system goal section, i.e.,
`SYSGOAL:;`, is equivalent to `[]<>True`.


<h2 id="initconditions">Interpretations of initial conditions</h2>

Several definitions of initial conditions have been used in the literature for
GR(1) games and other reactive synthesis problems.  **gr1c** provides several
variations that are normatively defined in the documentation for
check_realizable() (cf. solve.h).  Here we briefly introduce each.  While these
are not part of the input specification file format, the manner in which
`ENVINIT` and `SYSINIT` are used to select initial states relies on the declared
interpretation of initial conditions.  The API convention is for functions that
care to take an argument called `init_flags` in which one of these is selected.
Most of the command-line tools provide the `-n` switch for declaring an
interpretation, e.g., `gr1c` can be invoked with `-n ONE_SIDE_INIT` to use the
option described by the same name below.

Unless stated otherwise, `ENVINIT:;` is equivalent to `ENVINIT:True;` and
similarly for `SYSINIT`.

* `ALL_ENV_EXIST_SYS_INIT` (default) : For each initial valuation of environment
  variables that satisfies the `ENVINIT:...;` section, the strategy must provide
  some valuation of system variables that satisfies `SYSINIT:...;`.  Only SYS
  vars can appear in SYSINIT, and only ENV vars can appear in ENVINIT.

* `ALL_INIT` : Any state that satisfies the conjunction of the `ENVINIT:...;`
  and `SYSINIT:...;` sections can occur initially.  Any variable can appear in
  each, but for the sake of syntactic compatibility with ALL_ENV_EXIST_SYS_INIT,
  the convention is to only use `ENV` variables in `ENVINIT` and `SYS` variables
  in `SYSINIT`.

* `ONE_SIDE_INIT` : At most one of `ENVINIT` and `SYSINIT` is nonempty.  Both
  being empty is equivalent to `ENVINIT: True;`.  Let `f` be a (nonempty)
  boolean formula in terms of environment (ENV) and system (SYS) variables.  If
  `ENVINIT: f;`, then the strategy must regard *any* state that satisfies `f` as
  possible initially.  If `SYSINIT: f;`, then the strategy need only choose one
  initial state satisfying `f`.


Incomplete summary of the grammar
---------------------------------

C-like, infix syntax. Comments begin with `#` and continue until the end of
line. The grammar below is not complete (nor normative), but should be enough
for you to compose specifications.  Cf. the `examples` directory.  NUMBER must
be a nonnegative integer.

    /* Define which variables are controlled and uncontrolled. */
    envvarlist ::= 'ENV:' VARIABLE | envvarlist VARIABLE
                   | envvarlist VARIABLE [0,NUMBER]
    sysvarlist ::= 'SYS:' VARIABLE | sysvarlist VARIABLE
                   | sysvarlist VARIABLE [0,NUMBER]

    propformula ::= "False" | "True" | VARIABLE
                    | '!' propformula
                    | propformula '&' propformula
                    | propformula '|' propformula
                    | propformula "->" propformula
                    | propformula "<->" propformula
                    | VARIABLE '=' NUMBER
                    | VARIABLE '!=' NUMBER
                    | VARIABLE '<' NUMBER
                    | VARIABLE '<=' NUMBER
                    | VARIABLE '>' NUMBER
                    | VARIABLE '>=' NUMBER
                    | '(' propformula ')'

    /* The only difference between propformula and tpropformula is
       variables can be primed (next operator) in the latter. */
    tpropformula ::= "False" | "True" | VARIABLE | VARIABLE '\'' | ...

    transformula ::= "[]" tpropformula | transformula '&' transformula
    goalformula ::= "[]<>" propformula | goalformua '&' goalformula


<h2 id="reachgames">Specifying reachability games</h2>

gr1c also treats reachability games, which can be solved from the command-line
using the program `rg`.  We take a *reachability game* to be formulated in a
manner similar to that of GR(1) synthesis except that there is only one system
goal and it need only be reached once.  Accordingly, the above description of
the specification input format holds also for `rg` with the following
modifications.

* The specification can have at most one single system goal, and if present, it
  is preceded by `<>` (as the "eventually" operator).

* If there is no system goal, i.e., the `SYSGOAL` section is omitted, then the
  environment must be blocked.

Concerning the form of automata providing strategies:

* Since there is only one set of states to reach, the notion of `mode` as in the
  anode_t structure (cf. automaton.h) is not pertinent.  In the case of
  successful synthesis, all nodes in the output of `rg` will have `mode` `-1`.

* There may be nodes that have no outgoing edges, even when the corresponding
  state reached is not a dead-end for the environment.  (Hint: a system goal
  state must have been reached!)
