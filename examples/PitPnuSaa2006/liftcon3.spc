ENV: b1 b2 b3;
SYS: f1 f2 f3;

ENVINIT: !b1 & !b2 & !b3;  # Initially no requests ("no buttons pressed").
ENVTRANS:
  [](((b1 & f1) -> !b1') & ((b1 & !f1) -> b1'))
& [](((b2 & f2) -> !b2') & ((b2 & !f2) -> b2'))
& [](((b3 & f3) -> !b3') & ((b3 & !f3) -> b3'));
ENVGOAL:;

SYSINIT: f1 & !f2 & !f3;  # Start on first floor
SYSTRANS:
  [](((f1 & f2') | (f2 & f3')) -> (b1 | b2 | b3))
& [](!(f1 & f2) & !(f1 & f3) & !(f2 & f3))
& [](f1 -> (f1' | f2'))
& [](f2 -> (f1' | f2' | f3'))
& [](f3 -> (f2' | f3'));
SYSGOAL:
  []<>(f1 | b1 | b2 | b3)
& []<>(b1 -> f1)
& []<>(b2 -> f2)
& []<>(b3 -> f3);
