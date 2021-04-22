#!/bin/bash
set -e

get_abs_path() {
  echo "$(cd "$1" && pwd)"
}

SCRIPT_ROOT=$(get_abs_path `dirname $0`)
GIT_ROOT=$(get_abs_path $SCRIPT_ROOT/../..)
#echo $SCRIPT_ROOT
echo "Path to git repo: $GIT_ROOT"

cd $GIT_ROOT
git submodule update --init --recursive
GIT_BRANCH=$(git symbolic-ref --short HEAD)
mkdir build || true
cd build

# libzmq
if [ -e libzmq ]; then
  git pull
else
  git clone --depth=1 --branch=master https://github.com/zeromq/libzmq
fi
cd libzmq
./autogen.sh
./configure
echo "Install libzmq"
sudo make install
cd $GIT_ROOT/build

# cpython
$SCRIPT_ROOT/build_python.sh $GIT_ROOT/build/python
export CMAKE_OPTIONS="-DPython3_ROOT_DIR=$GIT_ROOT/build/python -DWITH_EMBED_PYTHON=ON"
cd $GIT_ROOT/build

# build
cmake ${CMAKE_OPTIONS} ..
make install

# zip it
git fetch --all --tags
cd bin
zip -r gazebosc_osx_${GIT_BRANCH}_$( git describe --tag --always --dirty --abbrev=4).zip * -x "*/test/*" "*/__pycache__/*"
scp *.zip buildbot@pong.hku.nl:public_html/nightly/
