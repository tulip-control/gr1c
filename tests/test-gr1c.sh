#!/bin/sh
# Tests not targeted at particular units.
#
# Unless the "-n" switch is explicitly given, tests involving command-line tools
# rely on the assumption that the default init_flags is ALL_ENV_EXIST_SYS_INIT
#
#
# SCL; 2012-2014.

set -e

BUILD_ROOT=..
TESTDIR=tests
PREFACE="============================================================\nERROR:"

if test -z $VERBOSE; then
    VERBOSE=0
fi


################################################################
# Test realizability

REFSPECS="gridworld_bool.spc gridworld_env.spc arbiter4.spc trivial_2var.spc free_counter.spc empty.spc trivial_mustblock.spc"
UNREALIZABLE_REFSPECS="trivial_un.spc"

if test $VERBOSE -eq 1; then
    echo "\nChecking specifications that should be realizable..."
fi
for k in `echo $REFSPECS`; do
    if test $VERBOSE -eq 1; then
        echo "\t gr1c -r $TESTDIR/specs/$k"
    fi
    if ! $BUILD_ROOT/gr1c -r specs/$k > /dev/null; then
        echo $PREFACE "realizable specs/${k} detected as unrealizable\n"
        exit 1
    fi
done

if test $VERBOSE -eq 1; then
    echo "\nChecking specifications that should be unrealizable..."
fi
for k in `echo $UNREALIZABLE_REFSPECS`; do
    if test $VERBOSE -eq 1; then
        echo "\t gr1c -r $TESTDIR/specs/$k"
    fi
    if $BUILD_ROOT/gr1c -r specs/$k > /dev/null; then
        echo $PREFACE "unrealizable specs/${k} detected as realizable\n"
        exit 1
    fi
done


# Testing init_flags besides ALL_ENV_EXIST_SYS_INIT
if test $VERBOSE -eq 1; then
    echo "\t gr1c -r -n ALL_INIT $TESTDIR/specs/trivial_partwin.spc"
fi
if $BUILD_ROOT/gr1c -r -n ALL_INIT specs/trivial_partwin.spc > /dev/null; then
    echo $PREFACE "unrealizable specs/trivial_partwin.spc with init_flags ALL_INIT detected as realizable\n"
    exit 1
fi


################################################################
# Synthesis regression tests


REFSPECS="trivial_2var.spc free_counter.spc count_onestep.spc trivial_mustblock.spc"

if test $VERBOSE -eq 1; then
    echo "\nPerforming regression tests for vanilla GR(1) synthesis..."
fi
for k in `echo $REFSPECS`; do
    if test $VERBOSE -eq 1; then
        echo "\tComparing  gr1c -t txt $TESTDIR/specs/$k \n\t\tagainst $TESTDIR/expected_outputs/${k}.listdump.out"
    fi
    if ! ($BUILD_ROOT/gr1c -t txt specs/$k | cmp -s expected_outputs/${k}.listdump.out -); then
        echo $PREFACE "synthesis regression test failed for specs/${k}\n"
        exit 1
    fi
done


if test $VERBOSE -eq 1; then
    echo "\nRegression tests for GR(1) synthesis with other init_flags..."
fi
for q in ALL_INIT; do
    for k in count_onestep.spc; do
        if test $VERBOSE -eq 1; then
            echo "\tComparing  gr1c -n ${q} -t txt $TESTDIR/specs/$k \n\t\tagainst $TESTDIR/expected_outputs/${k}.${q}.listdump.out"
        fi
        if ! ($BUILD_ROOT/gr1c -n ${q} -t txt specs/$k | cmp -s expected_outputs/${k}.${q}.listdump.out -); then
            echo $PREFACE "synthesis regression test failed for specs/${k} with init_flags ${q}\n"
            exit 1
        fi
    done
done


################################################################
# Checking output formats

if test $VERBOSE -eq 1; then
    echo "\nChecking syntax for examples of DOT output..."
fi
if ! ($BUILD_ROOT/gr1c -t dot specs/trivial_2var.spc | dot -Tsvg > /dev/null); then
    echo $PREFACE "syntax error in DOT output from gr1c on specs/trivial_2var.spc\n"
    exit 1
fi

if test $VERBOSE -eq 1; then
    echo "\nChecking syntax for examples of JSON output..."
fi
if ! ($BUILD_ROOT/gr1c -t json specs/trivial_2var.spc | python -m json.tool > /dev/null); then
    echo $PREFACE "syntax error in JSON output from gr1c on specs/trivial_2var.spc\n"
    exit 1
fi


################################################################
# Reachability game synthesis regression tests

REFSPECS="reach_2var.spc reach_2var_mustblock.spc reach_free_counter.spc reach_free_counter_mustblock.spc reach_count_onestep.spc"

if test $VERBOSE -eq 1; then
    echo "\nPerforming regression tests for reachability games..."
fi
for k in `echo $REFSPECS`; do
    if test $VERBOSE -eq 1; then
        echo "\tComparing  gr1c-rg -t txt $TESTDIR/specs/$k \n\t\tagainst $TESTDIR/expected_outputs/${k}.listdump.out"
    fi
    if ! ($BUILD_ROOT/gr1c-rg -t txt specs/$k | cmp -s expected_outputs/${k}.listdump.out -); then
        echo $PREFACE "Reachability game synthesis regression test failed for specs/${k}\n"
        exit 1
    fi
done


################################################################
# Test interaction;  N.B., quite fragile depending on interface

REFSPECS="trivial_partwin gridworld_bool"

if test $VERBOSE -eq 1; then
    echo "\nPerforming regression tests for interaction..."
fi
for k in `echo $REFSPECS`; do
    if test $VERBOSE -eq 1; then
        echo "\tComparing  gr1c -i $TESTDIR/specs/${k}.spc < $TESTDIR/interaction_scripts/${k}_IN.txt \n\t\tagainst $TESTDIR/interaction_scripts/${k}_OUT.txt"
    fi
    if ! $BUILD_ROOT/gr1c -i specs/${k}.spc < interaction_scripts/${k}_IN.txt | diff - interaction_scripts/${k}_OUT.txt > /dev/null; then
        echo $PREFACE "unexpected behavior in scripted interaction using specs/${k}\n"
        exit 1
    fi
done


################################################################
# gr1c specification file syntax

if test $VERBOSE -eq 1; then
    echo "\nChecking detection of flawed specification files..."
fi
for k in `ls flawed_specs/*.spc`; do
    if test $VERBOSE -eq 1; then
        echo "\t gr1c -s $k"
    fi
    if $BUILD_ROOT/gr1c -s $k > /dev/null 2>&1; then
        echo $PREFACE "Flawed ${k} detected as OK\n"
        exit 1
    fi
done


################################################################
# rg specification file syntax

if test $VERBOSE -eq 1; then
    echo "\nChecking detection of flawed reachability game (rg) specification files..."
fi
for k in `ls flawed_reach_specs/*.spc`; do
    if test $VERBOSE -eq 1; then
        echo "\t gr1c-rg -s $k"
    fi
    if $BUILD_ROOT/gr1c-rg -s $k > /dev/null 2>&1; then
        echo $PREFACE "Flawed reachability game spec ${k} detected as OK\n"
        exit 1
    fi
done
