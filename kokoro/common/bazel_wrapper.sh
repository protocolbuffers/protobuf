#!/bin/bash

# Wrapper for invoking bazel on Kokoro.
#
# This script adds extra flags to a bazel invocation when it is run from Kokoro.
# When the special environment variables are not present (e.g., if you run
# Kokoro build scripts locally), this script is equivalent to the "bazel"
# command.
#
# Example of running directly:
#   path/to/bazel_wrapper.sh build //...
#
# Example of `source`ing:
#   source path/to/bazel_wrapper.sh
#   bazel_wrapper build //...

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
  [[ -n ${KOKORO_BES_PROJECT_ID:-} ]] || return

  local -a _flags
  _flags+=(
    --bes_backend=${KOKORO_BES_BACKEND_ADDRESS:-buildeventservice.googleapis.com}
    --bes_results_url=https://source.cloud.google.com/results/invocations/
    --invocation_id=$(bazel_wrapper::gen_invocation_id)
    --project_id=${KOKORO_BES_PROJECT_ID}  # --bes_instance_name in Bazel 5+
    --remote_cache=https://storage.googleapis.com/protobuf-bazel-cache
  )
  if [[ -n ${KOKORO_BAZEL_AUTH_CREDENTIAL:-} ]]; then
    _flags+=( --google_credentials=${KOKORO_BAZEL_AUTH_CREDENTIAL} )
  else
    _flags+=( --google_default_credentials=true )
  fi

  echo "${_flags[@]}"
}

# Runs bazel with Kokoro flags, if appropriate.
function bazel_wrapper() {
  local -a _flags

  # We might need to add flags. They need to come after any startup flags and
  # the command, but before any terminating "--", so copy them into the _flags
  # variable.
  until (( ${#@} == 0 )) || [[ $1 == "--" ]]; do
    _flags+=( "${1}" ); shift
  done

  # Set the `BAZEL` env variable to override the actual bazel binary to use:
  ${BAZEL:=bazel} "${_flags[@]}" $(bazel_wrapper::kokoro_flags) "${@}"
}

# If this script was called directly, run bazel. Otherwise (i.e., this script
# was `source`d), the sourcing script will call bazel_wrapper themselves.
(( ${#BASH_SOURCE[@]} == 1 )) && bazel_wrapper "${@}"
