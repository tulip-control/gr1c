#!/bin/sh -e
#
# Fetch and build tools that are not required for basic usage of gr1c,
# but that facilitate advanced usage or interesting capabilities of gr1c.

SPINVER=645
SHA256SUM=44081282eb63cd9df763ebbcf8bad19dbeefecbebf8ac2cc090ea92e2ab71875
URI=http://spinroot.com/spin/Src/spin${SPINVER}.tar.gz

mkdir -p extern/src
if [ ! -f extern/spin${SPINVER}.tar.gz ]
then
    curl -sS $URI > extern/spin${SPINVER}.tar.gz
fi

if hash sha256sum >/dev/null 2>&1; then
    FILECHECKSUM=`sha256sum extern/spin${SPINVER}.tar.gz| sed 's/ .*//'`
elif hash shasum >/dev/null 2>&1; then
    FILECHECKSUM=`shasum -a 256 extern/spin${SPINVER}.tar.gz| sed 's/ .*//'`
else
    echo "neither `sha256sum` nor `shasum` found in the PATH."
    rm extern/spin${SPINVER}.tar.gz
    exit 1
fi

if [ "$SHA256SUM" = "$FILECHECKSUM" ]
then
    cd extern/src
    tar -xzf ../spin${SPINVER}.tar.gz
else
    echo "Fetched file ($URI) has unexpected SHA checksum."
    false
fi
echo "Successfully fetched Spin"
