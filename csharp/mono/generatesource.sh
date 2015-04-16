#!/bin/bash

echo Compiling protobufs
rm -rf tmp
mkdir tmp
PROTOS_DIR=../protos

./protoc --proto_path=$PROTOS_DIR --descriptor_set_out=tmp/compiled.pb \
  $PROTOS_DIR/google/protobuf/descriptor.proto \
  $PROTOS_DIR/google/protobuf/csharp_options.proto \
  $PROTOS_DIR/google/protobuf/unittest.proto \
  $PROTOS_DIR/google/protobuf/unittest_csharp_options.proto \
  $PROTOS_DIR/google/protobuf/unittest_custom_options.proto \
  $PROTOS_DIR/google/protobuf/unittest_embed_optimize_for.proto \
  $PROTOS_DIR/google/protobuf/unittest_import.proto \
  $PROTOS_DIR/google/protobuf/unittest_mset.proto \
  $PROTOS_DIR/google/protobuf/unittest_optimize_for.proto \
  $PROTOS_DIR/tutorial/addressbook.proto

cd tmp
echo Generating new source
mono ../bin/ProtoGen.exe compiled.pb

echo Copying source into place
cp DescriptorProtoFile.cs CSharpOptions.cs ../../src/ProtocolBuffers/DescriptorProtos
cp UnitTest*.cs ../../src/ProtocolBuffers.Test/TestProtos
cp AddressBookProtos.cs ../../src/AddressBook
cd ..
rm -rf tmp
