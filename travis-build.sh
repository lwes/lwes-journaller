#!/bin/sh

set -e

export PATH="$PATH:`pwd`/deps/bin"
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:`pwd`/deps/lib/pkgconfig"
export CFLAGS="$CFLAGS `pkg-config --cflags mondemand-4.0`"
export LIBS="$LIBS `pkg-config --libs mondemand-4.0` -ldl"
export LDFLAGS="$LDFLAGS -Wl,--no-as-needed"
./bootstrap
./configure --with-mondemand && make && make check
