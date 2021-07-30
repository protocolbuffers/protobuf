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

trap print_test_logs EXIT
bazel test --copt=-Werror --host_copt=-Werror \
  //:build_files_updated_unittest \
  //java:tests \
  //:protoc \
  //:protobuf \
  //:protobuf_python \
  //:protobuf_test \
  @com_google_protobuf//:cc_proto_blacklist_test
trap - EXIT

cd examples
bazel build //...
