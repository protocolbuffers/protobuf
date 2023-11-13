// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/feature_resolver.h"

#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/log/die_if_null.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/parser.h"
#include "google/protobuf/cpp_features.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unittest_custom_options.pb.h"
#include "google/protobuf/unittest_features.pb.h"
#include "google/protobuf/stubs/status_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace {

using ::testing::AllOf;
using ::testing::ExplainMatchResult;
using ::testing::HasSubstr;

// TODO: Use the gtest versions once that's available in OSS.
template <typename T>
absl::Status GetStatus(const absl::StatusOr<T>& s) {
  return s.status();
}

MATCHER_P(HasError, msg_matcher, "") {
  return GetStatus(arg).code() == absl::StatusCode::kFailedPrecondition &&
         ExplainMatchResult(msg_matcher, GetStatus(arg).message(),
                            result_listener);
}

MATCHER_P(StatusIs, status,
          absl::StrCat(".status() is ", testing::PrintToString(status))) {
  return GetStatus(arg).code() == status;
}
#define EXPECT_OK(x) EXPECT_THAT(x, StatusIs(absl::StatusCode::kOk))
#define ASSERT_OK(x) ASSERT_THAT(x, StatusIs(absl::StatusCode::kOk))

template <typename ExtensionT>
const FieldDescriptor* GetExtension(
    const ExtensionT& ext,
    const Descriptor* descriptor = FeatureSet::descriptor()) {
  return ABSL_DIE_IF_NULL(descriptor->file()->pool()->FindExtensionByNumber(
      descriptor, ext.number()));
}

template <typename... Extensions>
absl::StatusOr<FeatureResolver> SetupFeatureResolver(Edition edition,
                                                     Extensions... extensions) {
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(),
                                       {GetExtension(extensions)...},
                                       EDITION_2023, EDITION_99997_TEST_ONLY);
  RETURN_IF_ERROR(defaults.status());
  return FeatureResolver::Create(edition, *defaults);
}

absl::StatusOr<FeatureSet> GetDefaults(Edition edition,
                                       const FeatureSetDefaults& defaults) {
  absl::StatusOr<FeatureResolver> resolver =
      FeatureResolver::Create(edition, defaults);
  RETURN_IF_ERROR(resolver.status());
  FeatureSet parent, child;
  return resolver->MergeFeatures(parent, child);
}

template <typename... Extensions>
absl::StatusOr<FeatureSet> GetDefaults(Edition edition,
                                       Extensions... extensions) {
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(),
                                       {GetExtension(extensions)...},
                                       EDITION_2023, EDITION_99999_TEST_ONLY);
  RETURN_IF_ERROR(defaults.status());
  return GetDefaults(edition, *defaults);
}

FileDescriptorProto GetProto(const FileDescriptor* file) {
  FileDescriptorProto proto;
  file->CopyTo(&proto);
  return proto;
}

