#!/bin/bash

cd $(dirname $0)/../../..

export DOCKERFILE_DIR=kokoro/linux/64-bit
export DOCKER_RUN_SCRIPT=kokoro/linux/pull_request_in_docker.sh
export OUTPUT_DIR=testoutput
export TEST_SET="benchmark"
./kokoro/linux/build_and_run_docker.sh
