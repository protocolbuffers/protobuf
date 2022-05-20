#!/bin/bash
#
# Build file to set up and run tests using bazel-build dist archive
#
# Note that the builds use WORKSPACE to fetch external sources, not
# git submodules.

set -eu

use_bazel.sh 5.0.0 || true
bazel version

# Change to repo root
cd $(dirname $0)/../../..
source kokoro/common/pyenv.sh
source kokoro/common/bazel_wrapper.sh

# Build distribution archive
echo "============================================================"
echo -e "[[ $(date) ]] Building distribution archive...\n"
bazel build //pkg:dist_all_tar
DIST_ARCHIVE=$(readlink $(${BAZEL} info bazel-bin)/pkg/dist_all_tar.tar.gz)
${BAZEL} shutdown

# Extract the dist archive.
echo "============================================================"
echo -e "[[ $(date) ]] Extracting distribution archive...\n"

# Construct temp directory for running the dist build.
# If you want to run locally and keep the build dir, create a directory
# and pass it in the DIST_WORK_ROOT env var.
if [[ -z ${DIST_WORK_ROOT:-} ]]; then
  : ${DIST_WORK_ROOT:=$(mktemp -d)}
  function dist_cleanup() {
    rm -rf ${DIST_WORK_ROOT}
    cleanup_invocation_ids
  }
  trap dist_cleanup ERR
else
  function dist_cleanup() { :; }
  trap cleanup_invocation_ids ERR
fi

DIST_WORKSPACE=${DIST_WORK_ROOT}/protobuf
mkdir -p ${DIST_WORKSPACE}
tar -C ${DIST_WORKSPACE} --strip-components=1 -axf bazel-bin/pkg/dist_all_tar.tar.gz

echo "============================================================"
echo -e "[[ $(date) ]] Building extracted archive...\n"

cd ${DIST_WORKSPACE}

bazel_args=(
  test
  --keep_going
  --test_output=errors
  --
  //...
  -//objectivec/...  # only works on macOS
  @com_google_protobuf_examples//...
)
bazel "${bazel_args[@]}"
dist_cleanup
