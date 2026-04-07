// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/generator.h"

#include <cstddef>
#include <memory>
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

}  // namespace
}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
