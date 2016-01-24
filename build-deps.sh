#!/bin/sh -e
#
# Build dependencies locally under extern/

CUDDVER=3.0.0

cd extern/src/cudd-$CUDDVER
./configure --prefix=`pwd`/../..
make
make install
