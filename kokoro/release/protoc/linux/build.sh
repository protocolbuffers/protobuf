#!/bin/bash

set -x

# Change to repo root.
cd $(dirname $0)/../../../..

# Initialize any submodules.
git submodule update --init --recursive

# Generate the configure script.
./autogen.sh

# Cross-build for aarch64, ppc64le and s390x. Note: we do these builds first to avoid
# file permission issues. The Docker builds will create directories owned by
# root, which causes problems if we try to add new artifacts to those
# directories afterward.

sudo apt install -y g++-aarch64-linux-gnu
protoc-artifacts/build-protoc.sh linux aarch_64 protoc

sudo apt install -y g++-powerpc64le-linux-gnu
protoc-artifacts/build-protoc.sh linux ppcle_64 protoc

sudo apt install -y g++-s390x-linux-gnu
protoc-artifacts/build-protoc.sh linux s390x protoc

# Use docker image to build linux artifacts.
DOCKER_IMAGE_NAME=protobuf/protoc_$(sha1sum protoc-artifacts/Dockerfile | cut -f1 -d " ")
docker pull $DOCKER_IMAGE_NAME

docker run -v $(pwd):/var/local/protobuf --rm $DOCKER_IMAGE_NAME \
    bash -l /var/local/protobuf/protoc-artifacts/build-protoc.sh \
    linux x86_64 protoc || {
  echo "Failed to build protoc for linux + x86_64."
  exit 1
}

docker run -v $(pwd):/var/local/protobuf --rm $DOCKER_IMAGE_NAME \
    bash -l /var/local/protobuf/protoc-artifacts/build-protoc.sh \
    linux x86_32 protoc || {
  echo "Failed to build protoc for linux + x86_32."
  exit 1
}
