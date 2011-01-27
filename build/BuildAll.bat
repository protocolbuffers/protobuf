@ECHO OFF

SET PREV_WORKING_DIR=%CD%
CD %~dp0

REM -- 3.5 Debug build, ensure this continues to work
%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe build.csproj /t:Rebuild /p:BuildConfiguration=Debug /p:Platform="Any CPU" /p:BuildTools=3.5 /toolsversion:3.5"
IF ERRORLEVEL 1 GOTO ERROR

REM -- 4.0 Debug build
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /t:Rebuild /p:BuildConfiguration=Debug /p:Platform="Any CPU"
IF ERRORLEVEL 1 GOTO ERROR

REM -- 4.0 Release build
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /t:Rebuild /p:BuildConfiguration=Release /p:Platform="Any CPU"
IF ERRORLEVEL 1 GOTO ERROR

IF EXIST "%ProgramFiles%\MSBuild\Microsoft\Silverlight\v2.0\Microsoft.Silverlight.CSharp.targets" GOTO SILVERLIGHT
IF EXIST "%ProgramFiles(x86)%\MSBuild\Microsoft\Silverlight\v2.0\Microsoft.Silverlight.CSharp.targets" GOTO SILVERLIGHT

ECHO Unable to locate %ProgramFiles(x86)%\MSBuild\Microsoft\Silverlight\v2.0
GOTO ERROR

:SILVERLIGHT

REM -- 4.0 Debug_Silverlight2 build
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /t:Rebuild /p:BuildConfiguration=Debug_Silverlight2 /p:Platform="Any CPU"
IF ERRORLEVEL 1 GOTO ERROR

REM -- 4.0 Release_Silverlight2 build
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /t:Rebuild /p:BuildConfiguration=Release_Silverlight2 /p:Platform="Any CPU"
IF ERRORLEVEL 1 GOTO ERROR

GOTO END

:ERROR
CD %PREV_WORKING_DIR%
PAUSE

:END
CD %PREV_WORKING_DIR%
SET PREV_WORKING_DIR=