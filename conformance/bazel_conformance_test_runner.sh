#!/bin/bash
# This is an internal file that should only be called from Bazel rules.  For
# custom conformance tests outside of Bazel use CMAKE with
# -Dprotobuf_BUILD_CONFORMANCE=ON to build the test runner.

set -x
echo $@

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

TESTEE=unset
FAILURE_LIST=unset
TEXT_FORMAT_FAILURE_LIST=unset

while [[ -n "$@" ]]; do
  arg="$1"; shift
  val="$1"; shift
  case "$arg" in
    "--testee") TESTEE="$val" ;;
    "--failure_list") FAILURE_LIST="$val" ;;
    "--text_format_failure_list") TEXT_FORMAT_FAILURE_LIST="$val" ;;
    *) echo "Flag $arg is not recognized." && exit 1 ;;
  esac
done

conformance_test_runner=$(rlocation com_google_protobuf/conformance/conformance_test_runner)
conformance_testee=$(rlocation $TESTEE)
args=(--enforce_recommended)

failure_list=$(rlocation $FAILURE_LIST) || unset
if [ -n "$failure_list" ] ; then
  args+=(--failure_list $failure_list)
fi

text_format_failure_list=$(rlocation $TEXT_FORMAT_FAILURE_LIST) || unset
if [ -n "$text_format_failure_list" ]; then
  args+=(--text_format_failure_list $text_format_failure_list)
fi

$conformance_test_runner "${args[@]}" $conformance_testee
