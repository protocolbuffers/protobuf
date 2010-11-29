:BEGIN
@ECHO OFF
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /t:Build /p:BuildConfiguration=Debug
IF ERRORLEVEL 1 GOTO END

:SILVERLIGHT2
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /t:Build /p:BuildConfiguration=Debug_Silverlight2

:END
PAUSE