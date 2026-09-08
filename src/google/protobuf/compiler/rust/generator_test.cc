// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/generator.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/escaping.h"
#include "google/protobuf/compiler/command_line_interface_tester.h"


namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
namespace {

using ::testing::HasSubstr;
using Semantic = ::google::protobuf::GeneratedCodeInfo::Annotation::Semantic;

// The Rust generator will append this comment somewhere near the end of the
// generated source file, followed by a base64-encoded wire format message
// and an endline.
constexpr absl::string_view kMetadataComment = "// google.protobuf.GeneratedCodeInfo ";

class RustGeneratorTest : public CommandLineInterfaceTester {
 protected:
  RustGeneratorTest() {
    RegisterGenerator("--rust_out", "--rust_opt",
                      std::make_unique<RustGenerator>(), "Rust test generator");

    // Generate built-in protos.
    CreateTempFile(
        google::protobuf::DescriptorProto::descriptor()->file()->name(),
        google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  }

  void MustGetMetadataFor(absl::string_view schema) {
    CreateTempFile("foo.proto", schema);
    RunProtoc(
        "protocol_compiler --proto_path=$tmpdir "
        "--rust_out=$tmpdir "
        "--rust_opt=experimental-codegen=enabled,kernel=cpp,annotate_code=true "
        "foo.proto");

    ExpectNoErrors();
    file_contents_ = FileContents("foo.c.pb.rs");
    auto metadata_comment_pos = file_contents_.find(kMetadataComment);
    ASSERT_NE(metadata_comment_pos, std::string::npos);
    metadata_comment_pos += kMetadataComment.size();
    auto metadata_comment_end_pos =
        file_contents_.find('\n', metadata_comment_pos);
    ASSERT_NE(metadata_comment_end_pos, std::string::npos);
    auto metadata_comment = file_contents_.substr(
        metadata_comment_pos, metadata_comment_end_pos - metadata_comment_pos);
    std::string decoded_metadata;
    ASSERT_TRUE(absl::Base64Unescape(metadata_comment, &decoded_metadata));
    ASSERT_TRUE(generated_code_info_.ParseFromString(decoded_metadata));
  }

  void CountMatchingAnnotations(std::vector<int> path,
                                absl::string_view content, Semantic semantic,
                                size_t expected_count) {
    size_t count = 0;
    for (const auto& annotation : generated_code_info_.annotation()) {
      if (annotation.path_size() == path.size()) {
        bool match = true;
        for (int i = 0; i < path.size(); ++i) {
          if (annotation.path(i) != path[i]) {
            match = false;
            break;
          }
        }
        EXPECT_EQ(annotation.source_file(), "foo.proto");
        ASSERT_LE(annotation.begin(), file_contents_.size());
        ASSERT_LE(annotation.end(), file_contents_.size());
        if (match) {
          if (file_contents_.substr(annotation.begin(),
                                    annotation.end() - annotation.begin()) ==
              content) {
            EXPECT_EQ(annotation.semantic(), semantic);
            count++;
          }
        }
      }
    }
    EXPECT_EQ(count, expected_count);
  }

 private:
  std::string file_contents_;
  google::protobuf::GeneratedCodeInfo generated_code_info_;
};

TEST_F(RustGeneratorTest, EmitsNoMessageMetadataByDefault) {
  constexpr absl::string_view kFooProto = R"schema(
    syntax = "proto2";
    package foo;
    message Message {
    })schema";
  CreateTempFile("foo.proto", kFooProto);
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "foo.proto");
  ExpectNoErrors();
  EXPECT_THAT(FileContents("foo.c.pb.rs"), Not(HasSubstr(kMetadataComment)));
}

TEST_F(RustGeneratorTest, EmitsNoMessageMetadataOnFalseArgument) {
  constexpr absl::string_view kFooProto = R"schema(
    syntax = "proto2";
    package foo;
    message Message {
    })schema";
  CreateTempFile("foo.proto", kFooProto);
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp,annotate_code=false "
      "foo.proto");
  ExpectNoErrors();
  EXPECT_THAT(FileContents("foo.c.pb.rs"), Not(HasSubstr(kMetadataComment)));
}

