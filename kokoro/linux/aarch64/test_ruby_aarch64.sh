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

# crosscompile protoc as we will later need it for the ruby build.
# we build it under the dockcross/manylinux2014-aarch64 image so that the resulting protoc binary is compatible
# with a wide range of linux distros (including any docker images we will use later to build and test ruby)
kokoro/linux/aarch64/dockcross_helpers/run_dockcross_manylinux2014_aarch64.sh kokoro/linux/aarch64/protoc_crosscompile_aarch64.sh

# use an actual aarch64 docker image (with a real aarch64 ruby) to run build & test protobuf ruby under an emulator
# * mount the protobuf root as /work to be able to access the crosscompiled files
# * to avoid running the process inside docker as root (which can pollute the workspace with files owned by root), we force
#   running under current user's UID and GID. To be able to do that, we need to provide a home directory for the user
#   otherwise the UID would be homeless under the docker container and pip install wouldn't work. For simplicity,
#   we just run map the user's home to a throwaway temporary directory

docker run $DOCKER_TTY_ARGS --rm --user "$(id -u):$(id -g)" -e "HOME=/home/fake-user" -v "$(mktemp -d):/home/fake-user" -v "$(pwd)":/work -w /work arm64v8/ruby:2.7.3-buster kokoro/linux/aarch64/ruby_build_and_run_tests_with_qemu_aarch64.sh