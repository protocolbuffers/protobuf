// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/cpp/generator.h"

#include <memory>
#include <string>

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/command_line_interface_tester.h"
#include "google/protobuf/cpp_features.pb.h"
#include "google/protobuf/cpp_file_options.pb.h"


namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {

class CppGeneratorTest : public CommandLineInterfaceTester {
 protected:
  CppGeneratorTest() {
    RegisterGenerator("--cpp_out", "--cpp_opt",
                      std::make_unique<CppGenerator>(), "C++ test generator");

    // Generate built-in protos.
    CreateTempFile(
        "google/protobuf/descriptor.proto",
        google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
    CreateTempFile("google/protobuf/cpp_features.proto",
                   pb::CppFeatures::descriptor()->file()->DebugString());
    CreateTempFile(
        "google/protobuf/cpp_file_options.proto",
        pb::file::CppFileOptions::descriptor()->file()->DebugString());
  }
};

TEST_F(CppGeneratorTest, Basic) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto2";
    message Foo {
      optional int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

TEST_F(CppGeneratorTest, BasicError) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto2";
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "foo.proto:4:7: Expected \"required\", \"optional\", or \"repeated\"");
}

TEST_F(CppGeneratorTest, LegacyClosedEnumOnNonEnumField) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    import "google/protobuf/cpp_features.proto";

    message Foo {
      int32 bar = 1 [features.(pb.cpp).legacy_closed_enum = true];
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "Field Foo.bar specifies the legacy_closed_enum feature but has non-enum "
      "type.");
}

TEST_F(CppGeneratorTest, LegacyClosedEnum) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    import "google/protobuf/cpp_features.proto";

    enum TestEnum {
      TEST_ENUM_UNKNOWN = 0;
    }
    message Foo {
      TestEnum bar = 1 [features.(pb.cpp).legacy_closed_enum = true];
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectWarningSubstring(
      "foo.proto:9:16: warning: pb.CppFeatures.legacy_closed_enum has "
      "been deprecated in edition 2023:");
}

TEST_F(CppGeneratorTest, LegacyClosedEnumInherited) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    import "google/protobuf/cpp_features.proto";
    option features.(pb.cpp).legacy_closed_enum = true;

    enum TestEnum {
      TEST_ENUM_UNKNOWN = 0;
    }
    message Foo {
      TestEnum bar = 1;
      int32 baz = 2;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectWarningSubstring(
      "foo.proto: warning: pb.CppFeatures.legacy_closed_enum has "
      "been deprecated in edition 2023");
}

TEST_F(CppGeneratorTest, LegacyClosedEnumImplicit) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    import "google/protobuf/cpp_features.proto";
    option features.(pb.cpp).legacy_closed_enum = true;

    enum TestEnum {
      TEST_ENUM_UNKNOWN = 0;
    }
    message Foo {
      TestEnum bar = 1 [features.field_presence = IMPLICIT];
      int32 baz = 2;
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "Field Foo.bar has a closed enum type with implicit presence.");
}

TEST_F(CppGeneratorTest, ValidRepeatedFieldLegacyClosedEnumImplicit) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    import "google/protobuf/cpp_features.proto";
    option features.(pb.cpp).legacy_closed_enum = true;
    option features.field_presence = IMPLICIT;

    enum TestEnum {
      TEST_ENUM_UNKNOWN = 0;
    }
    message Foo {
      repeated TestEnum bar = 1;
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

TEST_F(CppGeneratorTest, ValidOneofFieldLegacyClosedEnumImplicit) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    import "google/protobuf/cpp_features.proto";
    option features.(pb.cpp).legacy_closed_enum = true;
    option features.field_presence = IMPLICIT;

    enum TestEnum {
      TEST_ENUM_UNKNOWN = 0;
    }
    message Foo {
      oneof o {
        TestEnum bar = 1;
      }
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

TEST_F(CppGeneratorTest, AllowStringTypeForEdition2023) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    import "google/protobuf/cpp_features.proto";

    message Foo {
      int32 bar = 1;
      bytes baz = 2 [features.(pb.cpp).string_type = CORD];
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

TEST_F(CppGeneratorTest, ErrorsOnBothStringTypeAndCtype) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    import "google/protobuf/cpp_features.proto";

    message Foo {
      int32 bar = 1;
      bytes baz = 2 [ctype = CORD, features.(pb.cpp).string_type = VIEW];
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "Foo.baz specifies both string_type and ctype which is not supported.");
}

TEST_F(CppGeneratorTest, StringTypeForCord) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2024";
    import "google/protobuf/cpp_features.proto";

    message Foo {
      int32 bar = 1;
      bytes baz = 2 [features.(pb.cpp).string_type = CORD];
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectNoErrors();
}

TEST_F(CppGeneratorTest, CtypeForCord) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";

    message Foo {
      int32 bar = 1;
      bytes baz = 2 [ctype = CORD];
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

TEST_F(CppGeneratorTest, StringTypeForStringFieldsOnly) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2024";
    import "google/protobuf/cpp_features.proto";

    message Foo {
      int32 bar = 1;
      int32 baz = 2 [features.(pb.cpp).string_type = CORD];
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectErrorSubstring(
      "Field Foo.baz specifies string_type, but is not a string nor bytes "
      "field.");
}

TEST_F(CppGeneratorTest, StringTypeCordNotForExtension) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2024";
    import "google/protobuf/cpp_features.proto";

    message Foo {
      extensions 1 to max;
    }
    extend Foo {
      bytes bar = 1 [features.(pb.cpp).string_type = CORD];
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectErrorSubstring(
      "Extension bar specifies CORD string type which is not supported for "
      "extensions");
}

TEST_F(CppGeneratorTest, InheritedStringTypeCordNotForExtension) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2024";
    import "google/protobuf/cpp_features.proto";
    option features.(pb.cpp).string_type = CORD;

    message Foo {
      extensions 1 to max;
    }
    extend Foo {
      bytes bar = 1;
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectNoErrors();
}

TEST_F(CppGeneratorTest, CtypeOnNonStringFieldTest) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    message Foo {
      int32 bar = 1 [ctype=STRING];
    })schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Field Foo.bar specifies string_type, but is not a string nor bytes "
      "field.");
}

