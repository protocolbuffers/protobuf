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
# Run tests under release docker image.
#
DOCKER_IMAGE_NAME=protobuf/protoc_$(sha1sum protoc-artifacts/Dockerfile | cut -f1 -d " ")
until docker pull $DOCKER_IMAGE_NAME; do sleep 10; done

# Some flags are set conditionally.
DOCKER_FLAGS=(
  # Set variables used by cmake/ctest:
  -e CMAKE_GENERATOR=Ninja
  -e CTEST_PARALLEL_LEVEL=$(nproc)

  # Work inside our extracted sources.
  -v ${DIST_WORK_ROOT}:/var/local/protobuf

  # Kokoro scripts are not part of the distribution. We only need common:
  -v ${PWD}/kokoro/common:/var/local/protobuf/kokoro/common
)
if [[ -n ${KOKORO_ARTIFACTS_DIR:-} ]]; then
  # Expose KOKORO_ARTIFACTS_DIR at the same mount point for caplog.
  DOCKER_FLAGS+=(
    -e KOKORO_ARTIFACTS_DIR
    -v ${KOKORO_ARTIFACTS_DIR}:${KOKORO_ARTIFACTS_DIR}
  )
fi

docker run --rm \
  ${DOCKER_FLAGS[@]} \
  $DOCKER_IMAGE_NAME \
  /var/local/protobuf/kokoro/common/cmake.sh
