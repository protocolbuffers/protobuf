#include <string>

#include "tools/cpp/runfiles/runfiles.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/java_features.pb.h"
#include "google/protobuf/cpp_features.pb.h"
#include "editions/defaults_test_embedded.h"
#include "editions/defaults_test_embedded_base64.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/message.h"
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/unittest_features.pb.h"
#include "google/protobuf/stubs/status_macros.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

#define ASSERT_OK(x) ASSERT_TRUE(x.ok()) << x.status().message();

namespace google {
namespace protobuf {
namespace {

absl::StatusOr<FeatureSetDefaults> ReadDefaults(absl::string_view name) {
  auto runfiles = absl::WrapUnique(bazel::tools::cpp::runfiles::Runfiles::CreateForTest());
  std::string file = runfiles->Rlocation(absl::StrCat(
      "com_google_protobuf/editions/",
      name, ".binpb"));
  std::string data;
  RETURN_IF_ERROR(File::GetContents(file, &data, true));
  FeatureSetDefaults defaults;
  if (!defaults.ParseFromString(data)) {
    return absl::InternalError("Could not parse edition defaults!");
  }
  return defaults;
}

TEST(DefaultsTest, Check2023) {
  auto defaults = ReadDefaults("test_defaults_2023");
  ASSERT_OK(defaults);
  ASSERT_EQ(defaults->defaults().size(), 3);
  ASSERT_EQ(defaults->minimum_edition(), EDITION_2023);
  ASSERT_EQ(defaults->maximum_edition(), EDITION_2023);

  EXPECT_EQ(defaults->defaults()[0].edition(), EDITION_LEGACY);
  EXPECT_EQ(defaults->defaults()[1].edition(), EDITION_PROTO3);
  EXPECT_EQ(defaults->defaults()[2].edition(), EDITION_2023);
  EXPECT_EQ(defaults->defaults()[2].overridable_features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[2]
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::VALUE3);
}

TEST(DefaultsTest, CheckFuture) {
  auto defaults = ReadDefaults("test_defaults_future");
  ASSERT_OK(defaults);
  ASSERT_EQ(defaults->defaults().size(), 5);
  ASSERT_EQ(defaults->minimum_edition(), EDITION_2023);
  ASSERT_EQ(defaults->maximum_edition(), EDITION_99997_TEST_ONLY);

  EXPECT_EQ(defaults->defaults()[0].edition(), EDITION_LEGACY);
  EXPECT_EQ(defaults->defaults()[1].edition(), EDITION_PROTO3);
  EXPECT_EQ(defaults->defaults()[2].edition(), EDITION_2023);
  EXPECT_EQ(defaults->defaults()[2].overridable_features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[2]
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::VALUE3);
  EXPECT_EQ(defaults->defaults()[3].edition(), EDITION_2024);
  EXPECT_EQ(defaults->defaults()[3].overridable_features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[3]
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::VALUE3);
  EXPECT_EQ(defaults->defaults()[4].edition(), EDITION_99997_TEST_ONLY);
  EXPECT_EQ(defaults->defaults()[4].overridable_features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[4]
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::VALUE4);
}

TEST(DefaultsTest, CheckFarFuture) {
  auto defaults = ReadDefaults("test_defaults_far_future");
  ASSERT_OK(defaults);
  ASSERT_EQ(defaults->defaults().size(), 7);
  ASSERT_EQ(defaults->minimum_edition(), EDITION_99997_TEST_ONLY);
  ASSERT_EQ(defaults->maximum_edition(), EDITION_99999_TEST_ONLY);

  EXPECT_EQ(defaults->defaults()[0].edition(), EDITION_LEGACY);
  EXPECT_EQ(defaults->defaults()[1].edition(), EDITION_PROTO3);
  EXPECT_EQ(defaults->defaults()[2].edition(), EDITION_2023);
  EXPECT_EQ(defaults->defaults()[2].overridable_features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[2]
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::VALUE3);
  EXPECT_EQ(defaults->defaults()[3].edition(), EDITION_2024);
  EXPECT_EQ(defaults->defaults()[3].overridable_features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[3]
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::VALUE3);
  EXPECT_EQ(defaults->defaults()[4].edition(), EDITION_99997_TEST_ONLY);
  EXPECT_EQ(defaults->defaults()[4].overridable_features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[4]
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::VALUE4);
  EXPECT_EQ(defaults->defaults()[5].edition(), EDITION_99998_TEST_ONLY);
  EXPECT_EQ(defaults->defaults()[5].overridable_features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[5]
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::VALUE5);
}

TEST(DefaultsTest, Embedded) {
  FeatureSetDefaults defaults;
  ASSERT_TRUE(defaults.ParseFromArray(DEFAULTS_TEST_EMBEDDED,
                                      sizeof(DEFAULTS_TEST_EMBEDDED) - 1))
      << "Could not parse embedded data";
  ASSERT_EQ(defaults.defaults().size(), 3);
  ASSERT_EQ(defaults.minimum_edition(), EDITION_2023);
  ASSERT_EQ(defaults.maximum_edition(), EDITION_2023);

  EXPECT_EQ(defaults.defaults()[0].edition(), EDITION_LEGACY);
  EXPECT_EQ(defaults.defaults()[1].edition(), EDITION_PROTO3);
  EXPECT_EQ(defaults.defaults()[2].edition(), EDITION_2023);
  EXPECT_EQ(defaults.defaults()[2].overridable_features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults.defaults()[2]
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::VALUE3);
}

TEST(DefaultsTest, EmbeddedBase64) {
  FeatureSetDefaults defaults;
  std::string data;
  ASSERT_TRUE(absl::Base64Unescape(
      absl::string_view{DEFAULTS_TEST_EMBEDDED_BASE64,
                        sizeof(DEFAULTS_TEST_EMBEDDED_BASE64) - 1},
      &data));
  ASSERT_TRUE(defaults.ParseFromString(data));
  ASSERT_EQ(defaults.defaults().size(), 3);
  ASSERT_EQ(defaults.minimum_edition(), EDITION_2023);
  ASSERT_EQ(defaults.maximum_edition(), EDITION_2023);

  EXPECT_EQ(defaults.defaults()[0].edition(), EDITION_LEGACY);
  EXPECT_EQ(defaults.defaults()[1].edition(), EDITION_PROTO3);
  EXPECT_EQ(defaults.defaults()[2].edition(), EDITION_2023);
  EXPECT_EQ(defaults.defaults()[2].overridable_features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults.defaults()[2]
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::VALUE3);
}

// Lock down that overridable defaults never change in released editions.  After
// an edition has been released these tests should never need to be touched.
class OverridableDefaultsTest : public ::testing::Test {
 public:
  OverridableDefaultsTest() = default;
  static void SetUpTestSuite() {
    google::protobuf::LinkExtensionReflection(pb::cpp);
    google::protobuf::LinkExtensionReflection(pb::java);
    DescriptorPool::generated_pool();
  }
};


TEST_F(OverridableDefaultsTest, Proto2) {
  auto feature_defaults = ReadDefaults("protobuf_defaults");
  ASSERT_OK(feature_defaults);
  ASSERT_GE(feature_defaults->defaults().size(), 1);
  auto defaults = feature_defaults->defaults(0);
  ASSERT_EQ(defaults.edition(), EDITION_LEGACY);


  EXPECT_THAT(defaults.overridable_features(), EqualsProto(R"pb([pb.cpp] {}
                                                                [pb.java] {}
              )pb"));
}
TEST_F(OverridableDefaultsTest, Proto3) {
  auto feature_defaults = ReadDefaults("protobuf_defaults");
  ASSERT_OK(feature_defaults);
  ASSERT_GE(feature_defaults->defaults().size(), 2);
  auto defaults = feature_defaults->defaults(1);
  ASSERT_EQ(defaults.edition(), EDITION_PROTO3);


  EXPECT_THAT(defaults.overridable_features(), EqualsProto(R"pb([pb.cpp] {}
                                                                [pb.java] {}
              )pb"));
}

// Lock down that 2023 overridable defaults never change.  Once Edition 2023 has
// been released this test should never need to be touched.
TEST_F(OverridableDefaultsTest, Edition2023) {
  auto feature_defaults = ReadDefaults("protobuf_defaults");
  ASSERT_OK(feature_defaults);
  ASSERT_GE(feature_defaults->defaults().size(), 3);
  auto defaults = feature_defaults->defaults(2);
  ASSERT_EQ(defaults.edition(), EDITION_2023);


  EXPECT_THAT(defaults.overridable_features(), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                [pb.cpp] { legacy_closed_enum: false string_type: STRING }
                [pb.java] { legacy_closed_enum: false utf8_validation: DEFAULT }
              )pb"));
}

}  // namespace
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
