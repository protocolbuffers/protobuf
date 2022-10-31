#!/bin/bash
#
# This is the top-level script we give to Kokoro as the entry point for
# running the "continuous" and "presubmit" jobs.

set -ex

# Change to repo root
cd $(dirname $0)/../../..

# Run the staleness tests and use them to update any stale files.
bazel test //src:cmake_lists_staleness_test || ./bazel-bin/src/cmake_lists_staleness_test --fix
bazel test //src/google/protobuf:well_known_types_staleness_test || ./bazel-bin/src/google/protobuf/well_known_types_staleness_test --fix

kokoro/linux/aarch64/qemu_helpers/prepare_qemu.sh

kokoro/linux/aarch64/test_python_aarch64.sh
