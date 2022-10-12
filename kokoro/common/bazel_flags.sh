#!/bin/bash

# Helper for setting up common bazel flags in Kokoro.
#
# This script prints extra flags to a bazel invocation when it is run from
# Kokoro.  When the special environment variables are not present (e.g., if you
# run Kokoro build scripts locally), this script only flips some debug settings.
#
# Example of running directly:
#   bazel test $(path/to/bazel_flags.sh) //...

function bazel_flags::gen_invocation_id() {
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
function bazel_flags::kokoro_flags() {
  [[ -n ${KOKORO_JOB_NAME:-} ]] || return

  local -a _flags
  _flags+=(
    --invocation_id=$(bazel_flags::gen_invocation_id)
    --remote_cache=https://storage.googleapis.com/protobuf-bazel-cache/${KOKORO_JOB_NAME}
  )
  if [[ -n ${KOKORO_BAZEL_AUTH_CREDENTIAL:-} ]]; then
    _flags+=( --google_credentials=${KOKORO_BAZEL_AUTH_CREDENTIAL} )
  else
    _flags+=( --google_default_credentials=true )
  fi

  echo "${_flags[@]}"
}

echo "$(bazel_flags::kokoro_flags) --keep_going --test_output=errors"
