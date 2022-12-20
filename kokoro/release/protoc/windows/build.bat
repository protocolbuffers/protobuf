set generator32=Visual Studio 15
set generator64=Visual Studio 15 Win64
set vcplatform32=win32
set vcplatform64=x64
set configuration=Release

echo Building protoc
cd github\protobuf

call kokoro\windows\prepare_build_win64.bat || goto :error

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
