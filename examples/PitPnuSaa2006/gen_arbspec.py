#!/usr/bin/env python
"""
Generate arbiter specification for case of N lines.

Use given filename FILE, or stdout if -
Default filename is "arbiterN.spc"

This example appears in N. Piterman, A. Pnueli, and Y. Sa'ar (2006).
Synthesis of Reactive(1) Designs. *In Proc. 7th International
Conference on Verification, Model Checking and Abstract Interpretation*.

SCL; 2012, 2015
"""
from __future__ import print_function
import sys
import itertools


def dump_arbiter_spec(N):
    """Generate arbiter specification for case of N lines."""
    output = "ENV: "+" ".join(["r"+str(i) for i in range(1, N+1)])+";\n"
    output += "SYS: "+" ".join(["g"+str(i) for i in range(1, N+1)])+";\n\n"

    output += "ENVINIT: "+" & ".join(["!r"+str(i) for i in range(1, N+1)])+";\n"
    output += "ENVTRANS:\n  "+"\n& ".join(["[](((r"+str(i)+" & !g"+str(i)+") | (!r"+str(i)+" & g"+str(i)+")) -> ((r"+str(i)+"' & r"+str(i)+") | (!r"+str(i)+"' & !r"+str(i)+")))" for i in range(1, N+1)])+";\n"
    output += "ENVGOAL:\n  "+"\n& ".join(["[]<>!(r"+str(i)+" & g"+str(i)+")" for i in range(1, N+1)])+";\n"

    output += "\nSYSINIT: "+" & ".join(["!g"+str(i) for i in range(1, N+1)])+";\n"
    output += "SYSTRANS:\n  "
    if N > 1:
        output += "\n& ".join(["[](!g"+str(i)+"' | !g"+str(j)+"')" for (i, j) in itertools.combinations(range(1, N+1), 2)])+"\n& "
    output += "\n& ".join(["[](((r"+str(i)+" & g"+str(i)+") | (!r"+str(i)+" & !g"+str(i)+")) -> ((g"+str(i)+" & g"+str(i)+"') | (!g"+str(i)+" & !g"+str(i)+"')))" for i in range(1, N+1)])+";\n"
    output += "SYSGOAL:\n  "+"\n& ".join(["[]<>((r"+str(i)+" & g"+str(i)+") | (!r"+str(i)+" & !g"+str(i)+"))" for i in range(1, N+1)])+";\n"
    return output


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: "+sys.argv[0]+" N [FILE|-]")
        exit(1)

    N = int(sys.argv[1])
    if N < 1:
        print("Number of input/output lines must be positive integer.")
        exit(1)
    if len(sys.argv) >= 3:
        fname = sys.argv[2]
    else:
        fname = "arbiterN.spc"

    if fname == "-":
        print(dump_arbiter_spec(N))
    else:
        with open(fname, "w") as f:
            f.write(dump_arbiter_spec(N))
