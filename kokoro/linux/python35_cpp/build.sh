#!/bin/bash
#
# This is the top-level script we give to Kokoro as the entry point for
# running the "pull request" project:
#
# This script selects a specific Dockerfile (for building a Docker image) and
# a script to run inside that image.  Then we delegate to the general
# build_and_run_docker.sh script.

# Change to repo root
cd $(dirname $0)/../../..

export DOCKERHUB_ORGANIZATION=protobuftesting
export DOCKERFILE_DIR=kokoro/linux/dockerfile/test/python_stretch
export DOCKER_RUN_SCRIPT=kokoro/linux/pull_request_in_docker.sh
export OUTPUT_DIR=testoutput
export TEST_SET="python35_cpp"
./kokoro/linux/build_and_run_docker.sh
