#!/bin/bash
# Copyright 2016, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Builds docker image and runs a command under it.
# You should never need to call this script on your own.

set -ex

cd $(dirname $0)/../..
git_root=$(pwd)
cd -

# Inputs
# DOCKERFILE_DIR - Directory in which Dockerfile file is located.
# DOCKER_RUN_SCRIPT - Script to run under docker (relative to protobuf repo root)
# OUTPUT_DIR - Directory that will be copied from inside docker after finishing.
# $@ - Extra args to pass to docker run

# Use image name based on Dockerfile location checksum
DOCKER_IMAGE_NAME=$(basename $DOCKERFILE_DIR)_$(sha1sum $DOCKERFILE_DIR/Dockerfile | cut -f1 -d\ )

# Make sure docker image has been built. Should be instantaneous if so.
docker build -t $DOCKER_IMAGE_NAME $DOCKERFILE_DIR

# Choose random name for docker container
CONTAINER_NAME="build_and_run_docker_$(uuidgen)"

# Ensure existence of ccache directory
CCACHE_DIR=/tmp/protobuf-ccache
mkdir -p $CCACHE_DIR

# Run command inside docker
docker run \
  "$@" \
  -e CCACHE_DIR=$CCACHE_DIR \
  -e EXTERNAL_GIT_ROOT="/var/local/jenkins/protobuf" \
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
