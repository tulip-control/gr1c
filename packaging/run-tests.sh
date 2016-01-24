#!/bin/sh
#
# Script for running regression tests against a binary release.
#
#
# SCL; 2014

VERBOSE=1


cd tests
VERBOSE=${VERBOSE} sh test-gr1c.sh
VERBOSE=${VERBOSE} sh test-grpatch.sh
echo "============================================================\nPASSED\n"
