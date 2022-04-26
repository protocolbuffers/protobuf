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

# crosscompile protoc as we will later need it for the java build.
# we build it under the dockcross/manylinux2014-aarch64 image so that the resulting protoc binary is compatible
# with a wide range of linux distros (including any docker images we will use later to build and test java)
kokoro/linux/aarch64/dockcross_helpers/run_dockcross_manylinux2014_aarch64.sh kokoro/linux/aarch64/protoc_crosscompile_aarch64.sh

# the command that will be used to build and test java under an emulator
# * IsValidUtf8Test and DecodeUtf8Test tests are skipped because they take very long under an emulator.
TEST_JAVA_COMMAND="mvn --batch-mode -DskipTests install && mvn --batch-mode -Dtest='**/*Test, !**/*IsValidUtf8Test, !**/*DecodeUtf8Test' -DfailIfNoTests=false -Dsurefire.failIfNoSpecifiedTests=false surefire:test"

# use an actual aarch64 docker image (with a real aarch64 java and maven) to run build & test protobuf java under an emulator
# * mount the protobuf root as /work to be able to access the crosscompiled files
# * to avoid running the process inside docker as root (which can pollute the workspace with files owned by root), we force
#   running under current user's UID and GID. To be able to do that, we need to provide a home directory for the user
#   otherwise the UID would be homeless under the docker container and pip install wouldn't work. For simplicity,
#   we just run map the user's home to a throwaway temporary directory
# * the JAVA_OPTS and MAVEN_CONFIG variables are being set mostly to silence warnings about non-existent home directory
#   and to avoid polluting the workspace.
docker run $DOCKER_TTY_ARGS --rm --user "$(id -u):$(id -g)" -e "HOME=/home/fake-user" -e "JAVA_OPTS=-Duser.home=/home/fake-user" -e "MAVEN_CONFIG=/home/fake-user/.m2" -v "$(mktemp -d):/home/fake-user" -v "$(pwd)":/work -w /work arm64v8/maven:3.8-openjdk-11 bash -c "cd java && $TEST_JAVA_COMMAND"
