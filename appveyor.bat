setlocal

IF %language%==cpp GOTO build_cpp
IF %language%==csharp GOTO build_csharp
IF %language%==php GOTO build_php

echo Unsupported language %language%. Exiting.
goto :error

:build_cpp_internal
pushd %working%
mkdir %build_msvc%
cd %build_msvc%
cmake -G "%generator%" -Dprotobuf_BUILD_SHARED_LIBS=%BUILD_DLL% -Dprotobuf_UNICODE=%UNICODE% %working%/cmake
msbuild protobuf.sln /p:Platform=%vcplatform% /logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll" || goto error
set PATH=%working%\%build_msvc%\%configuration%;%PATH%
popd
EXIT /b

:build_cpp
echo Building C++
REM CALL :build_cpp_internal
REM cd %build_msvc%\%configuration%
REM tests.exe || goto error
goto :EOF

:build_csharp
echo Building C#
REM cd csharp\src
REM REM The platform environment variable is implicitly used by msbuild;
REM REM we don't want it.
REM set platform=
REM dotnet restore
REM dotnet build -c %configuration% || goto error

REM echo Testing C#
REM dotnet run -c %configuration% -f netcoreapp1.0 -p Google.Protobuf.Test\Google.Protobuf.Test.csproj || goto error
REM dotnet run -c %configuration% -f net451 -p Google.Protobuf.Test\Google.Protobuf.Test.csproj || goto error

goto :EOF

:build_php
echo Building PHP

call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

REM Install PHP SDK
curl -L -o php-sdk.zip https://github.com/OSTC/php-sdk-binary-tools/archive/php-sdk-2.0.9.zip
7z x php-sdk.zip
del /Q php-sdk.zip
rename php-sdk-binary-tools-php-sdk-2.0.9 php-sdk
dir

REM Install PHP Binary
mkdir php71-nts
pushd php71-nts
curl -L -o php-windows.zip http://windows.php.net/downloads/releases/archives/php-7.1.8-nts-Win32-VC14-x64.zip
7z x php-windows.zip
del /Q php-windows.zip
popd
set OLDPATH=%PATH%
SET PATH=%working%\php71-nts;%OLDPATH%

REM Setting Environment
SET PHP_SDK_VC=vc14
SET PHP_SDK_BIN_PATH=%working%\php-sdk\bin
for %%a in ("%PHP_SDK_BIN_PATH%") do SET PHP_SDK_ROOT_PATH=%%~dpa
SET PHP_SDK_ROOT_PATH=%PHP_SDK_ROOT_PATH:~0,-1%
SET PHP_SDK_MSYS2_PATH=%PHP_SDK_ROOT_PATH%\msys2\usr\bin
SET PHP_SDK_PHP_CMD=%PHP_SDK_BIN_PATH%\php\do_php.bat
SET PATH=%PHP_SDK_BIN_PATH%;%PHP_SDK_MSYS2_PATH%;%PATH%
pushd php-sdk
CMD /c bin\phpsdk_buildtree.bat phpdev
dir

echo "Download PHP source"
REM Download PHP source
curl -L -o php-src.zip https://github.com/php/php-src/archive/php-7.1.8.zip || goto :error
7z x php-src.zip
del /Q php-src.zip
rename php-src-php-7.1.8 php-7.1.8
move php-7.1.8 phpdev\vc14\x64\php-7.1.8
dir
pushd phpdev\vc14\x64\php-7.1.8
dir
REM cmd /c phpsdk_deps --update --branch 7.1 || goto :error

REM Build Extension
echo %cd%
MKDIR ..\pecl\protobuf
XCOPY %working%\php\ext\google\protobuf ..\pecl\protobuf
echo "Compile Start"
CMD /c buildconf
cmd /c configure --disable-all --enable-cli --enable-protobuf=shared --disable-zts || goto :error
nmake || goto :error


goto :EOF

REM Build protoc
CALL :build_cpp_internal

cd php\tests
mkdir generated
dir
protoc --php_out=generated -I%working%\src -I. ^
    proto\test.proto                           ^
    proto\test_include.proto                   ^
    proto\test_no_namespace.proto              ^
    proto\test_prefix.proto                    ^
    proto\test_php_namespace.proto             ^
    proto\test_empty_php_namespace.proto       ^
    proto\test_reserved_enum_lower.proto       ^
    proto\test_reserved_enum_upper.proto       ^
    proto\test_reserved_enum_value_lower.proto ^
    proto\test_reserved_enum_value_upper.proto ^
    proto\test_reserved_message_lower.proto    ^
    proto\test_reserved_message_upper.proto    ^
    proto\test_service.proto                   ^
    proto\test_service_namespace.proto         ^
    proto\test_descriptors.proto               ^
    proto\test_import_descriptor_proto.proto


goto :EOF

:error
echo Failed!
EXIT /b %ERRORLEVEL%
