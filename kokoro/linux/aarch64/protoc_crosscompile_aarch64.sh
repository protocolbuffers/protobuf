#!/bin/bash
#
# Builds protobuf C++ with aarch64 crosscompiler.

set -ex

./autogen.sh
CXXFLAGS="-fPIC -g -O2" ./configure --host=aarch64
make -j8
