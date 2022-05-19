#!/bin/bash
#
# Build file to set up and run tests
set -ex

# Install Bazel 4.0.0.
use_bazel.sh 4.0.0
bazel version

# Change to repo root
cd $(dirname $0)/../../..

git submodule update --init --recursive

#  Disabled for now, re-enable if appropriate.
#  //:build_files_updated_unittest \

bazel test \
  -k --copt=-Werror --host_copt=-Werror --test_output=errors \
  //build_defs:all \
  //java:tests \
  //src/... \
  //:protobuf_python \
  @com_google_protobuf_examples//...

# Verify that we can build successfully from generated tar files.
./autogen.sh && ./configure && make -j$(nproc) dist
DIST=`ls *.tar.gz`
tar -xf $DIST
cd ${DIST//.tar.gz}
bazel build //:protobuf //:protobuf_java
