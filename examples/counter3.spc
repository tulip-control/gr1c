# Example 2
SYS: y [0,2];

SYSINIT: y=0;

SYSTRANS:
  []((y=2) -> (y'=1 | y'=2))
& []((y=1) -> (y'=0 | y'=1 | y'=2))
& []((y=0) -> (y'=0 | y'=1));

SYSGOAL: []<>y=0 & []<>y=2;