TEST(FeatureResolverTest, DefaultsCore2023) {
  absl::StatusOr<FeatureSet> merged = GetDefaults(EDITION_2023);
  ASSERT_OK(merged);

  EXPECT_EQ(merged->field_presence(), FeatureSet::EXPLICIT);
  EXPECT_EQ(merged->enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(merged->repeated_field_encoding(), FeatureSet::PACKED);
  EXPECT_EQ(merged->message_encoding(), FeatureSet::LENGTH_PREFIXED);
  EXPECT_FALSE(merged->HasExtension(pb::test));
}

TEST(FeatureResolverTest, DefaultsTest2023) {
  absl::StatusOr<FeatureSet> merged = GetDefaults(EDITION_2023, pb::test);
  ASSERT_OK(merged);

  EXPECT_EQ(merged->field_presence(), FeatureSet::EXPLICIT);
  EXPECT_EQ(merged->enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(merged->repeated_field_encoding(), FeatureSet::PACKED);
  EXPECT_EQ(merged->message_encoding(), FeatureSet::LENGTH_PREFIXED);

  const pb::TestFeatures& ext = merged->GetExtension(pb::test);
  EXPECT_EQ(ext.int_file_feature(), 1);
  EXPECT_EQ(ext.int_extension_range_feature(), 1);
  EXPECT_EQ(ext.int_message_feature(), 1);
  EXPECT_EQ(ext.int_field_feature(), 1);
  EXPECT_EQ(ext.int_oneof_feature(), 1);
  EXPECT_EQ(ext.int_enum_feature(), 1);
  EXPECT_EQ(ext.int_enum_entry_feature(), 1);
  EXPECT_EQ(ext.int_service_feature(), 1);
  EXPECT_EQ(ext.int_method_feature(), 1);
  EXPECT_EQ(ext.bool_field_feature(), false);
  EXPECT_FLOAT_EQ(ext.float_field_feature(), 1.1);
  EXPECT_THAT(ext.message_field_feature(),
              EqualsProto("bool_field: true int_field: 1 float_field: 1.5 "
                          "string_field: '2023'"));
  EXPECT_EQ(ext.enum_field_feature(), pb::TestFeatures::ENUM_VALUE1);
}

TEST(FeatureResolverTest, DefaultsTestMessageExtension) {
  absl::StatusOr<FeatureSet> merged =
      GetDefaults(EDITION_2023, pb::TestMessage::test_message);
  ASSERT_OK(merged);

  EXPECT_EQ(merged->field_presence(), FeatureSet::EXPLICIT);
  EXPECT_EQ(merged->enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(merged->repeated_field_encoding(), FeatureSet::PACKED);
  EXPECT_EQ(merged->message_encoding(), FeatureSet::LENGTH_PREFIXED);
  EXPECT_FALSE(merged->HasExtension(pb::test));

  const pb::TestFeatures& ext =
      merged->GetExtension(pb::TestMessage::test_message);
  EXPECT_EQ(ext.int_file_feature(), 1);
  EXPECT_EQ(ext.int_extension_range_feature(), 1);
  EXPECT_EQ(ext.int_message_feature(), 1);
  EXPECT_EQ(ext.int_field_feature(), 1);
  EXPECT_EQ(ext.int_oneof_feature(), 1);
  EXPECT_EQ(ext.int_enum_feature(), 1);
  EXPECT_EQ(ext.int_enum_entry_feature(), 1);
  EXPECT_EQ(ext.int_service_feature(), 1);
  EXPECT_EQ(ext.int_method_feature(), 1);
  EXPECT_EQ(ext.bool_field_feature(), false);
  EXPECT_FLOAT_EQ(ext.float_field_feature(), 1.1);
  EXPECT_THAT(ext.message_field_feature(),
              EqualsProto("bool_field: true int_field: 1 float_field: 1.5 "
                          "string_field: '2023'"));
  EXPECT_EQ(ext.enum_field_feature(), pb::TestFeatures::ENUM_VALUE1);
}

TEST(FeatureResolverTest, DefaultsTestNestedExtension) {
  absl::StatusOr<FeatureSet> merged =
      GetDefaults(EDITION_2023, pb::TestMessage::Nested::test_nested);
  ASSERT_OK(merged);

  EXPECT_EQ(merged->field_presence(), FeatureSet::EXPLICIT);
  EXPECT_EQ(merged->enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(merged->repeated_field_encoding(), FeatureSet::PACKED);
  EXPECT_EQ(merged->message_encoding(), FeatureSet::LENGTH_PREFIXED);
  EXPECT_FALSE(merged->HasExtension(pb::test));

  const pb::TestFeatures& ext =
      merged->GetExtension(pb::TestMessage::Nested::test_nested);
  EXPECT_EQ(ext.int_file_feature(), 1);
  EXPECT_EQ(ext.int_extension_range_feature(), 1);
  EXPECT_EQ(ext.int_message_feature(), 1);
  EXPECT_EQ(ext.int_field_feature(), 1);
  EXPECT_EQ(ext.int_oneof_feature(), 1);
  EXPECT_EQ(ext.int_enum_feature(), 1);
  EXPECT_EQ(ext.int_enum_entry_feature(), 1);
  EXPECT_EQ(ext.int_service_feature(), 1);
  EXPECT_EQ(ext.int_method_feature(), 1);
  EXPECT_EQ(ext.bool_field_feature(), false);
  EXPECT_FLOAT_EQ(ext.float_field_feature(), 1.1);
  EXPECT_THAT(ext.message_field_feature(),
              EqualsProto("bool_field: true int_field: 1 float_field: 1.5 "
                          "string_field: '2023'"));
  EXPECT_EQ(ext.enum_field_feature(), pb::TestFeatures::ENUM_VALUE1);
}

TEST(FeatureResolverTest, DefaultsGeneratedPoolCustom) {
  DescriptorPool pool;
  ASSERT_NE(
      pool.BuildFile(GetProto(google::protobuf::DescriptorProto::descriptor()->file())),
      nullptr);
  ASSERT_NE(pool.BuildFile(GetProto(pb::TestFeatures::descriptor()->file())),
            nullptr);
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(
          pool.FindMessageTypeByName("google.protobuf.FeatureSet"),
          {pool.FindExtensionByName("pb.test")}, EDITION_2023, EDITION_2023);
  ASSERT_OK(defaults);
  ASSERT_EQ(defaults->defaults().size(), 3);
  ASSERT_EQ(defaults->defaults().at(2).edition(), EDITION_2023);
  FeatureSet merged = defaults->defaults().at(2).features();

  EXPECT_EQ(merged.field_presence(), FeatureSet::EXPLICIT);
  EXPECT_TRUE(merged.HasExtension(pb::test));
  EXPECT_EQ(merged.GetExtension(pb::test).int_file_feature(), 1);
  EXPECT_FALSE(merged.HasExtension(pb::cpp));
}

TEST(FeatureResolverTest, DefaultsTooEarly) {
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(),
                                       {GetExtension(pb::test)}, EDITION_2023,
                                       EDITION_2023);
  ASSERT_OK(defaults);
  defaults->set_minimum_edition(EDITION_1_TEST_ONLY);
  absl::StatusOr<FeatureSet> merged =
      GetDefaults(EDITION_1_TEST_ONLY, *defaults);
  EXPECT_THAT(merged, HasError(AllOf(HasSubstr("No valid default found"),
                                     HasSubstr("1_TEST_ONLY"))));
}

TEST(FeatureResolverTest, DefaultsFarFuture) {
  absl::StatusOr<FeatureSet> merged =
      GetDefaults(EDITION_99999_TEST_ONLY, pb::test);
  ASSERT_OK(merged);

  pb::TestFeatures ext = merged->GetExtension(pb::test);
  EXPECT_EQ(ext.int_file_feature(), 3);
  EXPECT_THAT(ext.message_field_feature(),
              EqualsProto("bool_field: true int_field: 2  float_field: 1.5 "
                          "string_field: '2024'"));
  EXPECT_EQ(ext.enum_field_feature(), pb::TestFeatures::ENUM_VALUE3);
}

TEST(FeatureResolverTest, DefaultsMiddleEdition) {
  absl::StatusOr<FeatureSet> merged =
      GetDefaults(EDITION_99997_TEST_ONLY, pb::test);
  ASSERT_OK(merged);

  pb::TestFeatures ext = merged->GetExtension(pb::test);
  EXPECT_EQ(ext.int_file_feature(), 2);
  EXPECT_EQ(ext.enum_field_feature(), pb::TestFeatures::ENUM_VALUE2);
}

TEST(FeatureResolverTest, DefaultsMessageMerge) {
  {
    absl::StatusOr<FeatureSet> merged = GetDefaults(EDITION_2023, pb::test);
    ASSERT_OK(merged);
    pb::TestFeatures ext = merged->GetExtension(pb::test);
    EXPECT_THAT(ext.message_field_feature(),
                EqualsProto(R"pb(bool_field: true
                                 int_field: 1
                                 float_field: 1.5
                                 string_field: '2023')pb"));
  }
  {
    absl::StatusOr<FeatureSet> merged =
        GetDefaults(EDITION_99997_TEST_ONLY, pb::test);
    ASSERT_OK(merged);
    pb::TestFeatures ext = merged->GetExtension(pb::test);
    EXPECT_THAT(ext.message_field_feature(),
                EqualsProto(R"pb(bool_field: true
                                 int_field: 2
                                 float_field: 1.5
                                 string_field: '2023')pb"));
  }
  {
    absl::StatusOr<FeatureSet> merged =
        GetDefaults(EDITION_99998_TEST_ONLY, pb::test);
    ASSERT_OK(merged);
    pb::TestFeatures ext = merged->GetExtension(pb::test);
    EXPECT_THAT(ext.message_field_feature(),
                EqualsProto(R"pb(bool_field: true
                                 int_field: 2
                                 float_field: 1.5
                                 string_field: '2024')pb"));
  }
}

TEST(FeatureResolverTest, CreateFromUnsortedDefaults) {
  auto valid_defaults = FeatureResolver::CompileDefaults(
      FeatureSet::descriptor(), {}, EDITION_PROTO2, EDITION_2023);
  ASSERT_OK(valid_defaults);
  FeatureSetDefaults defaults = *valid_defaults;

  defaults.mutable_defaults()->SwapElements(0, 1);

  EXPECT_THAT(FeatureResolver::Create(EDITION_2023, defaults),
              HasError(AllOf(HasSubstr("not strictly increasing."),
                             HasSubstr("Edition PROTO3 is greater "
                                       "than or equal to edition PROTO2"))));
}

TEST(FeatureResolverTest, CreateUnknownEdition) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    minimum_edition: EDITION_UNKNOWN
    maximum_edition: EDITION_99999_TEST_ONLY
    defaults {
      edition: EDITION_UNKNOWN
      features {}
    }
  )pb");
  EXPECT_THAT(FeatureResolver::Create(EDITION_2023, defaults),
              HasError(HasSubstr("Invalid edition UNKNOWN")));
}

TEST(FeatureResolverTest, CreateMissingEdition) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    minimum_edition: EDITION_UNKNOWN
    maximum_edition: EDITION_99999_TEST_ONLY
    defaults { features {} }
  )pb");
  EXPECT_THAT(FeatureResolver::Create(EDITION_2023, defaults),
              HasError(HasSubstr("Invalid edition UNKNOWN")));
}

TEST(FeatureResolverTest, CreateUnknownEnumFeature) {
  auto valid_defaults = FeatureResolver::CompileDefaults(
      FeatureSet::descriptor(), {}, EDITION_2023, EDITION_2023);
  ASSERT_OK(valid_defaults);

  // Use reflection to make sure we validate every enum feature in FeatureSet.
  const Descriptor& descriptor = *FeatureSet::descriptor();
  for (int i = 0; i < descriptor.field_count(); ++i) {
    const FieldDescriptor& field = *descriptor.field(i);

    FeatureSetDefaults defaults = *valid_defaults;
    FeatureSet* features =
        defaults.mutable_defaults()->Mutable(0)->mutable_features();
    const Reflection& reflection = *features->GetReflection();

    // Clear the feature, which should be invalid.
    reflection.ClearField(features, &field);
    EXPECT_THAT(FeatureResolver::Create(EDITION_2023, defaults),
                HasError(AllOf(HasSubstr(field.name()),
                               HasSubstr("must resolve to a known value"))));
  }
}

TEST(FeatureResolverTest, CompileDefaultsMissingDescriptor) {
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(nullptr, {}, EDITION_2023, EDITION_2023),
      HasError(HasSubstr("find definition of google.protobuf.FeatureSet")));
}

TEST(FeatureResolverTest, CompileDefaultsMissingExtension) {
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(), {nullptr},
                                       EDITION_2023, EDITION_2023),
      HasError(HasSubstr("Unknown extension")));
}

