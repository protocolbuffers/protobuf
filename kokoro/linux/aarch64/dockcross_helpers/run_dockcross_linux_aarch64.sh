#!/bin/bash

set -e

# go to the repo root
cd $(dirname $0)/../../../..

if [[ -t 0 ]]; then
  DOCKER_TTY_ARGS="-it"
else
  # The input device on kokoro is not a TTY, so -it does not work.
  DOCKER_TTY_ARGS=
fi

# running dockcross image without any arguments generates a wrapper
# scripts that can be used to run commands under the dockcross image
# easily.
# See https://github.com/dockcross/dockcross#usage for details
docker run $DOCKER_TTY_ARGS --rm dockcross/linux-arm64 >dockcross-linux-arm64.sh
chmod +x dockcross-linux-arm64.sh

# the wrapper script has CRLF line endings and bash doesn't like that
# so we change CRLF line endings into LF.
sed -i 's/\r//g' dockcross-linux-arm64.sh

# The dockcross wrapper script runs arbitrary commands under the selected dockcross
# image with the following properties which make its use very convenient:
# * the current working directory is mounted under /work so the container can easily
#   access the current workspace
# * the processes in the container run under the same UID and GID as the host process so unlike
#   vanilla "docker run" invocations, the workspace doesn't get polluted with files
#   owned by root.
./dockcross-linux-arm64.sh "$@"
