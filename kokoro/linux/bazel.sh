#!/bin/bash

set -ex

if [[ -z "${CONTAINER_IMAGE}" ]]; then
  CONTAINER_IMAGE=gcr.io/protobuf-build/bazel/linux@sha256:2bfd061284eff8234f2fcca16d71d43c69ccf3a22206628b54c204a6a9aac277
fi

cd $(dirname $0)/../..
GIT_REPO_ROOT=`pwd`

ENVS=()

# Check for specific versions pinned to the docker image.  In these cases we
# want to forward the environment variable to tests, so that they can verify
# that the correct version is being picked up by Bazel.
ENVS+=("--test_env=KOKORO_JAVA_VERSION")
ENVS+=("--test_env=KOKORO_PYTHON_VERSION")
ENVS+=("--test_env=KOKORO_RUBY_VERSION")

if [ -n "$BAZEL_ENV" ]; then
  for env in $BAZEL_ENV; do
    ENVS+="--action_env=${env}"
  done
fi

function run {
  local CONFIG=$1
  local BAZEL_CONFIG=$2

  tmpfile=$(mktemp -u)

  rm -rf $GIT_REPO_ROOT/bazel-out $GIT_REPO_ROOT/bazel-bin
  rm -rf $GIT_REPO_ROOT/logs

  docker run \
    --cidfile $tmpfile \
    -v $GIT_REPO_ROOT:/workspace \
    $CONTAINER_IMAGE \
    test \
    --keep_going \
    --test_output=streamed \
    --cap-add=SYS_PTRACE \
    ${ENVS[@]} \
    $PLATFORM_CONFIG \
    $BAZEL_CONFIG \
    $BAZEL_EXTRA_FLAGS \
    $BAZEL_TARGETS

  # Save logs for Kokoro
  docker cp \
    `cat $tmpfile`:/workspace/logs $KOKORO_ARTIFACTS_DIR/$CONFIG
}

if [ -n "$BAZEL_CONFIGS" ]; then
  for config in $BAZEL_CONFIGS; do
    run $config "--config=$config"
  done
else
    run
fi
