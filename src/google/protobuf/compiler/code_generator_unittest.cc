// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/code_generator.h"

#include <cstdint>
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

#define ASSERT_OK(x) ASSERT_TRUE(x.ok()) << x.message();

using ::testing::HasSubstr;
using ::testing::NotNull;

class TestGenerator : public CodeGenerator {
 public:
  bool Generate(const FileDescriptor* file, const std::string& parameter,
                GeneratorContext* generator_context,
                std::string* error) const override {
    return true;
  }

  uint64_t GetSupportedFeatures() const override { return features_; }
  void set_supported_features(uint64_t features) { features_ = features; }

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
  uint64_t features_ = CodeGenerator::Feature::FEATURE_SUPPORTS_EDITIONS;
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
    option features.(pb.test).file_feature = VALUE5;
    option features.(pb.test).source_feature = VALUE6;
  )schema");
  ASSERT_THAT(file, NotNull());

  EXPECT_THAT(TestGenerator::GetUnresolvedSourceFeatures(*file, pb::test),
              google::protobuf::EqualsProto(R"pb(
                file_feature: VALUE5
                source_feature: VALUE6
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
    option features.(pb.test).file_feature = VALUE4;
    message EditionsMessage {
      option features.(pb.test).message_feature = VALUE5;
      option features.(pb.test).multiple_feature = VALUE6;

      string field = 1 [
        features.field_presence = EXPLICIT,
        features.(pb.test).multiple_feature = VALUE3,
        features.(pb.test).source_feature = VALUE2
      ];
    }
  )schema");
  ASSERT_THAT(file, NotNull());

  const FieldDescriptor* field =
      file->FindMessageTypeByName("EditionsMessage")->FindFieldByName("field");
  ASSERT_THAT(field, NotNull());

  EXPECT_THAT(TestGenerator::GetUnresolvedSourceFeatures(*field, pb::test),
              google::protobuf::EqualsProto(R"pb(
                multiple_feature: VALUE3
                source_feature: VALUE2
              )pb"));
}

TEST_F(CodeGeneratorTest, GetResolvedSourceFeaturesRoot) {
  TestGenerator generator;
  generator.set_feature_extensions({GetExtensionReflection(pb::test)});
  ASSERT_OK(pool_.SetFeatureSetDefaults(*generator.BuildFeatureSetDefaults()));

  ASSERT_THAT(BuildFile(DescriptorProto::descriptor()->file()), NotNull());
  ASSERT_THAT(BuildFile(pb::TestMessage::descriptor()->file()), NotNull());
  auto file = BuildFile(R"schema(
    edition = "2023";
    package protobuf_unittest;

    import "google/protobuf/unittest_features.proto";

    option features.field_presence = EXPLICIT;  // 2023 default
    option features.enum_type = CLOSED;         // override
    option features.(pb.test).file_feature = VALUE6;
    option features.(pb.test).source_feature = VALUE5;
  )schema");
  ASSERT_THAT(file, NotNull());

  const FeatureSet& features = TestGenerator::GetResolvedSourceFeatures(*file);
  const pb::TestFeatures& ext = features.GetExtension(pb::test);

  EXPECT_TRUE(features.has_repeated_field_encoding());
  EXPECT_TRUE(features.field_presence());
  EXPECT_EQ(features.field_presence(), FeatureSet::EXPLICIT);
  EXPECT_EQ(features.enum_type(), FeatureSet::CLOSED);

  EXPECT_EQ(ext.file_feature(), pb::EnumFeature::VALUE6);
  EXPECT_EQ(ext.source_feature(), pb::EnumFeature::VALUE5);
  EXPECT_EQ(ext.field_feature(), pb::EnumFeature::VALUE1);
}

TEST_F(CodeGeneratorTest, GetResolvedSourceFeaturesInherited) {
  TestGenerator generator;
  generator.set_feature_extensions({GetExtensionReflection(pb::test)});
  ASSERT_OK(pool_.SetFeatureSetDefaults(*generator.BuildFeatureSetDefaults()));

  ASSERT_THAT(BuildFile(DescriptorProto::descriptor()->file()), NotNull());
  ASSERT_THAT(BuildFile(pb::TestMessage::descriptor()->file()), NotNull());
  auto file = BuildFile(R"schema(
    edition = "2023";
    package protobuf_unittest;

    import "google/protobuf/unittest_features.proto";

    option features.enum_type = CLOSED;
    option features.(pb.test).source_feature = VALUE5;
    option features.(pb.test).file_feature = VALUE6;
    message EditionsMessage {
      option features.(pb.test).message_feature = VALUE4;
      option features.(pb.test).multiple_feature = VALUE3;
      option features.(pb.test).source_feature2 = VALUE2;

      string field = 1 [
        features.field_presence = IMPLICIT,
        features.(pb.test).multiple_feature = VALUE5,
        features.(pb.test).source_feature2 = VALUE3
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

  EXPECT_EQ(ext.message_feature(), pb::EnumFeature::VALUE4);
  EXPECT_EQ(ext.file_feature(), pb::EnumFeature::VALUE6);
  EXPECT_EQ(ext.multiple_feature(), pb::EnumFeature::VALUE5);
  EXPECT_EQ(ext.source_feature(), pb::EnumFeature::VALUE5);
  EXPECT_EQ(ext.source_feature2(), pb::EnumFeature::VALUE3);
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
                  overridable_features {}
                  fixed_features {
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
                  overridable_features {}
                  fixed_features {
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
                  overridable_features {
                    field_presence: EXPLICIT
                    enum_type: OPEN
                    repeated_field_encoding: PACKED
                    utf8_validation: VERIFY
                    message_encoding: LENGTH_PREFIXED
                    json_format: ALLOW
                  }
                  fixed_features {}
                }
                minimum_edition: EDITION_99997_TEST_ONLY
                maximum_edition: EDITION_99999_TEST_ONLY
              )pb")));
}

TEST_F(CodeGeneratorTest, BuildFeatureSetDefaultsUnsupported) {
  TestGenerator generator;
  generator.set_supported_features(0);
  generator.set_feature_extensions({});
  generator.set_minimum_edition(EDITION_99997_TEST_ONLY);
  generator.set_maximum_edition(EDITION_99999_TEST_ONLY);
  auto result = generator.BuildFeatureSetDefaults();

  ASSERT_TRUE(result.ok()) << result.status().message();
  EXPECT_EQ(result->minimum_edition(), PROTOBUF_MINIMUM_EDITION);
  EXPECT_EQ(result->maximum_edition(), PROTOBUF_MAXIMUM_EDITION);
}

#include "google/protobuf/port_undef.inc"

}  // namespace
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
