# Expected output after synthesis and calling list_aut_dump() is in
# tests/expected_outputs/trivial_2var.spc.listdump.out
#
# SCL; 18 Mar 2013.
ENV: x;
SYS: y;

ENVINIT:x;
ENVTRANS:;
ENVGOAL:[]<>x;

SYSINIT:y;
SYSTRANS:;
SYSGOAL:[]<>(y&x)
& []<>(!y);
