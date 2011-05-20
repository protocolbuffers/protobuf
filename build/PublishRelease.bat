REM @ECHO OFF
@IF "%1" == "version" @GOTO VERSION
@IF "%1" == "build" @GOTO BUILD
@IF "%1" == "fpush" @GOTO FILEPUSH
@IF "%1" == "push" @GOTO PUSH
@GOTO HELP

:VERSION
hg log -l 1 --template "Revision: {rev}" > %~dp0\..\build_temp\revision.txt
CMD.exe /Q /C "CD %~dp0\.. && lib\StampVersion.exe /major:2 /minor:3 /build:0 /revision:build_temp\revision.txt"
@TYPE src\ProtocolBuffers\Properties\AssemblyInfo.cs | FIND "AssemblyFileVersion"
@ECHO.
@ECHO NEXT: Use the above version number to run "%0 build {Version}"
@ECHO.
@GOTO EXIT

:BUILD
@IF "%2" == "" @GOTO HELP
IF EXIST "C:\Program Files\Microsoft SDKs\Windows\v7.0\Bin\sn.exe" SET WIN7SDK_DIR=C:\Program Files\Microsoft SDKs\Windows\v7.0\Bin\

IF NOT EXIST "%~dp0\..\release-key" hg clone https://bitbucket.org/rknapp/protobuf-csharp-port-keyfile %~dp0\..\release-key
SET PROTOBUF_KEY_FILE="/p:AssemblyOriginatorKeyFile=%~dp0\..\release-key\Google.ProtocolBuffers.snk"

MD "%~dp0\%2"
CMD.exe /Q /C "CD %~dp0 && GenerateCompletePackage.bat"
COPY /y %~dp0\..\build_output\AllBinariesAndSource.zip %~dp0\%2\protobuf-csharp-port-%2-full-binaries.zip
CMD.exe /Q /C "CD %~dp0 && GenerateReleasePackage.bat"
COPY /y %~dp0\..\build_output\ReleaseBinaries.zip %~dp0\%2\protobuf-csharp-port-%2-release-binaries.zip
hg archive  %~dp0\%2\protobuf-csharp-port-%2-source.zip

SET PROTOBUF_KEY_FILE=
"%WIN7SDK_DIR%sn.exe" -T build_output\Package\Release\Google.ProtocolBuffers.dll
@ECHO.
@ECHO ***********************************************************
@ECHO IMPORTANT: Verify the above key output is: 55f7125234beb589
@ECHO ***********************************************************
@ECHO.
@ECHO NEXT: Verify the output in %~dp0\%2 and then run "%0 push %2 {google-code-user} {google-code-password}"
@ECHO.
@GOTO EXIT

:PUSH
@IF "%2" == "" @GOTO HELP
@IF "%3" == "" @GOTO HELP
@IF "%4" == "" @GOTO HELP

hg commit -m "version %2"
hg tag %2
hg push

:FILEPUSH
SET GOOGLEUPLOAD=python.exe %~dp0\googlecode_upload.py --project protobuf-csharp-port --user "%3" --password "%4"

%GOOGLEUPLOAD% --labels Type-Source,Featured --summary "Version %2 source" %~dp0\%2\protobuf-csharp-port-%2-source.zip
%GOOGLEUPLOAD% --labels Type-Executable,Featured --summary "Version %2 binaries (all configurations)" %~dp0\%2\protobuf-csharp-port-%2-full-binaries.zip
%GOOGLEUPLOAD% --labels Type-Executable,Featured --summary "Version %2 binaries (release only)" %~dp0\%2\protobuf-csharp-port-%2-release-binaries.zip

@SET GOOGLEUPLOAD=
@GOTO EXIT

:HELP
@ECHO.
@ECHO Available commands, run in the following order:
@ECHO 1. %0 version
@ECHO 2. %0 build {version from step 1}
@ECHO 3. %0 push {version from step 1} {google-code-user} {google-code-password}
@ECHO.
@GOTO EXIT

:EXIT