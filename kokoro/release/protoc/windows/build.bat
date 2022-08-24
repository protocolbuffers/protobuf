set generator32=Visual Studio 15
set generator64=Visual Studio 15 Win64
set vcplatform32=win32
set vcplatform64=x64
set configuration=Release

:: VS2017 is installed, but will not be selected by default. This command sets
:: up the environment so that CMake will find and use it:
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Building protoc
cd github\protobuf

echo Update Submodules
echo This is needed because this build uses CMake <3.13.
git submodule update --init --recursive
set ABSL_ROOT_DIR=%cd%\third_party\abseil-cpp

mkdir build32
cd build32
cmake -G "%generator32%" -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_UNICODE=ON ..
msbuild protobuf.sln /p:Platform=%vcplatform32% || goto error
cd ..

mkdir build64
cd build64
cmake -G "%generator64%" -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_UNICODE=ON ..
msbuild protobuf.sln /p:Platform=%vcplatform64% || goto error
cd ..

goto :EOF

:error
echo Failed!
exit /b %ERRORLEVEL%
