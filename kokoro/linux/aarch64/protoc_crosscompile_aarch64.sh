#!/bin/bash
#
# Builds protobuf C++ with aarch64 crosscompiler.

set -ex

bazel build --cpu=aarch64 //:protoc
export PROTOC=$PWD/bazel-bin/protoc
