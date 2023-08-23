// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");

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
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectNoErrors();
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
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectNoErrors();
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
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectErrorSubstring(
      "Field Foo.bar has a closed enum type with implicit presence.");
}

TEST_F(CppGeneratorTest, Utf8ValidationMap) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    import "google/protobuf/cpp_features.proto";

    message Foo {
      map<string, string> map_field = 1 [
        features.(pb.cpp).utf8_validation = NONE
      ];
      map<int32, string> map_field_value = 2 [
        features.(pb.cpp).utf8_validation = VERIFY_PARSE
      ];
      map<string, int32> map_field_key = 3 [
        features.(pb.cpp).utf8_validation = VERIFY_DLOG
      ];
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectNoErrors();
}

TEST_F(CppGeneratorTest, Utf8ValidationNonString) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    import "google/protobuf/cpp_features.proto";

    message Foo {
      int64 bar = 1 [
        features.(pb.cpp).utf8_validation = NONE
      ];
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectErrorSubstring("Field Foo.bar specifies the utf8_validation feature");
}

TEST_F(CppGeneratorTest, Utf8ValidationNonStringMap) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    import "google/protobuf/cpp_features.proto";

    message Foo {
      map<int64, int64> bar = 1 [
        features.(pb.cpp).utf8_validation = VERIFY_PARSE
      ];
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectErrorSubstring("Field Foo.bar specifies the utf8_validation feature");
}

TEST_F(CppGeneratorTest, Utf8ValidationUnknownValue) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    import "google/protobuf/cpp_features.proto";

    message Foo {
      string bar = 1 [
        features.(pb.cpp).utf8_validation = UTF8_VALIDATION_UNKNOWN
      ];
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectErrorSubstring(
      "Field Foo.bar has an unknown value for the utf8_validation feature.");
}
#ifdef PROTOBUF_FUTURE_REMOVE_WRONG_CTYPE
TEST_F(CppGeneratorTest, CtypeOnNoneStringFieldTest) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    message Foo {
      int32 bar = 1 [ctype=STRING];
    })schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");
  ExpectErrorSubstring(
      "Field Foo.bar specifies ctype, but is not "
      "a string nor bytes field.");
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
      "protocol_compiler --proto_path=$tmpdir --cpp_out=$tmpdir "
      "--experimental_editions foo.proto");
  ExpectErrorSubstring(
      "Extension bar specifies ctype=CORD which is "
      "not supported for extensions.");
}
#endif  // !PROTOBUF_FUTURE_REMOVE_WRONG_CTYPE
}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
