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

CONTAINER_IMAGE=gcr.io/protobuf-build/php/32bit@sha256:824cbdff02ee543eb69ee4b02c8c58cc7887f70f49e41725a35765d92a898b4f

git submodule update --init --recursive

docker run \
  "$@" \
  -v $GIT_REPO_ROOT:/workspace \
  $CONTAINER_IMAGE \
  bash -l "/workspace/kokoro/linux/32-bit/test_php.sh"
