#!/bin/bash

CONFIG=Release
SRC=$(dirname $0)/src

set -ex

echo Building relevant projects.
dotnet restore $SRC/Google.Protobuf.sln
dotnet build -c $CONFIG $SRC/Google.Protobuf.sln

echo Running tests.
# Only test net6.0, which uses the .NET runtime.
# If we want to test the .NET 4.5 version separately, we could
# run Mono explicitly. However, we don't have any differences between
# the .NET 4.5 and netstandard2.1 assemblies.
dotnet test -c $CONFIG -f net6.0 $SRC/Google.Protobuf.Test/Google.Protobuf.Test.csproj
