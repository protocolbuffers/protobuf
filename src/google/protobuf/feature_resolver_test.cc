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

// TODO(b/234474291): Use the gtest versions once that's available in OSS.
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
absl::StatusOr<FeatureResolver> SetupFeatureResolver(absl::string_view edition,
                                                     Extensions... extensions) {
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(),
                                       {GetExtension(extensions)...}, "2023",
                                       "2023");
  RETURN_IF_ERROR(defaults.status());
  return FeatureResolver::Create(edition, *defaults);
}

absl::StatusOr<FeatureSet> GetDefaults(absl::string_view edition,
                                       const FeatureSetDefaults& defaults) {
  absl::StatusOr<FeatureResolver> resolver =
      FeatureResolver::Create(edition, defaults);
  RETURN_IF_ERROR(resolver.status());
  FeatureSet parent, child;
  return resolver->MergeFeatures(parent, child);
}

template <typename... Extensions>
absl::StatusOr<FeatureSet> GetDefaults(absl::string_view edition,
                                       Extensions... extensions) {
  absl::StatusOr<FeatureSetDefaults> defaults =
      FeatureResolver::CompileDefaults(FeatureSet::descriptor(),
                                       {GetExtension(extensions)...}, "0",
                                       "99999");
  RETURN_IF_ERROR(defaults.status());
  return GetDefaults(edition, *defaults);
}

FileDescriptorProto GetProto(const FileDescriptor* file) {
  FileDescriptorProto proto;
  file->CopyTo(&proto);
  return proto;
}

TEST(FeatureResolverTest, DefaultsCore2023) {
  absl::StatusOr<FeatureSet> merged = GetDefaults("2023");
  ASSERT_OK(merged);

  EXPECT_EQ(merged->field_presence(), FeatureSet::EXPLICIT);
  EXPECT_EQ(merged->enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(merged->repeated_field_encoding(), FeatureSet::PACKED);
  EXPECT_EQ(merged->message_encoding(), FeatureSet::LENGTH_PREFIXED);
  EXPECT_FALSE(merged->HasExtension(pb::test));
}

TEST(FeatureResolverTest, DefaultsTest2023) {
  absl::StatusOr<FeatureSet> merged = GetDefaults("2023", pb::test);
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
      GetDefaults("2023", pb::TestMessage::test_message);
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
      GetDefaults("2023", pb::TestMessage::Nested::test_nested);
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
          {pool.FindExtensionByName("pb.test")}, "2023", "2023");
  ASSERT_OK(defaults);
  ASSERT_EQ(defaults->defaults().size(), 1);
  ASSERT_EQ(defaults->defaults().at(0).edition(), "2023");
  FeatureSet merged = defaults->defaults().at(0).features();

  EXPECT_EQ(merged.field_presence(), FeatureSet::EXPLICIT);
  EXPECT_TRUE(merged.HasExtension(pb::test));
  EXPECT_EQ(merged.GetExtension(pb::test).int_file_feature(), 1);
  EXPECT_FALSE(merged.HasExtension(pb::cpp));
}

TEST(FeatureResolverTest, DefaultsTooEarly) {
  absl::StatusOr<FeatureSet> merged = GetDefaults("2022", pb::test);
  EXPECT_THAT(merged, HasError(AllOf(HasSubstr("No valid default found"),
                                     HasSubstr("2022"))));
}

TEST(FeatureResolverTest, DefaultsFarFuture) {
  absl::StatusOr<FeatureSet> merged = GetDefaults("3456", pb::test);
  ASSERT_OK(merged);

  pb::TestFeatures ext = merged->GetExtension(pb::test);
  EXPECT_EQ(ext.int_file_feature(), 5);
  EXPECT_THAT(ext.message_field_feature(),
              EqualsProto("bool_field: true int_field: 4  float_field: 1.5 "
                          "string_field: '2025'"));
  EXPECT_EQ(ext.enum_field_feature(), pb::TestFeatures::ENUM_VALUE5);
}

TEST(FeatureResolverTest, DefaultsMiddleEdition) {
  absl::StatusOr<FeatureSet> merged = GetDefaults("2024.1", pb::test);
  ASSERT_OK(merged);

  pb::TestFeatures ext = merged->GetExtension(pb::test);
  EXPECT_EQ(ext.int_file_feature(), 4);
  EXPECT_EQ(ext.enum_field_feature(), pb::TestFeatures::ENUM_VALUE4);
}

