#!/bin/sh
# Tests for the program gr1c-patch and not targeted at particular units.
#
# SCL; 2013.

set -e

BUILD_ROOT=..
TESTDIR=tests
PREFACE="============================================================\nERROR:"

if test -z $VERBOSE; then
    VERBOSE=0
fi


################################################################
# patch_localfixpoint() regression tests

REFSPECS="gridworld_2x20_1obs"

if test $VERBOSE -eq 1; then
    echo "\nPerforming regression tests for patch_localfixpoint()..."
fi
for k in `echo $REFSPECS`; do
    if test $VERBOSE -eq 1; then
        echo "\tComparing  gr1c -t aut $TESTDIR/specs/patching/${k}.spc \n\t\tagainst $TESTDIR/expected_outputs/patching/${k}.spc.autdump.out"
    fi
    if ! ($BUILD_ROOT/gr1c -t aut specs/patching/${k}.spc | cmp -s expected_outputs/patching/${k}.spc.autdump.out -); then
        echo $PREFACE "gr1c-patch, patch_localfixpoint() regression test failed for specs/${k}\n"
        exit 1
    fi
    if test $VERBOSE -eq 1; then
        echo "\tComparing  gr1c-patch -t aut -a $TESTDIR/expected_outputs/patching/${k}.spc.autdump.out -e $TESTDIR/specs/patching/${k}.edc $TESTDIR/specs/patching/${k}.spc \n\t\tagainst $TESTDIR/expected_outputs/patching/${k}.edc.autdump.out"
    fi
    if ! ($BUILD_ROOT/gr1c-patch -t aut -a expected_outputs/patching/${k}.spc.autdump.out -e specs/patching/${k}.edc specs/patching/${k}.spc | cmp -s expected_outputs/patching/${k}.edc.autdump.out -); then
        echo $PREFACE "gr1c-patch, patch_localfixpoint() regression test failed for specs/${k}\n"
        exit 1
    fi
done
