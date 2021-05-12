#!/bin/bash

set -e

# go to the repo root
cd $(dirname $0)/../../..

if [[ -t 0 ]]; then
  DOCKER_TTY_ARGS="-it"
else
  # The input device on kokoro is not a TTY, so -it does not work.
  DOCKER_TTY_ARGS=
fi

# crosscompile python extension and the binary wheel under dockcross/manylinux2014-aarch64 image
kokoro/linux/aarch64/dockcross_helpers/run_dockcross_manylinux2014_aarch64.sh kokoro/linux/aarch64/python_crosscompile_aarch64.sh

# once crosscompilation is done, use an actual aarch64 docker image (with a real aarch64 python) to run all the tests under an emulator
# * mount the protobuf root as /work to be able to access the crosscompiled files
# * intentionally use a different image than manylinux2014 so that we don't build and test on the same linux distribution
#   (manylinux_2_24 is debian-based while manylinux2014 is centos-based)
# * to avoid running the process inside docker as root (which can pollute the workspace with files owned by root), we force
#   running under current user's UID and GID. To be able to do that, we need to provide a home directory for the user
#   otherwise the UID would be homeless under the docker container and pip install wouldn't work. For simplicity,
#   we just run map the user's home to a throwaway temporary directory
docker run $DOCKER_TTY_ARGS --rm --user "$(id -u):$(id -g)" -e "HOME=/home/fake-user" -v "$(mktemp -d):/home/fake-user" -v "$(pwd)":/work -w /work quay.io/pypa/manylinux_2_24_aarch64 kokoro/linux/aarch64/python_run_tests_with_qemu_aarch64.sh
