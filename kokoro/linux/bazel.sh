#!/bin/bash

set -ex

if [[ -z "${CONTAINER_IMAGE}" ]]; then
  CONTAINER_IMAGE=us-docker.pkg.dev/protobuf-build/containers/common/linux/bazel:5.1.1-aec4d74f2eb6938fc53ef7d9a79a4bf2da24abc1
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

# Setup GAR for docker images.
gcloud components update --quiet
gcloud auth configure-docker us-docker.pkg.dev --quiet

function run {
  local CONFIG=$1
  local BAZEL_CONFIG=$2

  tmpfile=$(mktemp -u)

  rm -rf $GIT_REPO_ROOT/bazel-out $GIT_REPO_ROOT/bazel-bin
  rm -rf $GIT_REPO_ROOT/logs

  docker run \
    --cidfile $tmpfile \
    --cap-add=SYS_PTRACE \
    -v $GIT_REPO_ROOT:/workspace \
    $CONTAINER_IMAGE \
    test \
    $(${GIT_REPO_ROOT}/kokoro/common/bazel_flags.sh) \
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
