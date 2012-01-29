ENV: x;
SYS: y;

ENVINIT: x;
ENVTRANS:;
ENVGOAL:;

SYSINIT: !y;

SYSTRANS:  # Notice the safety formula spans two lines.
  [](y -> !(y'))
& [](!y -> (y'))
;

SYSGOAL: []<>y;
