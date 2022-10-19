#!/bin/bash

cd $(dirname $(readlink $BASH_SOURCE))

# Disable some unwanted dotnet options
set DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true
set DOTNET_CLI_TELEMETRY_OPTOUT=true

# Builds Google.Protobuf NuGet packages
dotnet pack --no-restore -c Release src/Google.Protobuf.sln -p:ContinuousIntegrationBuild=true
