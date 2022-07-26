#!/bin/bash
#
# Build file to set up and run tests based on distribution archive

set -eux

# Change to repo root
cd $(dirname $0)/../../..
GIT_REPO_ROOT=`pwd`

CONTAINER_IMAGE=gcr.io/protobuf-build/cmake/linux@sha256:7aaac41a2f06258b967facf2e6afbd17eec01e85fb6a14b44cb03c9372311363

# Update git submodules
git submodule update --init --recursive

tmpfile=$(mktemp -u)

docker run \
  --cidfile $tmpfile \
  -v $GIT_REPO_ROOT:/workspace \
  $CONTAINER_IMAGE \
  "/install.sh; \
  CMAKE_FLAGS=\"-Dprotobuf_REMOVE_INSTALLED_HEADERS=ON -Dprotobuf_BUILD_PROTOBUF_BINARIES=OFF -Dprotobuf_BUILD_CONFORMANCE=ON\" \
    /make.sh check conformance_cpp_test"


# Save logs for Kokoro
docker cp \
  `cat $tmpfile`:/workspace/logs $KOKORO_ARTIFACTS_DIR
