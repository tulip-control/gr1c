ENV: x;
SYS: y;

ENVINIT:x;
ENVTRANS:[](y->!x');
ENVGOAL: []<> x;

SYSINIT:!y;
SYSTRANS:;
SYSGOAL:<>y & <> !x;
