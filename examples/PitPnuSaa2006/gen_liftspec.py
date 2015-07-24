#!/usr/bin/env python
"""
Generate lift controller specification for case of N floors.

Use given filename FILE, or stdout if -
Default filename is "liftconN.spc"

This example appears in N. Piterman, A. Pnueli, and Y. Sa'ar (2006).
Synthesis of Reactive(1) Designs. *In Proc. 7th International
Conference on Verification, Model Checking and Abstract Interpretation*.

SCL; 2012, 2015
"""
from __future__ import print_function
import sys
import itertools


def dump_lift_spec(N):
    """Generate lift controller specification for case of N floors."""
    output = "ENV: "+" ".join(["b"+str(i) for i in range(1, N+1)])+";\n"
    output += "SYS: "+" ".join(["f"+str(i) for i in range(1, N+1)])+";\n\n"

    output += "ENVINIT: "+" & ".join(["!b"+str(i) for i in range(1, N+1)])+";\n"
    output += "ENVTRANS:\n  "+"\n& ".join(["[](((b"+str(i)+" & f"+str(i)+") -> !b"+str(i)+"') & ((b"+str(i)+" & !f"+str(i)+") -> b"+str(i)+"'))" for i in range(1, N+1)])+";\n"
    output += "ENVGOAL:;\n\n"

    output += "\nSYSINIT: f1 & "+" & ".join(["!f"+str(i) for i in range(2, N+1)])+";\n"
    output += "SYSTRANS:\n  "
    if N > 1:
        output += "[](("+" | ".join(["(f"+str(i)+" & f"+str(i+1)+"')" for i in range(1, N)])+") -> ("+" | ".join(["b"+str(i) for i in range(1, N+1)])+"))\n& "
    output += "[]("+" & ".join(["!(f"+str(i)+" & f"+str(j)+")" for (i,j) in itertools.combinations(range(1, N+1), 2)])+")\n& "
    if N > 1:
        output += "[](f1 -> (f1' | f2'))"
    else:
        output += "[](f1 -> f1')"
    if N > 2:
        output += "\n& "+"\n& ".join(["[](f"+str(i)+" -> (f"+str(i-1)+"' | f"+str(i)+"' | f"+str(i+1)+"'))" for i in range(2, N)])
    if N > 1:
        output += "\n& [](f"+str(N)+" -> (f"+str(N-1)+"' | f"+str(N)+"'))"
    output += ";\n"
    output += "SYSGOAL:\n  []<>(f1 | "+" | ".join(["b"+str(i) for i in range(1, N+1)])+")\n& "
    output += "\n& ".join(["[]<>(b"+str(i)+" -> f"+str(i)+")" for i in range(1, N+1)])+";\n"
    return output


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: "+sys.argv[0]+" N [FILE|-]")
        exit(1)

    N = int(sys.argv[1])
    if N < 1:
        print("Number of floors must be positive integer.")
        exit(1)
    if len(sys.argv) >= 3:
        fname = sys.argv[2]
    else:
        fname = "liftconN.spc"

    if fname == "-":
        print(dump_lift_spec(N))
    else:
        with open(fname, "w") as f:
            f.write(dump_lift_spec(N))
