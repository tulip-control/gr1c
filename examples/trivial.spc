ENV: x;
SYS: y;

ENVINIT: x | !z;
ENVTRANS:;
ENVGOAL:;

SYSINIT: !y & !(x=2 | foo);

SYSTRANS: ; # Notice the safety formula spans two lines.
#   [](y -> !(y'))
# & [](!y -> (y'))
# ;

SYSGOAL: ;#[]<>y;
