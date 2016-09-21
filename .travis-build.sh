#!/bin/sh

set -e
cwd=`pwd`
depdir="$cwd/deps"
mkdir -p $depdir
srcdir="$depdir/source"
mkdir -p $srcdir
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$depdir/lib/pkgconfig

./bootstrap && ./configure --disable-hardcore --with-mondemand && make && make check
