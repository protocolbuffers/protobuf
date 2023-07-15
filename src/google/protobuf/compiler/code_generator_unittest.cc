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

#include "google/protobuf/compiler/code_generator.h"

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_log.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/parser.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/unittest_features.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace {

using ::testing::NotNull;

class TestGenerator : public CodeGenerator {
 public:
  bool Generate(const FileDescriptor* file, const std::string& parameter,
                GeneratorContext* generator_context,
                std::string* error) const override {
    return true;
  }

  // Expose the protected methods for testing.
  using CodeGenerator::GetRuntimeProto;
  using CodeGenerator::GetSourceFeatures;
  using CodeGenerator::GetSourceRawFeatures;
};

class SimpleErrorCollector : public io::ErrorCollector {
 public:
  void RecordError(int line, int column, absl::string_view message) override {
    ABSL_LOG(ERROR) << absl::StrFormat("%d:%d:%s", line, column, message);
  }
};

class CodeGeneratorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_THAT(BuildFile(DescriptorProto::descriptor()->file()), NotNull());
    ASSERT_THAT(BuildFile(pb::TestMessage::descriptor()->file()), NotNull());
  }

  const FileDescriptor* BuildFile(absl::string_view schema) {
    io::ArrayInputStream input_stream(schema.data(),
                                      static_cast<int>(schema.size()));
    SimpleErrorCollector error_collector;
    io::Tokenizer tokenizer(&input_stream, &error_collector);
    Parser parser;
    parser.RecordErrorsTo(&error_collector);
    FileDescriptorProto proto;
    ABSL_CHECK(parser.Parse(&tokenizer, &proto)) << schema;
    proto.set_name("test.proto");
    return pool_.BuildFile(proto);
  }

  const FileDescriptor* BuildFile(const FileDescriptor* file) {
    FileDescriptorProto proto;
    file->CopyTo(&proto);
    return pool_.BuildFile(proto);
  }

  DescriptorPool pool_;
};

TEST_F(CodeGeneratorTest, GetSourceRawFeaturesRoot) {
  auto file = BuildFile(R"schema(
    edition = "2023";
    package protobuf_unittest;

    import "google/protobuf/unittest_features.proto";

    option features.field_presence = EXPLICIT;  // 2023 default
    option features.enum_type = CLOSED;         // override
    option features.(pb.test).int_file_feature = 8;
    option features.(pb.test).string_source_feature = "file";
  )schema");
  ASSERT_THAT(file, NotNull());

  EXPECT_THAT(TestGenerator::GetSourceRawFeatures(*file),
              google::protobuf::EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: CLOSED
                [pb.test] { int_file_feature: 8 string_source_feature: "file" }
              )pb"));
}

