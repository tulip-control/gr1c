#!/bin/sh
#
# Build source distribution.  Requires environment variable GR1C_VERSION.

git archive --format=tar --prefix=gr1c-$GR1C_VERSION/ v$GR1C_VERSION | gzip -9 > gr1c-$GR1C_VERSION.tar.gz
