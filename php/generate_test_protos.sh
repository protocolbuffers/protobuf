#!/bin/bash

set -e

cd `dirname $0`

if [[ -d tmp && -z $(find tests/proto ../src/protoc -newer tmp) ]]; then
  # Generated protos are already present and up to date, so we can skip protoc.
  #
  # Protoc is very fast, but sometimes it is not available (like if we haven't
  # built it in Docker). Skipping it helps us proceed in this case.
  echo "Test protos are up-to-date, skipping protoc."
  exit 0
fi

rm -rf tmp
mkdir -p tmp

find tests/proto -type f -name "*.proto"| xargs ../src/protoc --php_out=tmp -I../src -Itests

if [ "$1" = "--aggregate_metadata" ]; then
  # Overwrite some of the files to use aggregation.
  AGGREGATED_FILES="tests/proto/test.proto tests/proto/test_include.proto tests/proto/test_import_descriptor_proto.proto"
  ../src/protoc --php_out=aggregate_metadata=foo#bar:tmp -I../src -Itests $AGGREGATED_FILES
fi

echo "Generated test protos from tests/proto -> tmp"
