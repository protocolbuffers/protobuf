#!/bin/bash
#
# Build file to set up and run tests using CMake with the Ninja generator.

set -eux

# Change to repo root
cd $(dirname $0)/../../..
GIT_REPO_ROOT=`pwd`

CONTAINER_IMAGE=gcr.io/protobuf-build/cmake/linux@sha256:bd17c13255c8c7af2821c6597ad96568d714c0ea9d9e27d81b108fcc195ca858

# Update git submodules and regenerate files
git submodule update --init --recursive
use_bazel.sh 5.1.1
sudo ./kokoro/common/setup_kokoro_environment.sh
./regenerate_stale_files.sh

tmpfile=$(mktemp -u)

docker run \
  --cidfile $tmpfile \
  -v $GIT_REPO_ROOT:/workspace \
  $CONTAINER_IMAGE \
  /test.sh -G Ninja -Dprotobuf_BUILD_CONFORMANCE=ON -DCMAKE_CXX_STANDARD=14

# Save logs for Kokoro
docker cp \
  `cat $tmpfile`:/workspace/logs $KOKORO_ARTIFACTS_DIR
