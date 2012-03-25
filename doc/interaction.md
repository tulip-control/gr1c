Interaction
===========

If started with the command-line flag "-i", **gr1c** will compute the
sublevel sets from the fixpoint computation of the winning set and
then accept commands until "quit" or end-of-file reached.  States
should be given as space separated values.


Example session
===============




Recognized commands
===================

* **winning STATE**

  is STATE in winning set?

* **envnext STATE**

  list of possible next valuations of environment variables, given
  current STATE; each is on a separate line, with termination
  indicated by a final line of "---".

* **sysnext STATE1 STATE2ENV GOALMODE**

  list of possible next valuations of system variables that are
  consistent with progression of a winning strategy that currently has
  goal index GOALMODE, given current STATE1 and environment move
  STATE2ENV from that state; each is on a separate line, with
  termination indicated by a final line of "---".

* **sysnexta STATE1 STATE2ENV**

  list of possible next valuations of system variables regardless of
  whether the resulting next state would be consistent with any
  winning strategy, given current STATE1 and environment move
  STATE2ENV from that state.

* **restrict STATE1 STATE2**

  remove edge from STATE1 to STATE2, if present. The behavior of this
  command depends on the length of the second argument. If STATE2 has
  length equal to the number of environment variables, then it is
  treated as an environment move from STATE1, hence this command
  removes the corresponding uncontrolled edge. If STATE2 is a complete
  state vector, i.e., it has length equal to the total number of
  environment and system variables, then it is treated as a system
  move, hence this command removes the corresponding controlled edge.

* **relax STATE1 STATE2**

  add an edge from STATE1 to STATE2. See description of "restrict"
  command regarding how the length of STATE2 affects interpretation of
  this command.

* **clear**

  clear all restrictions and relaxations thus far.

* **unreach STATE**

  make STATE unreachable, i.e., all ingoing edges are removed.  See
  description of "restrict" command regarding how the length of STATE
  affects interpretation of this command.

* **getindex STATE GOALMODE**

  get reachability index of STATE for goal index GOALMODE.

* **setindex STATE GOALMODE**

  set reachability index of STATE for goal index GOALMODE; internally
  this changes sublevel membership of STATE.

* **refresh winning**

  compute winning set (presumably after transition (safety) rules have
  changed from restrict and relax commands); this does *not* change
  the sublevel sets.

* **refresh levels**

  compute (sub)level sets; this command also causes the winning set to
  be computed (again).

* **addvar env (sys) VARIABLE**

  add VARIABLE to list of environment (resp. system) variables if
  "env" (resp. "sys").

* **envvar**

  print list of environment variables in order.

* **sysvar**

  print list of system variables in order.

* **var**

  print all variable names with indices; your state values must match
  this order.

* **numgoals**

  print number of system goals.

* **printgoal GOALMODE**

  print formula for system goal index GOALMODE.

* **enable (disable) autoreorder**

  enable (resp. disable) automatic BDD reordering.

* **quit**

  terminate gr1c.
