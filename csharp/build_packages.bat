@rem Builds Google.Protobuf NuGet packages

@rem Adjust the location of nuget.exe
set NUGET=C:\nuget\nuget.exe

@rem Build src/Google.Protobuf.sln solution in Release configuration first.
%NUGET% pack src\Google.Protobuf\Google.Protobuf.nuspec -Symbols || goto :error

goto :EOF

:error
echo Failed!
exit /b %errorlevel%