TEST(FeatureResolverTest, DefaultsDifferentDigitCount) {
  absl::StatusOr<FeatureSet> merged = GetDefaults("2023.90", pb::test);
  ASSERT_OK(merged);

  pb::TestFeatures ext = merged->GetExtension(pb::test);
  EXPECT_EQ(ext.int_file_feature(), 3);
  EXPECT_EQ(ext.enum_field_feature(), pb::TestFeatures::ENUM_VALUE3);
}

TEST(FeatureResolverTest, DefaultsMessageMerge) {
  {
    absl::StatusOr<FeatureSet> merged = GetDefaults("2023", pb::test);
    ASSERT_OK(merged);
    pb::TestFeatures ext = merged->GetExtension(pb::test);
    EXPECT_THAT(ext.message_field_feature(),
                EqualsProto(R"pb(bool_field: true
                                 int_field: 1
                                 float_field: 1.5
                                 string_field: '2023')pb"));
  }
  {
    absl::StatusOr<FeatureSet> merged = GetDefaults("2023.1", pb::test);
    ASSERT_OK(merged);
    pb::TestFeatures ext = merged->GetExtension(pb::test);
    EXPECT_THAT(ext.message_field_feature(),
                EqualsProto(R"pb(bool_field: true
                                 int_field: 2
                                 float_field: 1.5
                                 string_field: '2023')pb"));
  }
  {
    absl::StatusOr<FeatureSet> merged = GetDefaults("2024", pb::test);
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
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    minimum_edition: "2022"
    maximum_edition: "2024"
    defaults {
      edition: "2022"
      features {}
    }
    defaults {
      edition: "2024"
      features {}
    }
    defaults {
      edition: "2023"
      features {}
    }
  )pb");
  EXPECT_THAT(
      FeatureResolver::Create("2023", defaults),
      HasError(AllOf(
          HasSubstr("not strictly increasing."),
          HasSubstr("Edition 2024 is greater than or equal to edition 2023"))));
}

TEST(FeatureResolverTest, CompileDefaultsMissingDescriptor) {
  EXPECT_THAT(FeatureResolver::CompileDefaults(nullptr, {}, "2023", "2023"),
              HasError(HasSubstr("find definition of google.protobuf.FeatureSet")));
}

TEST(FeatureResolverTest, CompileDefaultsMissingExtension) {
  EXPECT_THAT(FeatureResolver::CompileDefaults(FeatureSet::descriptor(),
                                               {nullptr}, "2023", "2023"),
              HasError(HasSubstr("Unknown extension")));
}

TEST(FeatureResolverTest, CompileDefaultsInvalidExtension) {
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(
          FeatureSet::descriptor(),
          {GetExtension(protobuf_unittest::file_opt1, FileOptions::descriptor())},
          "2023", "2023"),
      HasError(HasSubstr("is not an extension of")));
}

TEST(FeatureResolverTest, MergeFeaturesChildOverrideCore) {
  absl::StatusOr<FeatureResolver> resolver = SetupFeatureResolver("2023");
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
      SetupFeatureResolver("2023", pb::test);
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
      SetupFeatureResolver("2023", pb::test);
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

TEST(FeatureResolverTest, MergeFeaturesFieldPresenceUnknown) {
  absl::StatusOr<FeatureResolver> resolver = SetupFeatureResolver("2023");
  ASSERT_OK(resolver);
  FeatureSet child = ParseTextOrDie(R"pb(
    field_presence: FIELD_PRESENCE_UNKNOWN
  )pb");
  EXPECT_THAT(
      resolver->MergeFeatures(FeatureSet(), child),
      HasError(AllOf(HasSubstr("field google.protobuf.FeatureSet.field_presence"),
                     HasSubstr("FIELD_PRESENCE_UNKNOWN"))));
}

TEST(FeatureResolverTest, MergeFeaturesEnumTypeUnknown) {
  absl::StatusOr<FeatureResolver> resolver = SetupFeatureResolver("2023");
  ASSERT_OK(resolver);
  FeatureSet child = ParseTextOrDie(R"pb(
    enum_type: ENUM_TYPE_UNKNOWN
  )pb");
  EXPECT_THAT(resolver->MergeFeatures(FeatureSet(), child),
              HasError(AllOf(HasSubstr("field google.protobuf.FeatureSet.enum_type"),
                             HasSubstr("ENUM_TYPE_UNKNOWN"))));
}

TEST(FeatureResolverTest, MergeFeaturesRepeatedFieldEncodingUnknown) {
  absl::StatusOr<FeatureResolver> resolver = SetupFeatureResolver("2023");
  ASSERT_OK(resolver);
  FeatureSet child = ParseTextOrDie(R"pb(
    repeated_field_encoding: REPEATED_FIELD_ENCODING_UNKNOWN
  )pb");
  EXPECT_THAT(resolver->MergeFeatures(FeatureSet(), child),
              HasError(AllOf(
                  HasSubstr("field google.protobuf.FeatureSet.repeated_field_encoding"),
                  HasSubstr("REPEATED_FIELD_ENCODING_UNKNOWN"))));
}

TEST(FeatureResolverTest, MergeFeaturesMessageEncodingUnknown) {
  absl::StatusOr<FeatureResolver> resolver = SetupFeatureResolver("2023");
  ASSERT_OK(resolver);
  FeatureSet child = ParseTextOrDie(R"pb(
    message_encoding: MESSAGE_ENCODING_UNKNOWN
  )pb");
  EXPECT_THAT(
      resolver->MergeFeatures(FeatureSet(), child),
      HasError(AllOf(HasSubstr("field google.protobuf.FeatureSet.message_encoding"),
                     HasSubstr("MESSAGE_ENCODING_UNKNOWN"))));
}

TEST(FeatureResolverTest, MergeFeaturesExtensionEnumUnknown) {
  absl::StatusOr<FeatureResolver> resolver =
      SetupFeatureResolver("2023", pb::test);
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
  EXPECT_THAT(SetupFeatureResolver("1000"),
              HasError(AllOf(HasSubstr("Edition 1000"),
                             HasSubstr("minimum supported edition 2023"))));
}

TEST(FeatureResolverTest, MergeFeaturesDistantFuture) {
  EXPECT_THAT(SetupFeatureResolver("9999"),
              HasError(AllOf(HasSubstr("Edition 9999"),
                             HasSubstr("maximum supported edition 2023"))));
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
    auto defaults =
        FeatureResolver::CompileDefaults(feature_set_, {}, "2023", "2023");
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
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2023"),
      HasError(
          AllOf(HasSubstr("test.bar"), HasSubstr("is not of message type"))));
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
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2023"),
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
        edition_defaults = { edition: "2023", value: "" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2023"),
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
          edition_defaults = { edition: "2023", value: "1" }
        ];
        string string_field = 2 [
          targets = TARGET_TYPE_FIELD,
          edition_defaults = { edition: "2023", value: "'hello'" }
        ];
      }
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2023"),
      HasError(
          AllOf(HasSubstr("test.Foo"), HasSubstr("oneof feature fields"))));
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
        edition_defaults = { edition: "2023", value: "" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2023"),
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
        edition_defaults = { edition: "2023", value: "1" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2023"),
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
        edition_defaults = { edition: "2023", value: "1" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2023"),
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
        edition_defaults = { edition: "2023", value: "9987" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2023"),
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
        edition_defaults = { edition: "2025", value: "int_field: 2" },
        edition_defaults = { edition: "2024", value: "int_field: 1" },
        edition_defaults = { edition: "2023", value: "9987" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2025"),
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
        edition_defaults = { edition: "2024", value: "int_field: 2" },
        edition_defaults = { edition: "2023", value: "int_field: 1" },
        edition_defaults = { edition: "2025", value: "9987" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  auto defaults =
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2024");
  ASSERT_OK(defaults);

  auto resolver = FeatureResolver::Create("2023", *defaults);
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
        edition_defaults = { edition: "2023", value: "1.23" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2023"),
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
        edition_defaults = { edition: "2024", value: "1.5" },
        edition_defaults = { edition: "2023", value: "1" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  auto defaults =
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2023");
  ASSERT_OK(defaults);

  auto resolver = FeatureResolver::Create("2023", *defaults);
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
        edition_defaults = { edition: "2022", value: "1" }
      ];
    }
  )schema");
  ASSERT_NE(file, nullptr);

  const FieldDescriptor* ext = file->extension(0);
  EXPECT_THAT(
      FeatureResolver::CompileDefaults(feature_set_, {ext}, "2023", "2023"),
      HasError(HasSubstr("No valid default found for edition 2022")));
}

}  // namespace
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
