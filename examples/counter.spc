# Deterministic demonstration of variables with nonboolean domains of
# the form [0,n] where n is a positive integer.  Thus, the domain of y
# below is {0,1,2,3,4}.  The specification describes a counter that
# must reach 0 and 4 infinitely often and can change the number at
# each time step by at most 1.


ENV:;
SYS: y [0,4];

ENVINIT:;
ENVTRANS:;
ENVGOAL:;

SYSINIT: y=4;

SYSTRANS:
  []((y=4) -> (y'=3 | y'=4))
& []((y=3) -> (y'=2 | y'=3 | y'=4))
& []((y=2) -> (y'=1 | y'=2 | y'=3))
& []((y=1) -> (y'=0 | y'=1 | y'=2))
& []((y=0) -> (y'=0 | y'=1));

SYSGOAL: []<>y=0 & []<>y=4;
