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

function bazel_wrapper::gen_invocation_id() {
  # Create a new invocation ID and store in the artifacts dir.
  local _invocation_id=$(uuidgen | tr A-Z a-z)

  # Put the new invocation ID at the start of the output IDs file. Some
  # Google-internal tools only look at the first entry, so this ensures the most
  # recent entry is first.
  local _ids_file=${KOKORO_ARTIFACTS_DIR}/bazel_invocation_ids
  local _temp_ids=$(mktemp)
  echo ${_invocation_id} > ${_temp_ids}
  [[ -e ${_ids_file} ]] && cat ${_ids_file} >> ${_temp_ids}
  mv -f ${_temp_ids} ${_ids_file}

  echo -n ${_invocation_id}
}

# Prints flags to use on Kokoro.
function bazel_wrapper::kokoro_flags() {
  local -a _flags
  _flags+=(
    --invocation_id=$(bazel_wrapper::gen_invocation_id)
    --remote_cache=https://storage.googleapis.com/protobuf-bazel-cache/protobuf/${KOKORO_JOB_NAME}
  )
  if [[ -n ${KOKORO_BAZEL_AUTH_CREDENTIAL:-} ]]; then
    _flags+=( --google_credentials=${KOKORO_BAZEL_AUTH_CREDENTIAL} )
  else
    _flags+=( --google_default_credentials=true )
  fi

  echo "${_flags[@]}"
}

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
    $(bazel_wrapper::kokoro_flags) \
    --keep_going \
    --test_output=streamed \
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
