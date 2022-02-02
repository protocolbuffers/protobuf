setlocal

if %PYTHON%==C:\python36_32bit set generator=Visual Studio 14
if %PYTHON%==C:\python36_32bit set vcplatform=Win32

if %PYTHON%==C:\python36 set generator=Visual Studio 14 Win64
if %PYTHON%==C:\python36 set vcplatform=x64

if %PYTHON%==C:\python37_32bit set generator=Visual Studio 14
if %PYTHON%==C:\python37_32bit set vcplatform=Win32

if %PYTHON%==C:\python37 set generator=Visual Studio 14 Win64
if %PYTHON%==C:\python37 set vcplatform=x64

if %PYTHON%==C:\python38_32bit set generator=Visual Studio 14
if %PYTHON%==C:\python38_32bit set vcplatform=Win32

if %PYTHON%==C:\python38 set generator=Visual Studio 14 Win64
if %PYTHON%==C:\python38 set vcplatform=x64

if %PYTHON%==C:\python39_32bit set generator=Visual Studio 14
if %PYTHON%==C:\python39_32bit set vcplatform=Win32

if %PYTHON%==C:\python39 set generator=Visual Studio 14 Win64
if %PYTHON%==C:\python39 set vcplatform=x64

if %PYTHON%==C:\python310_32bit set generator=Visual Studio 14
if %PYTHON%==C:\python310_32bit set vcplatform=Win32

if %PYTHON%==C:\python310 set generator=Visual Studio 14 Win64
if %PYTHON%==C:\python310 set vcplatform=x64

REM Prepend newly installed Python to the PATH of this build (this cannot be
REM done from inside the powershell script as it would require to restart
REM the parent CMD process).
SET PATH=C:\Program Files\CMake\bin;%PYTHON%;%PYTHON%\Scripts;%OLD_PATH%
python -m pip install -U pip
pip install wheel

REM Check that we have the expected version and architecture for Python
python --version
python -c "import struct; print(struct.calcsize('P') * 8)"

rmdir /s/q %REPO_DIR%
xcopy /s  %REPO_DIR_STAGE% "%REPO_DIR%\"

REM Checkout release commit
cd %REPO_DIR%

REM ======================
REM Build Protobuf Library
REM ======================

mkdir src\.libs

mkdir vcprojects
pushd vcprojects
cmake -G "%generator%" -Dprotobuf_BUILD_SHARED_LIBS=%BUILD_DLL% -Dprotobuf_UNICODE=%UNICODE% -Dprotobuf_BUILD_TESTS=OFF ../cmake || goto :error
msbuild protobuf.sln /p:Platform=%vcplatform% /p:Configuration=Release || goto :error
dir /s /b
popd
copy vcprojects\Release\libprotobuf.lib src\.libs\libprotobuf.a
copy vcprojects\Release\libprotobuf-lite.lib src\.libs\libprotobuf-lite.a
SET PATH=%cd%\vcprojects\Release;%PATH%
dir vcprojects\Release

REM ======================
REM Build python library
REM ======================

cd python

REM sed -i 's/\ extra_compile_args\ =\ \[\]/\ extra_compile_args\ =\ \[\'\/MT\'\]/g' setup.py

python setup.py bdist_wheel --cpp_implementation --compile_static_extension
dir dist
copy dist\* %ARTIFACT_DIR%
dir %ARTIFACT_DIR%
cd ..\..

goto :EOF

:error
exit /b %errorlevel%
