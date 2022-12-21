#!/bin/bash
#
# Build file to set up and run tests using CMake

set -eux

# Change to repo root
cd $(dirname $0)/../../..
GIT_REPO_ROOT=`pwd`

CONTAINER_IMAGE=gcr.io/protobuf-build/cmake/linux@sha256:79e6ed9d7f3f8e56167a3309a521e5b7e6a212bfb19855c65ee1cbb6f9099671

# Update git submodules and regenerate files
git submodule update --init --recursive
use_bazel.sh 5.1.1
./regenerate_stale_files.sh

tmpfile=$(mktemp -u)

docker run \
  --cidfile $tmpfile \
  -v $GIT_REPO_ROOT:/workspace \
  $CONTAINER_IMAGE \
  /test.sh -Dprotobuf_BUILD_CONFORMANCE=ON -Dprotobuf_BUILD_EXAMPLES=ON

# Save logs for Kokoro
docker cp \
  `cat $tmpfile`:/workspace/logs $KOKORO_ARTIFACTS_DIR
