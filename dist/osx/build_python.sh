#!/bin/bash
set -ev
ROOT=`pwd`
git clone https://github.com/python/cpython.git --branch=3.8 --depth=1
cd cpython
./configure --enable-framework=$HOME/Library/Frameworks --with-pydebug --with-openssl=$(brew --prefix openssl) --prefix=$ROOT/pyroot
make -s
make install > /dev/null
