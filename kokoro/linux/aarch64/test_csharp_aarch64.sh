#!/bin/bash

set -ex

# go to the repo root
cd $(dirname $0)/../../..

if [[ -t 0 ]]; then
  DOCKER_TTY_ARGS="-it"
else
  # The input device on kokoro is not a TTY, so -it does not work.
  DOCKER_TTY_ARGS=
fi

# First, build protobuf C# tests under x86_64 docker image
# Tests are built "dotnet publish" because we want all the dependencies to the copied to the destination directory
# (we want to avoid references to ~/.nuget that won't be available in the subsequent docker run)
CSHARP_BUILD_COMMAND="dotnet publish -c Release -f net60 csharp/src/Google.Protobuf.Test/Google.Protobuf.Test.csproj"
docker run $DOCKER_TTY_ARGS --rm --user "$(id -u):$(id -g)" -e "HOME=/home/fake-user" -e "DOTNET_CLI_TELEMETRY_OPTOUT=true" -e "DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true" -v "$(mktemp -d):/home/fake-user" -v "$(pwd)":/work -w /work mcr.microsoft.com/dotnet/sdk:6.0.100-bullseye-slim bash -c "$CSHARP_BUILD_COMMAND"

# Use an actual aarch64 docker image to run protobuf C# tests with an emulator. "dotnet vstest" allows
# running tests from a pre-built project.
# * mount the protobuf root as /work to be able to access the crosscompiled files
# * to avoid running the process inside docker as root (which can pollute the workspace with files owned by root), we force
#   running under current user's UID and GID. To be able to do that, we need to provide a home directory for the user
#   otherwise the UID would be homeless under the docker container and pip install wouldn't work. For simplicity,
#   we just run map the user's home to a throwaway temporary directory
CSHARP_TEST_COMMAND="dotnet vstest csharp/src/Google.Protobuf.Test/bin/Release/net60/publish/Google.Protobuf.Test.dll"
docker run $DOCKER_TTY_ARGS --rm --user "$(id -u):$(id -g)" -e "HOME=/home/fake-user" -e "DOTNET_CLI_TELEMETRY_OPTOUT=true" -e "DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true" -v "$(mktemp -d):/home/fake-user" -v "$(pwd)":/work -w /work mcr.microsoft.com/dotnet/sdk:6.0.100-bullseye-slim-arm64v8 bash -c "$CSHARP_TEST_COMMAND"
