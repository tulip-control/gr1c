# Expected output after synthesis and calling list_aut_dump() is in
# tests/expected_outputs/count_onestep.spc.listdump.out

ENV: ;
SYS: y [0,2];
ENVINIT: ;
ENVTRANS:;
ENVGOAL: ;

SYSINIT: ;
SYSTRANS: ;

# The subformula y != 0 is included as a regression test for erroneous
# != operator expansion (cf. https://github.com/tulip-control/gr1c/issues/17)
SYSGOAL: []<>(y = 1 & y != 0);
