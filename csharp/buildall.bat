@rem Builds Google.Protobuf and runs the tests

dotnet build src/Google.Protobuf.sln || goto :error

echo Running tests.

dotnet test src/Google.Protobuf.Test/Google.Protobuf.Test.csproj || goto :error

goto :EOF

:error
echo Failed!
exit /b %errorlevel%
