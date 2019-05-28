#!/bin/bash
#
# Build file to set up and run tests
set -ex

# Change to repo root
cd $(dirname $0)/../../..

git submodule update --init --recursive
use_bazel.sh 0.25.3
bazel test :protobuf_test --copt=-Werror --host_copt=-Werror

cd examples
bazel build :all
