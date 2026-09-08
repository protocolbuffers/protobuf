// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/generator.h"

#include <gtest/gtest.h>

#include <memory>

#include "google/protobuf/compiler/command_line_interface_tester.h"
#include "google/protobuf/descriptor.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {
namespace {

class ObjectiveCGeneratorTest : public CommandLineInterfaceTester {
 protected:
  ObjectiveCGeneratorTest() {
    RegisterGenerator("--objc_out", "--objc_opt",
                      std::make_unique<ObjectiveCGenerator>(),
                      "Objective-C test generator");

    // Generate built-in protos.
    CreateTempFile(
        "google/protobuf/descriptor.proto",
        google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  }
};

TEST_F(ObjectiveCGeneratorTest, InvalidObjCClassPrefixRejected) {
  // The prefix is pasted straight into @interface/@implementation declarations
  // and into a C string literal, so without validation it can break out of
  // those constructs.
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";
    option objc_class_prefix = "A\";void pwn(){}//";
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --objc_out=$tmpdir foo.proto");

  ExpectErrorSubstring("Invalid 'option objc_class_prefix");
}

// The expected-prefixes machinery has two documented opt-outs: a path of "-"
// disables the checks entirely, and a file can be listed in
// expected_prefixes_suppressions. Neither may permit a prefix that can inject
// code into the generated sources, so the character check runs ahead of both.
TEST_F(ObjectiveCGeneratorTest,
       InvalidObjCClassPrefixRejectedWithExpectedPrefixesDisabled) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";
    option objc_class_prefix = "A\";void pwn(){}//";
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --objc_out=$tmpdir "
      "--objc_opt=expected_prefixes_path=- foo.proto");

  ExpectErrorSubstring("Invalid 'option objc_class_prefix");
}

TEST_F(ObjectiveCGeneratorTest, InvalidObjCClassPrefixRejectedWhenSuppressed) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";
    option objc_class_prefix = "A\";void pwn(){}//";
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --objc_out=$tmpdir "
      "--objc_opt=expected_prefixes_suppressions=foo.proto foo.proto");

  ExpectErrorSubstring("Invalid 'option objc_class_prefix");
}

TEST_F(ObjectiveCGeneratorTest, ValidObjCClassPrefixAccepted) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";
    option objc_class_prefix = "FOO";
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --objc_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

TEST_F(ObjectiveCGeneratorTest, ObjCClassPrefixWithUnderscoreAccepted) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";
    option objc_class_prefix = "My_App1";
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --objc_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

}  // namespace
}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
