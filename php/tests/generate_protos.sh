#!/bin/bash

set -ex

cd `dirname $0`

AGGREGATE_METADATA=""

if [ "$1" = "--aggregate_metadata" ]; then
  AGGREGATE_METADATA="aggregate_metadata=foo#bar:"
fi

rm -rf generated
mkdir -p generated
find proto -type f | xargs ../../src/protoc --php_out=${AGGREGATE_METADATA}generated -I../../src -I.
../../src/protoc --php_out=${AGGREGATE_METADATA}generated -I../../src -I. proto/test_import_descriptor_proto.proto
