# Change which SYSGOAL is commented-out below in order to get a
# strategy winning with finite play vs. only with infinite plays.
# Note that SYSGOAL may be entirely omitted, instead of writing
# "SYSGOAL:;"
#
# SCL; 26 Aug 2012.

ENV: x;
SYS: y;

ENVINIT: x;
ENVTRANS:;
ENVGOAL: []<>x&y;

SYSINIT: !y;
SYSTRANS:;
SYSGOAL:<>y;  # Block environment or reach a y-state.
#SYSGOAL:;  # No set to reach means environment must be blocked.
