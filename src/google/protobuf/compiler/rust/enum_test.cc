#include "google/protobuf/compiler/rust/enum.h"

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"

namespace {

using ::google::protobuf::compiler::rust::EnumValues;
using ::google::protobuf::compiler::rust::RustEnumValue;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Field;
using ::testing::IsEmpty;

template <class Aliases>
auto EnumValue(absl::string_view name, int32_t number, Aliases aliases) {
  return AllOf(Field("name", &RustEnumValue::name, Eq(name)),
               Field("number", &RustEnumValue::number, Eq(number)),
               Field("aliases", &RustEnumValue::aliases, aliases));
}

auto EnumValue(absl::string_view name, int32_t number) {
  return EnumValue(name, number, IsEmpty());
}

TEST(EnumTest, EnumValues) {
  EXPECT_THAT(EnumValues("Enum", {{"ENUM_FOO", 1}, {"ENUM_BAR", 2}}),
              ElementsAre(EnumValue("Foo", 1), EnumValue("Bar", 2)));
  EXPECT_THAT(EnumValues("Enum", {{"FOO", 1}, {"ENUM_BAR", 2}}),
              ElementsAre(EnumValue("Foo", 1), EnumValue("Bar", 2)));
  EXPECT_THAT(EnumValues("Enum", {{"enumFOO", 1}, {"eNuM_BaR", 2}}),
              ElementsAre(EnumValue("Foo", 1), EnumValue("Bar", 2)));
  EXPECT_THAT(EnumValues("Enum", {{"ENUM_ENUM_UNKNOWN", 1}, {"ENUM_ENUM", 2}}),
              ElementsAre(EnumValue("EnumUnknown", 1), EnumValue("Enum", 2)));
  EXPECT_THAT(EnumValues("Enum", {{"ENUM_VAL", 1}, {"ENUM_ALIAS", 1}}),
              ElementsAre(EnumValue("Val", 1, ElementsAre("Alias"))));
  EXPECT_THAT(
      EnumValues("Enum",
                 {{"ENUM_VAL", 1}, {"ENUM_ALIAS", 1}, {"ENUM_ALIAS2", 1}}),
      ElementsAre(EnumValue("Val", 1, ElementsAre("Alias", "Alias2"))));
  EXPECT_THAT(EnumValues("Enum", {{"ENUM_ENUM", 1}, {"ENUM", 1}}),
              ElementsAre(EnumValue("Enum", 1, IsEmpty())));
}

}  // namespace
