# The type and domain of a variable is specified in its declaration
# (in the lists "ENV:" and "SYS:").  Currently two types are
# supported, boolean and unsigned integer.  The former is the default,
# and the latter is achieved by following a variable name with an
# interval of the form [0,n] where n is a positive integer.  The only
# currently supported domain for nonboolean variables is all integers
# from 0 to n, inclusive.
#
# Thus, in the example below, the domain of y is {0,1,2,3,4}.

# Internally variables of non-Boolean domains are mapped into a set of
# Boolean variables that use the original variable name as a prefix
# and then append the corresponding bit index. E.g., y with domain of
# {0,1,2} would translate to 2 variables, y0, y1 that are used to
# encode the bitvectors 00, 01, and 10. (11, i.e., y0 and y1 both set
# is unreachable).


ENV:;
SYS: y [0,4];

ENVINIT:;
ENVTRANS:;
ENVGOAL:;

SYSINIT: y=4;

SYSTRANS:
  []((y=4) -> (y'=3 | y'=4))
& []((y=3) -> (y'=2 | y'=3 | y'=4))
& []((y=2) -> (y'=1 | y'=2 | y'=3))
& []((y=1) -> (y'=0 | y'=1 | y'=2))
& []((y=0) -> (y'=0 | y'=1));

SYSGOAL: []<>y=0 & []<>y=4;
