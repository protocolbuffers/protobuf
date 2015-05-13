#!/bin/bash
# Generates C# source files from .proto files.
# You first need to make sure protoc has been built (see instructions on
# building protoc in root of this repository)

set -ex

# cd to repository root
cd $(dirname $0)/..

# protocol buffer compiler to use
PROTOC=src/protoc

# Descriptor proto
#TODO(jtattermusch): generate descriptor.proto

# ProtocolBuffers.Test protos
$PROTOC -Isrc --csharp_out=csharp/src/ProtocolBuffers.Test/TestProtos \
    src/google/protobuf/unittest.proto \
    src/google/protobuf/unittest_custom_options.proto \
    src/google/protobuf/unittest_drop_unknown_fields.proto \
    src/google/protobuf/unittest_enormous_descriptor.proto \
    src/google/protobuf/unittest_import.proto \
    src/google/protobuf/unittest_import_public.proto \
    src/google/protobuf/unittest_mset.proto \
    src/google/protobuf/unittest_optimize_for.proto \
    src/google/protobuf/unknown_enum_test.proto

$PROTOC -Icsharp/protos/extest --csharp_out=csharp/src/ProtocolBuffers.Test/TestProtos \
    csharp/protos/extest/unittest_extras_xmltest.proto \
    csharp/protos/extest/unittest_issues.proto

$PROTOC -Icsharp --csharp_out=csharp/src/ProtocolBuffers.Test/TestProtos \
    csharp/protos/google/protobuf/field_presence_test.proto

$PROTOC -Ibenchmarks --csharp_out=csharp/src/ProtocolBuffers.Test/TestProtos \
    benchmarks/google_size.proto \
    benchmarks/google_speed.proto

# ProtocolBuffersLite.Test protos
$PROTOC -Isrc --csharp_out=csharp/src/ProtocolBuffersLite.Test/TestProtos \
    src/google/protobuf/unittest.proto \
    src/google/protobuf/unittest_import.proto \
    src/google/protobuf/unittest_import_lite.proto \
    src/google/protobuf/unittest_import_public.proto \
    src/google/protobuf/unittest_import_public_lite.proto \
    src/google/protobuf/unittest_lite.proto \
    src/google/protobuf/unittest_lite_imports_nonlite.proto

$PROTOC -Icsharp/protos/extest --csharp_out=csharp/src/ProtocolBuffersLite.Test/TestProtos \
    csharp/protos/extest/unittest_extras_full.proto \
    csharp/protos/extest/unittest_extras_lite.proto
