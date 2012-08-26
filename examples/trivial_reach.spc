# Change which SYSGOAL is commented-out below in order to see a
# strategy winning with finite play vs. only with infinite plays.
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
