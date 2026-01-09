#include "editions/edition_defaults_test_utils.h"

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/types/optional.h"
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/unittest_features.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace {

TEST(TestUtilsTest, FindEditionDefault) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    defaults {
      edition: EDITION_PROTO3
      overridable_features {
        [pb.test] { file_feature: VALUE2 }
      },
      fixed_features { field_presence: IMPLICIT enum_type: OPEN }
    }
    defaults {
      edition: EDITION_2023
      overridable_features {
        [pb.test] { file_feature: VALUE3 }
      },
      fixed_features { field_presence: EXPLICIT enum_type: OPEN }
    }
  )pb");

  const auto edition_defaults = FindEditionDefault(defaults, EDITION_2023);
  ASSERT_TRUE(edition_defaults.has_value());
  EXPECT_EQ(edition_defaults->edition(), EDITION_2023);
  EXPECT_EQ(edition_defaults->overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::EnumFeature::VALUE3);
}

TEST(TestUtilsTest, FindEditionDefaultNull) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    defaults {
      edition: EDITION_PROTO3
      overridable_features {
        [pb.test] { file_feature: VALUE2 }
      },
      fixed_features { field_presence: IMPLICIT enum_type: OPEN }
    }
    defaults {
      edition: EDITION_2023
      overridable_features {
        [pb.test] { file_feature: VALUE3 }
      },
      fixed_features { field_presence: EXPLICIT enum_type: OPEN }
    }
  )pb");

  EXPECT_EQ(FindEditionDefault(defaults, EDITION_99999_TEST_ONLY),
            absl::nullopt);
}

TEST(TestUtilsTest, FindEditionDefaultEmptyDefaults) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb()pb");

  EXPECT_EQ(FindEditionDefault(defaults, EDITION_2023), absl::nullopt);
}

TEST(TestUtilsTest, FindEditionDefaultDuplicateEditions) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    defaults {
      edition: EDITION_PROTO3
      overridable_features {
        [pb.test] { file_feature: VALUE2 }
      },
      fixed_features { field_presence: IMPLICIT enum_type: OPEN }
    }
    defaults {
      edition: EDITION_2023
      overridable_features {
        [pb.test] { file_feature: VALUE3 }
      },
      fixed_features { field_presence: EXPLICIT enum_type: OPEN }
    },
    defaults {
      edition: EDITION_2023
      overridable_features {
        [pb.test] { file_feature: VALUE3 }
      },
      fixed_features { field_presence: IMPLICIT enum_type: OPEN }
    },
  )pb");

  const auto edition_defaults = FindEditionDefault(defaults, EDITION_2023);
  ASSERT_TRUE(edition_defaults.has_value());
  EXPECT_EQ(edition_defaults->edition(), EDITION_2023);
  EXPECT_EQ(edition_defaults->fixed_features().field_presence(),
            FeatureSet::EXPLICIT);
}

}  // namespace
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
