# Type and domain are inferred from the specification.  If a variable
# is not used with an "=" (equality), then it is treated as Boolean.
# Otherwise, the domain is assumed to be the subset of integers from
# the minimum to the maximum number appearing in the specification,
# inclusive.

# Internally variables of non-Boolean domains are mapped into a set of
# Boolean variables that use the original variable name as a prefix
# and then append the corresponding bit index. E.g., y with domain of
# {0,1,2} would translate to 2 variables, y0, y1 that are used to
# encode the bitvectors 00, 01, and 10. (11, i.e., y0 and y1 both set
# is unreachable).

# N.B., currently we require that the minimum value in the domain is 0
# (despite the above).

# Thus, in the present example, the domain of y is {0,1,2,3,4}.


ENV:;
SYS: y;

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
