#!/bin/sh
# Tests not targeted at particular units.
#
# SCL; 29 Apr 2012.


BUILD_ROOT=..
PREFACE="============================================================\nERROR:"


############################################################
# Test realizability

REFSPECS="gridworld.spc gridworld_env.spc arbiter4.spc"
UNREALIZABLE_REFSPECS="trivial_un.spc"

for k in `echo $REFSPECS`; do
    if ! $BUILD_ROOT/gr1c -r specs/$k > /dev/null; then
	echo $PREFACE "realizable specs/${k} detected as unrealizable\n"
	exit -1
    fi
done

for k in `echo $UNREALIZABLE_REFSPECS`; do
    if $BUILD_ROOT/gr1c -r specs/$k > /dev/null; then
	echo $PREFACE "unrealizable specs/${k} detected as realizable\n"
	exit -1
    fi
done
