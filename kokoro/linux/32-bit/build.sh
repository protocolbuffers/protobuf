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

CONTAINER_IMAGE=us-docker.pkg.dev/protobuf-build/containers/test/linux/32bit@sha256:6651a299483f7368876db7aed0802ad4ebf038d626d8995ba7df08978ff43210

git submodule update --init --recursive
use_bazel.sh 5.1.1
sudo ./kokoro/common/setup_kokoro_environment.sh
./regenerate_stale_files.sh

gcloud components update --quiet
gcloud auth configure-docker us-docker.pkg.dev --quiet

docker run \
  "$@" \
  -v $GIT_REPO_ROOT:/workspace \
  $CONTAINER_IMAGE \
  bash -l "/workspace/kokoro/linux/32-bit/test_php.sh"
