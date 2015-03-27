# Expected output after synthesis and calling list_aut_dump() is in
# tests/expected_outputs/free_counter.spc.listdump.out

ENV:;
SYS: z y [0,4];

SYSINIT: y = 0;
SYSTRANS:   [] (y=0) -> (y'=2 | y'=4)
          & [] (y < 4);
SYSGOAL: []<> (y=3);
