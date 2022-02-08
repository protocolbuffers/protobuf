#!/bin/bash

set -ex

cd `dirname $0`

if ../src/protoc --help > /dev/null; then
  PROTOC=src/protoc
else
  # Bazel seems to be creating a problematic symlink in
  # _build/out/external/com_google_protobuf, so we remove the _build directory
  # before building protoc.
  (cd .. && bazel build -c opt :protoc)
  PROTOC=bazel-bin/protoc
fi

if [[ -d tmp && -z $(find tests/proto ../$PROTOC -newer tmp) ]]; then
  # Generated protos are already present and up to date, so we can skip protoc.
  #
  # Protoc is very fast, but sometimes it is not available (like if we haven't
  # built it in Docker). Skipping it helps us proceed in this case.
  echo "Test protos are up-to-date, skipping protoc."
  exit 0
fi

rm -rf tmp
mkdir -p tmp

cd ..
find php/tests/proto -type f -name "*.proto"| xargs $PROTOC --php_out=php/tmp -Isrc -Iphp/tests

if [ "$1" = "--aggregate_metadata" ]; then
  # Overwrite some of the files to use aggregation.
  AGGREGATED_FILES="tests/proto/test.proto tests/proto/test_include.proto tests/proto/test_import_descriptor_proto.proto"
  $PROTOC --php_out=aggregate_metadata=foo#bar:php/tmp -Isrc -Iphp/tests $AGGREGATED_FILES
fi

echo "Generated test protos from tests/proto -> tmp"
