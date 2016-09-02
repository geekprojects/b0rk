#!/bin/bash

export CC=`which $CC`
export CXX=`which $CXX`

git clone https://github.com/geekprojects/libgeek.git
cd libgeek
./autogen.sh
./configure
make

sudo make install
if [ $TRAVIS_OS_NAME == linux ]; then sudo ldconfig; fi

