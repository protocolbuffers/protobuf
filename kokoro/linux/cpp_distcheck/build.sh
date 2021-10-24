#!/bin/bash
#
# Build file to set up and run tests

set -ex  # exit immediately on error

# Change to repo root
cd $(dirname $0)/../../..

./tests.sh cpp_distcheck

# Run tests under release docker image.
DOCKER_IMAGE_NAME=protobuf/protoc_$(sha1sum protoc-artifacts/Dockerfile | cut -f1 -d " ")
until docker pull $DOCKER_IMAGE_NAME; do sleep 10; done

docker run -v $(pwd):/var/local/protobuf --rm $DOCKER_IMAGE_NAME \
  bash -l /var/local/protobuf/tests.sh cpp || FAILED="true"

# This directory is owned by root. We need to delete it, because otherwise
# Kokoro will attempt to rsync it and fail with a permission error.
rm -rf src/core

if [ "$FAILED" = "true" ]; then
  exit 1
fi
