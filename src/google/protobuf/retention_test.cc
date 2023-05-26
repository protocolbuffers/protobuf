// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

#include "google/protobuf/compiler/retention.h"

#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/parser.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unittest_retention.pb.h"
#include "google/protobuf/util/message_differencer.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

TEST(RetentionTest, DirectOptions) {
  const FileOptions& file_options =
      protobuf_unittest::OptionsMessage::descriptor()->file()->options();
  EXPECT_EQ(file_options.GetExtension(protobuf_unittest::plain_option), 1);
  EXPECT_EQ(
      file_options.GetExtension(protobuf_unittest::runtime_retention_option), 2);
  // RETENTION_SOURCE option should be stripped.
  EXPECT_FALSE(
      file_options.HasExtension(protobuf_unittest::source_retention_option));
  EXPECT_EQ(file_options.GetExtension(protobuf_unittest::source_retention_option),
            0);
}

void CheckOptionsMessageIsStrippedCorrectly(
    const protobuf_unittest::OptionsMessage& options) {
  EXPECT_EQ(options.plain_field(), 1);
  EXPECT_EQ(options.runtime_retention_field(), 2);
  // RETENTION_SOURCE field should be stripped.
  EXPECT_FALSE(options.has_source_retention_field());
  EXPECT_EQ(options.source_retention_field(), 0);
}

TEST(RetentionTest, FieldsNestedInRepeatedMessage) {
  const FileOptions& file_options =
      protobuf_unittest::OptionsMessage::descriptor()->file()->options();
  ASSERT_EQ(1, file_options.ExtensionSize(protobuf_unittest::repeated_options));
  const protobuf_unittest::OptionsMessage& options_message =
      file_options.GetRepeatedExtension(protobuf_unittest::repeated_options)[0];
  CheckOptionsMessageIsStrippedCorrectly(options_message);
}

TEST(RetentionTest, File) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::OptionsMessage::descriptor()
          ->file()
          ->options()
          .GetExtension(protobuf_unittest::file_option));
}

TEST(RetentionTest, TopLevelMessage) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::TopLevelMessage::descriptor()->options().GetExtension(
          protobuf_unittest::message_option));
}

TEST(RetentionTest, NestedMessage) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::TopLevelMessage::NestedMessage::descriptor()
          ->options()
          .GetExtension(protobuf_unittest::message_option));
}

TEST(RetentionTest, TopLevelEnum) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::TopLevelEnum_descriptor()->options().GetExtension(
          protobuf_unittest::enum_option));
}

TEST(RetentionTest, NestedEnum) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::TopLevelMessage::NestedEnum_descriptor()
          ->options()
          .GetExtension(protobuf_unittest::enum_option));
}

TEST(RetentionTest, EnumEntry) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::TopLevelEnum_descriptor()
          ->value(0)
          ->options()
          .GetExtension(protobuf_unittest::enum_entry_option));
}

TEST(RetentionTest, TopLevelExtension) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::TopLevelMessage::descriptor()
          ->file()
          ->FindExtensionByName("i")
          ->options()
          .GetExtension(protobuf_unittest::field_option));
}

TEST(RetentionTest, NestedExtension) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::TopLevelMessage::descriptor()
          ->extension(0)
          ->options()
          .GetExtension(protobuf_unittest::field_option));
}

TEST(RetentionTest, Field) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::TopLevelMessage::descriptor()
          ->field(0)
          ->options()
          .GetExtension(protobuf_unittest::field_option));
}

TEST(RetentionTest, Oneof) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::TopLevelMessage::descriptor()
          ->oneof_decl(0)
          ->options()
          .GetExtension(protobuf_unittest::oneof_option));
}

TEST(RetentionTest, ExtensionRange) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::TopLevelMessage::descriptor()
          ->extension_range(0)
          ->options()
          .GetExtension(protobuf_unittest::extension_range_option));
}

TEST(RetentionTest, Service) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::TopLevelMessage::descriptor()
          ->file()
          ->service(0)
          ->options()
          .GetExtension(protobuf_unittest::service_option));
}

TEST(RetentionTest, Method) {
  CheckOptionsMessageIsStrippedCorrectly(
      protobuf_unittest::TopLevelMessage::descriptor()
          ->file()
          ->service(0)
          ->method(0)
          ->options()
          .GetExtension(protobuf_unittest::method_option));
}


