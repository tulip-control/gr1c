#!/bin/sh -e
#
# Fetch dependencies, and place them in location that Makefile expects

CUDDVER=2.5.0
SHA1SUM=7d0d8b4b03f5c1819fe77a82f3b947421a72d629
URI=ftp://vlsi.colorado.edu/pub/cudd-$CUDDVER.tar.gz

mkdir -p extern
curl $URI > extern/cudd-$CUDDVER.tar.gz

FILECHECKSUM=`sha1sum extern/cudd-$CUDDVER.tar.gz| sed 's/ .*//'`
if [ "$SHA1SUM" = "$FILECHECKSUM" ]
then
    cd extern
    tar -xzf cudd-$CUDDVER.tar.gz
else
    echo "Fetched file ($URI) has unexpected SHA-1 checksum."
    false
fi
echo "Successfully fetched CUDD; ready to build!"
