#!/bin/sh
# Tests based on verification of synthesis output.
#
#
# SCL; 2016

set -e

BUILD_ROOT=..
TESTDIR=tests
PREFACE="============================================================\nERROR:"

SPINVER=6.4.5


if test -z $VERBOSE; then
    VERBOSE=0
fi

SPINEXE=${BUILD_ROOT}/extern/src/Spin/Src${SPINVER}/spin
if test $VERBOSE -eq 1; then
    ${SPINEXE} -V
fi

REFSPECS="count_onestep.spc empty.spc free_counter.spc gridworld_bool.spc gridworld_env.spc trivial_2var.spc trivial_mustblock.spc"
for REFSPC in `echo $REFSPECS`; do
    if test $VERBOSE -eq 1; then
        echo "\nConstructing strategy for ${TESTDIR}/specs/${REFSPC}"
        echo "\tgr1c -t aut ${TESTDIR}/specs/${REFSPC} > ${REFSPC}.aut"
    fi
    $BUILD_ROOT/gr1c -t aut specs/${REFSPC} > ${REFSPC}.aut
    if test $VERBOSE -eq 1; then
        echo "\nVerifying it using Spin..."
        echo "\tgr1c-autman -i specs/${REFSPC} ${REFSPC}.aut -P -o ${REFSPC}.aut.pml"
    fi
    FORMULA=`$BUILD_ROOT/gr1c-autman -i specs/${REFSPC} ${REFSPC}.aut -P -o ${REFSPC}.aut.pml`
    if test $VERBOSE -eq 1; then
        echo "\tspin -f \"!(${FORMULA})\" >> ${REFSPC}.aut.pml"
    fi
    ${SPINEXE} -f "!(${FORMULA})" >> ${REFSPC}.aut.pml
    if test $VERBOSE -eq 1; then
        echo "\tspin -a ${REFSPC}.aut.pml"
        echo "\tcc -o pan pan.c && ./pan -a"
    fi
    ${SPINEXE} -a ${REFSPC}.aut.pml
    cc -o pan pan.c
    if test `./pan -a | grep errors| cut -d: -f2` -ne 0; then
        echo $PREFACE "Strategy does not satisfy specification ${TESTDIR}/specs/${REFSPC}\n"
        exit 1
    fi
done
