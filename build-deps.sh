#!/bin/sh -e
#
# Build dependencies locally under extern/
# Also build extras if present, e.g., after get-extra-deps.sh

CUDDVER=3.0.0
SPINVER=6.4.5

cd extern/src/cudd-$CUDDVER
./configure --prefix=`pwd`/../..
make
make install


if [ -d ../Spin/Src${SPINVER} ]; then
    cd ../Spin/Src${SPINVER}
    make
fi
