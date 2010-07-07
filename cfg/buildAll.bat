@echo off

%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /t:BuildAll /p:CompileGroup=BuildAll /m

pause