TEST_F(RustGeneratorTest, EmitsMessageMetadata) {
  MustGetMetadataFor(R"schema(
    syntax = "proto2";
    package foo;
    message Message {
    })schema");
  // We expect the Rust name for the message type to be `Message`. Check to see
  // the annotation covers that string.
  CountMatchingAnnotations(
      {google::protobuf::FileDescriptorProto::kMessageTypeFieldNumber, 0}, "Message",
      GeneratedCodeInfo::Annotation::NONE, 1);
}

TEST_F(RustGeneratorTest, EmitsFieldMetadata) {
  MustGetMetadataFor(R"schema(
    syntax = "proto2";
    package foo;
    message Message {
      optional int32 int32_field = 1;
      optional string string_field = 2;
    })schema");
  CountMatchingAnnotations(
      {google::protobuf::FileDescriptorProto::kMessageTypeFieldNumber, 0}, "Message",
      GeneratedCodeInfo::Annotation::NONE, 1);
  CountMatchingAnnotations(
      {google::protobuf::FileDescriptorProto::kMessageTypeFieldNumber, 0,
       google::protobuf::DescriptorProto::kFieldFieldNumber, 0},
      "int32_field", GeneratedCodeInfo::Annotation::NONE, 3);
  CountMatchingAnnotations(
      {google::protobuf::FileDescriptorProto::kMessageTypeFieldNumber, 0,
       google::protobuf::DescriptorProto::kFieldFieldNumber, 0},
      "set_int32_field", GeneratedCodeInfo::Annotation::SET, 2);
  CountMatchingAnnotations(
      {google::protobuf::FileDescriptorProto::kMessageTypeFieldNumber, 0,
       google::protobuf::DescriptorProto::kFieldFieldNumber, 1},
      "string_field", GeneratedCodeInfo::Annotation::NONE, 3);
  CountMatchingAnnotations(
      {google::protobuf::FileDescriptorProto::kMessageTypeFieldNumber, 0,
       google::protobuf::DescriptorProto::kFieldFieldNumber, 1},
      "set_string_field", GeneratedCodeInfo::Annotation::SET, 2);
}

TEST_F(RustGeneratorTest, EmitsEnumMetadata) {
  MustGetMetadataFor(R"schema(
    syntax = "proto2";
    package foo;
    enum Enum {
      option allow_alias = true;
      ENUMERATOR_0 = 0;
      ENUMERATOR_1 = 1;
      ALIAS_1 = 1;
    })schema");

  CountMatchingAnnotations(
      {google::protobuf::FileDescriptorProto::kEnumTypeFieldNumber, 0}, "Enum",
      GeneratedCodeInfo::Annotation::NONE, 1);
  CountMatchingAnnotations(
      {google::protobuf::FileDescriptorProto::kEnumTypeFieldNumber, 0,
       google::protobuf::EnumDescriptorProto::kValueFieldNumber, 0},
      "Erator0", GeneratedCodeInfo::Annotation::NONE, 1);
  CountMatchingAnnotations(
      {google::protobuf::FileDescriptorProto::kEnumTypeFieldNumber, 0,
       google::protobuf::EnumDescriptorProto::kValueFieldNumber, 1},
      "Erator1", GeneratedCodeInfo::Annotation::NONE, 1);
  CountMatchingAnnotations(
      {google::protobuf::FileDescriptorProto::kEnumTypeFieldNumber, 0,
       google::protobuf::EnumDescriptorProto::kValueFieldNumber, 2},
      "Alias1", GeneratedCodeInfo::Annotation::NONE, 1);
}

TEST_F(RustGeneratorTest, EmitsPublicFileModuleInEntryPoint) {
  constexpr absl::string_view kFooProto = R"schema(
    syntax = "proto2";
    package foo;
    message Message {
    })schema";
  CreateTempFile("foo.proto", kFooProto);
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "foo.proto");
  ExpectNoErrors();

  std::string entry_point = FileContents("generated.rs");
  EXPECT_THAT(entry_point, HasSubstr("pub mod foo_proto;"));
  EXPECT_THAT(entry_point, HasSubstr("pub use foo_proto::*;"));
  EXPECT_THAT(entry_point, Not(HasSubstr("internal_do_not_use_")));
  EXPECT_THAT(entry_point, Not(HasSubstr("#[doc(hidden)]")));
}

