#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

./tests.sh cpp_tcmalloc

# Run tests under release docker image.
DOCKER_IMAGE_NAME=protobuf/protoc_$(sha1sum kokoro/linux/dockerfile/test/cpp_tcmalloc/Dockerfile | cut -f1 -d " ")
docker pull $DOCKER_IMAGE_NAME

docker run -v $(pwd):/var/local/protobuf --rm $DOCKER_IMAGE_NAME \
  bash -l /var/local/protobuf/tests.sh cpp_tcmalloc || FAILED="true"

if [ "$FAILED" = "true" ]; then
  exit 1
fi
