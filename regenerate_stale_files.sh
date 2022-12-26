#!/bin/bash

# This script runs the staleness tests and uses them to update any stale
# generated files.

set -ex

# Cd to the repo root.
cd $(dirname -- "$0")

# Run and fix all staleness tests.
bazel test //src:cmake_lists_staleness_test || ./bazel-bin/src/cmake_lists_staleness_test --fix
bazel test //src/google/protobuf:well_known_types_staleness_test || ./bazel-bin/src/google/protobuf/well_known_types_staleness_test --fix
