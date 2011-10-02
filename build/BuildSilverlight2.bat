@echo off
CMD.exe /Q /C "CD %~dp0 && %WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe build.csproj /toolsversion:4.0 /t:Rebuild %1 %2 %3 %4 "/p:BuildConfiguration=Debug_Silverlight;TargetVersion=2"
