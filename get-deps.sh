#!/bin/sh -e
#
# Fetch dependencies, and place them in location that Makefile expects

CUDDVER=2.5.0
SHA256SUM=4f3bc49b35564af94b17135d8cb8c4063fb049cfaa442f80dc40ba73c6345a85
URI=ftp://vlsi.colorado.edu/pub/cudd-$CUDDVER.tar.gz

mkdir -p extern
curl $URI > extern/cudd-$CUDDVER.tar.gz

FILECHECKSUM=`sha256sum extern/cudd-$CUDDVER.tar.gz| sed 's/ .*//'`
if [ "$SHA256SUM" = "$FILECHECKSUM" ]
then
    cd extern
    tar -xzf cudd-$CUDDVER.tar.gz
else
    echo "Fetched file ($URI) has unexpected SHA checksum."
    false
fi
echo "Successfully fetched CUDD; ready to build!"
