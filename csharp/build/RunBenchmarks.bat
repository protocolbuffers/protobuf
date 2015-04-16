@echo off
CMD.exe /Q /C "CD %~dp0 && %WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe /nologo build.csproj /toolsversion:4.0 /t:RunBenchmarks %1 %2 %3 %4
