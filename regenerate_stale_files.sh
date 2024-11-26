#!/bin/bash

# This script runs the staleness tests and uses them to update any stale
# generated files.

set -ex

echo "::group::Regenerate stale files"

# Cd to the repo root.
cd $(dirname -- "$0")

readonly BazelBin="${BAZEL:-bazel} ${BAZEL_STARTUP_FLAGS}"

STALENESS_TESTS=(
  "csharp:generated_csharp_defaults_staleness_test"
  "java/core:generated_java_defaults_staleness_test"
  "upb/reflection:bootstrap_upb_defaults_staleness_test"
  "cmake:test_dependencies_staleness"
  "src:cmake_lists_staleness_test"
  "src/google/protobuf:well_known_types_staleness_test"
  "objectivec:well_known_types_staleness_test"
  "php:test_amalgamation_staleness"
  "php:proto_staleness_test"
  "ruby/ext/google/protobuf_c:test_amalgamation_staleness"
  "upb/cmake:test_generated_files"
  "upb/reflection:descriptor_upb_proto_staleness_test"
  "upb_generator:plugin_upb_proto_staleness_test"
)

# Run and fix all staleness tests.
for test in ${STALENESS_TESTS[@]}; do
  ${BazelBin} test $test "$@" || ./bazel-bin/${test%%:*}/${test#*:} --fix
done

# Generate C# code.
# This doesn't currently have Bazel staleness tests, but there's an existing
# shell script that generates everything required. The output files are stable,
# so just regenerating in place should be harmless. 
${BazelBin} build src/google/protobuf/compiler:protoc "$@"
(export PROTOC=$PWD/bazel-bin/protoc && cd csharp && ./generate_protos.sh)

echo "::endgroup::"
