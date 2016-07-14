#!/bin/bash

CONFIG=Release
SRC=$(dirname $0)/src

set -ex

echo Building relevant projects.
dotnet build -c $CONFIG $SRC/Google.Protobuf $SRC/Google.Protobuf.Test $SRC/Google.Protobuf.Conformance

echo Running tests.
# Only test netcoreapp1.0, which uses the .NET Core runtime.
# If we want to test the .NET 4.5 version separately, we could
# run Mono explicitly. However, we don't have any differences between
# the .NET 4.5 and netstandard1.0 assemblies.
dotnet test -c $CONFIG -f netcoreapp1.0 $SRC/Google.Protobuf.Test
