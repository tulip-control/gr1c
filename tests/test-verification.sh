#!/bin/sh
# Tests based on verification of synthesis output.
#
#
# SCL; 2016

set -e

BUILD_ROOT=..
SPINVER=6.4.5

if test -z $VERBOSE; then
    VERBOSE=0
fi

SPINEXE=${BUILD_ROOT}/extern/src/Spin/Src${SPINVER}/spin
if test $VERBOSE -eq 1; then
    echo
    ${SPINEXE} -V
fi
