#!/bin/bash

set -e

cd `dirname $0`

rm -rf tmp
mkdir -p tmp

find tests/proto -type f -name "*.proto"| xargs ../src/protoc --php_out=tmp -I../src -Itests

if [ "$1" = "--aggregate_metadata" ]; then
  # Overwrite some of the files to use aggregation.
  AGGREGATED_FILES="tests/proto/test.proto tests/proto/test_include.proto tests/proto/test_import_descriptor_proto.proto"
  ../src/protoc --php_out=aggregate_metadata=foo#bar:tmp -I../src -Itests $AGGREGATED_FILES
fi

echo "Generated test protos from tests/proto -> tmp"
