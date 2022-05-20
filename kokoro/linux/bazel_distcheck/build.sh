#!/bin/bash
#
# Build file to set up and run tests using bazel-build dist archive
#
# Note that the builds use WORKSPACE to fetch external sources, not
# git submodules.

set -eux

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
  //python:all
  //src/...
   @com_google_protobuf_examples//...
)

use_bazel.sh 5.0.0 || true
bazel version

# Change to repo root
cd $(dirname $0)/../../..
source kokoro/common/pyenv.sh

# Build distribution archive
date
bazel build //pkg:dist_all_tar
bazel shutdown

# Construct temp directory for running the dist build.
# If you want to run locally and keep the build dir, create a directory
# and pass it in the DIST_WORK_ROOT env var.
if [[ -z ${DIST_WORK_ROOT:-} ]]; then
  : ${DIST_WORK_ROOT:=$(mktemp -d)}
  function dist_cleanup() {
    rm -rf ${DIST_WORK_ROOT}
  }
  trap dist_cleanup EXIT
fi

# Extract the dist archive.
date
DIST_WORKSPACE=${DIST_WORK_ROOT}/protobuf
mkdir -p ${DIST_WORKSPACE}
tar -C ${DIST_WORKSPACE} --strip-components=1 -axf bazel-bin/pkg/dist_all_tar.tar.gz

# Perform build steps in the extracted dist sources.

cd ${DIST_WORKSPACE}
FAILED=false

date
bazel test --test_output=errors -k \
  "${BUILD_ONLY_TARGETS[@]}" "${TEST_TARGETS[@]}" || FAILED=true

if ${FAILED}; then
   echo FAILED
   exit 1
fi
echo PASS
