#!/bin/bash
#
# Build file to set up and run tests using CMake with the Ninja generator.

set -eux

# Change to repo root
cd $(dirname $0)/../../..
GIT_REPO_ROOT=`pwd`

CONTAINER_IMAGE=us-docker.pkg.dev/protobuf-build/containers/test/linux/cmake@sha256:cc23dbe065668158ca2732aa305a07bd0913a175b2079d27d9c16925d23f2335

# Update git submodules and regenerate files
git submodule update --init --recursive
use_bazel.sh 5.1.1
sudo ./kokoro/common/setup_kokoro_environment.sh
./regenerate_stale_files.sh

tmpfile=$(mktemp -u)

gcloud components update --quiet
gcloud auth configure-docker us-docker.pkg.dev --quiet

docker run \
  --cidfile $tmpfile \
  -v $GIT_REPO_ROOT:/workspace \
  $CONTAINER_IMAGE \
  /test.sh -G Ninja -Dprotobuf_BUILD_CONFORMANCE=ON -DCMAKE_CXX_STANDARD=14

# Save logs for Kokoro
docker cp \
  `cat $tmpfile`:/workspace/logs $KOKORO_ARTIFACTS_DIR
