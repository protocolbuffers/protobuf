@echo off
SET START_DIR=%CD%
CD %~dp0
%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe build.csproj /t:Build /p:BuildConfiguration=Debug /p:Platform="Any CPU" /p:TargetFramework="v2.0" /p:BuildTools="v3.5" /toolsversion:3.5
CD %START_DIR%
SET START_DIR=
