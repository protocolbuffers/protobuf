set REPO_DIR=protobuf
set PACKAGE_NAME=protobuf
set BUILD_COMMIT=v3.6.1
set BUILD_VERSION=3.6.1
set BUILD_DLL=OFF
set UNICODE=ON
set PB_TEST_DEP="six==1.9"
set OTHER_TEST_DEP="setuptools==38.5.1"
set OLD_PATH=C:\Program Files (x86)\MSBuild\14.0\bin\;%PATH%

dir "C:\Python35"
dir "C:\Python35-x64"
dir "C:\Program Files"
dir "C:\Program Files (x86)"

REM REM Move scripts to root
REM cd github\protobuf
REM copy kokoro\release\python\windows\build_wheel.bat build_wheel.bat
REM copy kokoro\release\python\windows\build_python_env.bat build_python_env.bat
REM copy kokoro\release\python\windows\build.bat build.bat
REM 
REM REM Fetch multibuild
REM git clone https://github.com/matthew-brett/multibuild.git
REM 
REM REM Install zlib
REM mkdir zlib
REM curl -L -o zlib.zip http://www.winimage.com/zLibDll/zlib123dll.zip
REM curl -L -o zlib-src.zip http://www.winimage.com/zLibDll/zlib123.zip
REM 7z x zlib.zip -ozlib
REM 7z x zlib-src.zip -ozlib\include
REM SET ZLIB_ROOT=%cd%\zlib
REM del /Q zlib.zip
REM del /Q zlib-src.zip
REM 
REM REM Build wheel
REM 
REM SET PYTHON=C:\Python35
REM SET PYTHON_VERSION=3.5
REM SET PYTHON_ARCH=32
REM CALL build_wheel.bat
REM 
REM SET PYTHON=C:\Python35-x64
REM SET PYTHON_VERSION=3.5
REM SET PYTHON_ARCH=64
REM CALL build_wheel.bat
REM 
REM SET PYTHON=C:\Python36
REM SET PYTHON_VERSION=3.6
REM SET PYTHON_ARCH=32
REM CALL build_wheel.bat
REM 
REM SET PYTHON=C:\Python36-x64
REM SET PYTHON_VERSION=3.6
REM SET PYTHON_ARCH=64
REM CALL build_wheel.bat
