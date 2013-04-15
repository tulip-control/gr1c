ENV:;
SYS: y [0,4];

SYSINIT: y = 0;
SYSTRANS:   [] (y=0) -> (y'=2 | y'=4)
          & [] (y < 4);
SYSGOAL: []<> (y=3);
