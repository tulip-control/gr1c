#!/bin/sh -e
#
# Build binary distribution.  Requires environment variable GR1C_VERSION.
# The target platform is guessed using the uname command.

CUDDVER=3.0.0

KERNEL=`uname -s`
ARCH=`uname -m`

if [ $KERNEL = "MacOS" ]; then
    ARCHNAME="MacOS"
elif [ $KERNEL = "Linux" ]; then
    if [ $ARCH = "x86_64" -o $ARCH = "x86-64" ]; then
        ARCHNAME="Linux_x86-64"
    else
        ARCHNAME="Linux_"$ARCH
    fi
else
    echo Unrecognized platform \"$KERNEL $ARCH\"
    exit 1
fi

DNAME=gr1c-${GR1C_VERSION}-$ARCHNAME

if [ -d $DNAME ]; then
    echo ERROR: Directory \"$DNAME\" already exists.
    exit 1
fi

make -j4 all
strip --strip-debug gr1c gr1c-patch gr1c-autman gr1c-rg

make check
mkdir $DNAME

cp -r examples gr1c gr1c-patch gr1c-autman gr1c-rg README.md CHANGELOG LICENSE.txt $DNAME/
if [ -d extern/cudd-$CUDDVER ]; then
    CUDD_SRC_DIR=extern/cudd-$CUDDVER
elif [ -d extern/src/cudd-$CUDDVER ]; then
    CUDD_SRC_DIR=extern/src/cudd-$CUDDVER
else
    echo ERROR: CUDD source directory was not found.
    exit 1
fi
cp ${CUDD_SRC_DIR}/LICENSE $DNAME/LICENSE-cudd-$CUDDVER
if [ $KERNEL = "MacOS" ]; then
    zip -r $DNAME.zip $DNAME
elif [ $KERNEL = "Linux" ]; then
    tar -c $DNAME | gzip -9 > $DNAME.tar.gz
fi
