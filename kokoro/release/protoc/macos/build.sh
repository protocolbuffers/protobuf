#!/bin/bash

set -ex
CXXFLAGS_COMMON="-std=c++14 -DNDEBUG -mmacosx-version-min=10.9"

cd github/protobuf

bazel build //:protoc --dynamic_mode=off
mkdir -p build64/src
cp bazel-bin/protoc build64/src/protoc
file bazel-bin/protoc
otool -L bazel-bin/protoc | grep dylib
