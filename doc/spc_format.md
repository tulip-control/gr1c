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

    $ gr1c examples/trivial.spc
    $ cat examples/trivial.spc | gr1c

Formatting possibilities for specification files are diverse.  The main cases
are covered here.  Each file consists of several sections each of which may span
multiple lines.  Each section begins with "ENV" or "SYS" and ends with ";".
Comments begin with "#", span the remainder of the line (after "#"), and may be
inserted anywhere.  For entirely deterministic problems (i.e., no *environment
variables*), all sections beginning with "ENV" may be omitted.

An empty transition rule section, e.g., `SYSTRANS:;`, is equivalent to
`SYSTRANS:[]True;`, thus imposing no restrictions.  Similarly, an empty initial
values section, e.g., `SYSINIT:;` is equivalent to `SYSINIT:True;`.

System (resp., environment) variables are declared by a section beginning with
`SYS:` (resp., `ENV:`) and consisting of a space-separated list of variable
names and domains if not Boolean.  The only supported nonboolean domains are of
the form `[0,n]` where `n` is a nonnegative integer.  E.g.,

    SYS: a b [0,5] c;

declares three variables: `a`, `b`, and `c`.  `a` and `c` have Boolean domains
(i.e., can only be `True` or `False`), while `b` can take any integer value in
the set `{0,1,2,3,4,5}`.


Incomplete summary of the grammar
---------------------------------

C-like, infix syntax. Comments begin with `#` and continue until the end of
line. The grammar below is not complete (nor normative), but should be enough
for you to compose specifications.  Cf. the `examples` directory.

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
