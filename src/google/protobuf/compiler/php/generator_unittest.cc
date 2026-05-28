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

TEST_F(PhpGeneratorTest, ImportPublic) {
  CreateTempFile("common.proto",
                 R"schema(
    syntax = "proto3";
    package prototest;
    message Common {
      string data = 1;
    })schema");

  CreateTempFile("prototest.proto",
                 R"schema(
    syntax = "proto3";
    package prototest;
    import public "common.proto";
    )schema");

  CreateTempFile("usecase.proto",
                 R"schema(
    syntax = "proto3";
    package usecasetest;
    import "prototest.proto";
    message UseCase {
      prototest.Common commonUse = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --php_out=$tmpdir "
      "common.proto prototest.proto usecase.proto");

  ExpectNoErrors();
}

TEST_F(PhpGeneratorTest, CustomJsonName) {
  std::string descriptor_package =
      std::string(DescriptorProto::descriptor()->file()->package());
  std::string import_path =
#ifdef PROTO2_OPENSOURCE
      "google/protobuf/descriptor.proto";
#else
      "google/protobuf/descriptor.proto";
#endif

  std::string options_proto =
      "edition = \"2023\";\n"
      "package pb.enumvalue;\n"
      "import \"" +
      import_path +
      "\";\n"
      "message JsonEnumValueOptions {\n"
      "  string string = 1;\n"
      "}\n"
      "extend ." +
      descriptor_package +
      ".EnumValueOptions {\n"
      "  JsonEnumValueOptions json = 998;\n"
      "}\n";
  CreateTempFile("options.proto", options_proto);

  std::string foo_proto =
      "edition = \"2023\";\n"
      "import \"options.proto\";\n"
      "enum MyEnum {\n"
      "  MY_ENUM_UNSPECIFIED = 0;\n"
      "  FOO = 1 [(pb.enumvalue.json).string = \"custom_foo\"];\n"
      "  BAR = 2;\n"
      "}\n";
  CreateTempFile("foo.proto", foo_proto);

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --php_out=$tmpdir foo.proto");

  ExpectNoErrors();

  ExpectFileContentContainsSubstring("MyEnum.php",
                                     "private static $valueToJsonName");
  ExpectFileContentContainsSubstring("MyEnum.php", "self::FOO => 'custom_foo'");
  ExpectFileContentContainsSubstring("MyEnum.php",
                                     "private static $jsonNameToValue");
  ExpectFileContentContainsSubstring("MyEnum.php", "'custom_foo' => self::FOO");
  ExpectFileContentContainsSubstring("MyEnum.php",
                                     "public static function jsonName($value)");
  ExpectFileContentContainsSubstring(
      "MyEnum.php", "public static function valueForJson($name)");
}

}  // namespace
}  // namespace php
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
