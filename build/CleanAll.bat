@echo off
CMD.exe /Q /C "CD %~dp0 && %WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe build.csproj /toolsversion:4.0 "/t:Clean" "/p:BuildConfiguration=Release;TargetVersion=2"
CMD.exe /Q /C "CD %~dp0 && %WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe build.csproj /toolsversion:4.0 "/t:Clean" "/p:BuildConfiguration=Debug;TargetVersion=2"
CMD.exe /Q /C "CD %~dp0 && %WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe build.csproj /toolsversion:4.0 "/t:Clean" "/p:BuildConfiguration=Release_Silverlight;TargetVersion=2"
CMD.exe /Q /C "CD %~dp0 && %WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe build.csproj /toolsversion:4.0 "/t:Clean" "/p:BuildConfiguration=Debug_Silverlight;TargetVersion=2"
