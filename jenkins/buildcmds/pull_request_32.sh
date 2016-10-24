#!/bin/bash
#
# This is the top-level script we give to Jenkins as the entry point for
# running the "pull request 32" project:
#
#   https://grpc-testing.appspot.com/view/Protocol%20Buffers/job/Protocol%20Buffers%20Pull%20Request%2032/
#
# This script selects a specific Dockerfile (for building a Docker image) and
# a script to run inside that image.  Then we delegate to the general
# build_and_run_docker.sh script.

export DOCKERFILE_DIR=jenkins/docker32
export DOCKER_RUN_SCRIPT=jenkins/pull_request_in_docker.sh
export OUTPUT_DIR=testoutput
export TEST_SET="php_all_32"
./jenkins/build_and_run_docker.sh
