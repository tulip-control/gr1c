ENV: x;
SYS: y;

ENVINIT: x;
ENVTRANS:;# [](x -> !x') & [](!x -> x');
ENVGOAL: []<>x;

# Blank lines are optional and can placed between sections or parts of
# formulas.

SYSINIT: y;

SYSTRANS: # Notice the safety formula spans two lines.
;#[](y -> !y')
#& [](!y -> y');

SYSGOAL: []<>y&x & []<>!y;