TEST_F(RustGeneratorTest, EmitsPublicFileModulesForMultiFileCrate) {
  constexpr absl::string_view kFooProto = R"schema(
    syntax = "proto2";
    package foo;
    message Config {
    })schema";
  constexpr absl::string_view kBarProto = R"schema(
    syntax = "proto2";
    package bar;
    message Config {
    })schema";
  CreateTempFile("foo.proto", kFooProto);
  CreateTempFile("bar.proto", kBarProto);
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "foo.proto bar.proto");
  ExpectNoErrors();

  std::string entry_point = FileContents("generated.rs");
  EXPECT_THAT(entry_point, HasSubstr("pub mod foo_proto;"));
  EXPECT_THAT(entry_point, HasSubstr("pub mod bar_proto;"));
  EXPECT_THAT(entry_point, Not(HasSubstr("internal_do_not_use_")));
}

TEST_F(RustGeneratorTest, EmitsValidIdentifierForLeadingDigitFile) {
  constexpr absl::string_view kProto = R"schema(
    syntax = "proto2";
    package sample;
    message Config {}
  )schema";
  CreateTempFile("0.1.proto", kProto);
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "0.1.proto");
  ExpectNoErrors();

  std::string entry_point = FileContents("generated.rs");
  // A leading non-letter gets a `pb_` prefix so the module name is a valid
  // identifier (`0.1.proto` must not produce `pub mod 0_1_proto;`).
  EXPECT_THAT(entry_point, HasSubstr("pub mod pb_0_1_proto;"));
  EXPECT_THAT(entry_point, HasSubstr("pub use pb_0_1_proto::*;"));
  EXPECT_THAT(entry_point, Not(HasSubstr("pub mod 0_1_proto;")));
}

TEST_F(RustGeneratorTest, EmitsReadableModuleNameForHyphenatedFile) {
  constexpr absl::string_view kProto = R"schema(
    syntax = "proto2";
    package sample;
    message A {}
  )schema";
  CreateTempFile("my-service.proto", kProto);
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "my-service.proto");
  ExpectNoErrors();

  std::string entry_point = FileContents("generated.rs");
  // Separators collapse to `_` and the `.proto` extension becomes `_proto`;
  // no `_2d_` hex escaping is used for a plain hyphen.
  EXPECT_THAT(entry_point, HasSubstr("pub mod my_service_proto;"));
  EXPECT_THAT(entry_point, Not(HasSubstr("_2d_")));
}

TEST_F(RustGeneratorTest, EmitsQualifiedPathForSameFileMessageReference) {
  // Foo uses Bar from the same file.
  constexpr absl::string_view kFooProto = R"schema(
    syntax = "proto2";
    package foo;
    message Bar {}
    message Foo {
      optional Bar bar = 1;
    })schema";
  CreateTempFile("foo.proto", kFooProto);
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "foo.proto");
  ExpectNoErrors();

  std::string foo = FileContents("foo.c.pb.rs");
  EXPECT_THAT(foo, HasSubstr("super::foo_proto::BarView"));
  EXPECT_THAT(foo, Not(HasSubstr("super::BarView")));
}

TEST_F(RustGeneratorTest, EmitsQualifiedPathForCrossFileMessageReference) {
  constexpr absl::string_view kBarProto = R"schema(
    syntax = "proto2";
    package bar;
    message Bar {})schema";

  // Foo uses Bar from a different file in the same crate.
  constexpr absl::string_view kFooProto = R"schema(
    syntax = "proto2";
    package foo;
    import "bar.proto";
    message Foo {
      optional bar.Bar bar = 1;
    })schema";
  CreateTempFile("bar.proto", kBarProto);
  CreateTempFile("foo.proto", kFooProto);
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "foo.proto bar.proto");
  ExpectNoErrors();

  std::string foo = FileContents("foo.c.pb.rs");
  EXPECT_THAT(foo, HasSubstr("super::bar_proto::BarView"));
  EXPECT_THAT(foo, Not(HasSubstr("super::BarView")));
}

TEST_F(RustGeneratorTest, EmitsQualifiedPathForNestedContainingType) {
  constexpr absl::string_view kProto = R"schema(
    syntax = "proto2";
    package foo;
    message Wrapper {
      message Nested {}
      enum Kind {
        K0 = 0;
        K1 = 1;
      }
    }
    message User {
      optional Wrapper.Nested nested = 1;
      optional Wrapper.Kind kind = 2;
    })schema";
  CreateTempFile("foo.proto", kProto);
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "foo.proto");
  ExpectNoErrors();

  std::string foo = FileContents("foo.c.pb.rs");
  EXPECT_THAT(foo, HasSubstr("super::foo_proto::wrapper::NestedView"));
  EXPECT_THAT(foo, HasSubstr("super::foo_proto::wrapper::Kind"));
}