TEST(FeatureResolverTest, CompileDefaultsInvalidExtension) {
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(
          FeatureSet::descriptor(),
          {GetExtension(protobuf_unittest::file_opt1, FileOptions::descriptor())},
          EDITION_2023, EDITION_2023),
      HasError(HasSubstr("is not an extension of")));
}

TEST(FeatureResolverTest, CompileDefaultsMinimumLaterThanMaximum) {
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(), {},
                                       EDITION_99999_TEST_ONLY, EDITION_2023),
      HasError(AllOf(HasSubstr("Invalid edition range"),
                     HasSubstr("99999_TEST_ONLY is newer"),
                     HasSubstr("2023"))));
}

TEST(FeatureResolverTest, MergeFeaturesChildOverrideCore) {
  absl::StatusOr<FeatureResolver> resolver = SetupFeatureResolver(EDITION_2023);
  ASSERT_OK(resolver);
  FeatureSet child = ParseTextOrDie(R"pb(
    field_presence: IMPLICIT
    repeated_field_encoding: EXPANDED
  )pb");
  absl::StatusOr<FeatureSet> merged =
      resolver->MergeFeatures(FeatureSet(), child);
  ASSERT_OK(merged);

  EXPECT_EQ(merged->field_presence(), FeatureSet::IMPLICIT);
  EXPECT_EQ(merged->enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(merged->repeated_field_encoding(), FeatureSet::EXPANDED);
  EXPECT_EQ(merged->message_encoding(), FeatureSet::LENGTH_PREFIXED);
}

TEST(FeatureResolverTest, MergeFeaturesChildOverrideComplex) {
  absl::StatusOr<FeatureResolver> resolver =
      SetupFeatureResolver(EDITION_2023, pb::test);
  ASSERT_OK(resolver);
  FeatureSet child = ParseTextOrDie(R"pb(
    field_presence: IMPLICIT
    repeated_field_encoding: EXPANDED
    [pb.test] {
      int_field_feature: 5
      enum_field_feature: ENUM_VALUE4
      message_field_feature { int_field: 10 }
    }
  )pb");
  absl::StatusOr<FeatureSet> merged =
      resolver->MergeFeatures(FeatureSet(), child);
  ASSERT_OK(merged);

  EXPECT_EQ(merged->field_presence(), FeatureSet::IMPLICIT);
  EXPECT_EQ(merged->enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(merged->repeated_field_encoding(), FeatureSet::EXPANDED);
  EXPECT_EQ(merged->message_encoding(), FeatureSet::LENGTH_PREFIXED);

  pb::TestFeatures ext = merged->GetExtension(pb::test);
  EXPECT_EQ(ext.int_file_feature(), 1);
  EXPECT_EQ(ext.int_field_feature(), 5);
  EXPECT_FLOAT_EQ(ext.float_field_feature(), 1.1);
  EXPECT_THAT(ext.message_field_feature(),
              EqualsProto("bool_field: true int_field: 10 float_field: 1.5 "
                          "string_field: '2023'"));
  EXPECT_EQ(ext.enum_field_feature(), pb::TestFeatures::ENUM_VALUE4);
}

TEST(FeatureResolverTest, MergeFeaturesParentOverrides) {
  absl::StatusOr<FeatureResolver> resolver =
      SetupFeatureResolver(EDITION_2023, pb::test);
  ASSERT_OK(resolver);
  FeatureSet parent = ParseTextOrDie(R"pb(
    field_presence: IMPLICIT
    repeated_field_encoding: EXPANDED
    [pb.test] {
      int_field_feature: 5
      enum_field_feature: ENUM_VALUE4
      message_field_feature { int_field: 10 string_field: "parent" }
    }
  )pb");
  FeatureSet child = ParseTextOrDie(R"pb(
    repeated_field_encoding: PACKED
    [pb.test] {
      int_field_feature: 9
      message_field_feature { bool_field: false int_field: 9 }
    }
  )pb");
  absl::StatusOr<FeatureSet> merged = resolver->MergeFeatures(parent, child);
  ASSERT_OK(merged);

  EXPECT_EQ(merged->field_presence(), FeatureSet::IMPLICIT);
  EXPECT_EQ(merged->enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(merged->repeated_field_encoding(), FeatureSet::PACKED);
  EXPECT_EQ(merged->message_encoding(), FeatureSet::LENGTH_PREFIXED);

  pb::TestFeatures ext = merged->GetExtension(pb::test);
  EXPECT_EQ(ext.int_file_feature(), 1);
  EXPECT_EQ(ext.int_extension_range_feature(), 1);
  EXPECT_EQ(ext.int_message_feature(), 1);
  EXPECT_EQ(ext.int_field_feature(), 9);
  EXPECT_EQ(ext.int_oneof_feature(), 1);
  EXPECT_EQ(ext.int_enum_feature(), 1);
  EXPECT_EQ(ext.int_enum_entry_feature(), 1);
  EXPECT_EQ(ext.int_service_feature(), 1);
  EXPECT_EQ(ext.int_method_feature(), 1);
  EXPECT_EQ(ext.bool_field_feature(), false);
  EXPECT_FLOAT_EQ(ext.float_field_feature(), 1.1);
  EXPECT_THAT(ext.message_field_feature(),
              EqualsProto("bool_field: false int_field: 9 float_field: 1.5 "
                          "string_field: 'parent'"));
  EXPECT_EQ(ext.enum_field_feature(), pb::TestFeatures::ENUM_VALUE4);
}

TEST(FeatureResolverTest, MergeFeaturesUnknownEnumFeature) {
  absl::StatusOr<FeatureResolver> resolver = SetupFeatureResolver(EDITION_2023);
  ASSERT_OK(resolver);

  // Use reflection to make sure we validate every enum feature in FeatureSet.
  const Descriptor& descriptor = *FeatureSet::descriptor();
  for (int i = 0; i < descriptor.field_count(); ++i) {
    const FieldDescriptor& field = *descriptor.field(i);

    FeatureSet features;
    const Reflection& reflection = *features.GetReflection();

    // Set the feature to a value of 0, which is unknown by convention.
    reflection.SetEnumValue(&features, &field, 0);
    EXPECT_THAT(
        resolver->MergeFeatures(FeatureSet(), features),
        HasError(AllOf(
            HasSubstr(field.name()), HasSubstr("must resolve to a known value"),
            HasSubstr(field.enum_type()->FindValueByNumber(0)->name()))));
  }
}

TEST(FeatureResolverTest, MergeFeaturesExtensionEnumUnknown) {
  absl::StatusOr<FeatureResolver> resolver =
      SetupFeatureResolver(EDITION_2023, pb::test);
  ASSERT_OK(resolver);
  FeatureSet child = ParseTextOrDie(R"pb(
    [pb.test] { enum_field_feature: TEST_ENUM_FEATURE_UNKNOWN }
  )pb");
  absl::StatusOr<FeatureSet> merged =
      resolver->MergeFeatures(FeatureSet(), child);
  ASSERT_OK(merged);
  EXPECT_EQ(merged->GetExtension(pb::test).enum_field_feature(),
            pb::TestFeatures::TEST_ENUM_FEATURE_UNKNOWN);
}

TEST(FeatureResolverTest, MergeFeaturesDistantPast) {
  EXPECT_THAT(SetupFeatureResolver(EDITION_1_TEST_ONLY),
              HasError(AllOf(HasSubstr("Edition 1_TEST_ONLY"),
                             HasSubstr("minimum supported edition 2023"))));
}

TEST(FeatureResolverTest, MergeFeaturesDistantFuture) {
  EXPECT_THAT(
      SetupFeatureResolver(EDITION_99998_TEST_ONLY),
      HasError(AllOf(HasSubstr("Edition 99998_TEST_ONLY"),
                     HasSubstr("maximum supported edition 99997_TEST_ONLY"))));
}

class FakeErrorCollector : public io::ErrorCollector {
 public:
  FakeErrorCollector() = default;
  ~FakeErrorCollector() override = default;
  void RecordWarning(int line, int column, absl::string_view message) override {
    ABSL_LOG(WARNING) << line << ":" << column << ": " << message;
  }
  void RecordError(int line, int column, absl::string_view message) override {
    ABSL_LOG(ERROR) << line << ":" << column << ": " << message;
  }
};

class FeatureResolverPoolTest : public testing::Test {
 protected:
  void SetUp() override {
    FileDescriptorProto file;
    FileDescriptorProto::GetDescriptor()->file()->CopyTo(&file);
    ASSERT_NE(pool_.BuildFile(file), nullptr);
    feature_set_ = pool_.FindMessageTypeByName("google.protobuf.FeatureSet");
    ASSERT_NE(feature_set_, nullptr);
    auto defaults = FeatureResolver::CompileDefaults(
        feature_set_, {}, EDITION_2023, EDITION_2023);
    ASSERT_OK(defaults);
    defaults_ = std::move(defaults).value();
  }

  const FileDescriptor* ParseSchema(absl::string_view schema) {
    FakeErrorCollector error_collector;
    io::ArrayInputStream raw_input(schema.data(), schema.size());
    io::Tokenizer input(&raw_input, &error_collector);
    compiler::Parser parser;
    parser.RecordErrorsTo(&error_collector);

    FileDescriptorProto file;

    ABSL_CHECK(parser.Parse(&input, &file));
    file.set_name("foo.proto");
    return pool_.BuildFile(file);
  }

  DescriptorPool pool_;
  FileDescriptorProto file_proto_;
  const Descriptor* feature_set_;
  FeatureSetDefaults defaults_;
};

TEST_F(FeatureResolverPoolTest, CompileDefaultsInvalidNonMessage) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    message Foo {}
    extend google.protobuf.FeatureSet {
      optional string bar = 9999;
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(FeatureResolver::CompileDefaults(feature_set_, {ext},
                                               EDITION_2023, EDITION_2023),
              HasError(AllOf(HasSubstr("test.bar"),
                             HasSubstr("is not of message type"))));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsInvalidRepeated) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    message Foo {}
    extend google.protobuf.FeatureSet {
      repeated Foo bar = 9999;
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, EDITION_2023,
                                       EDITION_2023),
      HasError(AllOf(HasSubstr("test.bar"), HasSubstr("repeated extension"))));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsInvalidWithExtensions) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    message Foo {
      extensions 1;
    }
    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    extend Foo {
      optional Foo bar2 = 1 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_2023, value: "" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, EDITION_2023,
                                       EDITION_2023),
      HasError(AllOf(HasSubstr("test.bar"), HasSubstr("Nested extensions"))));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsInvalidWithOneof) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      oneof x {
        int32 int_field = 1 [
          targets = TARGET_TYPE_FIELD,
          edition_defaults = { edition: EDITION_2023, value: "1" }
        ];
        string string_field = 2 [
          targets = TARGET_TYPE_FIELD,
          edition_defaults = { edition: EDITION_2023, value: "'hello'" }
        ];
      }
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(FeatureResolver::CompileDefaults(feature_set_, {ext},
                                               EDITION_2023, EDITION_2023),
              HasError(AllOf(HasSubstr("test.Foo"),
                             HasSubstr("oneof feature fields"))));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsInvalidWithRequired) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      required int32 required_field = 1 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_2023, value: "" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(FeatureResolver::CompileDefaults(feature_set_, {ext},
                                               EDITION_2023, EDITION_2023),
              HasError(AllOf(HasSubstr("test.Foo.required_field"),
                             HasSubstr("required field"))));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsInvalidWithRepeated) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      repeated int32 repeated_field = 1 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_2023, value: "1" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(FeatureResolver::CompileDefaults(feature_set_, {ext},
                                               EDITION_2023, EDITION_2023),
              HasError(AllOf(HasSubstr("test.Foo.repeated_field"),
                             HasSubstr("repeated field"))));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsInvalidWithMissingTarget) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      optional int32 int_field = 1 [
        edition_defaults = { edition: EDITION_2023, value: "1" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(FeatureResolver::CompileDefaults(feature_set_, {ext},
                                               EDITION_2023, EDITION_2023),
              HasError(AllOf(HasSubstr("test.Foo.int_field"),
                             HasSubstr("no target specified"))));
}

TEST_F(FeatureResolverPoolTest,
       CompileDefaultsInvalidDefaultsMessageParsingError) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      message MessageFeature {
        optional int32 int_field = 1;
      }
      optional MessageFeature message_field_feature = 12 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_PROTO2, value: "9987" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, EDITION_2023,
                                       EDITION_2023),
      HasError(AllOf(HasSubstr("in edition_defaults"), HasSubstr("9987"))));
}

