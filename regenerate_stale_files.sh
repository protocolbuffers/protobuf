#!/bin/bash

# This script runs the staleness tests and uses them to update any stale
# generated files.

set -e

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
  "upb/reflection:descriptor_upb_proto_staleness_test"
  "upb/reflection:json_enumvalue_options_upb_proto_staleness_test"
  "upb_generator:plugin_upb_proto_staleness_test"
)

CSHARP_TARGETS=(
  "src/google/protobuf/compiler:protoc"
  "//csharp/protos/unittest_deep_dependencies:generate_cached_protos"
  "//csharp/protos/unittest_deep_dependencies:generate_notcached_protos"
)

# Build all staleness tests and C# codegen targets in a single Bazel invocation
# to avoid repeating Bazel startup banners, rc announcements, and warnings 16 times.
# shellcheck disable=SC2086
${BazelBin} build --ui_event_filters=-info --noshow_progress \
  "${STALENESS_TESTS[@]}" "${CSHARP_TARGETS[@]}" "$@"

# Run and fix all staleness tests.
for test in "${STALENESS_TESTS[@]}"; do
  # shellcheck disable=SC2086
  "./bazel-bin/${test%%:*}/${test#*:}" ${STALENESS_FIX_FLAGS:---fix}
done

# Generate C# code.
# This doesn't currently have Bazel staleness tests, but there's an existing
# shell script that generates everything required. The output files are stable,
# so just regenerating in place should be harmless.
(export PROTOC=$PWD/bazel-bin/protoc && cd csharp && ./generate_protos.sh)

echo "::endgroup::"
