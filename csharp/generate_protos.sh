#!/bin/bash
# Generates C# source files from .proto files.
# You first need to make sure protoc has been built (see instructions on
# building protoc in root of this repository)

set -e

# cd to repository root
pushd $(dirname $0)/..

# Protocol buffer compiler to use. If the PROTOC variable is set,
# use that. Otherwise, probe for expected locations under both
# Windows and Unix.
if [ -z "$PROTOC" ]; then
  # TODO(jonskeet): Use an array and a for loop instead?
  if [ -x cmake/build/Debug/protoc.exe ]; then
    PROTOC=cmake/build/Debug/protoc.exe
  elif [ -x cmake/build/Release/protoc.exe ]; then
    PROTOC=cmake/build/Release/protoc.exe
  elif [ -x src/protoc ]; then
    PROTOC=src/protoc
  else
    echo "Unable to find protocol buffer compiler."
    exit 1
  fi
fi

# descriptor.proto and well-known types
$PROTOC -Isrc --csharp_out=csharp/src/Google.Protobuf \
    --csharp_opt=base_namespace=Google.Protobuf \
    src/google/protobuf/descriptor.proto \
    src/google/protobuf/any.proto \
    src/google/protobuf/api.proto \
    src/google/protobuf/duration.proto \
    src/google/protobuf/empty.proto \
    src/google/protobuf/field_mask.proto \
    src/google/protobuf/source_context.proto \
    src/google/protobuf/struct.proto \
    src/google/protobuf/timestamp.proto \
    src/google/protobuf/type.proto \
    src/google/protobuf/wrappers.proto

# Test protos
# Note that this deliberately does *not* include old_extensions1.proto
# and old_extensions2.proto, which are generated with an older version
# of protoc.
$PROTOC -Isrc -Icsharp/protos \
    --csharp_out=csharp/src/Google.Protobuf.Test.TestProtos \
    --descriptor_set_out=csharp/src/Google.Protobuf.Test/testprotos.pb \
    --include_source_info \
    --include_imports \
    csharp/protos/map_unittest_proto3.proto \
    csharp/protos/unittest_issues.proto \
    csharp/protos/unittest_custom_options_proto3.proto \
    csharp/protos/unittest_proto3.proto \
    csharp/protos/unittest_import_proto3.proto \
    csharp/protos/unittest_import_public_proto3.proto \
    csharp/protos/unittest.proto \
    csharp/protos/unittest_import.proto \
    csharp/protos/unittest_import_public.proto \
    csharp/protos/unittest_issue6936_a.proto \
    csharp/protos/unittest_issue6936_b.proto \
    csharp/protos/unittest_issue6936_c.proto \
    src/google/protobuf/unittest_well_known_types.proto \
    src/google/protobuf/test_messages_proto3.proto \
    src/google/protobuf/test_messages_proto2.proto

# AddressBook sample protos
$PROTOC -Iexamples -Isrc --csharp_out=csharp/src/AddressBook \
    examples/addressbook.proto

$PROTOC -Iconformance -Isrc --csharp_out=csharp/src/Google.Protobuf.Conformance \
    conformance/conformance.proto

# Benchmark protos
$PROTOC -Ibenchmarks \
  benchmarks/datasets/google_message1/proto3/*.proto \
  benchmarks/benchmarks.proto \
  --csharp_out=csharp/src/Google.Protobuf.Benchmarks

# C# only benchmark protos
$PROTOC -Isrc -Icsharp/src/Google.Protobuf.Benchmarks \
  csharp/src/Google.Protobuf.Benchmarks/*.proto \
  --csharp_out=csharp/src/Google.Protobuf.Benchmarks
