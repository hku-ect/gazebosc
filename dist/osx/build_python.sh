#!/bin/bash
set -ev
if [[ -z "$1" ]]
then
    BUILD_DIR=`pwd`/pythonbuild
else
    BUILD_DIR=$1
fi

git clone https://github.com/python/cpython.git --branch=3.8 --depth=1
cd cpython
./configure --with-openssl=$(brew --prefix openssl) --prefix=$BUILD_DIR --enable-shared
make -s
make install > /dev/null
