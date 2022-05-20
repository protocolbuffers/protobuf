#!/bin/bash
#
# Builds protobuf C++ with aarch64 crosscompiler.

set -ex

use_bazel.sh 5.1.1
bazel build --cpu=aarch64 //:protoc
export PROTOC=$PWD/bazel-bin/protoc
