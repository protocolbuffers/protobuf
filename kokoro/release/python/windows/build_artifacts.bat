REM Move scripts to root
set REPO_DIR_STAGE=%cd%\github\protobuf-stage
xcopy /S  github\protobuf "%REPO_DIR_STAGE%\"
cd github\protobuf
copy kokoro\release\python\windows\build_single_artifact.bat build_single_artifact.bat

REM Set environment variables
set PACKAGE_NAME=protobuf
set REPO_DIR=protobuf
set BUILD_DLL=OFF
set UNICODE=ON
set OTHER_TEST_DEP="setuptools==38.5.1"
set OLD_PATH=C:\Program Files (x86)\MSBuild\14.0\bin\;%PATH%

REM Fetch multibuild
git clone https://github.com/matthew-brett/multibuild.git
REM Pin multibuild scripts at a known commit to avoid potentially unwanted future changes from
REM silently creeping in (see https://github.com/protocolbuffers/protobuf/issues/9180).
REM IMPORTANT: always pin multibuild at the same commit for:
REM - linux/build_artifacts.sh
REM - linux/build_artifacts.sh
REM - windows/build_artifacts.bat
cd multibuild
git checkout b89bb903e94308be79abefa4f436bf123ebb1313
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

REM Create directory for artifacts
SET ARTIFACT_DIR=%cd%\artifacts
mkdir %ARTIFACT_DIR%

REM Build wheel

SET PYTHON=C:\python37_32bit
SET PYTHON_VERSION=3.7
SET PYTHON_ARCH=32
CALL build_single_artifact.bat || goto :error

SET PYTHON=C:\python37
SET PYTHON_VERSION=3.7
SET PYTHON_ARCH=64
CALL build_single_artifact.bat || goto :error

powershell -File kokoro/release/python/windows/install_python_interpreters.ps1

SET PYTHON=C:\python38_32bit
SET PYTHON_VERSION=3.8
SET PYTHON_ARCH=32
CALL build_single_artifact.bat || goto :error

SET PYTHON=C:\python38
SET PYTHON_VERSION=3.8
SET PYTHON_ARCH=64
CALL build_single_artifact.bat || goto :error

SET PYTHON=C:\python39_32bit
SET PYTHON_VERSION=3.9
SET PYTHON_ARCH=32
CALL build_single_artifact.bat || goto :error

SET PYTHON=C:\python39
SET PYTHON_VERSION=3.9
SET PYTHON_ARCH=64
CALL build_single_artifact.bat || goto :error

SET PYTHON=C:\python310_32bit
SET PYTHON_VERSION=3.10
SET PYTHON_ARCH=32
CALL build_single_artifact.bat || goto :error

SET PYTHON=C:\python310
SET PYTHON_VERSION=3.10
SET PYTHON_ARCH=64
CALL build_single_artifact.bat || goto :error

goto :EOF

:error
exit /b %errorlevel%
