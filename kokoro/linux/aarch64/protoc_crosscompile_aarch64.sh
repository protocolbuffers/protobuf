#!/bin/bash
#
# Builds protobuf C++ with aarch64 crosscompiler.

set -ex

cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -Dprotobuf_WITH_ZLIB=0 .
make -j8

# The Java build setup expects the protoc binary to be in the src/ directory.
ln -s $PWD/protoc ./src/protoc
