#!/bin/bash
#
# Build file to set up and run tests

set -ex  # exit immediately on error

# Change to repo root
cd $(dirname $0)/../../..

# Run tests under release docker image.
export DOCKERFILE_DIR=kokoro/linux/64-bit
export DOCKER_RUN_SCRIPT=kokoro/linux/pull_request_in_docker.sh
export OUTPUT_DIR=testoutput
export TEST_SET="dist_install"
./kokoro/linux/build_and_run_docker.sh
