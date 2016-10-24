#!/bin/sh

set -e
cwd=`pwd`
depdir="$cwd/deps"
mkdir -p $depdir
srcdir="$depdir/source"
mkdir -p $srcdir
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$depdir/lib/pkgconfig

LWES_VERSION=1.0.0
MONDEMAND_VERSION=4.4.2

# install lwes
cd $srcdir
wget https://github.com/lwes/lwes/releases/download/${LWES_VERSION}/lwes-${LWES_VERSION}.tar.gz
tar -xzvf lwes-${LWES_VERSION}.tar.gz
cd lwes-${LWES_VERSION} \
  && ./configure --disable-hardcore --prefix=$depdir \
  && make install

# install mondemand
cd $srcdir;
wget https://github.com/mondemand/mondemand/releases/download/${MONDEMAND_VERSION}/mondemand-${MONDEMAND_VERSION}.tar.gz
tar -xzvf mondemand-${MONDEMAND_VERSION}.tar.gz;
cd mondemand-${MONDEMAND_VERSION} \
  && ./configure --disable-hardcore --prefix=$depdir \
  && make install
