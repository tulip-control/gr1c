File formats
============

The purpose of this page is to describe the file formats supported by gr1c,
besides the [format of input specifications](md_spc_format.html).  The
command-line argument "-t" is used to specify the format in which to output the
synthesized strategy.  A key to the choices, including functions that provide
it, is:

- `txt` : simple plaintext format (not standard); list_aut_dump()
- `dot` : [Graphviz dot](http://www.graphviz.org/); dot_aut_dump()
- `aut` : [gr1c automaton format](#gr1cautformat); aut_aut_dump()
- `json` : [strategy in JSON](#gr1cjson); json_aut_dump()
- `tulip` : [tulipcon XML](#tulipconxml); tulip_aut_dump()

Several of the patching routines need to be given a description of changes to
the game edge set.  This is achieved using the [edge changes file
format](#edgechangeset).  The relevant command-line argument is "-e FILE".


<h2 id="gr1cjson">strategy in JSON</h2>

The gross file formatting is [JSON](http://json.org/).  The details of what gr1c
provides are versioned.  The current version is 1.  The only difference with
version 0 is the addition of the node field "initial".  A key to entries is:

- `version` : format version number
- `gr1c` : version of gr1c that generated the output
- `date` : timestamp in UTC; e.g., invocation time of json_aut_dump()
- `extra` : an arbitrary, printable string
- `ENV`, `SYS` : ordered lists of environment and system variables,
  respectively, including domains.  Possible domains are the same as accepted in
  the [specification input](md_spc_format.html):
  * `"boolean"` : 1 (`True`) or 0 (`False`).  Note that JSON includes `true` and
    `false` values, but we do not use them here.
  * `[0,n]` : integers `0, 1, ..., n`, where `n` is a nonnegative integer.
- `nodes` : object of automaton nodes, each of which is uniquely named and has
  as value an object organized by the various members of anode_t
  (cf. automaton.h).  Released changes to the members of anode_t will cause this
  format version number to increment.

Note that the notion of "object" in JSON maps to `dict` (i.e., "dictionary
objects") in Python.  An example is the following.

    {"version": 1,
     "gr1c": "0.8.4",
     "date": "2014-09-19 18:06:49",
     "extra": "",

     "ENV": [{"x": "boolean"}],
     "SYS": [{"y": "boolean"}],

     "nodes": {
    "0x101090": {
        "state": [0, 0],
        "mode": 0,
        "rgrad": 1,
        "initial": false,
        "trans": ["0x101040"] },
    "0x101040": {
        "state": [1, 1],
        "mode": 1,
        "rgrad": 1,
        "initial": false,
        "trans": ["0x101090"] },
    "0x101010": {
        "state": [0, 1],
        "mode": 0,
        "rgrad": 1,
        "initial": true,
        "trans": ["0x101040"] }
    }}


<h2 id="tulipconxml">tulipcon XML</h2>

A file format from before version 1 of [TuLiP](http://tulip-control.org).  At
the time of writing, it is not yet available in the current release of TuLiP.


<h2 id="gr1cautformat">gr1c automaton</h2>

This format is occasionally referred to as "gr1c aut format".  The relevant
command-line argument is "-a FILE".  Note that version numbers of this format
are independent (for most practical purposes) from the version of gr1c itself.
For all versions, the version number is given as a single nonnegative integer on
the first non-comment, non-blank line.  In the case of a missing version number,
[0 is assumed](#gr1cautformatv0) (also known as the [legacy
format](#gr1cautformatv0)).  Unless explicitly stated to the contrary in the
format version description below, blank lines and lines beginning with ``#``
(known as "comments") are always ignored.

For this format, the API includes functions aut_aut_load() and aut_aut_dump()
for reading and writing, respectively.  Signatures are in automaton.h.

<h3 id="gr1cautformatv1">version 1</h3>

Each line is of the form

    i S I m r t0 t1 ...

where `I` is `1` if the node is initial and `0` otherwise, corresponding
respectively to `True` and `False` of the `initial` field of the anode_t
structure (defined in automaton.h).  There is a [brief introduction to
interpretations of initial conditions](md_spc_format.html#initconditions) in the
documentation of the specification input format. The other elements are as in
the [aut format v0](#gr1cautformatv0).

<h3 id="gr1cautformatv0">version 0 (legacy)</h3>

The order of the state vector matches that from the specification (as in most
other places in the source code).  Each line is of the form

    i S m r t0 t1 ...

where `i` is the node ID, `S` is a space-separated list of values taken by
variables at this node (i.e., its state label), `m` the goal mode, `r` is a
reach annotation value (-1 if not available), and all remaining integers are IDs
of nodes in the outgoing transition set of this node.

The list of IDs is assumed to be tight, meaning if there are N nodes, then the
file must contain indices 0 through N-1 (not necessarily in order).


<h2 id="edgechangeset">game edge set changes</h2>

Files of this form consist of two parts: first a list (one per line) of states
considered to be in the neighborhood; and second a sequence of **restrict**,
**relax**, or **blocksys** commands (as defined in the section
[Interaction](md_interaction.html)). Blank lines and lines beginning with ``#``
are ignored.

For example, if there are two variables, and we have declared the "neighborhood"
to consist of states 00, 01, and 11 (with states given as bitvectors), and we
wish to change the game graph by removing the controlled edge from 00 to 01,
then the file would be

    # This is a comment.
    0 0
    0 1
    1 1
    restrict 0 0 0 1

If patching was successful, then the new strategy is output to stdout (in the
format specified by the flag "-t" if given; else, the default). Otherwise, a
nonzero integer is returned by the program.