TEST_F(RustGeneratorTest, EmitsQualifiedPathForCrossCrateMessageReference) {
  constexpr absl::string_view kBarProto = R"schema(
    syntax = "proto2";
    package bar;
    message Bar {})schema";
  // Foo uses Bar from a different crate.
  constexpr absl::string_view kFooProto = R"schema(
    syntax = "proto2";
    package foo;
    import "bar.proto";
    message Foo {
      optional bar.Bar bar = 1;
    })schema";
  CreateTempFile("bar.proto", kBarProto);
  CreateTempFile("foo.proto", kFooProto);
  CreateTempFile("mapping.txt", "bar_crate\n1\nbar.proto\n");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp,"
      "crate_mapping=$tmpdir/mapping.txt "
      "foo.proto");
  ExpectNoErrors();

  std::string foo = FileContents("foo.c.pb.rs");
  EXPECT_THAT(foo, HasSubstr("::bar_crate::bar_proto::BarView"));
  EXPECT_THAT(foo, HasSubstr("IntoProxied<::bar_crate::bar_proto::Bar>"));
  EXPECT_THAT(foo, Not(HasSubstr("::bar_crate::BarView")));
}

TEST_F(RustGeneratorTest, EmitsQualifiedExtendeePathForExtensions) {
  constexpr absl::string_view kFooProto = R"schema(
    syntax = "proto2";
    package foo;
    message Target {
      optional int32 val = 1;
      extensions 100 to 200;
    }
    extend Target {
      optional int32 top_ext = 100;
    }
    message Container {
      extend Target {
        optional int32 nested_ext = 101;
      }
    })schema";
  CreateTempFile("foo.proto", kFooProto);
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "foo.proto");
  ExpectNoErrors();

  std::string foo = FileContents("foo.c.pb.rs");
  // top_ext
  EXPECT_THAT(foo, HasSubstr("ExtensionId<super::foo_proto::Target, i32>"));
  // nested_ext
  EXPECT_THAT(foo,
              HasSubstr("ExtensionId<super::super::foo_proto::Target, i32>"));
}

TEST_F(RustGeneratorTest, DropsReexportForEntireCrate) {
  CreateTempFile("a.proto", R"schema(
    syntax = "proto2";
    package pkg_a;
    message Foo { optional int32 x = 1; })schema");
  CreateTempFile("b.proto", R"schema(
    syntax = "proto2";
    package pkg_b;
    message Foo { optional int32 y = 1; })schema");
  CreateTempFile("c.proto", R"schema(
    syntax = "proto2";
    package pkg_c;
    message Unique { optional int32 z = 1; })schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "a.proto b.proto c.proto");
  ExpectNoErrors();

  std::string entry_point = FileContents("generated.rs");
  EXPECT_THAT(entry_point, HasSubstr("pub mod a_proto;"));
  EXPECT_THAT(entry_point, HasSubstr("pub mod b_proto;"));
  EXPECT_THAT(entry_point, HasSubstr("pub mod c_proto;"));
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use a_proto::*;")));
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use b_proto::*;")));
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use c_proto::*;")));
}

TEST_F(RustGeneratorTest, DropsReexportOnEnumCollision) {
  CreateTempFile("a.proto", R"schema(
    syntax = "proto2";
    package pkg_a;
    enum Status { UNKNOWN = 0; OK = 1; })schema");
  CreateTempFile("b.proto", R"schema(
    syntax = "proto2";
    package pkg_b;
    enum Status { PENDING = 0; DONE = 1; })schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "a.proto b.proto");
  ExpectNoErrors();

  std::string entry_point = FileContents("generated.rs");
  EXPECT_THAT(entry_point, HasSubstr("pub mod a_proto;"));
  EXPECT_THAT(entry_point, HasSubstr("pub mod b_proto;"));
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use a_proto::*;")));
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use b_proto::*;")));
}

