#!/bin/bash -eux
#
# Build file to set up and run tests

set -o pipefail

if [[ -h /tmpfs ]] && [[ ${PWD} == /tmpfs/src ]]; then
  # Workaround for internal Kokoro bug: b/227401944
  cd /Volumes/BuildData/tmpfs/src
fi

# Default environment variables used by cmake build:
: ${CMAKE_CONFIG_TYPE:=Debug}
export CMAKE_CONFIG_TYPE
: ${CTEST_PARALLEL_LEVEL:=4}
export CTEST_PARALLEL_LEVEL

# Run from the project root directory.
cd $(dirname $0)/../../..

#
# Update submodules and regenerate files
#
git submodule update --init --recursive
./regenerate_stale_files.sh

# TODO(b/265232445) Remove this once the patch is upstreamed
sed -i '' 's:random/mock_helpers.h:random/internal/mock_helpers.h:' third_party/abseil-cpp/CMake/AbseilDll.cmake
sed -i '' 's:ABSL_INTERNA_TEST_DLL_TARGETS:ABSL_INTERNAL_TEST_DLL_TARGETS:' third_party/abseil-cpp/CMake/AbseilDll.cmake

#
# Run build
#
kokoro/common/cmake.sh
