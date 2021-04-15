#!/bin/bash

set -x
set -euo pipefail
# --- begin runfiles.bash initialization ---
if [[ ! -d "${RUNFILES_DIR:-/dev/null}" && ! -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
    if [[ -f "$0.runfiles_manifest" ]]; then
      export RUNFILES_MANIFEST_FILE="$0.runfiles_manifest"
    elif [[ -f "$0.runfiles/MANIFEST" ]]; then
      export RUNFILES_MANIFEST_FILE="$0.runfiles/MANIFEST"
    elif [[ -f "$0.runfiles/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
      export RUNFILES_DIR="$0.runfiles"
    fi
fi
if [[ -f "${RUNFILES_DIR:-/dev/null}/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
  source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"
elif [[ -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
  source "$(grep -m1 "^bazel_tools/tools/bash/runfiles/runfiles.bash " \
            "$RUNFILES_MANIFEST_FILE" | cut -d ' ' -f 2-)"
else
  echo >&2 "ERROR: cannot find @bazel_tools//tools/bash/runfiles:runfiles.bash"
  exit 1
fi
# --- end runfiles.bash initialization ---

conformance_test_runner=$(rlocation com_google_protobuf/conformance_test_runner)
conformance_testee=$(rlocation com_google_protobuf$TESTEE)
args=(--enforce_recommended)

failure_list=$(rlocation com_google_protobuf/conformance/failure_list_$LANG.txt)
if [ "$failure_list" != "1" ] ; then
  args+=(--failure_list $failure_list)
fi

text_format_failure_list=$(rlocation com_google_protobuf/conformance/text_format_failure_list_$LANG.txt)
if [ "$text_format_failure_list" != "1" ]; then
  args+=(--text_format_failure_list $text_format_failure_list)
fi

$conformance_test_runner "${args[@]}" $conformance_testee
