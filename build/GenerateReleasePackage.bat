@ECHO OFF
%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /t:Build;PreparePackageComponent /p:BuildConfiguration=Release
IF ERRORLEVEL 1 GOTO END

%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /t:Build;PreparePackageComponent /p:BuildConfiguration=Release_Silverlight2
IF ERRORLEVEL 1 GOTO END

%WINDIR%\Microsoft.NET\Framework\v4.0.30319\msbuild build.csproj /m /t:GeneratePackage /p:PackageName=ReleaseBinaries.zip

:END
PAUSE