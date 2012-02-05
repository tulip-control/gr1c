ENV: x foo;
SYS: y;

ENVINIT: x;
ENVTRANS:;
ENVGOAL:;

SYSINIT: y | !(x | foo); #x->y

SYSTRANS: ; # Notice the safety formula spans two lines.
#   [](y -> !(y'))
# & [](!y -> (y'))
# ;

SYSGOAL: ;#[]<>y;
