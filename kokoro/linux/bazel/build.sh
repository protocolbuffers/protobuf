#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

git submodule update --init --recursive
bazel test :protobuf_test
cat /tmpfs/tmp/bazel/execroot/com_google_protobuf/bazel-out/k8-fastbuild/testlogs/protobuf_test/test.log
