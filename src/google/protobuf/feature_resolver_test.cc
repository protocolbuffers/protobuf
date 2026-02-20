// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/feature_resolver.h"

#include <utility>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/log/die_if_null.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
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

using ::absl_testing::StatusIs;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::ExplainMatchResult;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

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
  EXPECT_EQ(ext.file_feature(), pb::VALUE3);
  EXPECT_EQ(ext.extension_range_feature(), pb::VALUE1);
  EXPECT_EQ(ext.message_feature(), pb::VALUE1);
  EXPECT_EQ(ext.field_feature(), pb::VALUE1);
  EXPECT_EQ(ext.oneof_feature(), pb::VALUE1);
  EXPECT_EQ(ext.enum_feature(), pb::VALUE1);
  EXPECT_EQ(ext.enum_entry_feature(), pb::VALUE1);
  EXPECT_EQ(ext.service_feature(), pb::VALUE1);
  EXPECT_EQ(ext.method_feature(), pb::VALUE1);
  EXPECT_FALSE(ext.bool_field_feature());
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

  const pb::TestMessageFeatures& ext =
      merged->GetExtension(pb::TestMessage::test_message);
  EXPECT_EQ(ext.file_feature(), pb::VALUE3);
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

  const pb::TestNestedFeatures& ext =
      merged->GetExtension(pb::TestMessage::Nested::test_nested);
  EXPECT_EQ(ext.file_feature(), pb::VALUE3);
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
  ASSERT_EQ(defaults->defaults().size(), 4);
  ASSERT_EQ(defaults->defaults().at(2).edition(), EDITION_2023);
  FeatureSet merged = defaults->defaults().at(2).overridable_features();

  EXPECT_EQ(merged.field_presence(), FeatureSet::EXPLICIT);
  EXPECT_TRUE(merged.HasExtension(pb::test));
  EXPECT_EQ(merged.GetExtension(pb::test).file_feature(), pb::VALUE3);
  EXPECT_FALSE(merged.HasExtension(pb::cpp));
}

TEST(FeatureResolverTest, DefaultsMergedFeatures) {
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(),
                                       {GetExtension(pb::test)}, EDITION_2023,
                                       EDITION_2023);
  ASSERT_OK(defaults);
  ASSERT_EQ(defaults->defaults_size(), 4);

  defaults->mutable_defaults(2)
      ->mutable_fixed_features()
      ->MutableExtension(pb::test)
      ->set_file_feature(pb::VALUE7);
  defaults->mutable_defaults(2)
      ->mutable_fixed_features()
      ->MutableExtension(pb::test)
      ->set_multiple_feature(pb::VALUE6);
  defaults->mutable_defaults(2)
      ->mutable_overridable_features()
      ->MutableExtension(pb::test)
      ->clear_file_feature();
  defaults->mutable_defaults(2)
      ->mutable_overridable_features()
      ->MutableExtension(pb::test)
      ->set_multiple_feature(pb::VALUE8);

  absl::StatusOr<FeatureSet> features = GetDefaults(EDITION_2023, *defaults);
  ASSERT_OK(features);

  const pb::TestFeatures& ext = features->GetExtension(pb::test);
  EXPECT_EQ(ext.file_feature(), pb::VALUE7);
  EXPECT_EQ(ext.multiple_feature(), pb::VALUE8);
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
  EXPECT_EQ(ext.file_feature(), pb::VALUE5);
  EXPECT_TRUE(ext.bool_field_feature());
}

TEST(FeatureResolverTest, DefaultsMiddleEdition) {
  absl::StatusOr<FeatureSet> merged =
      GetDefaults(EDITION_99997_TEST_ONLY, pb::test);
  ASSERT_OK(merged);

  pb::TestFeatures ext = merged->GetExtension(pb::test);
  EXPECT_EQ(ext.file_feature(), pb::VALUE4);
  EXPECT_TRUE(ext.bool_field_feature());
}

TEST(FeatureResolverTest, CompileDefaultsFixedFutureFeature) {
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(),
                                       {GetExtension(pb::test)}, EDITION_PROTO2,
                                       EDITION_2023);
  ASSERT_OK(defaults);
  ASSERT_EQ(defaults->defaults_size(), 4);

  const auto& edition_defaults = defaults->defaults(2);
  ASSERT_EQ(edition_defaults.edition(), EDITION_2023);

  EXPECT_TRUE(edition_defaults.fixed_features()
                  .GetExtension(pb::test)
                  .has_future_feature());
  EXPECT_EQ(
      edition_defaults.fixed_features().GetExtension(pb::test).future_feature(),
      pb::VALUE1);
  EXPECT_FALSE(edition_defaults.overridable_features()
                   .GetExtension(pb::test)
                   .has_future_feature());
}

TEST(FeatureResolverTest, CompileDefaultsFixedRemovedFeature) {
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(),
                                       {GetExtension(pb::test)}, EDITION_PROTO2,
                                       EDITION_2024);
  ASSERT_OK(defaults);
  ASSERT_EQ(defaults->defaults_size(), 5);

  const auto& edition_defaults = defaults->defaults(3);
  ASSERT_EQ(edition_defaults.edition(), EDITION_2024);

  EXPECT_TRUE(edition_defaults.fixed_features()
                  .GetExtension(pb::test)
                  .has_removed_feature());
  EXPECT_EQ(edition_defaults.fixed_features()
                .GetExtension(pb::test)
                .removed_feature(),
            pb::VALUE3);
  EXPECT_FALSE(edition_defaults.overridable_features()
                   .GetExtension(pb::test)
                   .has_removed_feature());
}

TEST(FeatureResolverTest, CompileDefaultsOverridable) {
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(),
                                       {GetExtension(pb::test)}, EDITION_PROTO2,
                                       EDITION_2023);
  ASSERT_OK(defaults);
  ASSERT_EQ(defaults->defaults_size(), 4);

  const auto& edition_defaults = defaults->defaults(2);
  ASSERT_EQ(edition_defaults.edition(), EDITION_2023);

  EXPECT_FALSE(edition_defaults.fixed_features()
                   .GetExtension(pb::test)
                   .has_removed_feature());
  EXPECT_TRUE(edition_defaults.overridable_features()
                  .GetExtension(pb::test)
                  .has_removed_feature());
  EXPECT_EQ(edition_defaults.overridable_features()
                .GetExtension(pb::test)
                .removed_feature(),
            pb::VALUE2);
}

