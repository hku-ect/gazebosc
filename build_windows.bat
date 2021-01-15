set PROJECT_ROOT=%cd%

set INSTALL_PREFIX=%cd%/tmp/ci_build
set LIBZMQ_SOURCEDIR=%cd%/remote/projects/libzmq
set LIBZMQ_BUILDDIR=%LIBZMQ_SOURCEDIR%/build
git clone --depth 1 --quiet https://github.com/zeromq/libzmq.git "%LIBZMQ_SOURCEDIR%"
md "%LIBZMQ_BUILDDIR%"

cd "%LIBZMQ_BUILDDIR%"
cmake .. -DBUILD_STATIC=OFF -DBUILD_SHARED=ON -DZMQ_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%"
cmake --build . --config Debug --target install

set GZB_BUILDDIR="%PROJECT_ROOT%\build"
cd %PROJECT_ROOT%
git submodule update --init --recursive
md "%GZB_BUILDDIR%"
cd "%GZB_BUILDDIR%"
cmake .. -DCMAKE_PREFIX_PATH="%cd%/tmp/ci_build" -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%"
cmake --build . --config Debug --target install