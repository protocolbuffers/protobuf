@echo off

%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild %~dp0\build.csproj /m /t:RunBenchmarks /p:BuildConfiguration=Release /p:Platform="Any CPU"

pause