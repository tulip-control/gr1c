# Example 1
ENV: x;
SYS: y;

ENVINIT: !x;
ENVGOAL: []<>x;

SYSINIT: y;
SYSGOAL:
  []<>(y&x)
& []<>(!y);
