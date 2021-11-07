#!/bin/bash
set -ev
if [[ -z "$1" ]]
then
    BUILD_DIR=`pwd`/pythonbuild
else
    BUILD_DIR=$1
fi

if [ -e cpython ]
then
  git pull
else
  git clone https://github.com/python/cpython.git --branch=3.8 --depth=1 
fi
cd cpython
./configure --with-openssl=$(brew --prefix openssl) --enable-framework=$BUILD_DIR
make -s
make install > /dev/null
