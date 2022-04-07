#!/bin/bash -eux
#
# Build file to set up and run tests

set -o pipefail

if [[ -h /tmpfs ]] && [[ ${PWD} == /tmpfs/src ]]; then
  # Workaround for internal Kokoro bug: b/227401944
  cd /Volumes/BuildData/tmpfs/src
fi

# These vars can be changed when running manually, e.g.:
#
#   % BUILD_CONFIG=RelWithDebInfo path/to/build.sh

# By default, build using Debug config.
: ${BUILD_CONFIG:=Debug}

# By default, find the sources based on this script path.
: ${SOURCE_DIR:=$(cd $(dirname $0)/../../..; pwd)}

# By default, put outputs under <git root>/cmake/build.
: ${BUILD_DIR:=${SOURCE_DIR}/cmake/build}

source ${SOURCE_DIR}/kokoro/caplog.sh

#
# Update submodules
#
git -C "${SOURCE_DIR}" submodule update --init --recursive

#
# Configure and build in a separate directory
#
mkdir -p "${BUILD_DIR}"

caplog 01_configure \
  cmake -S "${SOURCE_DIR}" -B "${BUILD_DIR}" ${CAPLOG_CMAKE_ARGS:-}

if [[ -n ${CAPLOG_DIR:-} ]]; then
  mkdir -p "${CAPLOG_DIR}/CMakeFiles"
  cp "${BUILD_DIR}"/CMakeFiles/CMake*.log "${CAPLOG_DIR}/CMakeFiles"
fi

caplog 02_build \
  cmake --build "${BUILD_DIR}" --config "${BUILD_CONFIG}"

#
# Run tests
#
(
  cd "${BUILD_DIR}"
  caplog 03_combined_testlog \
    ctest -C "${BUILD_CONFIG}" -j4 ${CAPLOG_CTEST_ARGS:-}
)
