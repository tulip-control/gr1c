ENV: x foo;
SYS: y;

ENVINIT: x | !x;
ENVTRANS: [](x -> x') & []!x -> !x';
ENVGOAL:;

# Blank lines are optional and can placed between sections or parts of
# formulas.

SYSINIT: y; #| !(x | foo); #x->y

SYSTRANS: # Notice the safety formula spans two lines.
   [](y -> !(y'))
 & [](!y -> y')
;

SYSGOAL:; #[]<>y;
