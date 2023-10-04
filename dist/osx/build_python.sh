#!/bin/bash
set -ev
if [[ -z "$1" ]]
then
    BUILD_DIR=`pwd`/pythonbuild
else
    BUILD_DIR=$1
fi

if [[ -z "$2" ]]
then
    PY_VER=3.9
else
    PY_VER=$2
fi


#if [ true ]
#then
#    git clone https://github.com/gregneagle/relocatable-python.git
#    cd relocatable-python
#    ./make_relocatable_python_framework.py --python-version $PY_VER --os-version 11 --destination $BUILD_DIR
#    exit $?
#fi

if [ -e cpython ]
then
  cd cpython
  git pull
else
  git clone https://github.com/python/cpython.git --branch=$PY_VER --depth=1
  cd cpython
fi
brew remove --ignore-dependencies gettext  # Fix https://bugs.python.org/issue46975
./configure --with-openssl=$(brew --prefix --installed openssl@1.1) --prefix=$BUILD_DIR --enable-optimizations
make -s
make altinstall > /dev/null
# we might need to fix Python rpath
#install_name_tool -change $BUILD_DIR/Python.framework/Versions/Current/Python @executable_path/../../Python python3
