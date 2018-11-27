REM Move scripts to root
cd github\protobuf
copy kokoro\release\python\windows\build_single_artifact.bat build_single_artifact.bat

REM Set environment variables
set REPO_DIR=protobuf
set PACKAGE_NAME=protobuf
set BUILD_DLL=OFF
set UNICODE=ON
set PB_TEST_DEP="six==1.9"
set OTHER_TEST_DEP="setuptools==38.5.1"
set OLD_PATH=C:\Program Files (x86)\MSBuild\14.0\bin\;%PATH%
for /f "tokens=*" %%i in ( 'grep -i "version" python/google/protobuf/__init__.py ^| grep -o "'.*'"' ) do set BUILD_VERSION=%%i
set BUILD_COMMIT=v%BUILD_VERSION:'=%

REM Fetch multibuild
git clone https://github.com/matthew-brett/multibuild.git

REM Install zlib
mkdir zlib
curl -L -o zlib.zip http://www.winimage.com/zLibDll/zlib123dll.zip
curl -L -o zlib-src.zip http://www.winimage.com/zLibDll/zlib123.zip
7z x zlib.zip -ozlib
7z x zlib-src.zip -ozlib\include
SET ZLIB_ROOT=%cd%\zlib
del /Q zlib.zip
del /Q zlib-src.zip

REM Create directory for artifacts
SET ARTIFACT_DIR=%cd%\artifacts
mkdir %ARTIFACT_DIR%

REM Build wheel

SET PYTHON=C:\python35_32bit
SET PYTHON_VERSION=3.5
SET PYTHON_ARCH=32
CALL build_single_artifact.bat

SET PYTHON=C:\python35
SET PYTHON_VERSION=3.5
SET PYTHON_ARCH=64
CALL build_single_artifact.bat

SET PYTHON=C:\python36_32bit
SET PYTHON_VERSION=3.6
SET PYTHON_ARCH=32
CALL build_single_artifact.bat

SET PYTHON=C:\python36
SET PYTHON_VERSION=3.6
SET PYTHON_ARCH=64
CALL build_single_artifact.bat

SET PYTHON=C:\python37_32bit
SET PYTHON_VERSION=3.7
SET PYTHON_ARCH=32
CALL build_single_artifact.bat

SET PYTHON=C:\python37
SET PYTHON_VERSION=3.7
SET PYTHON_ARCH=64
CALL build_single_artifact.bat
