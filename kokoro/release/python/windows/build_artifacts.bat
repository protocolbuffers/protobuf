set REPO_DIR=protobuf
set PACKAGE_NAME=protobuf
set BUILD_COMMIT=v3.6.1
set BUILD_VERSION=3.6.1
set BUILD_DLL=OFF
set UNICODE=ON
set PB_TEST_DEP="six==1.9"
set OTHER_TEST_DEP="setuptools==38.5.1"
set OLD_PATH=%PATH%

REM Fetch multibuild
git clone https://github.com/matthew-brett/multibuild.git

REM Fix MSVC builds for 64-bit Python. See:
REM http://stackoverflow.com/questions/32091593/cannot-install-windows-sdk-7-1-on-windows-10
echo "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64 > "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64/vcvars64.bat"

REM Fix MSVC builds for 64-bit Python2.7. See:
REM https://help.appveyor.com/discussions/kb/38-visual-studio-2008-64-bit-builds
curl -L -o vs2008_patch.zip https://github.com/menpo/condaci/raw/master/vs2008_patch.zip
7z x vs2008_patch.zip -ovs2008_patch
cd vs2008_patch
CALL setup_x64.bat
dir "C:\Program Files (x86)\"
copy "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin\vcvars64.bat" "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin\amd64\vcvarsamd64.bat"
cd ..

REM Install zlib
mkdir zlib
curl -L -o zlib.zip http://www.winimage.com/zLibDll/zlib123dll.zip
curl -L -o zlib-src.zip http://www.winimage.com/zLibDll/zlib123.zip
7z x zlib.zip -ozlib
7z x zlib-src.zip -ozlib\include
SET ZLIB_ROOT=%cd%\zlib
del /Q zlib.zip
del /Q zlib-src.zip

REM Build wheel

SET PYTHON=C:\Python35
SET PYTHON_VERSION=3.5
SET PYTHON_ARCH=32
CALL build_wheel.bat

SET PYTHON=C:\Python35-x64
SET PYTHON_VERSION=3.5
SET PYTHON_ARCH=64
CALL build_wheel.bat

SET PYTHON=C:\Python36
SET PYTHON_VERSION=3.6
SET PYTHON_ARCH=32
CALL build_wheel.bat

SET PYTHON=C:\Python36-x64
SET PYTHON_VERSION=3.6
SET PYTHON_ARCH=64
CALL build_wheel.bat
