// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/code_generator.h"

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/parser.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/unittest_features.pb.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace {

using ::testing::HasSubstr;
using ::testing::NotNull;

class TestGenerator : public CodeGenerator {
 public:
  bool Generate(const FileDescriptor* file, const std::string& parameter,
                GeneratorContext* generator_context,
                std::string* error) const override {
    return true;
  }

  std::vector<const FieldDescriptor*> GetFeatureExtensions() const override {
    return feature_extensions_;
  }
  void set_feature_extensions(std::vector<const FieldDescriptor*> extensions) {
    feature_extensions_ = extensions;
  }

  Edition GetMinimumEdition() const override { return minimum_edition_; }
  void set_minimum_edition(Edition minimum_edition) {
    minimum_edition_ = minimum_edition;
  }

  Edition GetMaximumEdition() const override { return maximum_edition_; }
  void set_maximum_edition(Edition maximum_edition) {
    maximum_edition_ = maximum_edition;
  }

  // Expose the protected methods for testing.
  using CodeGenerator::GetResolvedSourceFeatures;
  using CodeGenerator::GetUnresolvedSourceFeatures;

 private:
  Edition minimum_edition_ = PROTOBUF_MINIMUM_EDITION;
  Edition maximum_edition_ = PROTOBUF_MAXIMUM_EDITION;
  std::vector<const FieldDescriptor*> feature_extensions_ = {
      GetExtensionReflection(pb::test)};
};

class SimpleErrorCollector : public io::ErrorCollector {
 public:
  void RecordError(int line, int column, absl::string_view message) override {
    ABSL_LOG(ERROR) << absl::StrFormat("%d:%d:%s", line, column, message);
  }
};

class CodeGeneratorTest : public ::testing::Test {
 protected:
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

TEST_F(CodeGeneratorTest, GetUnresolvedSourceFeaturesRoot) {
  ASSERT_THAT(BuildFile(DescriptorProto::descriptor()->file()), NotNull());
  ASSERT_THAT(BuildFile(pb::TestMessage::descriptor()->file()), NotNull());
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

  EXPECT_THAT(TestGenerator::GetUnresolvedSourceFeatures(*file, pb::test),
              google::protobuf::EqualsProto(R"pb(
                int_file_feature: 8
                string_source_feature: "file"
              )pb"));
}

TEST_F(CodeGeneratorTest, GetUnresolvedSourceFeaturesInherited) {
  ASSERT_THAT(BuildFile(DescriptorProto::descriptor()->file()), NotNull());
  ASSERT_THAT(BuildFile(pb::TestMessage::descriptor()->file()), NotNull());
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

  EXPECT_THAT(TestGenerator::GetUnresolvedSourceFeatures(*field, pb::test),
              google::protobuf::EqualsProto(R"pb(
                int_multiple_feature: 9
                string_source_feature: "field"
              )pb"));
}

TEST_F(CodeGeneratorTest, GetResolvedSourceFeaturesRoot) {
  TestGenerator generator;
  generator.set_feature_extensions({GetExtensionReflection(pb::test)});
  pool_.SetFeatureSetDefaults(*generator.BuildFeatureSetDefaults());

  ASSERT_THAT(BuildFile(DescriptorProto::descriptor()->file()), NotNull());
  ASSERT_THAT(BuildFile(pb::TestMessage::descriptor()->file()), NotNull());
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

  const FeatureSet& features = TestGenerator::GetResolvedSourceFeatures(*file);
  const pb::TestFeatures& ext = features.GetExtension(pb::test);

  EXPECT_TRUE(features.has_repeated_field_encoding());
  EXPECT_TRUE(features.field_presence());
  EXPECT_EQ(features.field_presence(), FeatureSet::EXPLICIT);
  EXPECT_EQ(features.enum_type(), FeatureSet::CLOSED);

  EXPECT_TRUE(ext.has_int_message_feature());
  EXPECT_EQ(ext.int_file_feature(), 8);
  EXPECT_EQ(ext.string_source_feature(), "file");
}

TEST_F(CodeGeneratorTest, GetResolvedSourceFeaturesInherited) {
  TestGenerator generator;
  generator.set_feature_extensions({GetExtensionReflection(pb::test)});
  pool_.SetFeatureSetDefaults(*generator.BuildFeatureSetDefaults());

  ASSERT_THAT(BuildFile(DescriptorProto::descriptor()->file()), NotNull());
  ASSERT_THAT(BuildFile(pb::TestMessage::descriptor()->file()), NotNull());
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
  const FeatureSet& features = TestGenerator::GetResolvedSourceFeatures(*field);
  const pb::TestFeatures& ext = features.GetExtension(pb::test);

  EXPECT_EQ(features.enum_type(), FeatureSet::CLOSED);
  EXPECT_EQ(features.field_presence(), FeatureSet::IMPLICIT);

  EXPECT_EQ(ext.int_message_feature(), 7);
  EXPECT_EQ(ext.int_file_feature(), 6);
  EXPECT_EQ(ext.int_multiple_feature(), 9);
  EXPECT_EQ(ext.int_source_feature(), 5);
  EXPECT_EQ(ext.string_source_feature(), "field");
}

// TODO: Use the gtest versions once that's available in OSS.
MATCHER_P(HasError, msg_matcher, "") {
  return arg.status().code() == absl::StatusCode::kFailedPrecondition &&
         ExplainMatchResult(msg_matcher, arg.status().message(),
                            result_listener);
}
MATCHER_P(IsOkAndHolds, matcher, "") {
  return arg.ok() && ExplainMatchResult(matcher, *arg, result_listener);
}

TEST_F(CodeGeneratorTest, BuildFeatureSetDefaultsInvalidExtension) {
  TestGenerator generator;
  generator.set_feature_extensions({nullptr});
  EXPECT_THAT(generator.BuildFeatureSetDefaults(),
              HasError(HasSubstr("Unknown extension")));
}

TEST_F(CodeGeneratorTest, BuildFeatureSetDefaults) {
  TestGenerator generator;
  generator.set_feature_extensions({});
  generator.set_minimum_edition(EDITION_99997_TEST_ONLY);
  generator.set_maximum_edition(EDITION_99999_TEST_ONLY);
  EXPECT_THAT(generator.BuildFeatureSetDefaults(),
              IsOkAndHolds(EqualsProto(R"pb(
                defaults {
                  edition: EDITION_PROTO2
                  features {
                    field_presence: EXPLICIT
                    enum_type: CLOSED
                    repeated_field_encoding: EXPANDED
                    utf8_validation: NONE
                    message_encoding: LENGTH_PREFIXED
                    json_format: LEGACY_BEST_EFFORT
                  }
                }
                defaults {
                  edition: EDITION_PROTO3
                  features {
                    field_presence: IMPLICIT
                    enum_type: OPEN
                    repeated_field_encoding: PACKED
                    utf8_validation: VERIFY
                    message_encoding: LENGTH_PREFIXED
                    json_format: ALLOW
                  }
                }
                defaults {
                  edition: EDITION_2023
                  features {
                    field_presence: EXPLICIT
                    enum_type: OPEN
                    repeated_field_encoding: PACKED
                    utf8_validation: VERIFY
                    message_encoding: LENGTH_PREFIXED
                    json_format: ALLOW
                  }
                }
                minimum_edition: EDITION_99997_TEST_ONLY
                maximum_edition: EDITION_99999_TEST_ONLY
              )pb")));
}

#include "google/protobuf/port_undef.inc"

}  // namespace
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
