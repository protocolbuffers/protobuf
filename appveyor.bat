setlocal

IF %platform%==MinGW GOTO build_mingw
IF %language%==cpp GOTO build_cpp
IF %language%==csharp GOTO build_csharp

echo Unsupported language %language% and platform %platform%. Exiting.
goto :error

:build_mingw
echo Building MinGW
set PATH=C:\mingw-w64\x86_64-7.2.0-posix-seh-rt_v5-rev1\mingw64\bin;%PATH:C:\Program Files\Git\usr\bin;=%
mkdir build_mingw
cd build_mingw
cmake -G "%generator%" -Dprotobuf_BUILD_SHARED_LIBS=%BUILD_DLL% -Dprotobuf_UNICODE=%UNICODE% -Dprotobuf_BUILD_TESTS=0 ../cmake
mingw32-make -j8 all || goto error
rem cd %configuration%
rem tests.exe || goto error
goto :EOF

:build_cpp
echo Building C++
mkdir build_msvc
cd build_msvc
cmake -G "%generator%" -Dprotobuf_BUILD_SHARED_LIBS=%BUILD_DLL% -Dprotobuf_UNICODE=%UNICODE% ../cmake
msbuild protobuf.sln /p:Platform=%vcplatform% /logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll" || goto error
cd %configuration%
tests.exe || goto error
goto :EOF

:build_csharp
echo Building C#
cd csharp\src
REM The platform environment variable is implicitly used by msbuild;
REM we don't want it.
set platform=
dotnet restore
dotnet build -c %configuration% || goto error

echo Testing C#
dotnet test -c %configuration% -f net6.0 Google.Protobuf.Test\Google.Protobuf.Test.csproj || goto error
dotnet test -c %configuration% -f net462 Google.Protobuf.Test\Google.Protobuf.Test.csproj || goto error

goto :EOF

:error
echo Failed!
EXIT /b %ERRORLEVEL%
