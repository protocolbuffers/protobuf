// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/java/generator.h"

#include <memory>

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "google/protobuf/compiler/java/java_features.pb.h"
#include "google/protobuf/compiler/command_line_interface_tester.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
namespace {

class JavaGeneratorTest : public CommandLineInterfaceTester {
 protected:
  JavaGeneratorTest() {
    RegisterGenerator("--java_out", "--java_opt",
                      std::make_unique<JavaGenerator>(), "Java test generator");

    // Generate built-in protos.
    CreateTempFile(
        "google/protobuf/descriptor.proto",
        google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
    CreateTempFile("third_party/java/protobuf/java_features.proto",
                   pb::JavaFeatures::descriptor()->file()->DebugString());
  }
};

TEST_F(JavaGeneratorTest, Basic) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto2";
    message Foo {
      optional int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

TEST_F(JavaGeneratorTest, BasicError) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto2";
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "foo.proto:4:7: Expected \"required\", \"optional\", or \"repeated\"");
}


}  // namespace
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
