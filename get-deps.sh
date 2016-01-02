#!/bin/sh -e
#
# Fetch dependencies, and place them in location that Makefile expects

CUDDVER=2.5.1
SHA256SUM=4b19c34328d8738a839b994c6b9395f3895ff981d2f3495ce62e7ba576ead88b
URI=ftp://vlsi.colorado.edu/pub/cudd-$CUDDVER.tar.gz

mkdir -p extern
curl -sS $URI > extern/cudd-$CUDDVER.tar.gz

if hash sha256sum >/dev/null 2>&1; then
    FILECHECKSUM=`sha256sum extern/cudd-$CUDDVER.tar.gz| sed 's/ .*//'`
elif hash shasum >/dev/null 2>&1; then
    FILECHECKSUM=`shasum -a 256 extern/cudd-$CUDDVER.tar.gz| sed 's/ .*//'`
else
    echo "neither `sha256sum` nor `shasum` found in the PATH."
    rm extern/cudd-$CUDDVER.tar.gz
    exit 1
fi

if [ "$SHA256SUM" = "$FILECHECKSUM" ]
then
    cd extern
    tar -xzf cudd-$CUDDVER.tar.gz
else
    echo "Fetched file ($URI) has unexpected SHA checksum."
    false
fi
echo "Successfully fetched CUDD; ready to build!"

