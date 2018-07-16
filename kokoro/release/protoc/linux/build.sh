#!/bin/bash

set -x

# Change to repo root.
cd $(dirname $0)/../../../..

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

# Cross-build for some architectures.
sudo apt install g++-aarch64-linux-gnu
# TODO(xiaofeng): It currently fails with "machine `aarch64' not recognized"
# error.
# protoc-artifacts/build-protoc.sh linux aarch_64 protoc
