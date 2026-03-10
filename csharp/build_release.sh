#!/bin/bash

cd $(dirname $(readlink $BASH_SOURCE))

# Disable some unwanted dotnet options
export DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true
export DOTNET_CLI_TELEMETRY_OPTOUT=true

# Builds Google.Protobuf NuGet packages
dotnet restore -s /lib/csharp/ src/Google.Protobuf/Google.Protobuf.csproj
dotnet pack --no-restore -c Release src/Google.Protobuf.sln -p:ContinuousIntegrationBuild=true
