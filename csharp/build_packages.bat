@rem Builds Google.Protobuf NuGet packages

dotnet restore src
dotnet pack -c Release src\Google.Protobuf || goto :error

goto :EOF

:error
echo Failed!
exit /b %errorlevel%
