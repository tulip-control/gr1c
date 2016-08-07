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

$BUILD_ROOT/gr1c -t aut specs/free_counter.spc > free_counter.spc.aut
$BUILD_ROOT/gr1c-autman -i specs/free_counter.spc free_counter.spc.aut -P > free_counter.spc.aut.pml
${SPINEXE} -f '!(X sysinit && [] (!checkstrans || systrans) && []<> sysgoal0000 && [] !pmlfault)' >> free_counter.spc.aut.pml
${SPINEXE} -a free_counter.spc.aut.pml
cc -o pan pan.c
if test `./pan -a | grep errors| cut -d: -f2` -ne 0; then
    exit 1
fi
