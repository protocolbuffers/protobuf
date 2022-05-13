#!/bin/bash
#
# Run tests under Bazel.
#
# Tests are run from the directory where this script is run. Arguments to this
# script are passed to bazel. The default bazel command is just `bazel`, but it
# can be overridden by setting the `BAZEL` environment variable.

BUILD_ONLY_TARGETS=(
  //pkg:all
  #//:protoc
  #//:protobuf
  #//:protobuf_python
)

TEST_TARGETS=(
  #//build_defs:all
  #//conformance:all
  #//java:tests
  #//python:all
  //src/...
  #@com_google_protobuf_examples//...
)

set -eu
set -o pipefail

# Wrapper to run bazel.
# This might conditionally add flags on Kokoro.
function _bazel() {
  local -a _flags
  # We might need to add flags, and they need to come before any terminating
  # "--", so copy them into the _flags variable.
  while (( ${#@} > 0 )); do
    if [[ $1 == "--" ]]; then
      break
    fi
    _flags+=( "${1}" )
    shift
  done

  # If appropriate, add special flags for Kokoro.
  if [[ -n ${KOKORO_BES_PROJECT_ID:-} ]]; then
    # Create a new invocation ID and store in the artifacts dir.
    local _invocation_id=$(uuidgen)
    echo ${_invocation_id} >> ${KOKORO_ARTIFACTS_DIR}/bazel_invocation_ids
    _flags+=(
      --bes_backend=${KOKORO_BES_BACKEND_ADDRESS}
      --google_auth_scopes=https://www.googleapis.com/auth/cloud-source-tools
      --google_default_credentials=true
      --invocation_id=${_invocation_id}
    )
  fi

  # Add any remaining args (they are targets after a terminating "--").
  _flags+=( "${@}" )

  echo -n "[[ $(date) ]]"
  ( set -x ; ${BAZEL:-bazel} "${_flags[@]}" )
}

function main() {
  local _failed=false
  # Include any arguments passed to the script:
  _bazel build -k "${@}" -- "${BUILD_ONLY_TARGETS[@]}" || _failed=true
  _bazel test -k --test_output=errors "${@}" -- "${TEST_TARGETS[@]}" || _failed=true

  echo
  if ${_failed}; then
    echo FAIL
    exit 1
  fi
  echo PASS
}
main
