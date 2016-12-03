ENV: x;
SYS: y x;

ENVINIT: !x;
ENVTRANS: [](x <-> !x');
ENVGOAL: []<>x;

SYSINIT: y;
SYSGOAL:
  []<>(y&x)
& []<>(!y);
