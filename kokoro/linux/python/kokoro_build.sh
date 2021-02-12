#!/bin/bash -eux
#
# This is the top-level script we give to Kokoro as the entry point for
# running Python tests. The tests themselves run in a Docker container,
# specified by test_container.sh.

# The following values can be overridden by environment variable:
#   DOCKER_IMAGE
#   PROTOBUF_WORKSPACE
#   PROTOBUF_PYTHON_RUNTIME (used in build_python.sh)

# The Docker image to run the build.
#
# The official Python images are based on buildpack-deps, which is
# sufficient to build the Protobuf project. The versioned builds specify
# the image via environment variable, but this is a reasonable default:
: ${DOCKER_IMAGE:=python:buster}

# The location for the source checkout.
: ${PROTOBUF_WORKSPACE:=${KOKORO_ARTIFACTS_DIR}/github/protobuf}

docker run \
  "${DOCKER_IMAGE}" \
  -e PROTOBUF_PYTHON_RUNTIME \
  -v "${PROTOBUF_WORKSPACE}:/workspace/protobuf:rw" \
  -w "/workspace/protobuf" \
  "kokoro/linux/python/build_python.sh"

# Run the tests inside of the given image.
docker run \
  "${DOCKER_IMAGE}" \
  -e PROTOBUF_PYTHON_RUNTIME \
  -v "${PROTOBUF_WORKSPACE}:/workspace/protobuf:rw" \
  -w "/workspace/protobuf" \
  "kokoro/linux/python/test_python.sh"
