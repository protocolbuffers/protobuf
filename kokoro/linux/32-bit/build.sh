#!/bin/bash
#
# This is the top-level script we give to Kokoro as the entry point for
# running the "pull request 32" project:
#
# This script selects a specific Dockerfile (for building a Docker image) and
# a script to run inside that image.

set -ex

# Change to repo root
cd $(dirname $0)/../../..
GIT_REPO_ROOT=$(pwd)

CONTAINER_IMAGE=gcr.io/protobuf-build/php/32bit@sha256:5be6b5853a13d413e4ef557ed1cbf6cf0ba4896769c7c37422fada653e0cdee5

docker run \
  "$@" \
  -v $GIT_REPO_ROOT:/workspace \
  $CONTAINER_IMAGE \
  bash -l "/workspace/kokoro/linux/32-bit/test.sh"