TEST_F(RustGeneratorTest, KeepsReexportForNonCollidingFiles) {
  CreateTempFile("a.proto", R"schema(
    syntax = "proto2";
    package pkg_a;
    message Alpha {
      optional int32 x = 1;
    })schema");
  CreateTempFile("b.proto", R"schema(
    syntax = "proto2";
    package pkg_b;
    message Beta {
      optional int32 y = 1;
    })schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "a.proto b.proto");
  ExpectNoErrors();

  std::string entry_point = FileContents("generated.rs");
  EXPECT_THAT(entry_point, HasSubstr("pub use a_proto::*;"));
  EXPECT_THAT(entry_point, HasSubstr("pub use b_proto::*;"));
}

TEST_F(RustGeneratorTest, DropsReexportOnViewCollision) {
  CreateTempFile("a.proto", R"schema(
    syntax = "proto2";
    package pkg_a;
    message BarView { optional int32 x = 1; })schema");
  CreateTempFile("b.proto", R"schema(
    syntax = "proto2";
    package pkg_b;
    message Bar { optional int32 y = 1; })schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "a.proto b.proto");
  ExpectNoErrors();

  std::string entry_point = FileContents("generated.rs");
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use a_proto::*;")));
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use b_proto::*;")));
}

TEST_F(RustGeneratorTest, DropsReexportOnMutCollision) {
  CreateTempFile("a.proto", R"schema(
    syntax = "proto2";
    package pkg_a;
    message QuxMut { optional int32 x = 1; })schema");
  CreateTempFile("b.proto", R"schema(
    syntax = "proto2";
    package pkg_b;
    message Qux { optional int32 y = 1; })schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "a.proto b.proto");
  ExpectNoErrors();

  std::string entry_point = FileContents("generated.rs");
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use a_proto::*;")));
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use b_proto::*;")));
}

TEST_F(RustGeneratorTest, DropsReexportOnSubmoduleCollision) {
  CreateTempFile("a.proto", R"schema(
    syntax = "proto2";
    package pkg_a;
    message data { optional int32 x = 1; })schema");
  CreateTempFile("b.proto", R"schema(
    syntax = "proto2";
    package pkg_b;
    message Data {
      message Row { optional int32 v = 1; }
      optional Row row = 1;
    })schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "a.proto b.proto");
  ExpectNoErrors();

  std::string entry_point = FileContents("generated.rs");
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use a_proto::*;")));
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use b_proto::*;")));
}

TEST_F(RustGeneratorTest, DropsReexportOnExtensionCollision) {
  CreateTempFile("a.proto", R"schema(
    syntax = "proto2";
    package pkg_a;
    message TargetA { extensions 100 to 200; }
    extend TargetA { optional int32 shared = 100; })schema");
  CreateTempFile("b.proto", R"schema(
    syntax = "proto2";
    package pkg_b;
    message TargetB { extensions 100 to 200; }
    extend TargetB { optional int32 shared = 100; })schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "a.proto b.proto");
  ExpectNoErrors();

  std::string entry_point = FileContents("generated.rs");
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use a_proto::*;")));
  EXPECT_THAT(entry_point, Not(HasSubstr("pub use b_proto::*;")));
}

TEST_F(RustGeneratorTest, KeepsReexportForCrateWithFeaturesButNoCollision) {
  CreateTempFile("a.proto", R"schema(
    syntax = "proto2";
    package pkg_a;
    message AlphaMsg {
      message AlphaInner { optional int32 v = 1; }
      extensions 100 to 200;
      oneof alpha_choice {
        int32 a = 1;
        int32 b = 2;
      }
    }
    enum AlphaEnum { AE0 = 0; AE1 = 1; }
    extend AlphaMsg { optional int32 alpha_ext = 100; })schema");
  CreateTempFile("b.proto", R"schema(
    syntax = "proto2";
    package pkg_b;
    message BetaMsg {
      message BetaInner { optional int32 v = 1; }
      extensions 100 to 200;
      oneof beta_choice {
        int32 a = 1;
        int32 b = 2;
      }
    }
    enum BetaEnum { BE0 = 0; BE1 = 1; }
    extend BetaMsg { optional int32 beta_ext = 100; })schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp "
      "a.proto b.proto");
  ExpectNoErrors();

  std::string entry_point = FileContents("generated.rs");
  EXPECT_THAT(entry_point, HasSubstr("pub use a_proto::*;"));
  EXPECT_THAT(entry_point, HasSubstr("pub use b_proto::*;"));
}

}  // namespace
}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
