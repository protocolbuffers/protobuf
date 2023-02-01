#!/bin/bash

# This script runs the staleness tests and uses them to update any stale
# generated files.

set -ex

# Cd to the repo root.
cd $(dirname -- "$0")

# Run and fix all staleness tests.
# LINT.IfChange(staleness_tests)
bazel test //src:cmake_lists_staleness_test "$@" || ./bazel-bin/src/cmake_lists_staleness_test --fix
bazel test //src/google/protobuf:well_known_types_staleness_test "$@" || ./bazel-bin/src/google/protobuf/well_known_types_staleness_test --fix
# LINT.ThenChange(//depot/google3/third_party/protobuf/github/kokoro/windows/prepare_build_win64.bat:staleness_tests)

# Generate C# code.
# This doesn't currently have Bazel staleness tests, but there's an existing
# shell script that generates everything required. The output files are stable,
# so just regenerating in place should be harmless. 
bazel build src/google/protobuf/compiler:protoc "$@"
(export PROTOC=$PWD/bazel-bin/protoc && cd csharp && ./generate_protos.sh)
