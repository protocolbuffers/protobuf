setlocal

IF %language%==cpp GOTO build_cpp
IF %language%==csharp GOTO build_csharp

echo Unsupported language %language%. Exiting.
goto :error

:build_cpp
echo Building C++
mkdir build_msvc
cd build_msvc
cmake -G "%generator%" -Dprotobuf_BUILD_SHARED_LIBS=%BUILD_DLL% ../cmake
msbuild protobuf.sln /p:Platform=%vcplatform% /logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll" || goto error
cd %configuration%
tests.exe || goto error
goto :EOF

:build_csharp
echo Building C#
cd csharp\src
dotnet restore
dotnet build -c %configuration% Google.Protobuf Google.Protobuf.Test Google.Protobuf.Conformance || goto error

echo Testing C#
dotnet test -c %configuration% Google.Protobuf.Test || goto error

goto :EOF

:error
echo Failed!
EXIT /b %ERRORLEVEL%
