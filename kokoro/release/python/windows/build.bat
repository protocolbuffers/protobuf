setlocal

REM Checkout release commit
cd %REPO_DIR%
git checkout %BUILD_COMMIT%

REM ======================
REM Build Protobuf Library
REM ======================

mkdir src\.libs

mkdir vcprojects
pushd vcprojects
cmake -G "%generator%" -Dprotobuf_BUILD_SHARED_LIBS=%BUILD_DLL% -Dprotobuf_UNICODE=%UNICODE% -Dprotobuf_BUILD_TESTS=OFF ../cmake
msbuild protobuf.sln /p:Platform=%vcplatform% /p:Configuration=Release
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
cd ..\..
