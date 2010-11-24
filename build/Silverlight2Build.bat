@echo off

%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /p:BuildConfiguration=Silverlight2

pause