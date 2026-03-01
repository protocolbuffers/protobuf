// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/generator.h"

#include <memory>

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
};

// The Rust generator will append this comment somewhere near the end of the
// generated source file, followed by a base64-encoded wire format message
// and an endline.
constexpr absl::string_view kMetadataComment = "// google.protobuf.GeneratedCodeInfo ";

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
  constexpr absl::string_view kFooProto = R"schema(
    syntax = "proto2";
    package foo;
    message Message {
    })schema";
  CreateTempFile("foo.proto", kFooProto);
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir "
      "--rust_out=$tmpdir "
      "--rust_opt=experimental-codegen=enabled,kernel=cpp,annotate_code=true "
      "foo.proto");

  ExpectNoErrors();
  auto file_contents = FileContents("foo.c.pb.rs");
  auto metadata_comment_pos = file_contents.find(kMetadataComment);
  ASSERT_NE(metadata_comment_pos, std::string::npos);
  metadata_comment_pos += kMetadataComment.size();
  auto metadata_comment_end_pos =
      file_contents.find('\n', metadata_comment_pos);
  ASSERT_NE(metadata_comment_end_pos, std::string::npos);
  auto metadata_comment = file_contents.substr(
      metadata_comment_pos, metadata_comment_end_pos - metadata_comment_pos);
  std::string decoded_metadata;
  ASSERT_TRUE(absl::Base64Unescape(metadata_comment, &decoded_metadata));
  google::protobuf::GeneratedCodeInfo generated_code_info;
  ASSERT_TRUE(generated_code_info.ParseFromString(decoded_metadata));

  ASSERT_EQ(generated_code_info.annotation_size(), 1);
  const auto& annotation = generated_code_info.annotation(0);
  ASSERT_EQ(annotation.path_size(), 2);
  EXPECT_EQ(annotation.path(0),
            google::protobuf::FileDescriptorProto::kMessageTypeFieldNumber);
  EXPECT_EQ(annotation.path(1), 0);
  EXPECT_EQ(annotation.source_file(), "foo.proto");
  ASSERT_LE(annotation.begin(), file_contents.size());
  ASSERT_LE(annotation.end(), file_contents.size());
  // We expect the Rust name for the message type to be `Message`. Check to see
  // the annotation covers that string.
  ASSERT_EQ(file_contents.substr(annotation.begin(),
                                 annotation.end() - annotation.begin()),
            "Message");
}

}  // namespace
}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
