#!/bin/bash
#
# Build file to set up and run tests

set -eux
set -o pipefail

# Run from the project root directory.
cd $(dirname $0)/../../..

# Prepare worker environment to run tests
source kokoro/macos/prepare_build_macos_rc

#
# Run build
#
bazel test $(kokoro/common/bazel_flags.sh) //src/...
