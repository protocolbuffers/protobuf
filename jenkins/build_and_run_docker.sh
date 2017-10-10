#!/bin/bash
#
# Builds docker image and runs a command under it.
# This is a generic script that is configured with the following variables:
#
# DOCKERFILE_DIR - Directory in which Dockerfile file is located.
# DOCKER_RUN_SCRIPT - Script to run under docker (relative to protobuf repo root)
# OUTPUT_DIR - Directory that will be copied from inside docker after finishing.
# $@ - Extra args to pass to docker run


set -ex

cd $(dirname $0)/..
git_root=$(pwd)
cd -

# Use image name based on Dockerfile location checksum
DOCKER_IMAGE_NAME=$(basename $DOCKERFILE_DIR)_$(sha1sum $DOCKERFILE_DIR/Dockerfile | cut -f1 -d\ )

# Make sure docker image has been built. Should be instantaneous if so.
docker build -t $DOCKER_IMAGE_NAME $DOCKERFILE_DIR

# Ensure existence of ccache directory
CCACHE_DIR=/tmp/protobuf-ccache
mkdir -p $CCACHE_DIR

# Choose random name for docker container
CONTAINER_NAME="build_and_run_docker_$(uuidgen)"

# Run command inside docker
docker run \
  "$@" \
  -e CCACHE_DIR=$CCACHE_DIR \
  -e EXTERNAL_GIT_ROOT="/var/local/jenkins/protobuf" \
  -e TEST_SET="$TEST_SET" \
  -e THIS_IS_REALLY_NEEDED='see https://github.com/docker/docker/issues/14203 for why docker is awful' \
  -v "$git_root:/var/local/jenkins/protobuf:ro" \
  -v $CCACHE_DIR:$CCACHE_DIR \
  -w /var/local/git/protobuf \
  --name=$CONTAINER_NAME \
  $DOCKER_IMAGE_NAME \
  bash -l "/var/local/jenkins/protobuf/$DOCKER_RUN_SCRIPT" || FAILED="true"

# Copy output artifacts
if [ "$OUTPUT_DIR" != "" ]
then
  docker cp "$CONTAINER_NAME:/var/local/git/protobuf/$OUTPUT_DIR" "$git_root" || FAILED="true"
fi

# remove the container, possibly killing it first
docker rm -f $CONTAINER_NAME || true

if [ "$FAILED" != "" ]
then
  exit 1
fi
