#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

export DOCKERHUB_ORGANIZATION=protobuftesting
export DOCKERFILE_DIR=kokoro/linux/dockerfile/test/cpp_tcmalloc
export DOCKER_RUN_SCRIPT=kokoro/linux/pull_request_in_docker.sh
export OUTPUT_DIR=testoutput
export TEST_SET="cpp_tcmalloc"
./kokoro/linux/build_and_run_docker.sh
