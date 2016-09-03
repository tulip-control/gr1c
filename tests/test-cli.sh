#!/bin/sh
# Command-line interface (CLI) tests
#
#
# SCL; 2016

BUILD_ROOT=..
TESTDIR=tests
PREFACE="============================================================\nERROR:"

if test -z $VERBOSE; then
    VERBOSE=0
fi


export PATH=${BUILD_ROOT}

if test $VERBOSE -eq 1; then
    echo '\nChecking for detection of flawed command-line usage...'
fi
for args in '-notoption' '-hh' '--00f'; do
    if test $VERBOSE -eq 1; then
        echo "\t gr1c ${args}"
    fi
    gr1c ${args} > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo $PREFACE 'Failed to detect bogus command-line switch' ${args}
        echo
        exit 1
    fi
done
