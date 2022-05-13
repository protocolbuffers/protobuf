#!/bin/bash
#
# Run tests under Bazel.
#
# Tests are run from the directory where this script is run. Arguments to this
# script are passed to bazel. The default bazel command is just `bazel`, but it
# can be overridden by setting the `BAZEL` environment variable.

BUILD_ONLY_TARGETS=(
  //pkg:all
  //:protoc
  //:protobuf
  //:protobuf_python
)

TEST_TARGETS=(
  //build_defs:all
  //conformance:all
  //java:tests
  //:protobuf_test
  @com_google_protobuf_examples//...
)

set -eu
set -o pipefail

# Wrapper to run bazel, conditionally adding flags on Kokoro.
function _bazel() {
  local -a _flags
  # If we have auth credentials for result logging, set the appropriate flags.
  if [[ -n ${KOKORO_BAZEL_AUTH_CREDENTIAL:-} ]]; then
    # Any flags need to be added before the terminating "--".
    while (( ${#@} > 0 )); do
      if [[ $1 == "--" ]]; then
        break
      fi
      _flags+=( $1 )
      shift
    done
    local _invocation_id=$(uuidgen)
    echo ${_invocation_id} >> ${KOKORO_ARTIFACTS_DIR}/bazel_invocation_ids
    _flags+=(
      --google_default_credentials=true
      --google_credentials=${KOKORO_BAZEL_AUTH_CREDENTIAL}
      --google_auth_scopes=https://www.googleapis.com/auth/cloud-source-tools
      --invocation_id=${_invocation_id}
    )
  fi

  echo -n "[[ $(date) ]]"
  ( set -x ; ${BAZEL:-bazel} "${_flags[@]}" "${@}" )
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
