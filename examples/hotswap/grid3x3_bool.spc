#   ./gr1c -t aut examples/hotswap/grid3x3_bool.spc > grid-nominal.aut
#   ./grpatch -l -vv -t dot -a grid-nominal.aut -f 'Y_2_0 & (!Y_0_0) & (!Y_0_1) & (!Y_0_2) & (!Y_1_0) & (!Y_1_1) & (!Y_1_2) & (!Y_2_1) & (!Y_2_2)' examples/hotswap/grid3x3_bool.spc
#
# ...or try
#
#   ./grpatch -l -vv -t dot -a grid-nominal.aut -f 'Y_0_2 & (!Y_0_0) & (!Y_0_1) & (!Y_2_0) & (!Y_1_0) & (!Y_1_1) & (!Y_1_2) & (!Y_2_1) & (!Y_2_2)' examples/hotswap/grid3x3_bool.spc

ENV: ;
SYS: Y_0_0 Y_0_1 Y_0_2 Y_1_0 Y_1_1 Y_1_2 Y_2_0 Y_2_1 Y_2_2;

ENVINIT: ;
ENVTRANS:;
ENVGOAL:;

SYSINIT: ((Y_0_0 & !Y_0_1 & !Y_0_2 & !Y_1_0 & !Y_1_1 & !Y_1_2 & !Y_2_0 & !Y_2_1 & !Y_2_2));
SYSTRANS: [](Y_0_0 -> (Y_0_0' | Y_1_0' | Y_0_1'))
& [](Y_0_1 -> (Y_0_1' | Y_0_0' | Y_1_1' | Y_0_2'))
& [](Y_0_2 -> (Y_0_2' | Y_0_1' | Y_1_2'))
& [](Y_1_0 -> (Y_1_0' | Y_0_0' | Y_2_0' | Y_1_1'))
& [](Y_1_1 -> (Y_1_1' | Y_0_1' | Y_1_0' | Y_2_1' | Y_1_2'))
& [](Y_1_2 -> (Y_1_2' | Y_0_2' | Y_1_1' | Y_2_2'))
& [](Y_2_0 -> (Y_2_0' | Y_1_0' | Y_2_1'))
& [](Y_2_1 -> (Y_2_1' | Y_1_1' | Y_2_0' | Y_2_2'))
& [](Y_2_2 -> (Y_2_2' | Y_1_2' | Y_2_1'))
& []((Y_0_0' & (!Y_0_1') & (!Y_0_2') & (!Y_1_0') & (!Y_1_1') & (!Y_1_2') & (!Y_2_0') & (!Y_2_1') & (!Y_2_2'))
| (Y_0_1' & (!Y_0_0') & (!Y_0_2') & (!Y_1_0') & (!Y_1_1') & (!Y_1_2') & (!Y_2_0') & (!Y_2_1') & (!Y_2_2'))
| (Y_0_2' & (!Y_0_0') & (!Y_0_1') & (!Y_1_0') & (!Y_1_1') & (!Y_1_2') & (!Y_2_0') & (!Y_2_1') & (!Y_2_2'))
| (Y_1_0' & (!Y_0_0') & (!Y_0_1') & (!Y_0_2') & (!Y_1_1') & (!Y_1_2') & (!Y_2_0') & (!Y_2_1') & (!Y_2_2'))
| (Y_1_1' & (!Y_0_0') & (!Y_0_1') & (!Y_0_2') & (!Y_1_0') & (!Y_1_2') & (!Y_2_0') & (!Y_2_1') & (!Y_2_2'))
| (Y_1_2' & (!Y_0_0') & (!Y_0_1') & (!Y_0_2') & (!Y_1_0') & (!Y_1_1') & (!Y_2_0') & (!Y_2_1') & (!Y_2_2'))
| (Y_2_0' & (!Y_0_0') & (!Y_0_1') & (!Y_0_2') & (!Y_1_0') & (!Y_1_1') & (!Y_1_2') & (!Y_2_1') & (!Y_2_2'))
| (Y_2_1' & (!Y_0_0') & (!Y_0_1') & (!Y_0_2') & (!Y_1_0') & (!Y_1_1') & (!Y_1_2') & (!Y_2_0') & (!Y_2_2'))
| (Y_2_2' & (!Y_0_0') & (!Y_0_1') & (!Y_0_2') & (!Y_1_0') & (!Y_1_1') & (!Y_1_2') & (!Y_2_0') & (!Y_2_1')));

SYSGOAL: []<>(Y_0_0)
& []<>(Y_2_2);