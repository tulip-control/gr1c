# Compare with tests/specs/reach_2var.spc

ENV: x;
SYS: y;

ENVINIT:x;
ENVTRANS:[](y->!x');
ENVGOAL: []<> x;

SYSINIT:!y;
SYSTRANS:;
