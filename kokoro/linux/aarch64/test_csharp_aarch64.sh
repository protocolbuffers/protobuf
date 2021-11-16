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

# Use an actual aarch64 docker image to build and run protobuf C# tests with an emulator. We could also pre-build
# the project under x86_64 docker image and then run the tests under an emulator, but C# build is relatively
# quick even under the emulator (and its simpler that way).
# * mount the protobuf root as /work to be able to access the crosscompiled files
# * to avoid running the process inside docker as root (which can pollute the workspace with files owned by root), we force
#   running under current user's UID and GID. To be able to do that, we need to provide a home directory for the user
#   otherwise the UID would be homeless under the docker container and pip install wouldn't work. For simplicity,
#   we just run map the user's home to a throwaway temporary directory
CSHARP_TEST_COMMAND="dotnet test -c Release -f net60 csharp/src/Google.Protobuf.Test/Google.Protobuf.Test.csproj"
docker run $DOCKER_TTY_ARGS --rm --user "$(id -u):$(id -g)" -e "HOME=/home/fake-user" -e "DOTNET_CLI_TELEMETRY_OPTOUT=true" -e "DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true" -v "$(mktemp -d):/home/fake-user" -v "$(pwd)":/work -w /work mcr.microsoft.com/dotnet/sdk:6.0.100-bullseye-slim-arm64v8 bash -c "$CSHARP_TEST_COMMAND"
