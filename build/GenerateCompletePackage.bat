:BEGIN
@ECHO OFF
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /t:Build /p:BuildConfiguration=Release
IF ERRORLEVEL 1 GOTO END

:SILVERLIGHT2
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /t:Build /p:BuildConfiguration=Release_Silverlight2
IF ERRORLEVEL 1 GOTO END

:GENERATE_PACKAGE
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /t:GeneratePackage /p:PackageScope=AllBinariesAndSource

:END
PAUSE