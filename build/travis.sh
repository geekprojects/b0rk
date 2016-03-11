#!/bin/bash

git clone https://github.com/geekprojects/libgeek.git
cd libgeek
./autogen.sh
./configure
make

export CC=`which $CC`
export CXX=`which $CXX`
sudo make install

