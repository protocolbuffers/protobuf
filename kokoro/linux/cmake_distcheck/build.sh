#!/bin/bash
#
# Build file to set up and run tests based on distribution archive

set -eux

# Change to repo root
cd $(dirname $0)/../../..
GIT_REPO_ROOT=`pwd`

CONTAINER_IMAGE=cmake-linux
#gcr.io/protobuf-build/bazel/linux@sha256:2bfd061284eff8234f2fcca16d71d43c69ccf3a22206628b54c204a6a9aac277

# Update git submodules
git submodule update --init --recursive

docker run \
  --name $CONTAINER_NAME \
  -e CMAKE_FLAGS="-Dprotobuf_BUILD_CONFORMANCE=ON"
  -v $GIT_REPO_ROOT:/workspace \
  $CONTAINER_IMAGE \
  check conformance_cpp_test
