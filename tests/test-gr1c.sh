#!/bin/sh
# Tests not targeted at particular units.
#
# SCL; 2012, 2013.

set -e

BUILD_ROOT=..
TESTDIR=tests
PREFACE="============================================================\nERROR:"


################################################################
# Test realizability

REFSPECS="gridworld.spc gridworld_env.spc arbiter4.spc trivial_2var.spc free_counter.spc"
UNREALIZABLE_REFSPECS="trivial_un.spc"

if [[ $VERBOSE -eq 1 ]]; then
    echo "\nChecking specifications that should be realizable..."
fi
for k in `echo $REFSPECS`; do
    if [[ $VERBOSE -eq 1 ]]; then
	echo "\t gr1c -r $TESTDIR/specs/$k"
    fi
    if ! $BUILD_ROOT/gr1c -r specs/$k > /dev/null; then
	echo $PREFACE "realizable specs/${k} detected as unrealizable\n"
	exit -1
    fi
done

if [[ $VERBOSE -eq 1 ]]; then
    echo "\nChecking specifications that should be unrealizable..."
fi
for k in `echo $UNREALIZABLE_REFSPECS`; do
    if [[ $VERBOSE -eq 1 ]]; then
	echo "\t gr1c -r $TESTDIR/specs/$k"
    fi
    if $BUILD_ROOT/gr1c -r specs/$k > /dev/null; then
	echo $PREFACE "unrealizable specs/${k} detected as realizable\n"
	exit -1
    fi
done


################################################################
# Synthesis regression tests

REFSPECS="trivial_2var.spc free_counter.spc"

if [[ $VERBOSE -eq 1 ]]; then
    echo "\nPerforming regression tests for vanilla GR(1) synthesis..."
fi
for k in `echo $REFSPECS`; do
    if [[ $VERBOSE -eq 1 ]]; then
	echo "\tComparing  gr1c -t txt $TESTDIR/specs/$k \n\t\tagainst $TESTDIR/expected_outputs/${k}.listdump.out"
    fi
    if ! ($BUILD_ROOT/gr1c -t txt specs/$k | cmp -s expected_outputs/${k}.listdump.out -); then
	echo $PREFACE "synthesis regression test failed for specs/${k}\n"
	exit -1
    fi
done


################################################################
# Reachability game synthesis regression tests

REFSPECS="reach_2var.spc reach_2var_mustblock.spc reach_free_counter.spc reach_free_counter_mustblock.spc"

if [[ $VERBOSE -eq 1 ]]; then
    echo "\nPerforming regression tests for reachability games..."
fi
for k in `echo $REFSPECS`; do
    if [[ $VERBOSE -eq 1 ]]; then
	echo "\tComparing  rg -t txt $TESTDIR/specs/$k \n\t\tagainst $TESTDIR/expected_outputs/${k}.listdump.out"
    fi
    if ! ($BUILD_ROOT/rg -t txt specs/$k | cmp -s expected_outputs/${k}.listdump.out -); then
	echo $PREFACE "Reachability game synthesis regression test failed for specs/${k}\n"
	exit -1
    fi
done


################################################################
# Test interaction;  N.B., quite fragile depending on interface

REFSPECS="trivial_partwin gridworld"

if [[ $VERBOSE -eq 1 ]]; then
    echo "\nPerforming regression tests for interaction..."
fi
for k in `echo $REFSPECS`; do
    if [[ $VERBOSE -eq 1 ]]; then
	echo "\tComparing gr1c -i $TESTDIR/specs/${k}.spc < $TESTDIR/interaction_scripts/${k}_IN.txt \n\t\tagainst $TESTDIR/interaction_scripts/${k}_OUT.txt"
    fi
    if ! $BUILD_ROOT/gr1c -i specs/${k}.spc < interaction_scripts/${k}_IN.txt | diff - interaction_scripts/${k}_OUT.txt > /dev/null; then
        echo $PREFACE "unexpected behavior in scripted interaction using specs/${k}\n"
        exit -1
    fi
done


################################################################
# gr1c specification file syntax

if [[ $VERBOSE -eq 1 ]]; then
    echo "\nChecking detection of flawed specification files..."
fi
for k in `ls flawed_specs/*.spc`; do
    if [[ $VERBOSE -eq 1 ]]; then
	echo "\t gr1c -s $k"
    fi
    if $BUILD_ROOT/gr1c -s $k > /dev/null 2>&1; then
	echo $PREFACE "Flawed ${k} detected as OK\n"
	exit -1
    fi
done


################################################################
# rg specification file syntax

if [[ $VERBOSE -eq 1 ]]; then
    echo "\nChecking detection of flawed reachability game (rg) specification files..."
fi
for k in `ls flawed_reach_specs/*.spc`; do
    if [[ $VERBOSE -eq 1 ]]; then
	echo "\t gr1c -s $k"
    fi
    if $BUILD_ROOT/rg -s $k > /dev/null 2>&1; then
	echo $PREFACE "Flawed reachability game spec ${k} detected as OK\n"
	exit -1
    fi
done
