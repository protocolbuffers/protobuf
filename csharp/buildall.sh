#!/bin/bash

CONFIG=Release
SRC=$(dirname $0)/src

set -ex

echo Building relevant projects.
dotnet build -c $CONFIG $SRC/Google.Protobuf $SRC/Google.Protobuf.Test $SRC/Google.Protobuf.Conformance

echo Running tests.
dotnet test -c $CONFIG $SRC/Google.Protobuf.Test
