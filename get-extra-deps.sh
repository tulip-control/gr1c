#!/bin/sh -e
#
# Fetch and build tools that are not required for basic usage of gr1c,
# but that facilitate advanced usage or interesting capabilities of gr1c.

SPINVER=6.5.2
SHA256SUM=e46a3bd308c4cd213cc466a8aaecfd5cedc02241190f3cb9a1d1b87e5f37080a
# N.B., prior releases are available at https://spinroot.com/spin/Archive/
URI=https://github.com/nimble-code/Spin/archive/refs/tags/version-${SPINVER}.tar.gz

mkdir -p extern/src
if [ ! -f extern/spin${SPINVER}.tar.gz ]
then
    curl -f -L -sS -o extern/spin${SPINVER}.tar.gz $URI
fi

if hash sha256sum >/dev/null 2>&1; then
    FILECHECKSUM=$(sha256sum extern/spin${SPINVER}.tar.gz| sed 's/ .*//')
elif hash shasum >/dev/null 2>&1; then
    FILECHECKSUM=$(shasum -a 256 extern/spin${SPINVER}.tar.gz| sed 's/ .*//')
else
    echo 'neither `sha256sum` nor `shasum` found in the PATH.'
    rm extern/spin${SPINVER}.tar.gz
    exit 1
fi

if [ "$SHA256SUM" = "$FILECHECKSUM" ]
then
    cd extern/src
    tar -xzf ../spin${SPINVER}.tar.gz
else
    echo "Fetched file ($URI) has unexpected SHA checksum,"
    echo "expected: $SHA256SUM"
    echo "observed: $FILECHECKSUM"
    false
fi
echo "Successfully fetched Spin"
