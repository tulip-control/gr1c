# 8-connected gridworld of size 3x8.  Dynamic obstacle in column 2.


ENV: obs_x obs_y;
SYS: x y;

ENVINIT: obs_x=2 & obs_y=1;
ENVTRANS: [](obs_x' = 2);
ENVGOAL: []<>(obs_x=2 & obs_y=1);

SYSINIT: x=0 & y=0;

SYSTRANS:
  []((x=7) -> (x'=6 | x'=7))
& []((x=6) -> (x'=5 | x'=6 | x'=7))
& []((x=5) -> (x'=4 | x'=5 | x'=6))
& []((x=4) -> (x'=3 | x'=4 | x'=5))
& []((x=3) -> (x'=2 | x'=3 | x'=4))
& []((x=2) -> (x'=1 | x'=2 | x'=3))
& []((x=1) -> (x'=0 | x'=1 | x'=2))
& []((x=0) -> (x'=0 | x'=1))

& []((y=2) -> (y'=1 | y'=2))
& []((y=1) -> (y'=0 | y'=1 | y'=2))
& []((y=0) -> (y'=0 | y'=1))

& []!((obs_y'=0 & y'=0) | (obs_y'=1 & y'=1) | (obs_y'=2 & y'=2))
;

SYSGOAL: []<>(x=0) & []<>(x=7);
