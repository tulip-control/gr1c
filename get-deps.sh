#!/bin/sh -e
#
# Fetch dependencies, and place them in location that build-deps.sh expects

CUDDVER=3.0.0
SHA256SUM=b8e966b4562c96a03e7fbea239729587d7b395d53cadcc39a7203b49cf7eeb69
URI=https://sourceforge.net/projects/cudd-mirror/files/cudd-$CUDDVER.tar.gz/download
#URI=ftp://vlsi.colorado.edu/pub/cudd-$CUDDVER.tar.gz

mkdir -p extern/src
if [ ! -f extern/cudd-$CUDDVER.tar.gz ]
then
    curl -L -sS $URI > extern/cudd-$CUDDVER.tar.gz
fi

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
    cd extern/src
    tar -xzf ../cudd-$CUDDVER.tar.gz
else
    echo "Fetched file ($URI) has unexpected SHA checksum."
    false
fi
echo "Successfully fetched CUDD; next run build-deps.sh"
