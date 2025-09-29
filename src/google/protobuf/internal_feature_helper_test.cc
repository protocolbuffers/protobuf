#include "google/protobuf/internal_feature_helper.h"

#include <cstdint>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/parser.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/feature_resolver.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/unittest_features.pb.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace {
#define ASSERT_OK(x) ASSERT_TRUE(x.ok()) << x.message();

using ::testing::NotNull;

class SimpleErrorCollector : public io::ErrorCollector {
 public:
  void RecordError(int line, int column, absl::string_view message) override {
    ABSL_LOG(ERROR) << absl::StrFormat("%d:%d:%s", line, column, message);
  }
};
}  // namespace

class InternalFeatureHelperTest : public ::testing::Test {
 protected:
  template <typename DescriptorT, typename TypeTraitsT, uint8_t field_type,
            bool is_packed>
  static auto GetResolvedSourceFeatureExtension(
      const DescriptorT& desc,
      const google::protobuf::internal::ExtensionIdentifier<
          FeatureSet, TypeTraitsT, field_type, is_packed>& extension) {
    return ::google::protobuf::internal::InternalFeatureHelper::
        GetResolvedFeatureExtension(desc, extension);
  }

  const FileDescriptor* BuildFile(absl::string_view schema) {
    io::ArrayInputStream input_stream(schema.data(),
                                      static_cast<int>(schema.size()));
    SimpleErrorCollector error_collector;
    io::Tokenizer tokenizer(&input_stream, &error_collector);
    compiler::Parser parser;
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
  FeatureSetDefaults GetFeatureSetDefaults() {
    auto defaults = FeatureResolver::CompileDefaults(
        FeatureSet::descriptor(), {GetExtensionReflection(pb::test)},
        EDITION_PROTO2, EDITION_2024);
    ABSL_CHECK_OK(defaults);
    return *defaults;
  }

  DescriptorPool pool_;
};

namespace {
TEST_F(InternalFeatureHelperTest, GetResolvedSourceFeatureExtension) {
  ASSERT_OK(pool_.SetFeatureSetDefaults(GetFeatureSetDefaults()));

  ASSERT_THAT(BuildFile(DescriptorProto::descriptor()->file()), NotNull());
  ASSERT_THAT(BuildFile(pb::TestMessage::descriptor()->file()), NotNull());
  auto file = BuildFile(R"schema(
    edition = "2023";
    package proto2_unittest;

    import "google/protobuf/unittest_features.proto";

    option features.(pb.test).file_feature = VALUE6;
    option features.(pb.test).source_feature = VALUE5;
  )schema");
  ASSERT_THAT(file, NotNull());
  const pb::TestFeatures& ext1 =
      GetResolvedSourceFeatureExtension(*file, pb::test);
  const pb::TestFeatures& ext2 =
      InternalFeatureHelper::GetFeatures(*file).GetExtension(pb::test);
  EXPECT_EQ(ext1.enum_feature(), pb::EnumFeature::VALUE1);
  EXPECT_EQ(ext1.field_feature(), pb::EnumFeature::VALUE1);
  EXPECT_EQ(ext1.file_feature(), pb::EnumFeature::VALUE6);
  EXPECT_EQ(ext1.source_feature(), pb::EnumFeature::VALUE5);
  EXPECT_EQ(ext2.enum_feature(), ext1.enum_feature());
  EXPECT_EQ(ext2.field_feature(), ext1.field_feature());
  EXPECT_EQ(ext2.file_feature(), ext1.file_feature());
  EXPECT_EQ(ext2.source_feature(), ext1.source_feature());
}

TEST_F(InternalFeatureHelperTest,
       GetResolvedSourceFeatureExtensionEditedDefaults) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    minimum_edition: EDITION_PROTO2
    maximum_edition: EDITION_2024
    defaults {
      edition: EDITION_LEGACY
      overridable_features {}
      fixed_features {
        field_presence: EXPLICIT
        enum_type: CLOSED
        repeated_field_encoding: EXPANDED
        utf8_validation: NONE
        message_encoding: LENGTH_PREFIXED
        json_format: LEGACY_BEST_EFFORT
        enforce_naming_style: STYLE_LEGACY
        default_symbol_visibility: EXPORT_ALL
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
        [pb.test] {
          file_feature: VALUE3
          field_feature: VALUE15
          enum_feature: VALUE14
          source_feature: VALUE1
        }
      }
      fixed_features {
        enforce_naming_style: STYLE_LEGACY
        default_symbol_visibility: EXPORT_ALL
      }
    }
  )pb");
  ASSERT_OK(pool_.SetFeatureSetDefaults(defaults));

  ASSERT_THAT(BuildFile(DescriptorProto::descriptor()->file()), NotNull());
  ASSERT_THAT(BuildFile(pb::TestMessage::descriptor()->file()), NotNull());
  auto file = BuildFile(R"schema(
    edition = "2023";
    package proto2_unittest;

    import "google/protobuf/unittest_features.proto";

    option features.(pb.test).file_feature = VALUE6;
    option features.(pb.test).source_feature = VALUE5;
  )schema");
  ASSERT_THAT(file, NotNull());
  const pb::TestFeatures& ext =
      GetResolvedSourceFeatureExtension(*file, pb::test);

  EXPECT_EQ(ext.enum_feature(), pb::EnumFeature::VALUE14);
  EXPECT_EQ(ext.field_feature(), pb::EnumFeature::VALUE15);
  EXPECT_EQ(ext.file_feature(), pb::EnumFeature::VALUE6);
  EXPECT_EQ(ext.source_feature(), pb::EnumFeature::VALUE5);
}

TEST_F(InternalFeatureHelperTest,
       GetResolvedSourceFeatureExtensionDefaultsFromFeatureSetExtension) {
  ASSERT_THAT(BuildFile(DescriptorProto::descriptor()->file()), NotNull());
  ASSERT_THAT(BuildFile(pb::TestMessage::descriptor()->file()), NotNull());
  auto file = BuildFile(R"schema(
    edition = "2023";
    package proto2_unittest;

    import "google/protobuf/unittest_features.proto";

    option features.(pb.test).file_feature = VALUE6;
    option features.(pb.test).source_feature = VALUE5;
  )schema");
  ASSERT_THAT(file, NotNull());

  const pb::TestFeatures& ext1 =
      GetResolvedSourceFeatureExtension(*file, pb::test);
  const pb::TestFeatures& ext2 =
      InternalFeatureHelper::GetFeatures(*file).GetExtension(pb::test);

  EXPECT_EQ(ext1.enum_feature(), pb::EnumFeature::VALUE1);
  EXPECT_EQ(ext1.field_feature(), pb::EnumFeature::VALUE1);
  EXPECT_EQ(ext1.file_feature(), pb::EnumFeature::VALUE6);
  EXPECT_EQ(ext1.source_feature(), pb::EnumFeature::VALUE5);
  EXPECT_EQ(ext2.enum_feature(), pb::EnumFeature::TEST_ENUM_FEATURE_UNKNOWN);
  EXPECT_EQ(ext2.field_feature(), pb::EnumFeature::TEST_ENUM_FEATURE_UNKNOWN);
  EXPECT_EQ(ext2.file_feature(), pb::EnumFeature::VALUE6);
  EXPECT_EQ(ext2.source_feature(), pb::EnumFeature::VALUE5);
}

#include "google/protobuf/port_undef.inc"
}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
