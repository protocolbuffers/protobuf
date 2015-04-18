@echo off
SET BUILD_VERSION=%~1
SET BUILD_TARGET=%~2
SET BUILD_CONFIG=%~3

IF NOT "%BUILD_VERSION%"=="" GOTO RUN
ECHO.
ECHO Usage: build.bat platform [target] [config] [msbuild arguments]
ECHO.
ECHO - platform:  CF20, CF35, NET20, NET35, NET40, PL40, SL20, SL30, or SL40
ECHO - [target]:  Rebuild, Clean, Build, Test, or Publish
ECHO - [config]:  Debug or Release
ECHO.
EXIT /B 1

:RUN
IF "%BUILD_TARGET%"=="" SET BUILD_TARGET=Rebuild
IF "%BUILD_CONFIG%"=="" SET BUILD_CONFIG=Debug

CMD.exe /Q /C "CD %~dp0 && %WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe /nologo target.csproj /toolsversion:4.0 %4 %5 %6 "/t:%BUILD_TARGET%" "/p:Configuration=%BUILD_CONFIG%;TargetVersion=%BUILD_VERSION%"
