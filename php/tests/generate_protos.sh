#!/bin/bash

set -ex

cd `dirname $0`

rm -rf generated
mkdir -p generated

find proto -type f -name "*.proto"| xargs ../../src/protoc --experimental_allow_proto3_optional --php_out=generated -I../../src -I.

if [ "$1" = "--aggregate_metadata" ]; then
  # Overwrite some of the files to use aggregation.
  AGGREGATED_FILES="proto/test.proto proto/test_include.proto proto/test_import_descriptor_proto.proto"
  ../../src/protoc --experimental_allow_proto3_optional --php_out=aggregate_metadata=foo#bar:generated -I../../src -I. $AGGREGATED_FILES
fi
