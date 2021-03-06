ENV: r1 r2 r3 r4;
SYS: g1 g2 g3 g4;

ENVINIT: !r1 & !r2 & !r3 & !r4;
ENVTRANS:
  [](((r1 & !g1) | (!r1 & g1)) -> ((r1' & r1) | (!r1' & !r1)))
& [](((r2 & !g2) | (!r2 & g2)) -> ((r2' & r2) | (!r2' & !r2)))
& [](((r3 & !g3) | (!r3 & g3)) -> ((r3' & r3) | (!r3' & !r3)))
& [](((r4 & !g4) | (!r4 & g4)) -> ((r4' & r4) | (!r4' & !r4)));
ENVGOAL:
  []<>!(r1 & g1)
& []<>!(r2 & g2)
& []<>!(r3 & g3)
& []<>!(r4 & g4);

SYSINIT: !g1 & !g2 & !g3 & !g4;
SYSTRANS:
  [](!g1' | !g2')
& [](!g1' | !g3')
& [](!g1' | !g4')
& [](!g2' | !g3')
& [](!g2' | !g4')
& [](!g3' | !g4')
& [](((r1 & g1) | (!r1 & !g1)) -> ((g1 & g1') | (!g1 & !g1')))
& [](((r2 & g2) | (!r2 & !g2)) -> ((g2 & g2') | (!g2 & !g2')))
& [](((r3 & g3) | (!r3 & !g3)) -> ((g3 & g3') | (!g3 & !g3')))
& [](((r4 & g4) | (!r4 & !g4)) -> ((g4 & g4') | (!g4 & !g4')));
SYSGOAL:
  []<>((r1 & g1) | (!r1 & !g1))
& []<>((r2 & g2) | (!r2 & !g2))
& []<>((r3 & g3) | (!r3 & !g3))
& []<>((r4 & g4) | (!r4 & !g4));
