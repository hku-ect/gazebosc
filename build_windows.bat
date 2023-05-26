set PROJECT_ROOT=%cd%

set GZB_BUILDDIR=%PROJECT_ROOT%/build

set INSTALL_PREFIX=%cd%/tmp/ci_build
set LIBZMQ_SOURCEDIR=%cd%/remote/projects/libzmq
set LIBZMQ_BUILDDIR=%LIBZMQ_SOURCEDIR%/build

set PYTHON_SOURCEDIR=C:\projects\cpython
git clone https://github.com/python/cpython.git --branch=3.11 --depth=1 "%PYTHON_SOURCEDIR%"

set Python3_ROOT_DIR=%GZB_BUILDDIR%/python

python --version
set PY_INSTALLDIR=%GZB_BUILDDIR%/python
set PY_EXE=python.exe
set PY_LAYOUTOPTS=--include-dev --include-pip
md "%PY_INSTALLDIR%"
cd "%PYTHON_SOURCEDIR%\PCbuild"
start /wait build.bat -p x64      
start /wait build.bat -p x64 -c Debug
cd amd64
.\%PY_EXE% ../../PC/layout -b . -s %PYTHON_SOURCEDIR% %PY_LAYOUTOPTS% --copy %PY_INSTALLDIR%
set PY_LAYOUTOPTS=-d --include-dev --include-pip --include-symbols
.\%PY_EXE% ../../PC/layout -b . -s %PYTHON_SOURCEDIR% %PY_LAYOUTOPTS% --copy %PY_INSTALLDIR%

cd %PROJECT_ROOT%
git clone --depth 1 --quiet https://github.com/zeromq/libzmq.git "%LIBZMQ_SOURCEDIR%"
md "%LIBZMQ_BUILDDIR%"
cd "%LIBZMQ_BUILDDIR%"
cmake .. -DBUILD_STATIC=OFF -DBUILD_SHARED=ON -DZMQ_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%"
cmake --build . --config Debug --target install

cd %PROJECT_ROOT%
git submodule update --init --recursive
md "%GZB_BUILDDIR%"
cd "%GZB_BUILDDIR%"
cmake .. -DCMAKE_PREFIX_PATH="%cd%/tmp/ci_build" -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" -DCMAKE_IGNORE_PATH="C:/tmp/ci_build" -DPython3_ROOT_DIR="%GZB_BUILDDIR%/python" -DWITH_EMBED_PYTHON=ON -DWITH_DEV=OFF

md "%GZB_BUILDDIR%\bin\Debug\"
md "%GZB_BUILDDIR%\bin\Release\"

xcopy "%PY_INSTALLDIR%" "%GZB_BUILDDIR%\bin\Debug\python\" /E /Y
xcopy "%PROJECT_ROOT%\misc" "%GZB_BUILDDIR%\bin\Debug\misc\" /E /Y
copy "%PY_INSTALLDIR%\*.dll" "%GZB_BUILDDIR%\bin\Debug\*.dll"
copy "%LIBZMQ_BUILDDIR%\bin\Debug\*.dll" "%GZB_BUILDDIR%\bin\Debug\*.dll"
copy "%PROJECT_ROOT%\tmp\ci_build\*.dll" "%GZB_BUILDDIR%\bin\Debug\*.dll"

xcopy "%PY_INSTALLDIR%" "%GZB_BUILDDIR%\bin\Release\python\" /E /Y
xcopy "%PROJECT_ROOT%\misc" "%GZB_BUILDDIR%\bin\Release\misc\" /E /Y
copy "%PY_INSTALLDIR%\*.dll" "%GZB_BUILDDIR%\bin\Release\*.dll"
copy "%LIBZMQ_BUILDDIR%\bin\Debug\*.dll" "%GZB_BUILDDIR%\bin\Release\*.dll"
copy "%PROJECT_ROOT%\tmp\ci_build\*.dll" "%GZB_BUILDDIR%\bin\Release\*.dll"

cd %PROJECT_ROOT%