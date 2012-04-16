#!/bin/sh

BUILD_ROOT=..
PREFACE="============================================================\nERROR:"


############################################################
# Test realizability

REFSPECS=(gridworld.spc gridworld_env.spc)
UNREALIZABLE_REFSPECS=(trivial_un.spc)

for k in ${REFSPECS[@]}; do
    if ! $BUILD_ROOT/gr1c -r specs/$k > /dev/null; then
	echo $PREFACE "realizable specs/${k} detected as unrealizable\n"
	exit -1
    fi
done

for k in ${UNREALIZABLE_REFSPECS[@]}; do
    if $BUILD_ROOT/gr1c -r specs/$k > /dev/null; then
	echo $PREFACE "unrealizable specs/${k} detected as realizable\n"
	exit -1
    fi
done
