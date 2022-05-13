#!/bin/bash
#
# Build file to set up and run tests
set -ex

# Install Bazel 4.0.0.
use_bazel.sh 5.1.1
bazel version

# Change to repo root
cd $(dirname $0)/../../..

git submodule update --init --recursive

#  Disabled for now, re-enable if appropriate.
#  //:build_files_updated_unittest \

export SCRIPT_ROOT=$(pwd)
${SCRIPT_ROOT}/kokoro/common/bazel_test.sh

exit 0

# Verify that we can build successfully from generated tar files.
./autogen.sh && ./configure && make -j$(nproc) dist
DIST=`ls *.tar.gz`
tar -xf $DIST
cd ${DIST//.tar.gz}
bazel build //:protobuf //:protobuf_java
