#!/bin/bash
#
# Build file to set up and run tests
set -ex

# Change to repo root
cd $(dirname $0)/../../..

git submodule update --init --recursive
bazel test :protobuf_test --copt=-Werror --host_copt=-Werror

cd examples
bazel build :all