TEST_F(FeatureResolverPoolTest,
       CompileDefaultsInvalidDefaultsMessageParsingErrorMerged) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      message MessageFeature {
        optional int32 int_field = 1;
      }
      optional MessageFeature message_field_feature = 12 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_99998_TEST_ONLY, value: "int_field: 2" },
        edition_defaults = { edition: EDITION_99997_TEST_ONLY, value: "int_field: 1" },
        edition_defaults = { edition: EDITION_PROTO2, value: "" },
        edition_defaults = { edition: EDITION_2023, value: "9987" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, EDITION_2023,
                                       EDITION_99998_TEST_ONLY),
      HasError(AllOf(HasSubstr("in edition_defaults"), HasSubstr("9987"))));
}

TEST_F(FeatureResolverPoolTest,
       CompileDefaultsInvalidDefaultsMessageParsingErrorSkipped) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      message MessageFeature {
        optional int32 int_field = 1;
      }
      optional MessageFeature message_field_feature = 12 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_99997_TEST_ONLY, value: "int_field: 2" },
        edition_defaults = { edition: EDITION_2023, value: "int_field: 1" },
        edition_defaults = { edition: EDITION_99998_TEST_ONLY, value: "9987" },
        edition_defaults = { edition: EDITION_PROTO2, value: "" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  auto defaults = FeatureResolver::CompileDefaults(
      feature_set_, {ext}, EDITION_2023, EDITION_99997_TEST_ONLY);
  ASSERT_OK(defaults);

  auto resolver = FeatureResolver::Create(EDITION_2023, *defaults);
  ASSERT_OK(resolver);
  FeatureSet parent, child;
  EXPECT_OK(resolver->MergeFeatures(parent, child));
}

