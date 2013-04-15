ENV: x;
SYS: y;

ENVINIT:x;
ENVINIT:;
ENVTRANS:[](y->x');
ENVGOAL:[]<>x;

SYSINIT:y;
SYSTRANS:;
SYSGOAL:[]<>(y&x)
& []<>(!y);
