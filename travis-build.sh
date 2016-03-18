#!/bin/sh

set -e

PATH=$PATH:`pwd`/deps/bin
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:`pwd`/deps/lib/pkgconfig
CFLAGS="$CFLAGS `pkg-config --cflags mondemand-4.0`"
LIBS="$LIBS `pkg-config --libs mondemand-4.0` -ldl"
LDFLAGS="$LDFLAGS -Wl,--no-as-needed"
./bootstrap
./configure --with-mondemand && make && make check