TEST(RetentionTest, StripSourceRetentionOptionsWithSourceCodeInfo) {
  // The tests above make assertions against the generated code, but this test
  // case directly examines the result of the StripSourceRetentionOptions()
  // function instead.
  std::string proto_file =
      absl::Substitute(R"(
      syntax = "proto2";

      package google.protobuf.internal;

      import "$0";

      option (source_retention_option) = 123;
      option (options) = {
        i1: 123
        i2: 456
        c { s: "abc" }
        rc { s: "abc" }
      };
      option (repeated_options) = {
        i1: 111 i2: 222
      };

      message Options {
        optional int32 i1 = 1 [retention = RETENTION_SOURCE];
        optional int32 i2 = 2;
        message ChildMessage {
          optional string s = 1 [retention = RETENTION_SOURCE];
        }
        optional ChildMessage c = 3;
        repeated ChildMessage rc = 4;
      }

      extend google.protobuf.FileOptions {
        optional int32 source_retention_option = 50000 [retention = RETENTION_SOURCE];
        optional Options options = 50001;
        repeated Options repeated_options = 50002;
      })",
                       FileDescriptorSet::descriptor()->file()->name());
  io::ArrayInputStream input_stream(proto_file.data(),
                                    static_cast<int>(proto_file.size()));
  io::ErrorCollector error_collector;
  io::Tokenizer tokenizer(&input_stream, &error_collector);
  compiler::Parser parser;
  FileDescriptorProto file_descriptor;
  ASSERT_TRUE(parser.Parse(&tokenizer, &file_descriptor));
  file_descriptor.set_name("retention.proto");

  DescriptorPool pool;
  FileDescriptorProto descriptor_proto_descriptor;
  FileDescriptorSet::descriptor()->file()->CopyTo(&descriptor_proto_descriptor);
  pool.BuildFile(descriptor_proto_descriptor);
  pool.BuildFile(file_descriptor);

  FileDescriptorProto stripped_file = compiler::StripSourceRetentionOptions(
      *pool.FindFileByName("retention.proto"),
      /*include_source_code_info=*/true);
  EXPECT_EQ(stripped_file.source_code_info().location_size(), 63);
}

TEST(RetentionTest, RemoveEmptyOptions) {
  // If an options message is completely empty after stripping, that message
  // should be removed.
  std::string proto_file =
      absl::Substitute(R"(
      syntax = "proto2";

      package google.protobuf.internal;

      import "$0";

      message Extendee {
        extensions 1 to max [declaration = {
          number: 1,
          full_name: ".my.ext",
          type: ".my.Message",
        }];
      })",
                       FileDescriptorSet::descriptor()->file()->name());
  io::ArrayInputStream input_stream(proto_file.data(),
                                    static_cast<int>(proto_file.size()));
  io::ErrorCollector error_collector;
  io::Tokenizer tokenizer(&input_stream, &error_collector);
  compiler::Parser parser;
  FileDescriptorProto file_descriptor;
  ASSERT_TRUE(parser.Parse(&tokenizer, &file_descriptor));
  file_descriptor.set_name("retention.proto");

  DescriptorPool pool;
  FileDescriptorProto descriptor_proto_descriptor;
  FileDescriptorSet::descriptor()->file()->CopyTo(&descriptor_proto_descriptor);
  pool.BuildFile(descriptor_proto_descriptor);
  pool.BuildFile(file_descriptor);

  FileDescriptorProto stripped_file = compiler::StripSourceRetentionOptions(
      *pool.FindFileByName("retention.proto"));
  ASSERT_EQ(stripped_file.message_type_size(), 1);
  ASSERT_EQ(stripped_file.message_type(0).extension_range_size(), 1);
  EXPECT_FALSE(stripped_file.message_type(0).extension_range(0).has_options());
}

TEST(RetentionTest, InvalidDescriptor) {
  // This test creates an invalid descriptor and makes sure we can strip its
  // options without crashing.
  std::string proto_file =
      absl::Substitute(R"(
      syntax = "proto3";

      package google.protobuf.internal;

      import "$0";

      // String option with invalid UTF-8
      option (s) = "\xff";

      extend google.protobuf.FileOptions {
        optional string s = 50000;
      })",
                       FileDescriptorSet::descriptor()->file()->name());
  io::ArrayInputStream input_stream(proto_file.data(),
                                    static_cast<int>(proto_file.size()));
  io::ErrorCollector error_collector;
  io::Tokenizer tokenizer(&input_stream, &error_collector);
  compiler::Parser parser;
  FileDescriptorProto file_descriptor_proto;
  ASSERT_TRUE(parser.Parse(&tokenizer, &file_descriptor_proto));
  file_descriptor_proto.set_name("retention.proto");

  DescriptorPool pool;
  FileDescriptorProto descriptor_proto_descriptor;
  FileDescriptorSet::descriptor()->file()->CopyTo(&descriptor_proto_descriptor);
  ASSERT_NE(pool.BuildFile(descriptor_proto_descriptor), nullptr);
  const FileDescriptor* file_descriptor = pool.BuildFile(file_descriptor_proto);
  ASSERT_NE(file_descriptor, nullptr);

  FileDescriptorProto stripped_file =
      compiler::StripSourceRetentionOptions(*file_descriptor);
}

