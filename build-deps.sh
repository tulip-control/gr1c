#!/bin/sh -e
#
# Build dependencies locally under extern/
# Also build extras if present, e.g., after get-extra-deps.sh

CUDDVER=3.0.0
SPINVER=6.5.2

cd extern/src/cudd-$CUDDVER
make distclean || true  # Fails before first ./configure
./configure --prefix="$(pwd)/../.."
make
make install


if [ -d ../Spin-version-${SPINVER} ]; then
    cd ../Spin-version-${SPINVER}
    make clean
    make
fi