TEST(FeatureResolverTest, CreateFromUnsortedDefaults) {
  auto valid_defaults = FeatureResolver::CompileDefaults(
      FeatureSet::descriptor(), {}, EDITION_LEGACY, EDITION_2023);
  ASSERT_OK(valid_defaults);
  FeatureSetDefaults defaults = *valid_defaults;

  defaults.mutable_defaults()->SwapElements(0, 1);

  EXPECT_THAT(FeatureResolver::Create(EDITION_2023, defaults),
              HasError(AllOf(HasSubstr("not strictly increasing."),
                             HasSubstr("Edition PROTO3 is greater "
                                       "than or equal to edition LEGACY"))));
}

TEST(FeatureResolverTest, CreateUnknownEdition) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    minimum_edition: EDITION_UNKNOWN
    maximum_edition: EDITION_99999_TEST_ONLY
    defaults { edition: EDITION_UNKNOWN }
  )pb");
  EXPECT_THAT(FeatureResolver::Create(EDITION_2023, defaults),
              HasError(HasSubstr("Invalid edition UNKNOWN")));
}

TEST(FeatureResolverTest, CreateMissingEdition) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    minimum_edition: EDITION_UNKNOWN
    maximum_edition: EDITION_99999_TEST_ONLY
    defaults {}
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

    // Clear the feature, which should be invalid.
    FeatureSetDefaults defaults = *valid_defaults;
    FeatureSet* features =
        defaults.mutable_defaults()->Mutable(0)->mutable_overridable_features();
    features->GetReflection()->ClearField(features, &field);
    features =
        defaults.mutable_defaults()->Mutable(0)->mutable_fixed_features();
    features->GetReflection()->ClearField(features, &field);

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
          {GetExtension(proto2_unittest::file_opt1, FileOptions::descriptor())},
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

TEST(FeatureResolverTest, CompileDefaultsUnstableLaterThanMaximum) {
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(), {},
                                       EDITION_2023, EDITION_2024);
  ASSERT_OK(defaults);
  EXPECT_OK(FeatureResolver::Create(EDITION_UNSTABLE, *defaults));
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
    [pb.test] { field_feature: VALUE5 }
  )pb");
  absl::StatusOr<FeatureSet> merged =
      resolver->MergeFeatures(FeatureSet(), child);
  ASSERT_OK(merged);

  EXPECT_EQ(merged->field_presence(), FeatureSet::IMPLICIT);
  EXPECT_EQ(merged->enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(merged->repeated_field_encoding(), FeatureSet::EXPANDED);
  EXPECT_EQ(merged->message_encoding(), FeatureSet::LENGTH_PREFIXED);

  pb::TestFeatures ext = merged->GetExtension(pb::test);
  EXPECT_EQ(ext.file_feature(), pb::VALUE3);
  EXPECT_EQ(ext.field_feature(), pb::VALUE5);
}

TEST(FeatureResolverTest, MergeFeaturesParentOverrides) {
  absl::StatusOr<FeatureResolver> resolver =
      SetupFeatureResolver(EDITION_2023, pb::test);
  ASSERT_OK(resolver);
  FeatureSet parent = ParseTextOrDie(R"pb(
    field_presence: IMPLICIT
    repeated_field_encoding: EXPANDED
    [pb.test] { message_feature: VALUE2 field_feature: VALUE5 }
  )pb");
  FeatureSet child = ParseTextOrDie(R"pb(
    repeated_field_encoding: PACKED
    [pb.test] { field_feature: VALUE7 }
  )pb");
  absl::StatusOr<FeatureSet> merged = resolver->MergeFeatures(parent, child);
  ASSERT_OK(merged);

  EXPECT_EQ(merged->field_presence(), FeatureSet::IMPLICIT);
  EXPECT_EQ(merged->enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(merged->repeated_field_encoding(), FeatureSet::PACKED);
  EXPECT_EQ(merged->message_encoding(), FeatureSet::LENGTH_PREFIXED);

  pb::TestFeatures ext = merged->GetExtension(pb::test);
  EXPECT_EQ(ext.file_feature(), pb::VALUE3);
  EXPECT_EQ(ext.extension_range_feature(), pb::VALUE1);
  EXPECT_EQ(ext.message_feature(), pb::VALUE2);
  EXPECT_EQ(ext.field_feature(), pb::VALUE7);
  EXPECT_EQ(ext.oneof_feature(), pb::VALUE1);
  EXPECT_EQ(ext.enum_feature(), pb::VALUE1);
  EXPECT_EQ(ext.enum_entry_feature(), pb::VALUE1);
  EXPECT_EQ(ext.service_feature(), pb::VALUE1);
  EXPECT_EQ(ext.method_feature(), pb::VALUE1);
  EXPECT_FALSE(ext.bool_field_feature());
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
    [pb.test] { field_feature: TEST_ENUM_FEATURE_UNKNOWN }
  )pb");
  absl::StatusOr<FeatureSet> merged =
      resolver->MergeFeatures(FeatureSet(), child);
  ASSERT_OK(merged);
  EXPECT_EQ(merged->GetExtension(pb::test).field_feature(),
            pb::TEST_ENUM_FEATURE_UNKNOWN);
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

TEST(FeatureResolverTest, GetEditionFeatureSetDefaults) {
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(),
                                       {GetExtension(pb::test)}, EDITION_LEGACY,
                                       EDITION_99997_TEST_ONLY);

  absl::StatusOr<FeatureSet> edition_2023_feature =
      internal::GetEditionFeatureSetDefaults(EDITION_2023, *defaults);
  absl::StatusOr<FeatureSet> edition_proto3_feature =
      internal::GetEditionFeatureSetDefaults(EDITION_PROTO3, *defaults);
  absl::StatusOr<FeatureSet> edition_proto2_feature =
      internal::GetEditionFeatureSetDefaults(EDITION_LEGACY, *defaults);
  absl::StatusOr<FeatureSet> edition_unstable_feature =
      internal::GetEditionFeatureSetDefaults(EDITION_UNSTABLE, *defaults);
  absl::StatusOr<FeatureSet> edition_test_feature =
      internal::GetEditionFeatureSetDefaults(EDITION_99998_TEST_ONLY,
                                             *defaults);
  EXPECT_OK(edition_2023_feature);
  EXPECT_EQ(edition_2023_feature->GetExtension(pb::test).file_feature(),
            pb::VALUE3);
  EXPECT_OK(edition_proto3_feature);
  EXPECT_EQ(edition_proto3_feature->GetExtension(pb::test).file_feature(),
            pb::VALUE2);
  EXPECT_OK(edition_proto2_feature);
  EXPECT_EQ(edition_proto2_feature->GetExtension(pb::test).file_feature(),
            pb::VALUE1);
  EXPECT_OK(edition_unstable_feature);
  EXPECT_EQ(
      edition_unstable_feature->GetExtension(pb::test).new_unstable_feature(),
      pb::UNSTABLE2);
  EXPECT_OK(edition_test_feature);
  EXPECT_EQ(edition_test_feature->GetExtension(pb::test).file_feature(),
            pb::VALUE4);
}

