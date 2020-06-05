#!/bin/bash

set -ex

cd `dirname $0`
rm -rf generated
mkdir -p generated
find proto -type f | xargs ../../src/protoc --php_out=generated -I../../src -I.
../../src/protoc --php_out=generated -I../../src -I. proto/test_import_descriptor_proto.proto