TEST_F(CppGeneratorTest, InvalidFullyQualifiedNamespace) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "UNSTABLE";
    import option "google/protobuf/cpp_file_options.proto";
    option (pb.file.cpp).namespace = "::foo::test";
    message Foo {
      int32 bar = 1;
    })schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir  "
      "--experimental_editions foo.proto");
  ExpectErrorSubstring("Namespace ::foo::test can not start with `::`.");
}


TEST_F(CppGeneratorTest, CtypeOnExtensionTest) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    message Foo {
      extensions 1 to max;
    }
    extend Foo {
      bytes bar = 1 [ctype=CORD];
    })schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Extension bar specifies CORD string type which is not supported for "
      "extensions");
}

TEST_F(CppGeneratorTest, DeprecatedNestedEnumValueImportIsNotSelfWarning) {
  // The class-scoped alias for a deprecated nested enum value initializes
  // itself from the deprecated enumerator, which would otherwise trigger
  // -Wdeprecated-declarations in code the user does not control. The alias
  // must be wrapped in the deprecation-suppression macros.
  // Regression test for protocolbuffers/protobuf#18205.
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto3";
    message Result {
      enum Status {
        UNSET = 0;
        OK = 1 [deprecated = true];
        FAILED = 2;
      }
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectFileContentContainsSubstring(
      "foo.pb.h",
      "PROTOBUF_IGNORE_DEPRECATION_START\n"
      "  [[deprecated]] static constexpr Status OK = Result_Status_OK;\n"
      "  PROTOBUF_IGNORE_DEPRECATION_STOP");
}

TEST_F(CppGeneratorTest, NonDeprecatedEnumValueImportHasNoSuppression) {
  // Enums without deprecated values must not emit suppression macros.
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto3";
    message Result {
      enum Status {
        UNSET = 0;
        OK = 1;
      }
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectFileContentNotContainsSubstring("foo.pb.h",
                                        "PROTOBUF_IGNORE_DEPRECATION_START");
}

TEST_F(CppGeneratorTest, MapOptionsDeterministicSerialization) {
  CreateTempFile("map_option.proto",
                 absl::Substitute(
                     R"schema(
    syntax = "proto2";
    package test;
    import "$0";

    message MapOption {
      map<string, string> values = 1;
    }

    extend $1 {
      optional MapOption file_opt = 50000;
    }
    extend $2 {
      optional MapOption msg_opt = 50001;
    }
    extend $3 {
      optional MapOption field_opt = 50002;
    }
  )schema",
                     google::protobuf::FileOptions::descriptor()->file()->name(),
                     google::protobuf::FileOptions::descriptor()->full_name(),
                     google::protobuf::MessageOptions::descriptor()->full_name(),
                     google::protobuf::FieldOptions::descriptor()->full_name()));

  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto2";
    package test;
    import "map_option.proto";

    option (test.file_opt) = {
      values: [
        { key: "a", value: "1" },
        { key: "b", value: "2" },
        { key: "c", value: "3" },
        { key: "d", value: "4" }
      ]
    };

    message Foo {
      option (test.msg_opt) = {
        values: [
          { key: "x", value: "24" },
          { key: "y", value: "25" },
          { key: "z", value: "26" }
        ]
      };
      optional int32 bar = 1 [(test.field_opt) = {
        values: [
          { key: "m", value: "13" },
          { key: "n", value: "14" }
        ]
      }];
    }
  )schema");

  CreateTempDir("out1");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir/out1 "
      "foo.proto");
  ExpectNoErrors();
  std::string file_content1 = FileContents("out1/foo.pb.cc");

  CreateTempDir("out2");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir/out2 "
      "foo.proto");
  ExpectNoErrors();
  std::string file_content2 = FileContents("out2/foo.pb.cc");

  EXPECT_EQ(file_content1, file_content2);
}


}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
