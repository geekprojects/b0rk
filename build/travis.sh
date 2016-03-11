#!/bin/bash

git clone https://github.com/geekprojects/libgeek.git
cd libgeek
./autogen.sh
./configure
make
sudo make install

