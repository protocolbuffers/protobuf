#!/bin/bash
#
# Build file to set up and run tests based on distribution archive

set -eux

# Change to repo root
cd $(dirname $0)/../../..

#
# Update git submodules
#
git submodule update --init --recursive

#
# Build distribution archive
#
# TODO: this should use Bazel-built dist archives.
date ; ./autogen.sh
date ; ./configure
date ; make dist
date

DIST_ARCHIVE=( $(ls protobuf-*.tar.gz) )
if (( ${#DIST_ARCHIVE[@]} != 1 )); then
  echo >&2 "Distribution archive not found. ${#DIST_ARCHIVE[@]} matches:"
  echo >&2 "${DIST_ARCHIVE[@]}"
  exit 1
fi

#
# Check for all expected files
#
kokoro/common/check_missing_dist_files.sh ${DIST_ARCHIVE}

#
# Extract to a temporary directory
#
if [[ -z ${DIST_WORK_ROOT:-} ]]; then
  # If you want to preserve the extracted sources, set the DIST_WORK_ROOT
  # environment variable to an existing directory that should be used.
  DIST_WORK_ROOT=$(mktemp -d)
  function cleanup_work_root() {
    echo "Cleaning up temporary directory ${DIST_WORK_ROOT}..."
    rm -rf ${DIST_WORK_ROOT}
  }
  trap cleanup_work_root EXIT
fi

tar -C ${DIST_WORK_ROOT} --strip-components=1 -axf ${DIST_ARCHIVE}

#
# Run tests using extracted sources
#
if SOURCE_DIR=${DIST_WORK_ROOT} \
  CMAKE_GENERATOR=Ninja \
  CTEST_PARALLEL_LEVEL=$(nproc) \
  kokoro/common/cmake.sh; then
  # TODO: remove this conditional.
  # The cmake build is expected to fail due to missing abseil sources.
  echo "$0: Expected failure, but build passed." >&2
  echo "Please update $(basename $0) to remove failure expectation." >&2
  echo "FAIL" >&2
  exit 1
fi
echo "PASS"
