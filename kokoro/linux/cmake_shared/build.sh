#!/bin/bash
#
# Build file to set up and run tests via CMake using shared libraries

set -eux

# Change to repo root
cd $(dirname $0)/../../..
GIT_REPO_ROOT=`pwd`

CONTAINER_IMAGE=gcr.io/protobuf-build/bazel/linux-san:b6bfa3bb505e83f062af0cb0ed23abf1e89b9ed

# Update git submodules
git submodule update --init --recursive

# Run the staleness tests and use them to update any stale files.
bazel test //src:cmake_lists_staleness_test || ./bazel-bin/src/cmake_lists_staleness_test --fix
bazel test //src/google/protobuf:well_known_types_staleness_test || ./bazel-bin/src/google/protobuf/well_known_types_staleness_test --fix

tmpfile=$(mktemp -u)

docker run \
  --cidfile $tmpfile \
  -v $GIT_REPO_ROOT:/workspace \
  $CONTAINER_IMAGE \
  /test.sh -Dprotobuf_BUILD_CONFORMANCE=ON -Dprotobuf_BUILD_SHARED_LIBS=ON

# Save logs for Kokoro
docker cp \
  `cat $tmpfile`:/workspace/logs $KOKORO_ARTIFACTS_DIR
