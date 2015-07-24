#!/bin/sh -e
#
# Build binary distribution.  Requires environment variable GR1C_VERSION.
# The target platform is guessed using the uname command.

CUDDVER=2.5.0

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
fi

DNAME=gr1c-${GR1C_VERSION}-$KERNEL

if [ -d $DNAME ]; then
    echo ERROR: Directory \"$DNAME\" already exists.
    exit 1
fi

make all
make check
mkdir $DNAME

cp -r examples gr1c gr1c-patch autman gr1c-rg README.md LICENSE.txt $DNAME/
cp extern/cudd-$CUDDVER/LICENSE $DNAME/LICENSE-cudd-$CUDDVER
if [ $KERNEL = "MacOS" ]; then
    zip -r $DNAME.zip $DNAME
elif [ $KERNEL = "Linux" ]; then
    tar -c $DNAME | gzip -9 > $DNAME.tar.gz
fi
