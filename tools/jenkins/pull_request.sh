#!/bin/bash


export DOCKERFILE_DIR=tools/docker
export DOCKER_RUN_SCRIPT=tools/run_tests/jenkins.sh
export OUTPUT_DIR=testoutput
./tools/jenkins/build_and_run_docker.sh