TEST_F(CodeGeneratorTest, GetSourceRawFeaturesInherited) {
  auto file = BuildFile(R"schema(
    edition = "2023";
    package protobuf_unittest;

    import "google/protobuf/unittest_features.proto";

    option features.enum_type = OPEN;
    option features.(pb.test).int_file_feature = 6;
    message EditionsMessage {
      option features.(pb.test).int_message_feature = 7;
      option features.(pb.test).int_multiple_feature = 8;

      string field = 1 [
        features.field_presence = EXPLICIT,
        features.(pb.test).int_multiple_feature = 9,
        features.(pb.test).string_source_feature = "field"
      ];
    }
  )schema");
  ASSERT_THAT(file, NotNull());

  const FieldDescriptor* field =
      file->FindMessageTypeByName("EditionsMessage")->FindFieldByName("field");
  ASSERT_THAT(field, NotNull());

  EXPECT_THAT(
      TestGenerator::GetSourceRawFeatures(*field), google::protobuf::EqualsProto(R"pb(
        field_presence: EXPLICIT
        [pb.test] { int_multiple_feature: 9 string_source_feature: "field" }
      )pb"));
}

TEST_F(CodeGeneratorTest, GetSourceFeaturesRoot) {
  auto file = BuildFile(R"schema(
    edition = "2023";
    package protobuf_unittest;

    import "google/protobuf/unittest_features.proto";

    option features.field_presence = EXPLICIT;  // 2023 default
    option features.enum_type = CLOSED;         // override
    option features.(pb.test).int_file_feature = 8;
    option features.(pb.test).string_source_feature = "file";
  )schema");
  ASSERT_THAT(file, NotNull());

  const FeatureSet& features = TestGenerator::GetSourceFeatures(*file);
  const pb::TestFeatures& ext = features.GetExtension(pb::test);

  EXPECT_TRUE(features.has_repeated_field_encoding());
  EXPECT_TRUE(features.field_presence());
  EXPECT_EQ(features.field_presence(), FeatureSet::EXPLICIT);
  EXPECT_EQ(features.enum_type(), FeatureSet::CLOSED);

  EXPECT_TRUE(ext.has_int_message_feature());
  EXPECT_EQ(ext.int_file_feature(), 8);
  EXPECT_EQ(ext.string_source_feature(), "file");
}

TEST_F(CodeGeneratorTest, GetSourceFeaturesInherited) {
  auto file = BuildFile(R"schema(
    edition = "2023";
    package protobuf_unittest;

    import "google/protobuf/unittest_features.proto";

    option features.enum_type = CLOSED;
    option features.(pb.test).int_source_feature = 5;
    option features.(pb.test).int_file_feature = 6;
    message EditionsMessage {
      option features.(pb.test).int_message_feature = 7;
      option features.(pb.test).int_multiple_feature = 8;
      option features.(pb.test).string_source_feature = "message";

      string field = 1 [
        features.field_presence = IMPLICIT,
        features.(pb.test).int_multiple_feature = 9,
        features.(pb.test).string_source_feature = "field"
      ];
    }
  )schema");
  ASSERT_THAT(file, NotNull());

  const FieldDescriptor* field =
      file->FindMessageTypeByName("EditionsMessage")->FindFieldByName("field");
  ASSERT_THAT(field, NotNull());
  const FeatureSet& features = TestGenerator::GetSourceFeatures(*field);
  const pb::TestFeatures& ext = features.GetExtension(pb::test);

  EXPECT_EQ(features.enum_type(), FeatureSet::CLOSED);
  EXPECT_EQ(features.field_presence(), FeatureSet::IMPLICIT);

  EXPECT_EQ(ext.int_message_feature(), 7);
  EXPECT_EQ(ext.int_file_feature(), 6);
  EXPECT_EQ(ext.int_multiple_feature(), 9);
  EXPECT_EQ(ext.int_source_feature(), 5);
  EXPECT_EQ(ext.string_source_feature(), "field");
}

TEST_F(CodeGeneratorTest, GetRuntimeProtoTrivial) {
  auto file = BuildFile(R"schema(
    edition = "2023";
    package protobuf_unittest;
  )schema");
  ASSERT_THAT(file, NotNull());

  FileDescriptorProto proto = TestGenerator::GetRuntimeProto(*file);
  const FeatureSet& features = proto.options().features();

  EXPECT_TRUE(features.has_raw_features());
  EXPECT_THAT(features.raw_features(), EqualsProto(R"pb()pb"));
}

TEST_F(CodeGeneratorTest, GetRuntimeProtoRoot) {
  auto file = BuildFile(R"schema(
    edition = "2023";
    package protobuf_unittest;

    import "google/protobuf/unittest_features.proto";

    option features.enum_type = CLOSED;
    option features.(pb.test).int_source_feature = 5;
    option features.(pb.test).int_file_feature = 6;
  )schema");
  ASSERT_THAT(file, NotNull());

  FileDescriptorProto proto = TestGenerator::GetRuntimeProto(*file);
  const FeatureSet& features = proto.options().features();
  const pb::TestFeatures& ext = features.GetExtension(pb::test);

  EXPECT_THAT(features.raw_features(),
              EqualsProto(R"pb(enum_type: CLOSED
                               [pb.test] { int_file_feature: 6 })pb"));
  EXPECT_EQ(features.enum_type(), FeatureSet::CLOSED);
  EXPECT_TRUE(features.has_field_presence());
  EXPECT_EQ(features.field_presence(), FeatureSet::EXPLICIT);

  EXPECT_FALSE(ext.has_int_source_feature());
  EXPECT_EQ(ext.int_file_feature(), 6);
}

TEST_F(CodeGeneratorTest, GetRuntimeProtoInherited) {
  auto file = BuildFile(R"schema(
    edition = "2023";
    package protobuf_unittest;

    import "google/protobuf/unittest_features.proto";

    option features.enum_type = CLOSED;
    option features.(pb.test).int_source_feature = 5;
    option features.(pb.test).int_file_feature = 6;
    message EditionsMessage {
      option features.(pb.test).int_message_feature = 7;
      option features.(pb.test).int_multiple_feature = 8;

      string field = 1 [
        features.field_presence = IMPLICIT,
        features.(pb.test).int_multiple_feature = 9,
        features.(pb.test).string_source_feature = "field"
      ];
    }
  )schema");
  ASSERT_THAT(file, NotNull());

  FileDescriptorProto proto = TestGenerator::GetRuntimeProto(*file);
  const FieldDescriptorProto& field = proto.message_type(0).field(0);
  const FeatureSet& features = field.options().features();
  const pb::TestFeatures& ext = features.GetExtension(pb::test);

  EXPECT_THAT(features.raw_features(), google::protobuf::EqualsProto(R"pb(
                field_presence: IMPLICIT
                [pb.test] { int_multiple_feature: 9 }
              )pb"));
  EXPECT_EQ(features.enum_type(), FeatureSet::CLOSED);
  EXPECT_EQ(features.field_presence(), FeatureSet::IMPLICIT);
  EXPECT_EQ(ext.int_multiple_feature(), 9);
  EXPECT_EQ(ext.int_message_feature(), 7);
  EXPECT_EQ(ext.int_file_feature(), 6);
  EXPECT_FALSE(ext.has_int_source_feature());
  EXPECT_FALSE(ext.has_string_source_feature());
}

}  // namespace
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
