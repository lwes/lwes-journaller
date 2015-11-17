#!/bin/sh

set -e

depdir="`pwd`/deps"
mkdir -p $depdir
srcdir="$depdir/source"
mkdir -p $srcdir

# install framewerk
cd $srcdir;
wget https://github.com/dukesoferl/fw/archive/rpm-0_9_2-noarch.tar.gz -O fw-0.9.2.tar.gz;
tar -xzvf fw-0.9.2.tar.gz;
cd fw-rpm-0_9_2-noarch && ./bootstrap && ./configure --prefix=$depdir && make && make install;

# make sure it's in the path
PATH=$PATH:$depdir/bin

# install fw-template-c
cd $srcdir;
wget https://github.com/dukesoferl/fw-template-C/archive/rpm-0_0_11-noarch.tar.gz -O fw-template-C-0.0.11.tar.gz;
tar -xzvf fw-template-C-0.0.11.tar.gz;
cd fw-template-C-rpm-0_0_11-noarch && ./bootstrap && ./configure --prefix=$depdir && make && make install;

# install lwes
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$depdir/lib/pkgconfig
cd $srcdir;
wget https://github.com/lwes/lwes/archive/lwes-0.24.0.tar.gz -O lwes-0.24.0.tar.gz;
tar -xzvf lwes-0.24.0.tar.gz;
cd lwes-lwes-0.24.0 && ./bootstrap && ./configure --prefix=$depdir && make install;

# install mondemand
export CFLAGS="$CFLAGS `pkg-config --cflags lwes-0`"
export LIBS="$LIBS `pkg-config --libs lwes-0`"
cd $srcdir;
wget https://github.com/mondemand/mondemand/archive/rpm-4_2_1-x86_64.tar.gz -O mondemand-4.2.1.tar.gz;
tar -xzvf mondemand-4.2.1.tar.gz;
cd mondemand-rpm-4_2_1-x86_64 && ./bootstrap && ./configure --prefix=$depdir && make install;
