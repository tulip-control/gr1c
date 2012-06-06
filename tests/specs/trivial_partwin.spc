# For example, regarding states as bitvectors, 1011 is not in winning
# set, while 1010 is. (Ordering is x ze y zs.)

ENV: x ze;
SYS: y zs;

ENVINIT: x & !ze;
ENVTRANS: [] (zs -> ze') & []((!ze & !zs) -> !ze');
ENVGOAL: []<>x;

SYSINIT: y;
SYSTRANS:;
SYSGOAL: []<>y&x & []<>!y & []<> !ze;
