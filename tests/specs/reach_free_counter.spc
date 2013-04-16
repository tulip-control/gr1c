ENV: x [0,2];
SYS: y [0,4];

ENVINIT: x=0;
ENVTRANS:  []((x=0) -> (x'=0 | x'=1))
         & []((x=1) -> (x'=0 | x'=1 | x'=2))
         & []((x=2) -> (x'=1 | x'=2))

         & []((y=2) -> (x'=0 | x'=2));

SYSINIT: y = 0;
SYSTRANS:  []((y=0) -> (y'=2 | y'=4))
         & [](y' < 4);
SYSGOAL: <> (y=3);
