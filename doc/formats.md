File formats
============

The purpose of this page is to describe the file formats supported by gr1c.  The
command-line argument "-t" is used to specify the format in which to output the
synthesized strategy.  A key to the choices, including functions that provide
it, is:

- `txt` : simple plaintext format (not standard); list_aut_dump()
- `dot` : [Graphviz dot](http://www.graphviz.org/); dot_aut_dump()
- `aut` : [gr1c automaton format](#gr1cautformat); aut_aut_dump()
- `tulip` : [tulipcon XML](#tulipconxml); tulip_aut_dump()
- `tulip0` : for legacy support; *do not* use this in new applications.

Several of the patching routines need to be given a description of changes to
the game edge set.  This is achieved using the [edge changes file
format](#edgechangeset).  The relevant command-line argument is "-e FILE".


<h2 id="tulipconxml">tulipcon XML</h2>

See [nTLP documentation](http://slivingston.github.io/nTLP/doc/), specifically
the [page about data
formats](http://slivingston.github.io/nTLP/doc/data_formats.html#tulipcon-xml).


<h2 id="gr1cautformat">gr1c automaton</h2>

This format is occasionally referred to as "gr1c aut format".  The relevant
command-line argument is "-a FILE".

The order of the state vector matches that from the specification (as in most
other places in the source code).  Each line is of the form

    i S m r t0 t1 ...

where i is the node ID, S is a space-separated list of values taken by variables
at this node (i.e., its state label), m the goal mode, r is a reach annotation
value (-1 if not available), and all remaining integers are IDs of nodes in the
outgoing transition set of this node.

The list of IDs is assumed to be tight, meaning if there are N nodes, then the
file must contain indices 0 through N-1 (not necessarily in order).

For this format, the API includes functions aut_aut_load() and aut_aut_dump()
for reading and writing, respectively.  Signatures are in automaton.h.


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
