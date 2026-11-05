#!/bin/bash
# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
# Smoke test for a cmake_bazel() install directory of protobuf: the installed
# protoc runs, and the installed CMake package config and libraries exist.
#
# Usage: cmake_bazel_smoke_test.sh <install dir>

set -euo pipefail

prefix="$1"

for f in \
    bin/protoc \
    lib/cmake/protobuf/protobuf-config.cmake \
    lib/libprotobuf.a \
    lib/libprotobuf-lite.a \
    lib/libprotoc.a \
    lib/libupb.a \
    include/google/protobuf/message.h; do
  if [[ ! -e "${prefix}/${f}" ]]; then
    echo "missing from the install directory: ${f}" >&2
    exit 1
  fi
done

"${prefix}/bin/protoc" --version
