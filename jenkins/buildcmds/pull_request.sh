#!/bin/bash
#
# This is the top-level script we give to Jenkins as the entry point for
# running the "pull request" project:
#
#   https://grpc-testing.appspot.com/view/Protocol%20Buffers/job/protobuf_pull_request/
#
# This script selects a specific Dockerfile (for building a Docker image) and
# a script to run inside that image.  Then we delegate to the general
# build_and_run_docker.sh script.

export DOCKERFILE_DIR=jenkins/docker
export DOCKER_RUN_SCRIPT=jenkins/pull_request_in_docker.sh
export OUTPUT_DIR=testoutput
export TEST_SET="csharp java_jdk7 javanano_jdk7 java_oracle7 javanano_oracle7 python python_cpp ruby_all javascript golang php_all"
./jenkins/build_and_run_docker.sh