TEST_F(FeatureResolverPoolTest,
       CompileDefaultsInvalidDefaultsScalarParsingError) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      optional int32 int_field_feature = 12 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_PROTO2, value: "1.23" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, EDITION_2023,
                                       EDITION_2023),
      HasError(AllOf(HasSubstr("in edition_defaults"), HasSubstr("1.23"))));
}

TEST_F(FeatureResolverPoolTest,
       CompileDefaultsInvalidDefaultsScalarParsingErrorSkipped) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      optional int32 int_field_feature = 12 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_99997_TEST_ONLY, value: "1.5" },
        edition_defaults = { edition: EDITION_PROTO2, value: "1" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  auto defaults = FeatureResolver::CompileDefaults(feature_set_, {ext},
                                                   EDITION_2023, EDITION_2023);
  ASSERT_OK(defaults);

  auto resolver = FeatureResolver::Create(EDITION_2023, *defaults);
  ASSERT_OK(resolver);
  FeatureSet parent, child;
  EXPECT_OK(resolver->MergeFeatures(parent, child));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsInvalidDefaultsTooEarly) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      optional int32 int_field_feature = 12 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_2_TEST_ONLY, value: "1" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, EDITION_2023,
                                       EDITION_2023),
      HasError(HasSubstr("No valid default found for edition 2_TEST_ONLY")));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsMinimumTooEarly) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      optional int32 int_field_feature = 12 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_PROTO2, value: "1" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, EDITION_1_TEST_ONLY,
                                       EDITION_99997_TEST_ONLY),
      HasError(HasSubstr("No valid default found for edition 1_TEST_ONLY")));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsMinimumCovered) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      optional int32 int_file_feature = 1 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_99998_TEST_ONLY, value: "2" },
        edition_defaults = { edition: EDITION_2023, value: "1" },
        edition_defaults = { edition: EDITION_PROTO2, value: "0" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  auto defaults = FeatureResolver::CompileDefaults(
      feature_set_, {ext}, EDITION_99997_TEST_ONLY, EDITION_99999_TEST_ONLY);
  ASSERT_OK(defaults);

  EXPECT_THAT(*defaults, EqualsProto(R"pb(
    minimum_edition: EDITION_99997_TEST_ONLY
    maximum_edition: EDITION_99999_TEST_ONLY
    defaults {
      edition: EDITION_PROTO2
      features {
        field_presence: EXPLICIT
        enum_type: CLOSED
        repeated_field_encoding: EXPANDED
        utf8_validation: NONE
        message_encoding: LENGTH_PREFIXED
        json_format: LEGACY_BEST_EFFORT
        [pb.test] { int_file_feature: 0 }
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
        [pb.test] { int_file_feature: 0 }
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
        [pb.test] { int_file_feature: 1 }
      }
    }
    defaults {
      edition: EDITION_99998_TEST_ONLY
      features {
        field_presence: EXPLICIT
        enum_type: OPEN
        repeated_field_encoding: PACKED
        utf8_validation: VERIFY
        message_encoding: LENGTH_PREFIXED
        json_format: ALLOW
        [pb.test] { int_file_feature: 2 }
      }
    }
  )pb"));
}

}  // namespace
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
