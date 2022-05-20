# Wrapper for invoking bazel on Kokoro.
#
# This file should be `source`d. It provides a function `bazel` which adds flags
# before invoking the real bazel.

# Set the `BAZEL` env variable to override the actual bazel binary to use:
: ${BAZEL:=$(which bazel)}

# Prints flags to use on Kokoro.
function kokoro_bazel_flags() {
  [[ -n ${KOKORO_BES_PROJECT_ID:-} ]] || return

  # Create a new invocation ID and store in the artifacts dir.
  local _invocation_id=$(uuidgen | tr A-Z a-z)
  echo ${_invocation_id} >> ${KOKORO_ARTIFACTS_DIR}/bazel_invocation_ids

  local -a _flags
  _flags+=(
    --bes_backend=${KOKORO_BES_BACKEND_ADDRESS:-buildeventservice.googleapis.com}
    --bes_results_url=https://source.cloud.google.com/results/invocations/
    --invocation_id=${_invocation_id}
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

# Prints and reverses invocation IDs.
#
# Typical usage:
#   trap cleanup_invocation_ids ERR
#   bazel ...
#   cleanup_invocation_ids
#
# If there are multiple invocation IDs, only the first one might be used. Since
# the last command is usually the pertinent one (especially in case of a
# failure), we reverse the lines as well.
function cleanup_invocation_ids() {
  [[ -n ${KOKORO_ARTIFACTS_DIR:-} ]] || return
  local _ids_file=${KOKORO_ARTIFACTS_DIR}/bazel_invocation_ids
  [[ -e ${_ids_file} ]] || return
  local _id
  while read _id ; do
    echo https://source.cloud.google.com/results/invocations/${_id}
  done < ${_ids_file}
  local _temp_ids=$(mktemp)
  tac ${_ids_file} >> ${_temp_ids}
  mv ${_temp_ids} ${_ids_file}
}

# Runs bazel with Kokoro flags, if appropriate.
function bazel() {
  local -a _flags
  # We might need to add flags. They need to come after any startup flags and
  # the command, but before any terminating "--", so copy them into the _flags
  # variable.
  until (( ${#@} == 0 )) || [[ $1 == "--" ]]; do
    _flags+=( "${1}" ); shift
  done
  ${BAZEL} "${_flags[@]}" $(kokoro_bazel_flags) "${@}"
}
