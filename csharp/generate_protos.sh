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
PROTOC_LOCATIONS=(
  "bazel-bin/protoc"
  "solution/Debug/protoc.exe"
  "cmake/build/Debug/protoc.exe"
  "cmake/build/Release/protoc.exe"
)
if [ -z "$PROTOC" ]; then
  for protoc in "${PROTOC_LOCATIONS[@]}"; do
    if [ -x "$protoc" ]; then
      PROTOC="$protoc"
    fi
  done
  if [ -z "$PROTOC" ]; then
    echo "Unable to find protocol buffer compiler."
    exit 1
  fi
fi

# descriptor.proto and well-known types
$PROTOC -Isrc --csharp_out=csharp/src/Google.Protobuf \
    --csharp_opt=base_namespace=Google.Protobuf \
    --csharp_opt=file_extension=.pb.cs \
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
    src/google/protobuf/wrappers.proto \
    src/google/protobuf/compiler/plugin.proto

# Test protos
# Note that this deliberately does *not* include old_extensions1.proto
# and old_extensions2.proto, which are generated with an older version
# of protoc.
$PROTOC -Isrc -I. \
    --experimental_allow_proto3_optional \
    --csharp_out=csharp/src/Google.Protobuf.Test.TestProtos \
    --csharp_opt=file_extension=.pb.cs \
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
    csharp/protos/unittest_selfreferential_options.proto \
    src/google/protobuf/unittest_well_known_types.proto \
    src/google/protobuf/test_messages_proto3.proto \
    src/google/protobuf/test_messages_proto2.proto \
    src/google/protobuf/unittest_proto3_optional.proto

# AddressBook sample protos
$PROTOC -Iexamples -Isrc --csharp_out=csharp/src/AddressBook \
    --csharp_opt=file_extension=.pb.cs \
    examples/addressbook.proto

# Conformance tests
$PROTOC -I. --csharp_out=csharp/src/Google.Protobuf.Conformance \
    --csharp_opt=file_extension=.pb.cs \
    conformance/conformance.proto
