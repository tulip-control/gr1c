File formats
============

The command-line argument "-t" is used to specify the format in which
to output the synthesized strategy.  The command-line argument "-e
FILE" requests patching a strategy (given in "gr1c automaton" format)
read from stdin, given the edge set change to the game graph specified
in FILE (see format below).


tulipcon XML
------------

...as defined elsewhere.


gr1c automaton
--------------

The order of the state vector matches that from the specification (as
in most other places in the source code).  Each line is of the form

    i S m r t0 t1 ...

where i is the node ID, S is a space-separated list of values taken by
variables at this node (i.e., its state label), m the goal mode, r the
reachability index (-1 if not available), and all remaining integers
are IDs of nodes in the outgoing transition set of this node.

The list of IDs is assumed to be tight, meaning if there are N nodes,
then the file must contain indices 0 through N-1 (not necessarily in
order).


game edge set changes
---------------------

Files of this form consist of two parts: a sequence of **restrict**,
**relax**, **blocksys**, or **blockenv** commands (as defined in the
section [Interaction](md_interaction.html)); and a space-separated
list of node IDs (all on one line) considered to be in the
neighborhood for patching.  All nodes in the neighborhood are assumed
to have the same goal mode.  Node IDs correspond to those in the given
(to-be-patched) strategy.

For example, if there are two variables, and we wish to change the
game graph by removing the controlled edge from 0 0 to 0 1, and we are
interested in the "neighborhood" of nodes 3 5 6 1, then the file would
be

    restrict 0 0 0 1
    3 5 6 1

If patching was successful, then the new strategy is output to stdout
(in the format specified by the flag "-t" if given; else, the
default). Otherwise, a nonzero integer is returned by the program.
