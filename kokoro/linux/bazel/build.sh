#!/bin/bash
#
# Build file to set up and run tests
set -ex

# Install Bazel 4.0.0.
use_bazel.sh 4.0.0
bazel version

# Print bazel testlogs to stdout when tests failed.
function print_test_logs {
  # TODO(yannic): Only print logs of failing tests.
  testlogs_dir=$(bazel info bazel-testlogs)
  testlogs=$(find "${testlogs_dir}" -name "*.log")
  for log in $testlogs; do
    cat "${log}"
  done
}

# Change to repo root
cd $(dirname $0)/../../..

git submodule update --init --recursive

#  Disabled for now, re-enable if appropriate.
#  //:build_files_updated_unittest \

trap print_test_logs EXIT
bazel test -k --copt=-Werror --host_copt=-Werror \
  //build_defs:all \
  //java:tests \
  //:protoc \
  //:protobuf \
  //:protobuf_python \
  //:protobuf_test
trap - EXIT

pushd examples
bazel build //...
popd

# Verify that we can build successfully from generated tar files.
./autogen.sh && ./configure && make -j$(nproc) dist
DIST=`ls *.tar.gz`
tar -xf $DIST
cd ${DIST//.tar.gz}
bazel build //:protobuf //:protobuf_java