TEST(RetentionTest, MissingRequiredField) {
  // Retention stripping should work correctly for a descriptor that has
  // options with missing required fields.
  std::string proto_file =
      absl::Substitute(R"(
      syntax = "proto2";

      package google.protobuf.internal;

      import "$0";

      message WithRequiredField {
        required int32 required_field = 1;
        optional int32 optional_field = 2;
      }

      // Option with missing required field
      option (m).optional_field = 42;

      extend google.protobuf.FileOptions {
        optional WithRequiredField m = 50000;
      }

      message Extendee {
        extensions 1 to max [
          declaration = {number: 1 full_name: ".my.ext" type: ".my.Type"}
        ];
      })",
                       FileDescriptorSet::descriptor()->file()->name());
  io::ArrayInputStream input_stream(proto_file.data(),
                                    static_cast<int>(proto_file.size()));
  io::ErrorCollector error_collector;
  io::Tokenizer tokenizer(&input_stream, &error_collector);
  compiler::Parser parser;
  FileDescriptorProto file_descriptor_proto;
  ASSERT_TRUE(parser.Parse(&tokenizer, &file_descriptor_proto));
  file_descriptor_proto.set_name("retention.proto");

  DescriptorPool pool;
  FileDescriptorProto descriptor_proto_descriptor;
  FileDescriptorSet::descriptor()->file()->CopyTo(&descriptor_proto_descriptor);
  ASSERT_NE(pool.BuildFile(descriptor_proto_descriptor), nullptr);
  const FileDescriptor* file_descriptor = pool.BuildFile(file_descriptor_proto);
  ASSERT_NE(file_descriptor, nullptr);

  FileDescriptorProto stripped_file =
      compiler::StripSourceRetentionOptions(*file_descriptor);
  ASSERT_EQ(stripped_file.message_type_size(), 2);
  const DescriptorProto& extendee = stripped_file.message_type(1);
  EXPECT_EQ(extendee.name(), "Extendee");
  ASSERT_EQ(extendee.extension_range_size(), 1);
  EXPECT_EQ(extendee.extension_range(0).options().declaration_size(), 0);
}

TEST(RetentionTest, InvalidRecursionDepth) {
  // The excessive nesting in this proto file will make it impossible for us to
  // use a DynamicMessage to strip custom options, but we should still fall
  // back to stripping built-in options (specifically extension declarations).
  std::string proto_file =
      absl::Substitute(R"(
      syntax = "proto2";

      package google.protobuf.internal;

      import "$0";

      message Recursive {
        optional Recursive r = 1;
      }

      option (r).r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r
              .r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r
              .r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r.r
              .r.r.r.r.r.r.r.r.r.r.r.r = {};

      extend google.protobuf.FileOptions {
        optional Recursive r = 50000;
      }

      message Extendee {
        extensions 1 to max [
          declaration = {number: 1 full_name: ".my.ext" type: ".my.Type"}
        ];
      })",
                       FileDescriptorSet::descriptor()->file()->name());
  io::ArrayInputStream input_stream(proto_file.data(),
                                    static_cast<int>(proto_file.size()));
  io::ErrorCollector error_collector;
  io::Tokenizer tokenizer(&input_stream, &error_collector);
  compiler::Parser parser;
  FileDescriptorProto file_descriptor_proto;
  ASSERT_TRUE(parser.Parse(&tokenizer, &file_descriptor_proto));
  file_descriptor_proto.set_name("retention.proto");

  DescriptorPool pool;
  FileDescriptorProto descriptor_proto_descriptor;
  FileDescriptorSet::descriptor()->file()->CopyTo(&descriptor_proto_descriptor);
  ASSERT_NE(pool.BuildFile(descriptor_proto_descriptor), nullptr);
  const FileDescriptor* file_descriptor = pool.BuildFile(file_descriptor_proto);
  ASSERT_NE(file_descriptor, nullptr);

  FileDescriptorProto stripped_file =
      compiler::StripSourceRetentionOptions(*file_descriptor);
  ASSERT_EQ(stripped_file.message_type_size(), 2);
  const DescriptorProto& extendee = stripped_file.message_type(1);
  EXPECT_EQ(extendee.name(), "Extendee");
  ASSERT_EQ(extendee.extension_range_size(), 1);
  EXPECT_EQ(extendee.extension_range(0).options().declaration_size(), 0);
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
