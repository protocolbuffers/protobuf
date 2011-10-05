@echo off
PUSHD %~dp0
IF EXIST ..\build_output RMDIR /S /Q ..\build_output
IF EXIST ..\build_temp RMDIR /S /Q ..\build_temp
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe /nologo build.csproj /toolsversion:4.0 /t:Clean /v:m "/p:BuildConfiguration=Release;TargetVersion=2"
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe /nologo build.csproj /toolsversion:4.0 /t:Clean /v:m "/p:BuildConfiguration=Debug;TargetVersion=2"
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe /nologo build.csproj /toolsversion:4.0 /t:Clean /v:m "/p:BuildConfiguration=Release_Silverlight;TargetVersion=2"
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe /nologo build.csproj /toolsversion:4.0 /t:Clean /v:m "/p:BuildConfiguration=Debug_Silverlight;TargetVersion=2"
POPD