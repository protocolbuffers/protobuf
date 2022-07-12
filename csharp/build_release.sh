#!/bin/bash

cd $(dirname $(readlink $BASH_SOURCE))

# Disable some unwanted dotnet options
set DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true
set DOTNET_CLI_TELEMETRY_OPTOUT=true

# Work around https://github.com/dotnet/core/issues/5881
dotnet nuget locals all --clear

# Builds Google.Protobuf NuGet packages
dotnet restore src/Google.Protobuf.sln
dotnet pack -c Release src/Google.Protobuf.sln -p:ContinuousIntegrationBuild=true

# This requires built protoc executables as specified in the nusepc
nuget pack Google.Protobuf.Tools.nuspec
