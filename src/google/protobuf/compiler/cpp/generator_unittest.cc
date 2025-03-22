// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/cpp/generator.h"

#include <memory>

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "google/protobuf/compiler/command_line_interface_tester.h"
#include "google/protobuf/cpp_features.pb.h"

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
      "foo.proto:9:16: warning: Feature pb.CppFeatures.legacy_closed_enum has "
      "been deprecated in edition 2023");
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
      "foo.proto: warning: Feature pb.CppFeatures.legacy_closed_enum has "
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
}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
