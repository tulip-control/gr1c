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


export PATH=`pwd`/${BUILD_ROOT}:$PATH

if test $VERBOSE -eq 1; then
    echo '\nChecking for detection of flawed command-line usage...'
fi

# Format for each test case: ARGS[;FILE]
#
# where the command is `gr1c ARGS` and if FILE is given, then the test
# case fails if the file is found to already exist.  E.g., the
# optional FILE is motivated to avoid naming collisions.
for args in '-notoption' '-hh' '--00f' '-- rg  # where `rg` is not a file.;rg' 'f00f  # attempt to read from file that does not exist.;f00f'; do
    if test $VERBOSE -eq 1; then
        echo "\t gr1c ${args%%;*}"
    fi
    file=${args##*;}
    args=${args%%;*}
    if [ -n "${file}" ]; then
        if [ -r ${file} ]; then
            echo $PREFACE 'Local file `'${file}'` found in tests directory. It will interfere with tests.'
            echo
            exit 1
        fi
    fi
    gr1c ${args} > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo $PREFACE 'Failed to detect bogus command-line switch' ${args}
        echo
        exit 1
    fi
done


if test $VERBOSE -eq 1; then
    echo '\nChecking for basic tasks expressed through stdin...'
fi

# Format of each test case: ARGS<<<STDIN
# where the command is `echo STDIN | gr1c ARGS`.
# Each command is expected to succeed (exitcode 0).
for args in "<<<SYS: x;" "rg<<<SYS: x;
SYSGOAL:<>x;"; do
    stdin=${args##*<<<}
    args=${args%%<<<*}
    if test $VERBOSE -eq 1; then
        echo "\t gr1c ${args}  # with input:" ${stdin}
    fi
    echo "${stdin}" | gr1c ${args} > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo $PREFACE "Failed on basic task via stdin: echo ${stdin} | gr1c ${args}"
        echo
        exit 1
    fi
done


if test $VERBOSE -eq 1; then
   echo '\nParsing version message from `gr1c -V`...'
fi
VERSION_LINE=`gr1c -V | head -1`
echo $VERSION_LINE | grep dev0 > /dev/null 2>&1
if [ $? -ne 0 ]; then
    VERSION=`echo ${VERSION_LINE} | cut -d ' ' -f2`
    VERSION_SPLIT=`echo ${VERSION} | sed 's/\./ /g'`
    if test $VERBOSE -eq 1; then
        echo 'Found version:' ${VERSION}
    fi
    if [ `echo ${VERSION_SPLIT} | wc -w` -ne 3 ]; then
        echo $PREFACE 'detected version does not have 3 component numbers'
        echo
        exit 1
    fi
fi


if test $VERBOSE -eq 1; then
    echo '\nChecking redundant command-line switch combinations...'
fi


if (hash mktemp > /dev/null 2>&1); then
    MKTEMP='mktemp tmp.XXXXXXXXXXXX'
else
    MKTEMP=tempfile
fi

if test $VERBOSE -eq 1; then
    echo '\nComparing `gr1c help` and `gr1c -h`...'
fi
TMPFILE=`$MKTEMP`
gr1c help > $TMPFILE
if ! (gr1c -h | diff $TMPFILE -); then
    echo $PREFACE 'Outputs of `gr1c help` and gr1c -h` do not match'
    echo
    exit 1
fi

if test $VERBOSE -eq 1; then
    echo '\nComparing `gr1c --help` and `gr1c -h`...'
fi
TMPFILE=`$MKTEMP`
gr1c --help > $TMPFILE
if ! (gr1c -h | diff $TMPFILE -); then
    echo $PREFACE 'Outputs of `gr1c --help` and gr1c -h` do not match'
    echo
    exit 1
fi
