REM @ECHO OFF
@PUSHD %~dp0
@IF "%1" == "version" @GOTO VERSION
@IF "%1" == "build" @GOTO BUILD
@IF "%1" == "fpush" @GOTO FILEPUSH
@IF "%1" == "nupush" @GOTO NUGETPUSH
@IF "%1" == "push" @GOTO PUSH
@GOTO HELP

:VERSION
IF NOT EXIST "..\build_temp\" MD "..\build_temp\"
hg log -l 1 --template "Revision: {rev}" > ..\build_temp\revision.txt
CMD.exe /Q /C "CD .. && lib\StampVersion.exe /major:2 /minor:4 /build:1 /revision:build_temp\revision.txt"
@TYPE ..\src\ProtocolBuffers\Properties\AssemblyInfo.cs | FIND "AssemblyFileVersion"
@ECHO.
@ECHO NEXT: Use the above version number to run "%0 build {Version}"
@ECHO.
@GOTO EXIT

:BUILD
@IF "%2" == "" @GOTO HELP
IF EXIST "C:\Program Files\Microsoft SDKs\Windows\v7.0\Bin\sn.exe" SET WIN7SDK_DIR=C:\Program Files\Microsoft SDKs\Windows\v7.0\Bin\

IF NOT EXIST "..\release-key" hg clone https://bitbucket.org/rknapp/protobuf-csharp-port-keyfile ..\release-key

MD "%2"
CMD.exe /Q /C "BuildAll.bat /verbosity:minimal "/p:AssemblyOriginatorKeyFile=%~dp0..\release-key\Google.ProtocolBuffers.snk"

COPY /y ..\build_output\Release-v2.0.zip %2\protobuf-csharp-port-%2-net20-release-binaries.zip
COPY /y ..\build_output\Release-v3.5.zip %2\protobuf-csharp-port-%2-net35-release-binaries.zip
COPY /y ..\build_output\Release-v4.0.zip %2\protobuf-csharp-port-%2-net40-release-binaries.zip

COPY /y ..\build_output\Full-v2.0.zip %2\protobuf-csharp-port-%2-net20-full-binaries.zip
COPY /y ..\build_output\Full-v3.5.zip %2\protobuf-csharp-port-%2-net35-full-binaries.zip
COPY /y ..\build_output\Full-v4.0.zip %2\protobuf-csharp-port-%2-net40-full-binaries.zip

..\lib\NuGet.exe pack Google.ProtocolBuffers.nuspec -Symbols -Version %2 -NoPackageAnalysis -OutputDirectory %2
..\lib\NuGet.exe pack Google.ProtocolBuffersLite.nuspec -Symbols -Version %2 -NoPackageAnalysis -OutputDirectory %2

hg archive %2\protobuf-csharp-port-%2-source.zip

"%WIN7SDK_DIR%sn.exe" -T ..\build_output\v2.0\Release\Google.ProtocolBuffers.dll
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
@ECHO.
@ECHO NEXT: Verify the repository state and run "%0 fpush %2 {google-code-user} {google-code-password}"
@ECHO.
@GOTO EXIT

:FILEPUSH
SET GOOGLEUPLOAD=python.exe googlecode_upload.py --project protobuf-csharp-port --user "%3" --password "%4"

%GOOGLEUPLOAD% --labels Type-Source,Featured --summary "Version %2 source" %2\protobuf-csharp-port-%2-source.zip
%GOOGLEUPLOAD% --labels Type-Executable,Featured --summary "Version %2 binaries for .NET 2.0 (all configurations)" %2\protobuf-csharp-port-%2-net20-full-binaries.zip
%GOOGLEUPLOAD% --labels Type-Executable,Featured --summary "Version %2 binaries for .NET 3.5 (all configurations)" %2\protobuf-csharp-port-%2-net35-full-binaries.zip
%GOOGLEUPLOAD% --labels Type-Executable,Featured --summary "Version %2 binaries for .NET 4.0 (all configurations)" %2\protobuf-csharp-port-%2-net40-full-binaries.zip
%GOOGLEUPLOAD% --labels Type-Executable,Featured --summary "Version %2 binaries for .NET 2.0 (release only)" %2\protobuf-csharp-port-%2-net20-release-binaries.zip
%GOOGLEUPLOAD% --labels Type-Executable,Featured --summary "Version %2 binaries for .NET 3.5 (release only)" %2\protobuf-csharp-port-%2-net35-release-binaries.zip
%GOOGLEUPLOAD% --labels Type-Executable,Featured --summary "Version %2 binaries for .NET 4.0 (release only)" %2\protobuf-csharp-port-%2-net40-release-binaries.zip

@SET GOOGLEUPLOAD=
@ECHO.
@ECHO NEXT: Verify the uploads and run "%0 nupush %2"
@ECHO.
@GOTO EXIT

:NUGETPUSH

..\lib\NuGet.exe push "%2\Google.ProtocolBuffers.%2.nupkg"
..\lib\NuGet.exe push "%2\Google.ProtocolBuffersLite.%2.nupkg"
..\lib\NuGet.exe push "%2\Google.ProtocolBuffers.%2.symbols.nupkg"
..\lib\NuGet.exe push "%2\Google.ProtocolBuffersLite.%2.symbols.nupkg"

@ECHO.
@ECHO NEXT: Verify the nuget packages at http://nuget.org
@ECHO.
@GOTO EXIT

:HELP
@ECHO.
@ECHO Available commands, run in the following order:
@ECHO 1. %0 version
@ECHO 2. %0 build {version from step 1}
@ECHO 3. %0 push {version from step 1} {google-code-user} {google-code-password}
@ECHO 4. %0 fpush {version from step 1} {google-code-user} {google-code-password}
@ECHO 5. %0 nupush {version from step 1}
@ECHO.
@GOTO EXIT

:EXIT
@POPD