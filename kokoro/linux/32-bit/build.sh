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

CONTAINER_IMAGE=gcr.io/protobuf-build/php/32bit@sha256:8c3cf171ac8a3f91296517d822a26b1cbb6696035bdb723db68928d52bdbfc40

git submodule update --init --recursive
use_bazel.sh 5.1.1
./regenerate_stale_files.sh

docker run \
  "$@" \
  -v $GIT_REPO_ROOT:/workspace \
  $CONTAINER_IMAGE \
  bash -l "/workspace/kokoro/linux/32-bit/test_php.sh"
