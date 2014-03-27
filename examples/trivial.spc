ENV: x;
SYS: y;

ENVINIT: !x;

# The environment transition rule (a "safety" formula) forces x to
# alternate at each time step.  Try removing the restriction by placing
# ";#" immediately after the semicolon (thus "commenting it out").
ENVTRANS: [](x <-> !x');
ENVGOAL: []<>x;

# Blank lines are optional and can be placed between sections or parts
# of formulae.

SYSINIT: y;
SYSTRANS:;  # Empty means "True"; hence any transition is possible.
SYSGOAL:  # Notice the liveness formulae span two lines.
  []<>(y&x)
& []<>(!y);
