// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/java/generator.h"

#include <memory>
#include <string>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/java_features.pb.h"
#include "google/protobuf/compiler/command_line_interface_tester.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
namespace {

#define PACKAGE_PREFIX ""

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

  bool FileGenerated(absl::string_view filename) {
    std::string path = absl::StrCat(temp_directory(), "/", filename);
    return File::Exists(path);
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

TEST_F(JavaGeneratorTest, ImplicitPresenceLegacyClosedEnumDisallowed) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    import "third_party/java/protobuf/java_features.proto";
    option features.field_presence = IMPLICIT;
    enum Bar {
      AAA = 0;
    }
    message Foo {
      Bar bar = 1 [features.(pb.java).legacy_closed_enum = true];
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "foo.proto: Field Foo.bar has a closed enum type with implicit "
      "presence.");
}

TEST_F(JavaGeneratorTest, NestInFileClassFeatureDefaultEdition2024) {
  CreateTempFile("foo.proto",
                 R"schema(
      edition = "2024";
      package proto2_unittest;
      option java_generic_services = true;
      message MessageA {
        int32 unused = 1;
        message NestedMessageA {
          int32 unused = 1;
        }

        enum NestedEnumA {
          FOO_DEFAULT = 0;
          FOO_VALUE = 1;
        }
      }
      service MessageB {
        rpc Method(MessageA) returns (MessageA) {}
      }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectNoErrors();
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/FooProto.java"));
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/MessageA.java"));
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/MessageB.java"));
  EXPECT_FALSE(
      FileGenerated(PACKAGE_PREFIX "proto2_unittest/NestedMessageA.java"));
  EXPECT_FALSE(
      FileGenerated(PACKAGE_PREFIX "proto2_unittest/NestedEnumA.java"));
}

TEST_F(JavaGeneratorTest, NestInFileClassFeatureInNetsedMessageError) {
  CreateTempFile("foo.proto",
                 R"schema(
      edition = "2024";
      package proto2_unittest;
      import "third_party/java/protobuf/java_features.proto";
      message Message {
        int32 unused = 1;
        message NestedMessage {
          option features.(pb.java).nest_in_file_class = YES;
          int32 unused = 1;
        }
      }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectErrorSubstring(
      "Feature pb.java.nest_in_file_class only applies to top-level types and "
      "is not allowed to be set on the nested type: "
      "proto2_unittest.Message.NestedMessage");
}

TEST_F(JavaGeneratorTest, NestInFileClassFeatureInNetsedEnumError) {
  CreateTempFile("foo.proto",
                 R"schema(
      edition = "2024";
      package proto2_unittest;
      import "third_party/java/protobuf/java_features.proto";
      message Message {
        int32 unused = 1;
        enum NestedEnum {
          option features.(pb.java).nest_in_file_class = YES;
          FOO_DEFAULT = 0;
          FOO_VALUE = 1;
        }
      }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectErrorSubstring(
      "Feature pb.java.nest_in_file_class only applies to top-level types and "
      "is not allowed to be set on the nested type: "
      "proto2_unittest.Message.NestedEnum");
}

TEST_F(JavaGeneratorTest, SplitNestInFileClassMessageFeatureEdition2024) {
  CreateTempFile("foo.proto",
                 R"schema(
      edition = "2024";
      import "third_party/java/protobuf/java_features.proto";
      package proto2_unittest;
      message NestedInFileClassMessage {
        option features.(pb.java).nest_in_file_class = YES;
        int32 unused = 1;
      }
      message UnnestedMessage {
        int32 unused = 1;
      }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectNoErrors();
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/FooProto.java"));
  EXPECT_TRUE(
      FileGenerated(PACKAGE_PREFIX "proto2_unittest/UnnestedMessage.java"));
  EXPECT_FALSE(FileGenerated(PACKAGE_PREFIX
                             "proto2_unittest/NestedInFileClassMessage.java"));
}

TEST_F(JavaGeneratorTest, SplitNestInFileClassServiceFeatureEdition2024) {
  CreateTempFile("foo.proto",
                 R"schema(
      edition = "2024";
      import "third_party/java/protobuf/java_features.proto";
      package proto2_unittest;
      option java_generic_services = true;
      message Dummy {}
      service NestedInFileClassService {
        option features.(pb.java).nest_in_file_class = YES;
        rpc Method(Dummy) returns (Dummy) {}
      }
      service UnnestedService {
        rpc Method(Dummy) returns (Dummy) {}
      }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectNoErrors();
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/FooProto.java"));
  EXPECT_TRUE(
      FileGenerated(PACKAGE_PREFIX "proto2_unittest/UnnestedService.java"));
  EXPECT_FALSE(FileGenerated(PACKAGE_PREFIX
                             "proto2_unittest/NestedInFileClassService.java"));
}

TEST_F(JavaGeneratorTest, SplitNestInFileClassEnumFeatureEdition2024) {
  CreateTempFile("foo.proto",
                 R"schema(
      edition = "2024";
      import "third_party/java/protobuf/java_features.proto";
      package proto2_unittest;
      enum NestedInFileClassEnum {
        option features.(pb.java).nest_in_file_class = YES;

        FOO_DEFAULT = 0;
        FOO_VALUE = 1;
      }

      enum UnnestedEnum {
        BAR_DEFAULT = 0;
        BAR_VALUE = 1;
      }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectNoErrors();
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/FooProto.java"));
  EXPECT_TRUE(
      FileGenerated(PACKAGE_PREFIX "proto2_unittest/UnnestedEnum.java"));
  EXPECT_FALSE(FileGenerated(PACKAGE_PREFIX
                             "proto2_unittest/NestedInFileClassEnum.java"));
}

TEST_F(JavaGeneratorTest, LargeClosedEnumDisallowedEdition2024) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2024";

    import "third_party/java/protobuf/java_features.proto";

    option features.enum_type = CLOSED;

    enum Bar {
      option features.(pb.java).large_enum = true;

      AAA = 0;
      BBB = 1;
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir foo.proto "
      "--experimental_editions");

  ExpectErrorSubstring(
      "foo.proto: Bar is a closed enum and can not be used with the large_enum "
      "feature.  Please migrate to an open enum first, which is a better fit "
      "for extremely large enums.");
}

TEST_F(JavaGeneratorTest, LargeOpenEnumAllowedEdition2024) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2024";

    import "third_party/java/protobuf/java_features.proto";

    enum Bar {
      option features.(pb.java).large_enum = true;

      AAA = 0;
      BBB = 1;
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "foo.proto "
      "--experimental_editions");

  ExpectNoErrors();
}
TEST_F(JavaGeneratorTest, LargeEnumDisallowedEdition2023) {
  CreateTempFile("foo.proto",
                 R"schema(
edition = "2023";

import "third_party/java/protobuf/java_features.proto";

enum Bar {
option features.(pb.java).large_enum = true;

AAA = 0;
BBB = 1;
}
)schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir foo.proto "
      "--experimental_editions");

  ExpectErrorSubstring(
      "foo.proto:6:6: Feature pb.JavaFeatures.large_enum wasn't introduced "
      "until edition 2024 and can't be used in edition 2023");
}

TEST_F(JavaGeneratorTest,
       InvalidConflictingProtoSuffixedMessageNameEdition2024) {
  CreateTempFile("test_file_name.proto",
                 R"schema(
      edition = "2024";
      package foo;
      message TestFileNameProto {
        int32 field = 1;
      }
      )schema");

  RunProtoc(
      "protocol_compiler --experimental_editions --java_out=$tmpdir "
      "-I$tmpdir test_file_name.proto");

  ExpectErrorSubstring(
      "Cannot generate Java output because the file's outer "
      "class name, \"TestFileNameProto\", matches the name "
      "of one of the types declared inside it");
}
}  // namespace
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
