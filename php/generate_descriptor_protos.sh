#!/usr/bin/env bash

# Run this script to regenerate descriptor protos after the protocol compiler
# changes.

set -e

if test ! -e src/google/protobuf/stubs/common.h; then
  cat >&2 << __EOF__
Could not find source code.  Make sure you are running this script from the
root of the distribution tree.
__EOF__
  exit 1
fi

bazel-bin/protoc --php_out=internal:php/src google/protobuf/descriptor.proto
bazel-bin/protoc --php_out=internal_generate_c_wkt:php/src \
  src/google/protobuf/any.proto \
  src/google/protobuf/api.proto \
  src/google/protobuf/duration.proto \
  src/google/protobuf/empty.proto \
  src/google/protobuf/field_mask.proto \
  src/google/protobuf/source_context.proto \
  src/google/protobuf/struct.proto \
  src/google/protobuf/type.proto \
  src/google/protobuf/timestamp.proto \
  src/google/protobuf/wrappers.proto
popd
