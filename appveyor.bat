setlocal

IF %language%==cpp GOTO build_cpp
IF %language%==csharp GOTO build_csharp
IF %language%==php GOTO build_php

echo Unsupported language %language%. Exiting.
goto :error

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
dotnet run -c %configuration% -f netcoreapp1.0 -p Google.Protobuf.Test\Google.Protobuf.Test.csproj || goto error
dotnet run -c %configuration% -f net451 -p Google.Protobuf.Test\Google.Protobuf.Test.csproj || goto error

goto :EOF


:build_php
echo Preparing environment
echo Downloading PHP SDK
curl -L -o php-sdk.zip https://github.com/OSTC/php-sdk-binary-tools/archive/php-sdk-2.0.9.zip
7z x php-sdk.zip
del /Q php-sdk.zip
ren php-sdk-binary-tools-php-sdk-2.0.9 php-sdk
cd php-sdk
bin\phpsdk_setvars.bat
bin\phpsdk_buildtree.bat phpdev
curl -L -o php-src.zip https://github.com/php/php-src/archive/php-7.1.8.zip
7z x php-src.zip
del /Q php-src.zip
ren php-src-php-7.1.8 php-7.1.8
move php-7.1.8 phpdev\vc14\x64\php-7.1.8
cd phpdev\vc14\x64\php-7.1.8
set PHP_SDK_VC=vc14
phpsdk_deps --update --branch 7.1
mkdir ..\pecl\protobuf
xcopy ..\..\..\..\..\php\ext\google\protobuf ..\pecl\protobuf

echo Building Protobuf Extension for PHP
buildconf
configure --disable-all --enable-cli --enable-protobuf=shared --disable-zts
nmake || goto error

goto :EOF

echo Builing PHP
goto :EOF

:error
echo Failed!
EXIT /b %ERRORLEVEL%
