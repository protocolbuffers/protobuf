// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <memory>

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "google/protobuf/compiler/command_line_interface_tester.h"
#include "google/protobuf/compiler/php/php_generator.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace php {
namespace {

class PhpGeneratorTest : public CommandLineInterfaceTester {
 protected:
  PhpGeneratorTest() {
    RegisterGenerator("--php_out", "--php_opt", std::make_unique<Generator>(),
                      "PHP test generator");

    // Generate built-in protos.
    CreateTempFile(
        "google/protobuf/descriptor.proto",
        google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  }
};

TEST_F(PhpGeneratorTest, Basic) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";
    message Foo {
      optional int32 bar = 1;
      int32 baz = 2;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --php_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

TEST_F(PhpGeneratorTest, Proto2File) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto2";
    message Foo {
      optional int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --php_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

TEST_F(PhpGeneratorTest, RequiredFieldError) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto2";
    message FooBar {
      required int32 foo_message = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --php_out=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "Can't generate PHP code for required field FooBar.foo_message");
}

TEST_F(PhpGeneratorTest, GroupFieldError) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto2";
    message Foo {
      optional group Bar = 1 {
        optional int32 baz = 1;
      };
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --php_out=$tmpdir foo.proto");

  ExpectErrorSubstring("Can't generate PHP code for group field Foo.bar");
}

TEST_F(PhpGeneratorTest, ClosedEnumError) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto2";
    enum Foo {
      BAR = 0;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --php_out=$tmpdir foo.proto");

  ExpectErrorSubstring("Can't generate PHP code for closed enum Foo");
}

}  // namespace
}  // namespace php
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
