REM @ECHO OFF
@IF "%1" == "build" GOTO BUILD
@IF "%1" == "push" GOTO PUSH
GOTO HELP

:BUILD
@IF "%2" == "" GOTO HELP

MD "%~dp0\%2"
CMD.exe /Q /C "CD %~dp0 && GenerateCompletePackage.bat"
COPY /y %~dp0\..\build_output\AllBinariesAndSource.zip %~dp0\%2\protobuf-csharp-port-%2-full-binaries.zip
CMD.exe /Q /C "CD %~dp0 && GenerateReleasePackage.bat"
COPY /y %~dp0\..\build_output\ReleaseBinaries.zip %~dp0\%2\protobuf-csharp-port-%2-release-binaries.zip
hg archive  %~dp0\%2\protobuf-csharp-port-%2-source.zip

goto EXIT

:PUSH
@IF "%2" == "" GOTO HELP
@IF "%3" == "" GOTO HELP
@IF "%4" == "" GOTO HELP

hg commit -m "version %2"
hg tag %2
hg push
SET GOOGLEUPLOAD=C:\Python25\python.exe %~dp0\googlecode_upload.py --project protobuf-csharp-port --user "%3" --password "%4"

%GOOGLEUPLOAD% --labels Type-Source,Featured --summary "Version %2 source" %~dp0\%2\protobuf-csharp-port-%2-source.zip
%GOOGLEUPLOAD% --labels Type-Executable,Featured --summary "Version %2 binaries (all configurations)" %~dp0\%2\protobuf-csharp-port-%2-full-binaries.zip
%GOOGLEUPLOAD% --labels Type-Executable,Featured --summary "Version %2 binaries (release only)" %~dp0\%2\protobuf-csharp-port-%2-release-binaries.zip

@SET GOOGLEUPLOAD=
@goto EXIT

:HELP
@ECHO.
@ECHO Available commands for release
@ECHO %0 build 2.3.0.277
@ECHO %0 push 2.3.0.277 {google-code-user} {google-code-password}
@ECHO.
@goto EXIT

:EXIT