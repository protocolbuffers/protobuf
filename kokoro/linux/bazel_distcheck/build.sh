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
  //:protobuf_test
  //src/google/protobuf/compiler/...
  //src/google/protobuf/io:all
  //src/google/protobuf/stubs:all
  //src/google/protobuf/testing:all
  //src/google/protobuf/util/...
  @com_google_protobuf_examples//...
)

CONTAINER_NAME=gcr.io/protobuf-build/bazel/linux
CONTAINER_VERSION=5.1.1-e41ccfa1648716433276ebe077c665796550fcbb

use_bazel.sh 5.0.0 || true
bazel version

# Change to repo root
cd $(dirname $0)/../../..

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

# Let Bazel share the distdir.
TMP_DISTDIR=${DIST_WORK_ROOT}/bazel-distdir
mkdir -p ${TMP_DISTDIR}

# Build distribution archive
date
bazel fetch --distdir=${TMP_DISTDIR} //pkg:dist_all_tar
bazel build --distdir=${TMP_DISTDIR} //pkg:dist_all_tar
DIST_ARCHIVE=$(readlink $(bazel info bazel-bin)/pkg/dist_all_tar.tar.gz)
bazel shutdown

# The `pkg_tar` rule emits a symlink based on the rule name. The actual
# file is named with the current version.
date
echo "Resolved archive path: ${DIST_ARCHIVE}"

# Extract the dist archive.
date
DIST_WORKSPACE=${DIST_WORK_ROOT}/protobuf
mkdir -p ${DIST_WORKSPACE}
tar -C ${DIST_WORKSPACE} --strip-components=1 -axf ${DIST_ARCHIVE}

# Perform build steps in the extracted dist sources.

cd ${DIST_WORKSPACE}
FAILED=false

until docker pull gcr.io/protobuf-build/bazel/linux:${CONTAINER_VERSION}; do
  sleep 10
done

date
docker run --rm \
  -v ${DIST_WORKSPACE}:/workspace \
  -v ${TMP_DISTDIR}:${TMP_DISTDIR} \
  ${CONTAINER_NAME}:${CONTAINER_VERSION} \
  test --distdir=${TMP_DISTDIR} --test_output=errors -k \
  "${BUILD_ONLY_TARGETS[@]}" "${TEST_TARGETS[@]}" || FAILED=true

if ${FAILED}; then
   echo FAILED
   exit 1
fi
echo PASS