TEST(FeatureResolverTest, GetEditionFeatureSetDefaultsNotFound) {
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(),
                                       {GetExtension(pb::test)}, EDITION_2023,
                                       EDITION_2023);

  absl::StatusOr<FeatureSet> edition_2023_feature =
      internal::GetEditionFeatureSetDefaults(EDITION_1_TEST_ONLY, *defaults);
  EXPECT_THAT(edition_2023_feature, HasError(HasSubstr("No valid default")));
}

TEST(FeatureResolverLifetimesTest, ValidFeature) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { file_feature: VALUE1 }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2023,
                                                           features, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, ValidUnstableFeature) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { new_unstable_feature: UNSTABLE2 }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_UNSTABLE,
                                                           features, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, ValidOption) {
  FileOptions options = ParseTextOrDie(R"pb(
    java_multiple_files: true
  )pb");
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2023, options, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, DeprecatedFeature) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { removed_feature: VALUE1 }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2023,
                                                           features, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(
      results.warnings,
      ElementsAre(AllOf(HasSubstr("pb.TestFeatures.removed_feature"),
                        HasSubstr("deprecated in edition 2023"),
                        HasSubstr("Custom feature deprecation warning"))));
}

TEST(FeatureResolverLifetimesTest, CustomRemovedOption) {
  MessageOptions options = ParseTextOrDie(R"pb(
    [proto2_unittest.removed_option]: true
  )pb");
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2024, options, nullptr);
  EXPECT_THAT(results.errors,
              ElementsAre(AllOf(HasSubstr("proto2_unittest.removed_option"),
                                HasSubstr("has been removed in edition 2023"),
                                HasSubstr("removed_option removal error"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, CustomDeprecatedOption) {
  MessageOptions options = ParseTextOrDie(R"pb(
    [proto2_unittest.deprecated_option]: true
  )pb");
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2024, options, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(
      results.warnings,
      ElementsAre(AllOf(HasSubstr("proto2_unittest.deprecated_option"),
                        HasSubstr("has been deprecated in edition 2023"),
                        HasSubstr("deprecated_option deprecation warning"))));
}

TEST(FeatureResolverLifetimesTest, CustomRemovedMessageOption) {
  MessageOptions options = ParseTextOrDie(R"pb(
    [proto2_unittest.custom_option_lifetimes_message] { removed_option: "test" }
  )pb");
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2024, options, nullptr);
  EXPECT_THAT(
      results.errors,
      ElementsAre(AllOf(
          HasSubstr(
              "proto2_unittest.CustomOptionLifetimesMessage.removed_option"),
          HasSubstr("has been removed in edition 2023"),
          HasSubstr("removed_option removal error"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, CustomDeprecatedMessageOption) {
  MessageOptions options = ParseTextOrDie(R"pb(
    [proto2_unittest.custom_option_lifetimes_message] {
      deprecated_option: "test"
    }
  )pb");
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2024, options, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(
      results.warnings,
      ElementsAre(AllOf(
          HasSubstr(
              "proto2_unittest.CustomOptionLifetimesMessage.deprecated_option"),
          HasSubstr("has been deprecated in edition 2023"),
          HasSubstr("deprecated_option deprecation warning"))));
}

TEST(FeatureResolverLifetimesTest, NestedCustomMessageOption) {
  MessageOptions options = ParseTextOrDie(R"pb(
    [proto2_unittest.custom_option_lifetimes_message] {
      nested_custom_option_lifetimes_message { deprecated_option: "test" }
    }
  )pb");
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2024, options, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(
      results.warnings,
      ElementsAre(
          AllOf(HasSubstr("proto2_unittest.NestedCustomOptionLifetimesMessage."
                          "deprecated_option"),
                HasSubstr("has been deprecated in edition 2023"),
                HasSubstr("deprecated_option deprecation warning"))));
}

TEST(FeatureResolverLifetimesTest, NestedExention) {
  MessageOptions options = ParseTextOrDie(R"pb(
    [proto2_unittest.custom_option_lifetimes_message] {
      [proto2_unittest.custom_nested_option]: "test"
    }
  )pb");
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2024, options, nullptr);
  EXPECT_THAT(
      results.errors,
      ElementsAre(AllOf(HasSubstr("proto2_unittest.custom_nested_option"),
                        HasSubstr("has been removed in edition 2023"),
                        HasSubstr("custom_nested_option removal error"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, CustomEnumOption) {
  MessageOptions options = ParseTextOrDie(R"pb(
    [proto2_unittest.custom_option_lifetimes_enum]:
        CUSTOM_OPTION_ENUM_REMOVED_EDITION2023
  )pb");
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2024, options, nullptr);
  EXPECT_THAT(
      results.errors,
      ElementsAre(AllOf(
          HasSubstr("proto2_unittest.CUSTOM_OPTION_ENUM_REMOVED_EDITION2023"),
          HasSubstr("has been removed in edition 2023"),
          HasSubstr("CUSTOM_OPTION_ENUM_REMOVED_EDITION2023 removal error"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, RepeatedCustomEnumOption) {
  MessageOptions options = ParseTextOrDie(R"pb(
    [proto2_unittest.custom_option_lifetimes_message] {
      repeated_custom_option_lifetimes_enum:
          CUSTOM_OPTION_ENUM_REMOVED_EDITION2023
      repeated_custom_option_lifetimes_enum: CUSTOM_OPTION_ENUM_REMOVED_PROTO3
    }
  )pb");
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2024, options, nullptr);
  EXPECT_THAT(
      results.errors,
      UnorderedElementsAre(
          AllOf(HasSubstr(
                    "proto2_unittest.CUSTOM_OPTION_ENUM_REMOVED_EDITION2023"),
                HasSubstr("has been removed in edition 2023"),
                HasSubstr(
                    "CUSTOM_OPTION_ENUM_REMOVED_EDITION2023 removal error")),
          AllOf(HasSubstr("proto2_unittest.CUSTOM_OPTION_ENUM_REMOVED_PROTO3"),
                HasSubstr("has been removed in edition PROTO3"),
                HasSubstr("CUSTOM_OPTION_ENUM_REMOVED_PROTO3 removal error"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, RepeatedCustomMessageOption) {
  MessageOptions options = ParseTextOrDie(R"pb(
    [proto2_unittest.repeated_custom_option_lifetimes_message] {
      removed_option: "test1"
    }
    [proto2_unittest.repeated_custom_option_lifetimes_message] {
      removed_option: "test2"
    }
  )pb");
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2024, options, nullptr);
  EXPECT_THAT(
      results.errors,
      UnorderedElementsAre(
          AllOf(HasSubstr("proto2_unittest.CustomOptionLifetimesMessage."
                          "removed_option"),
                HasSubstr("has been removed in edition 2023"),
                HasSubstr("removed_option removal error")),
          AllOf(HasSubstr("proto2_unittest.CustomOptionLifetimesMessage."
                          "removed_option"),
                HasSubstr("has been removed in edition 2023"),
                HasSubstr("removed_option removal error"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, RemovedFeature) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { removed_feature: VALUE1 }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2024,
                                                           features, nullptr);
  EXPECT_THAT(results.errors,
              ElementsAre(AllOf(HasSubstr("pb.TestFeatures.removed_feature"),
                                HasSubstr("removed in edition 2024:"),
                                HasSubstr("Custom feature removal error"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, RemovedUnstableFeature) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { removed_unstable_feature: UNSTABLE1 }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_UNSTABLE,
                                                           features, nullptr);
  EXPECT_THAT(
      results.errors,
      ElementsAre(AllOf(HasSubstr("pb.TestFeatures.removed_unstable_feature"),
                        HasSubstr("removed in edition UNSTABLE:"),
                        HasSubstr("Custom feature removal error"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, RemovedOption) {
  FileOptions options = ParseTextOrDie(R"pb(
    java_multiple_files: true
  )pb");
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2024, options, nullptr);
  EXPECT_THAT(
      results.errors,
      ElementsAre(AllOf(
          HasSubstr("google.protobuf.FileOptions.java_multiple_files"),
          HasSubstr("removed in edition 2024:"),
          HasSubstr(
              "you can set `features.(pb.java).nest_in_file_class = YES`"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, RemovedFeatureWithNoRemovalError) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { same_edition_removed_feature: VALUE1 }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2023,
                                                           features, nullptr);
  EXPECT_THAT(results.errors,
              ElementsAre(AllOf(
                  HasSubstr("pb.TestFeatures.same_edition_removed_feature"),
                  HasSubstr("removed in edition 2023"), Not(HasSubstr(":")))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, NotIntroducedFeature) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { future_feature: VALUE1 }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2023,
                                                           features, nullptr);
  EXPECT_THAT(
      results.errors,
      ElementsAre(AllOf(HasSubstr("pb.TestFeatures.future_feature"),
                        HasSubstr("wasn't introduced until edition 2024"),
                        HasSubstr("can't be used in edition 2023"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, NotIntroducedOption) {
  FileOptions options = ParseTextOrDie(R"pb(
    java_multiple_files: true
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_1_TEST_ONLY,
                                                           options, nullptr);
  EXPECT_THAT(
      results.errors,
      ElementsAre(AllOf(HasSubstr("google.protobuf.FileOptions.java_multiple_files"),
                        HasSubstr("wasn't introduced until edition PROTO2"),
                        HasSubstr("can't be used in edition 1_TEST_ONLY"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, WarningsAndErrors) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { future_feature: VALUE1 removed_feature: VALUE1 }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2023,
                                                           features, nullptr);
  EXPECT_THAT(results.errors,
              ElementsAre(HasSubstr("pb.TestFeatures.future_feature")));
  EXPECT_THAT(
      results.warnings,
      ElementsAre(AllOf(HasSubstr("pb.TestFeatures.removed_feature"),
                        HasSubstr("Custom feature deprecation warning"))));
}

TEST(FeatureResolverLifetimesTest, MultipleErrors) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { future_feature: VALUE1 legacy_feature: VALUE1 }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2023,
                                                           features, nullptr);
  EXPECT_THAT(results.errors, UnorderedElementsAre(
                                  HasSubstr("pb.TestFeatures.future_feature"),
                                  HasSubstr("pb.TestFeatures.legacy_feature")));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, FeatureDynamicPool) {
  DescriptorPool pool;
  {
    FileDescriptorProto file;
    FileDescriptorProto::GetDescriptor()->file()->CopyTo(&file);
    ASSERT_NE(pool.BuildFile(file), nullptr);
  }
  {
    FileDescriptorProto file;
    pb::TestFeatures::GetDescriptor()->file()->CopyTo(&file);
    ASSERT_NE(pool.BuildFile(file), nullptr);
  }
  const Descriptor* feature_set =
      pool.FindMessageTypeByName("google.protobuf.FeatureSet");
  ASSERT_NE(feature_set, nullptr);

  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { future_feature: VALUE1 removed_feature: VALUE1 }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(
      EDITION_2023, features, feature_set);
  EXPECT_THAT(results.errors,
              ElementsAre(HasSubstr("pb.TestFeatures.future_feature")));
  EXPECT_THAT(results.warnings,
              ElementsAre(HasSubstr("pb.TestFeatures.removed_feature")));
}

TEST(FeatureResolverLifetimesTest, OptionDynamicPool) {
  DescriptorPool pool;
  const FileDescriptor* file_descriptor;
  {
    FileDescriptorProto file;
    FileDescriptorProto::GetDescriptor()->file()->CopyTo(&file);
    ASSERT_NE(pool.BuildFile(file), nullptr);
  }
  {
    FileDescriptorProto file;
    ASSERT_TRUE(TextFormat::ParseFromString(
        R"pb(
          name: "foo.proto"
          edition: EDITION_2024
          package: "proto2_unittest"
          dependency: "google/protobuf/descriptor.proto"
          extension {
            name: "some_option"
            number: 7739973
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            extendee: ".google.protobuf.FileOptions"
            options {
              feature_support {
                edition_introduced: EDITION_PROTO3
                edition_deprecated: EDITION_PROTO3
                deprecation_warning: "warning"
              }
            }
          }
          options {
            uninterpreted_option {
              name { name_part: "some_option" is_extension: true }
              string_value: "test"
            }
          })pb",
        &file));
    file_descriptor = pool.BuildFile(file);
    ASSERT_NE(file_descriptor, nullptr);
  }

  FileOptions options = file_descriptor->options();

  const Descriptor* option_set =
      pool.FindMessageTypeByName("google.protobuf.FileOptions");
  ASSERT_NE(option_set, nullptr);

  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2023,
                                                           options, option_set);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(
      results.warnings,
      ElementsAre(AllOf(HasSubstr("proto2_unittest.some_option"),
                        HasSubstr("has been deprecated in edition PROTO3"),
                        HasSubstr("warning"))));
}

TEST(FeatureResolverLifetimesTest, EmptyValueSupportValid) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { value_lifetime_feature: VALUE_LIFETIME_EMPTY_SUPPORT }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2023,
                                                           features, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, ValueSupportValid) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { value_lifetime_feature: VALUE_LIFETIME_SUPPORT }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(
      EDITION_99997_TEST_ONLY, features, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, ValueSupportBeforeIntroduced) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { value_lifetime_feature: VALUE_LIFETIME_FUTURE }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2023,
                                                           features, nullptr);
  EXPECT_THAT(results.errors,
              ElementsAre(AllOf(
                  HasSubstr("pb.VALUE_LIFETIME_FUTURE"),
                  HasSubstr("wasn't introduced until edition 99997_TEST_ONLY"),
                  HasSubstr("can't be used in edition 2023"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, ValueSupportBeforeIntroducedUnstable) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { unstable_existing_feature: UNSTABLE3 }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2023,
                                                           features, nullptr);
  EXPECT_THAT(
      results.errors,
      ElementsAre(AllOf(HasSubstr("pb.UNSTABLE3"),
                        HasSubstr("wasn't introduced until edition UNSTABLE"),
                        HasSubstr("can't be used in edition 2023"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, FeatureSupportBeforeIntroducedUnstable) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { new_unstable_feature: UNSTABLE2 }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2023,
                                                           features, nullptr);
  EXPECT_THAT(
      results.errors,
      ElementsAre(AllOf(HasSubstr("pb.TestFeatures.new_unstable_feature "),
                        HasSubstr("wasn't introduced until edition UNSTABLE"),
                        HasSubstr("can't be used in edition 2023"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, ValueSupportAfterRemoved) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { value_lifetime_feature: VALUE_LIFETIME_REMOVED }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(
      EDITION_99997_TEST_ONLY, features, nullptr);
  EXPECT_THAT(results.errors,
              ElementsAre(AllOf(HasSubstr("pb.VALUE_LIFETIME_REMOVED"),
                                HasSubstr("removed in edition 99997_TEST_ONLY"),
                                HasSubstr("Custom feature removal error"))));
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, ValueSupportDeprecated) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { value_lifetime_feature: VALUE_LIFETIME_DEPRECATED }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(
      EDITION_99997_TEST_ONLY, features, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(
      results.warnings,
      ElementsAre(AllOf(HasSubstr("pb.VALUE_LIFETIME_DEPRECATED"),
                        HasSubstr("deprecated in edition 99997_TEST_ONLY"),
                        HasSubstr("Custom feature deprecation warning"))));
}

TEST(FeatureResolverLifetimesTest, ValueAndFeatureSupportDeprecated) {
  FeatureSet features = ParseTextOrDie(R"pb(
    [pb.test] { value_lifetime_feature: VALUE_LIFETIME_DEPRECATED }
  )pb");
  auto results = FeatureResolver::ValidateFeatureLifetimes(
      EDITION_99998_TEST_ONLY, features, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(results.warnings,
              UnorderedElementsAre(
                  AllOf(HasSubstr("pb.VALUE_LIFETIME_DEPRECATED"),
                        HasSubstr("deprecated in edition 99997_TEST_ONLY"),
                        HasSubstr("Custom feature deprecation warning")),
                  AllOf(HasSubstr("pb.TestFeatures.value_lifetime_feature"),
                        HasSubstr("deprecated in edition 99998_TEST_ONLY"),
                        HasSubstr("Custom feature deprecation warning"))));
}

TEST(FeatureResolverLifetimesTest, ValueSupportAllowInvalidEnumFeatureNumber) {
  FeatureSet features;
  features.MutableExtension(pb::test)->set_value_lifetime_feature(
      static_cast<pb::ValueLifetimeFeature>(1234));
  auto results = FeatureResolver::ValidateFeatureLifetimes(EDITION_2023,
                                                           features, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest, ValueSupportAllowInvalidEnumOptionNumber) {
  MessageOptions options;
  options.MutableExtension(pb::custom_option_lifetimes_message)
      ->set_custom_enum(static_cast<pb::CustomOptionLifetimesEnum>(1234));
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2023, options, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(results.warnings, IsEmpty());
}

TEST(FeatureResolverLifetimesTest,
     ValueSupportAllowInvalidRepeatedEnumOptionNumber) {
  MessageOptions options;
  options.MutableExtension(pb::custom_option_lifetimes_message)
      ->add_repeated_custom_enum(
          static_cast<pb::CustomOptionLifetimesEnum>(1234));
  options.MutableExtension(pb::custom_option_lifetimes_message)
      ->add_repeated_custom_enum(
          static_cast<pb::CustomOptionLifetimesEnum>(5678));
  auto results =
      FeatureResolver::ValidateFeatureLifetimes(EDITION_2023, options, nullptr);
  EXPECT_THAT(results.errors, IsEmpty());
  EXPECT_THAT(results.warnings, IsEmpty());
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
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_LEGACY, value: "" }
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
          feature_support.edition_introduced = EDITION_2023,
          edition_defaults = { edition: EDITION_LEGACY, value: "1" }
        ];
        string string_field = 2 [
          targets = TARGET_TYPE_FIELD,
          feature_support.edition_introduced = EDITION_2023,
          edition_defaults = { edition: EDITION_LEGACY, value: "'hello'" }
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
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_LEGACY, value: "" }
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
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_LEGACY, value: "1" }
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
      optional bool bool_field = 1 [
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_LEGACY, value: "true" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(FeatureResolver::CompileDefaults(feature_set_, {ext},
                                               EDITION_2023, EDITION_2023),
              HasError(AllOf(HasSubstr("test.Foo.bool_field"),
                             HasSubstr("no target specified"))));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsInvalidWithMissingSupport) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      optional bool bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_LEGACY, value: "true" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(FeatureResolver::CompileDefaults(feature_set_, {ext},
                                               EDITION_2023, EDITION_2023),
              HasError(AllOf(HasSubstr("test.Foo.bool_field"),
                             HasSubstr("no feature support"))));
}

TEST_F(FeatureResolverPoolTest,
       CompileDefaultsInvalidWithMissingEditionIntroduced) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      optional bool bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {},
        edition_defaults = { edition: EDITION_LEGACY, value: "true" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(FeatureResolver::CompileDefaults(feature_set_, {ext},
                                               EDITION_2023, EDITION_2023),
              HasError(AllOf(HasSubstr("test.Foo.bool_field"),
                             HasSubstr("it was introduced in"))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidWithMissingDeprecationWarning) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    message Foo {
      optional bool bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {
          edition_introduced: EDITION_2023
          edition_deprecated: EDITION_2023
        },
        edition_defaults = { edition: EDITION_LEGACY, value: "true" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(FeatureResolver::ValidateFieldFeatureSupport(*field),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       AllOf(HasSubstr("test.Foo.bool_field"),
                             HasSubstr("deprecation warning"))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidWithMissingDeprecation) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    message Foo {
      optional bool bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {
          edition_introduced: EDITION_2023
          deprecation_warning: "some message"
        },
        edition_defaults = { edition: EDITION_LEGACY, value: "true" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(FeatureResolver::ValidateFieldFeatureSupport(*field),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       (AllOf(HasSubstr("test.Foo.bool_field"),
                              HasSubstr("is not marked deprecated")))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidDeprecatedBeforeIntroduced) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      optional bool bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {
          edition_introduced: EDITION_2024
          edition_deprecated: EDITION_2023
          deprecation_warning: "warning"
        },
        edition_defaults = { edition: EDITION_LEGACY, value: "true" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(
      FeatureResolver::ValidateFieldFeatureSupport(*field),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               (AllOf(HasSubstr("test.Foo.bool_field"),
                      HasSubstr("deprecated before it was introduced")))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidDeprecatedAfterRemoved) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    message Foo {
      optional bool bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {
          edition_introduced: EDITION_2023
          edition_deprecated: EDITION_2024
          deprecation_warning: "warning"
          edition_removed: EDITION_2024
        },
        edition_defaults = { edition: EDITION_LEGACY, value: "true" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(FeatureResolver::ValidateFieldFeatureSupport(*field),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       (AllOf(HasSubstr("test.Foo.bool_field"),
                              HasSubstr("deprecated after it was removed")))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidRemovedBeforeIntroduced) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    message Foo {
      optional bool bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {
          edition_introduced: EDITION_2024
          edition_removed: EDITION_2023
        },
        edition_defaults = { edition: EDITION_LEGACY, value: "true" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(FeatureResolver::ValidateFieldFeatureSupport(*field),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       (AllOf(HasSubstr("test.Foo.bool_field"),
                              HasSubstr("removed before it was introduced")))));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsInvalidMissingLegacyDefaults) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      optional bool bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {
          edition_introduced: EDITION_2024
        },
        edition_defaults = { edition: EDITION_2024, value: "true" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, EDITION_2023,
                                       EDITION_2023),
      HasError(AllOf(HasSubstr("test.Foo.bool_field"),
                     HasSubstr("no default specified for EDITION_LEGACY"))));
}

TEST_F(FeatureResolverPoolTest,
       CompileDefaultsInvalidDefaultsBeforeIntroduced) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      optional bool bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {
          edition_introduced: EDITION_2024
        },
        edition_defaults = { edition: EDITION_LEGACY, value: "true" },
        edition_defaults = { edition: EDITION_2023, value: "false" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(FeatureResolver::CompileDefaults(feature_set_, {ext},
                                               EDITION_2023, EDITION_2023),
              HasError(AllOf(HasSubstr("test.Foo.bool_field"),
                             HasSubstr("specified for edition 2023"),
                             HasSubstr("before it was introduced"))));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsInvalidDefaultsAfterRemoved) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message Foo {
      optional bool bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {
          edition_introduced: EDITION_PROTO2
          edition_removed: EDITION_2023
          removal_error: "Custom removal error"
        },
        edition_defaults = { edition: EDITION_LEGACY, value: "true" },
        edition_defaults = { edition: EDITION_2024, value: "true" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(FeatureResolver::CompileDefaults(feature_set_, {ext},
                                               EDITION_2023, EDITION_2023),
              HasError(AllOf(HasSubstr("test.Foo.bool_field"),
                             HasSubstr("specified for edition 2024"),
                             HasSubstr("after it was removed"))));
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
      optional bool field_feature = 12 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_LEGACY, value: "1.23" }
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
      optional bool field_feature = 12 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_99997_TEST_ONLY, value: "1.5" },
        edition_defaults = { edition: EDITION_LEGACY, value: "true" }
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
      optional bool field_feature = 12 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_2_TEST_ONLY, value: "true" },
        edition_defaults = { edition: EDITION_LEGACY, value: "false" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, EDITION_2023,
                                       EDITION_2023),
      HasError(HasSubstr("Minimum edition 2_TEST_ONLY is not EDITION_LEGACY")));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportIgnoreValueWithMissingParentFeatureSupport) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    enum FooValues {
      UNKNOWN = 0;
      VALUE = 1 [feature_support.edition_introduced = EDITION_2023];
    }
    message Foo {
      optional FooValues bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_LEGACY, value: "UNKNOWN" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(FeatureResolver::ValidateFieldFeatureSupport(*field),
              StatusIs(absl::StatusCode::kOk));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidValueWithMissingDeprecationWarning) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    enum FooValues {
      UNKNOWN = 0;
      VALUE = 1 [feature_support.edition_deprecated = EDITION_2023];
    }
    message Foo {
      optional FooValues bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_LEGACY, value: "UNKNOWN" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(FeatureResolver::ValidateFieldFeatureSupport(*field),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       (AllOf(HasSubstr("test.VALUE"),
                              HasSubstr("deprecation warning")))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidValueWithMissingDeprecation) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    enum FooValues {
      UNKNOWN = 0;
      VALUE = 1 [feature_support.deprecation_warning = "some message"];
    }
    message Foo {
      optional FooValues bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_LEGACY, value: "UNKNOWN" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(FeatureResolver::ValidateFieldFeatureSupport(*field),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       (AllOf(HasSubstr("test.VALUE"),
                              HasSubstr("is not marked deprecated")))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidValueDeprecatedBeforeIntroduced) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    enum FooValues {
      UNKNOWN = 0;
      VALUE = 1 [feature_support = {
        edition_introduced: EDITION_2024
        edition_deprecated: EDITION_2023
        deprecation_warning: "warning"
      }];
    }
    message Foo {
      optional FooValues bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_LEGACY, value: "UNKNOWN" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(
      FeatureResolver::ValidateFieldFeatureSupport(*field),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               (AllOf(HasSubstr("test.VALUE"),
                      HasSubstr("deprecated before it was introduced")))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidValueIntroducedInherited) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    enum FooValues {
      UNKNOWN = 0;
      VALUE = 1 [feature_support = {
        edition_deprecated: EDITION_2023
        deprecation_warning: "warning"
      }];
    }
    message Foo {
      optional FooValues bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2024,
        edition_defaults = { edition: EDITION_LEGACY, value: "UNKNOWN" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(
      FeatureResolver::ValidateFieldFeatureSupport(*field),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               (AllOf(HasSubstr("test.VALUE"),
                      HasSubstr("deprecated before it was introduced")))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidValueDeprecatedAfterRemoved) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    enum FooValues {
      UNKNOWN = 0;
      VALUE = 1 [feature_support = {
        edition_introduced: EDITION_2023
        edition_deprecated: EDITION_2024
        deprecation_warning: "warning"
        edition_removed: EDITION_2024
        removal_error: "Custom removal error"
      }];
    }
    message Foo {
      optional FooValues bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_LEGACY, value: "UNKNOWN" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(FeatureResolver::ValidateFieldFeatureSupport(*field),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       (AllOf(HasSubstr("test.VALUE"),
                              HasSubstr("deprecated after it was removed")))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidValueRemovedBeforeIntroduced) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    enum FooValues {
      UNKNOWN = 0;
      VALUE = 1 [feature_support = {
        edition_introduced: EDITION_2024
        edition_removed: EDITION_2023
        removal_error: "Custom removal error"
      }];
    }
    message Foo {
      optional FooValues bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_LEGACY, value: "UNKNOWN" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(FeatureResolver::ValidateFieldFeatureSupport(*field),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       (AllOf(HasSubstr("test.VALUE"),
                              HasSubstr("removed before it was introduced")))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidValueIntroducedBeforeFeature) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    enum FooValues {
      UNKNOWN = 0;
      VALUE = 1 [feature_support = {
        edition_introduced: EDITION_2023
      }];
    }
    message Foo {
      optional FooValues bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2024,
        edition_defaults = { edition: EDITION_LEGACY, value: "UNKNOWN" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(
      FeatureResolver::ValidateFieldFeatureSupport(*field),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               (AllOf(HasSubstr("test.VALUE"), HasSubstr("introduced before"),
                      HasSubstr("test.Foo.bool_field")))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidValueIntroducedAfterFeatureRemoved) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    enum FooValues {
      UNKNOWN = 0;
      VALUE = 1 [feature_support = {
        edition_introduced: EDITION_99997_TEST_ONLY
      }];
    }
    message Foo {
      optional FooValues bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {
          edition_introduced: EDITION_2023
          edition_removed: EDITION_2024
          removal_error: "Custom removal error"
        },
        edition_defaults = { edition: EDITION_LEGACY, value: "UNKNOWN" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(FeatureResolver::ValidateFieldFeatureSupport(*field),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       (AllOf(HasSubstr("test.VALUE"),
                              HasSubstr("removed before it was introduced")))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidValueRemovedAfterFeature) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    enum FooValues {
      UNKNOWN = 0;
      VALUE = 1 [feature_support = {
        edition_removed: EDITION_99997_TEST_ONLY
        removal_error: "Custom removal error"

      }];
    }
    message Foo {
      optional FooValues bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {
          edition_introduced: EDITION_2023
          edition_removed: EDITION_2024
          removal_error: "Custom removal error"
        },
        edition_defaults = { edition: EDITION_LEGACY, value: "UNKNOWN" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(
      FeatureResolver::ValidateFieldFeatureSupport(*field),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               (AllOf(HasSubstr("test.VALUE"), HasSubstr("removed after"),
                      HasSubstr("test.Foo.bool_field")))));
}

TEST_F(FeatureResolverPoolTest,
       ValidateFieldFeatureSupportInvalidValueDeprecatedAfterFeature) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    enum FooValues {
      UNKNOWN = 0;
      VALUE = 1 [feature_support = {
        edition_deprecated: EDITION_99997_TEST_ONLY
        deprecation_warning: "warning"
      }];
    }
    message Foo {
      optional FooValues bool_field = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {
          edition_introduced: EDITION_2023
          edition_deprecated: EDITION_2024
          deprecation_warning: "warning"
        },
        edition_defaults = { edition: EDITION_LEGACY, value: "UNKNOWN" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* field = file->message_type(0)->field(0);

  EXPECT_THAT(
      FeatureResolver::ValidateFieldFeatureSupport(*field),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               (AllOf(HasSubstr("test.VALUE"), HasSubstr("deprecated after"),
                      HasSubstr("test.Foo.bool_field")))));
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
      optional bool field_feature = 12 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_LEGACY, value: "true" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, EDITION_1_TEST_ONLY,
                                       EDITION_99997_TEST_ONLY),
      HasError(HasSubstr("edition 1_TEST_ONLY is earlier than the oldest")));
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsRemovedOnly) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    enum Bar {
      TEST_ENUM_FEATURE_UNKNOWN = 0;
      VALUE1 = 1;
      VALUE2 = 2;
    }
    message Foo {
      optional Bar file_feature = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support = {
          edition_introduced: EDITION_2023
          edition_removed: EDITION_99998_TEST_ONLY
          removal_error: "Custom feature removal error"
        },
        edition_defaults = { edition: EDITION_LEGACY, value: "VALUE1" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  auto compiled_defaults = FeatureResolver::CompileDefaults(
      feature_set_, {ext}, EDITION_99997_TEST_ONLY, EDITION_99999_TEST_ONLY);
  ASSERT_OK(compiled_defaults);
  const auto& defaults = *compiled_defaults->defaults().rbegin();
  EXPECT_THAT(defaults.edition(), EDITION_99998_TEST_ONLY);
  EXPECT_THAT(defaults.fixed_features().GetExtension(pb::test).file_feature(),
              pb::VALUE1);
  EXPECT_FALSE(defaults.overridable_features()
                   .GetExtension(pb::test)
                   .has_file_feature());
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsIntroducedOnly) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    enum Bar {
      TEST_ENUM_FEATURE_UNKNOWN = 0;
      VALUE1 = 1;
      VALUE2 = 2;
    }
    message Foo {
      optional Bar file_feature = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_99998_TEST_ONLY,
        edition_defaults = { edition: EDITION_LEGACY, value: "VALUE1" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  auto compiled_defaults = FeatureResolver::CompileDefaults(
      feature_set_, {ext}, EDITION_99997_TEST_ONLY, EDITION_99999_TEST_ONLY);
  ASSERT_OK(compiled_defaults);
  const auto& defaults = *compiled_defaults->defaults().rbegin();
  EXPECT_THAT(defaults.edition(), EDITION_99998_TEST_ONLY);
  EXPECT_THAT(
      defaults.overridable_features().GetExtension(pb::test).file_feature(),
      pb::VALUE1);
  EXPECT_FALSE(
      defaults.fixed_features().GetExtension(pb::test).has_file_feature());
}

TEST_F(FeatureResolverPoolTest, CompileDefaultsMinimumCovered) {
  const FileDescriptor* file = ParseSchema(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    enum Bar {
      TEST_ENUM_FEATURE_UNKNOWN = 0;
      VALUE1 = 1;
      VALUE2 = 2;
      VALUE3 = 3;
    }
    message Foo {
      optional Bar file_feature = 1 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_99998_TEST_ONLY, value: "VALUE3" },
        edition_defaults = { edition: EDITION_2023, value: "VALUE2" },
        edition_defaults = { edition: EDITION_LEGACY, value: "VALUE1" }
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
      edition: EDITION_LEGACY
      overridable_features {
        [pb.test] {}
      }
      fixed_features {
        field_presence: EXPLICIT
        enum_type: CLOSED
        repeated_field_encoding: EXPANDED
        utf8_validation: NONE
        message_encoding: LENGTH_PREFIXED
        json_format: LEGACY_BEST_EFFORT
        enforce_naming_style: STYLE_LEGACY
        default_symbol_visibility: EXPORT_ALL
        [pb.test] { file_feature: VALUE1 }
      }
    }
    defaults {
      edition: EDITION_PROTO3
      overridable_features {
        [pb.test] {}
      }
      fixed_features {
        field_presence: IMPLICIT
        enum_type: OPEN
        repeated_field_encoding: PACKED
        utf8_validation: VERIFY
        message_encoding: LENGTH_PREFIXED
        json_format: ALLOW
        enforce_naming_style: STYLE_LEGACY
        default_symbol_visibility: EXPORT_ALL
        [pb.test] { file_feature: VALUE1 }
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
        [pb.test] { file_feature: VALUE2 }
      }
      fixed_features {
        enforce_naming_style: STYLE_LEGACY
        default_symbol_visibility: EXPORT_ALL
        [pb.test] {}
      }
    }
    defaults {
      edition: EDITION_2024
      overridable_features {
        field_presence: EXPLICIT
        enum_type: OPEN
        repeated_field_encoding: PACKED
        utf8_validation: VERIFY
        message_encoding: LENGTH_PREFIXED
        json_format: ALLOW
        enforce_naming_style: STYLE2024
        default_symbol_visibility: EXPORT_TOP_LEVEL
        [pb.test] { file_feature: VALUE2 }
      }
      fixed_features {
        [pb.test] {}
      }
    }
    defaults {
      edition: EDITION_99998_TEST_ONLY
      overridable_features {
        field_presence: EXPLICIT
        enum_type: OPEN
        repeated_field_encoding: PACKED
        utf8_validation: VERIFY
        message_encoding: LENGTH_PREFIXED
        json_format: ALLOW
        enforce_naming_style: STYLE2024
        default_symbol_visibility: EXPORT_TOP_LEVEL
        [pb.test] { file_feature: VALUE3 }
      }
      fixed_features {
        [pb.test] {}
      }
    }
  )pb"));
}

class FeatureUnboundedTypeTest
    : public FeatureResolverPoolTest,
      public ::testing::WithParamInterface<absl::string_view> {};

TEST_P(FeatureUnboundedTypeTest, CompileDefaults) {
  const FileDescriptor* file = ParseSchema(absl::Substitute(R"schema(
    syntax = "proto2";
    package test;
    import "google/protobuf/descriptor.proto";

    extend google.protobuf.FeatureSet {
      optional Foo bar = 9999;
    }
    message SomeMessage {
      optional bool value = 1;
    }
    message Foo {
      optional $0 field_feature = 12 [
        targets = TARGET_TYPE_FIELD,
        feature_support.edition_introduced = EDITION_2023,
        edition_defaults = { edition: EDITION_LEGACY, value: "1" }
      ];
    }
  )schema",
                                                            GetParam()));
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, EDITION_1_TEST_ONLY,
                                       EDITION_99997_TEST_ONLY),
      HasError(HasSubstr("is not an enum or boolean")));
}

INSTANTIATE_TEST_SUITE_P(FeatureUnboundedTypeTestImpl, FeatureUnboundedTypeTest,
                         testing::Values("int32", "int64", "uint32", "string",
                                         "bytes", "float", "double",
                                         "SomeMessage"));

}  // namespace
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
