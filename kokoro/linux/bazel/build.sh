#!/bin/bash
#
# Build file to set up and run tests
set -ex

# Install the latest Bazel version available
use_bazel.sh latest
bazel version

# Change to repo root
cd $(dirname $0)/../../..

git submodule update --init --recursive
bazel test :protobuf_test --copt=-Werror --host_copt=-Werror

cd examples
bazel build :all
