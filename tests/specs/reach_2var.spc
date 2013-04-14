# Compare with tests/specs/reach_2var_mustblock.spc

ENV: x;
SYS: y;

ENVINIT:x;
ENVTRANS:[](y->!x');
ENVGOAL: []<> x;

SYSINIT:!y;
SYSTRANS:;
SYSGOAL:<>y;
