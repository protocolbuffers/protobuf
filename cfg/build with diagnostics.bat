@echo off

%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /l:FileLogger,Microsoft.Build.Engine;logfile=Build.log;append=true;verbosity=diagnostic;encoding=utf-8

pause