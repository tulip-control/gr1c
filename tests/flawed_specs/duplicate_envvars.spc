ENV: x x;
SYS: y;

ENVINIT: !x;
ENVTRANS: [](x <-> !x');
ENVGOAL: []<>x;

SYSINIT: y;
SYSGOAL:
  []<>(y&x)
& []<>(!y);
