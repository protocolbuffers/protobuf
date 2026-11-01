#!/bin/bash
# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
# Runs a test executable produced by the rules_foreign_cc CMake build
# (//cmake:protobuf_cmake) the way `ctest` would.
#
# CMake's `add_test()` entries in cmake/tests.cmake run from the protobuf
# source directory, and historically the CI build was in-source, so tests
# assume both of these hold for the current working directory:
#   * `src/google/protobuf/descriptor.cc` is reachable by walking up from
#     `.` (see TestSourceDir() in src/google/protobuf/testing/googletest.cc);
#   * helper executables (`test_plugin`, `fake_plugin`, `protoc`, ...) are
#     siblings in `.` (see CreatePluginArg() in
#     command_line_interface_unittest.cc).
# We recreate that layout under $TEST_TMPDIR with symlinks.
#
# Usage:
#   run_cmake_test.sh <source-tree-anchor> <test-binary> [helper-binary...] \
#       -- [args passed to the test binary...]
#
#   <source-tree-anchor> is any file at the root of the staged source tree
#   (we use CMakeLists.txt); its directory is the source tree.
#
# If PROTOBUF_CMAKE_LIB_DIR is set (shared-library configurations), it is the
# runfiles-relative directory holding the installed shared libraries, which is
# put on LD_LIBRARY_PATH for the test binary and the helpers it executes.

set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "usage: $0 <source-tree-anchor> <test-binary> [helper-binary...] -- [args...]" >&2
  exit 2
fi

SRC_TREE="$(cd "$(dirname "$1")" && pwd -P)"
TEST_BINARY="$(realpath "$2")"
shift 2

if [[ -n "${PROTOBUF_CMAKE_LIB_DIR:-}" ]]; then
  export LD_LIBRARY_PATH="$(realpath "${PROTOBUF_CMAKE_LIB_DIR}")${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
fi

HELPERS=()
while [[ $# -gt 0 && "$1" != "--" ]]; do
  HELPERS+=("$(realpath "$1")")
  shift
done
if [[ $# -gt 0 ]]; then
  shift  # drop "--"
fi

WORK_DIR="${TEST_TMPDIR:-/tmp}/cmake_test_workdir"
rm -rf "${WORK_DIR}"
mkdir -p "${WORK_DIR}"

# Mirror the top level of the source tree (src/, conformance/, ...).
for entry in "${SRC_TREE}"/* "${SRC_TREE}"/.[!.]*; do
  [[ -e "${entry}" ]] || continue
  ln -s "${entry}" "${WORK_DIR}/$(basename "${entry}")"
done

# Put the test binary and its helpers side by side, as an in-source build would.
ln -sf "${TEST_BINARY}" "${WORK_DIR}/$(basename "${TEST_BINARY}")"
for helper in "${HELPERS[@]}"; do
  ln -sf "${helper}" "${WORK_DIR}/$(basename "${helper}")"
done

cd "${WORK_DIR}"
export srcdir="${WORK_DIR}/src"
exec "./$(basename "${TEST_BINARY}")" "$@"
