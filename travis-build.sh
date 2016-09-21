#!/bin/sh

set -e

cwd=`pwd`
depdir="$cwd/deps"
mkdir -p $depdir
srcdir="$depdir/source"
mkdir -p $srcdir

# install lwes
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$depdir/lib/pkgconfig
cd $srcdir;
wget http://www.lwes.org/downloads/lwes-c/lwes-0.25.0.tar.gz;
tar -xzvf lwes-0.25.0.tar.gz;
cd lwes-0.25.0 && ./configure --prefix=$depdir && make install;

# install mondemand
export CFLAGS="$CFLAGS `pkg-config --cflags lwes-0`"
export LIBS="$LIBS `pkg-config --libs lwes-0`"
cd $srcdir;
wget http://www.mondemand.org/downloads/mondemand-c/mondemand-4.3.1.tar.gz;
tar -xzvf mondemand-4.3.1.tar.gz;
cd mondemand-4.3.1 && ./configure --prefix=$depdir && make install;

cd $cwd
./bootstrap && ./configure --with-mondemand && make && make check
