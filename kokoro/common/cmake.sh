#!/bin/bash
#
# Build tests under CMake.
#
# This script is used from macos and linux builds. It runs cmake and ctest in
# the current directory. Any additional setup should be done before running this
# script.
#
# This script uses `caplog` to save logfiles. See caplog.sh for details.

set -eu -o pipefail
: ${SCRIPT_ROOT:=$(cd $(dirname $0)/../..; pwd)}

################################################################################
# If you are using this script to run tests, you can set some environment
# variables to control behavior:
#
# By default, find the sources based on this script's path.
: ${SOURCE_DIR:=${SCRIPT_ROOT}}
#
# By default, put outputs under <git root>/cmake/build.
: ${BUILD_DIR:=${SOURCE_DIR}/cmake/build}
#
# CMAKE_BUILD_TYPE is supported in cmake 3.22+. If set, we pass the value of this
# variable explicitly for compatibility with older versions of cmake. See:
#   https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html
# (N.B.: not to be confused with CMAKE_CONFIG_TYPE.)
if [[ -n ${CMAKE_BUILD_TYPE:-} ]]; then
  CMAKE_BUILD_TYPE_FLAG="-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
else
  CMAKE_BUILD_TYPE_FLAG=
fi
#
# For several other CMake options, see docs here:
#   https://cmake.org/cmake/help/latest/manual/cmake-env-variables.7.html
#
# Some variables you may want to override (see cmake docs for details):
#   CMAKE_BUILD_PARALLEL_LEVEL
#   CMAKE_CONFIG_TYPE (N.B.: not to be confused with CMAKE_BUILD_TYPE)
#   CMAKE_GENERATOR
#   CTEST_PARALLEL_LEVEL
################################################################################

echo "Building using..."
echo "       Sources: ${SOURCE_DIR}"
echo "  Build output: ${BUILD_DIR}"
if [[ ${SOURCE_DIR} != ${SCRIPT_ROOT} ]]; then
  echo " Build scripts: ${SCRIPT_ROOT}"
fi
set -x
source ${SCRIPT_ROOT}/kokoro/common/caplog.sh

#
# Configure under $BUILD_DIR:
#
mkdir -p "${BUILD_DIR}"

(
  cd "${BUILD_DIR}"
  caplog 01_configure \
    cmake -S "${SOURCE_DIR}" \
    ${CMAKE_BUILD_TYPE_FLAG} \
    ${CAPLOG_CMAKE_ARGS:-} \
    -DCMAKE_CXX_STANDARD=14
)
if [[ -n ${CAPLOG_DIR:-} ]]; then
  # Save configuration logs.
  mkdir -p "${CAPLOG_DIR}/CMakeFiles"
  cp "${BUILD_DIR}"/CMakeFiles/CMake*.log "${CAPLOG_DIR}/CMakeFiles"
fi

#
# Build:
#
caplog 02_build \
  cmake --build "${BUILD_DIR}"

#
# Run tests
#
(
  cd "${BUILD_DIR}"
  caplog 03_combined_testlog \
    ctest ${CAPLOG_CTEST_ARGS:-}
)
