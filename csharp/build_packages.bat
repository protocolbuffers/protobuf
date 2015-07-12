@rem Builds Google.Protobuf NuGet packages

@rem Adjust the location of nuget.exe
set NUGET=C:\nuget\nuget.exe

@rem Build src/ProtocolBuffers.sln solution in Release configuration first.
%NUGET% pack src\ProtocolBuffers\Google.Protobuf.nuspec -Symbols || goto :error

goto :EOF

:error
echo Failed!
exit /b %errorlevel%
