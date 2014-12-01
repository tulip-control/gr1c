The `exp` directory contains code for experimental use of gr1c.  In cases where
there are relevant research publications, references are provided.  By default
(i.e., if you run `make` from the root of the gr1c source tree), none of these
programs are built.  To build them all, try `make exp`.  To build specific
codes, try `make FOO`, where "FOO" is a name appearing in the following list.

N.B., programs in `exp` rely heavily on routines that are defined elsewhere,
usually in source code found under the `src` directory.

* grjit:
  run experiments for "just in time" synthesis and related work.

* grpatch:
  apply incremental synthesis methods as provided by the functions
  patch_localfixpoint() and add_metric_sysgoal().  Consult `src/patching.h`.
