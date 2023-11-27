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
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/editions/defaults_test_embedded.h"
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
      "com_google_protobuf/src/google/protobuf/editions/",
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

  EXPECT_EQ(defaults->defaults()[0].edition(), EDITION_PROTO2);
  EXPECT_EQ(defaults->defaults()[1].edition(), EDITION_PROTO3);
  EXPECT_EQ(defaults->defaults()[2].edition(), EDITION_2023);
  EXPECT_EQ(defaults->defaults()[2].features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[2]
                .features()
                .GetExtension(pb::test)
                .int_file_feature(),
            1);
}

TEST(DefaultsTest, CheckFuture) {
  auto defaults = ReadDefaults("test_defaults_future");
  ASSERT_OK(defaults);
  ASSERT_EQ(defaults->defaults().size(), 4);
  ASSERT_EQ(defaults->minimum_edition(), EDITION_2023);
  ASSERT_EQ(defaults->maximum_edition(), EDITION_99997_TEST_ONLY);

  EXPECT_EQ(defaults->defaults()[0].edition(), EDITION_PROTO2);
  EXPECT_EQ(defaults->defaults()[1].edition(), EDITION_PROTO3);
  EXPECT_EQ(defaults->defaults()[2].edition(), EDITION_2023);
  EXPECT_EQ(defaults->defaults()[2].features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[2]
                .features()
                .GetExtension(pb::test)
                .int_file_feature(),
            1);
  EXPECT_EQ(defaults->defaults()[3].edition(), EDITION_99997_TEST_ONLY);
  EXPECT_EQ(defaults->defaults()[3].features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[3]
                .features()
                .GetExtension(pb::test)
                .int_file_feature(),
            2);
}

TEST(DefaultsTest, CheckFarFuture) {
  auto defaults = ReadDefaults("test_defaults_far_future");
  ASSERT_OK(defaults);
  ASSERT_EQ(defaults->defaults().size(), 5);
  ASSERT_EQ(defaults->minimum_edition(), EDITION_99997_TEST_ONLY);
  ASSERT_EQ(defaults->maximum_edition(), EDITION_99999_TEST_ONLY);

  EXPECT_EQ(defaults->defaults()[0].edition(), EDITION_PROTO2);
  EXPECT_EQ(defaults->defaults()[1].edition(), EDITION_PROTO3);
  EXPECT_EQ(defaults->defaults()[2].edition(), EDITION_2023);
  EXPECT_EQ(defaults->defaults()[2].features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[2]
                .features()
                .GetExtension(pb::test)
                .int_file_feature(),
            1);
  EXPECT_EQ(defaults->defaults()[3].edition(), EDITION_99997_TEST_ONLY);
  EXPECT_EQ(defaults->defaults()[3].features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[3]
                .features()
                .GetExtension(pb::test)
                .int_file_feature(),
            2);
  EXPECT_EQ(defaults->defaults()[4].edition(), EDITION_99998_TEST_ONLY);
  EXPECT_EQ(defaults->defaults()[4].features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults->defaults()[4]
                .features()
                .GetExtension(pb::test)
                .int_file_feature(),
            3);
}

TEST(DefaultsTest, Embedded) {
  FeatureSetDefaults defaults;
  ASSERT_TRUE(defaults.ParseFromArray(DEFAULTS_TEST_EMBEDDED,
                                      sizeof(DEFAULTS_TEST_EMBEDDED) - 1))
      << "Could not parse embedded data";
  ASSERT_EQ(defaults.defaults().size(), 3);
  ASSERT_EQ(defaults.minimum_edition(), EDITION_2023);
  ASSERT_EQ(defaults.maximum_edition(), EDITION_2023);

  EXPECT_EQ(defaults.defaults()[0].edition(), EDITION_PROTO2);
  EXPECT_EQ(defaults.defaults()[1].edition(), EDITION_PROTO3);
  EXPECT_EQ(defaults.defaults()[2].edition(), EDITION_2023);
  EXPECT_EQ(defaults.defaults()[2].features().field_presence(),
            FeatureSet::EXPLICIT);
  EXPECT_EQ(defaults.defaults()[2]
                .features()
                .GetExtension(pb::test)
                .int_file_feature(),
            1);
}

}  // namespace
}  // namespace protobuf
}  // namespace google
