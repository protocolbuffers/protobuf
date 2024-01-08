#include "google/protobuf/compiler/rust/enum.h"

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"

namespace {

using ::google::protobuf::compiler::rust::CamelToSnakeCase;
using ::google::protobuf::compiler::rust::EnumValues;
using ::google::protobuf::compiler::rust::RustEnumValue;
using ::google::protobuf::compiler::rust::ScreamingSnakeToUpperCamelCase;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Field;
using ::testing::IsEmpty;

TEST(EnumTest, CamelToSnakeCase) {
  // TODO: Review this behavior.
  EXPECT_EQ(CamelToSnakeCase("CamelCase"), "camel_case");
  EXPECT_EQ(CamelToSnakeCase("_CamelCase"), "_camel_case");
  EXPECT_EQ(CamelToSnakeCase("camelCase"), "camel_case");
  EXPECT_EQ(CamelToSnakeCase("Number2020"), "number2020");
  EXPECT_EQ(CamelToSnakeCase("Number_2020"), "number_2020");
  EXPECT_EQ(CamelToSnakeCase("camelCase_"), "camel_case_");
  EXPECT_EQ(CamelToSnakeCase("CamelCaseTrio"), "camel_case_trio");
  EXPECT_EQ(CamelToSnakeCase("UnderIn_Middle"), "under_in_middle");
  EXPECT_EQ(CamelToSnakeCase("Camel_Case"), "camel_case");
  EXPECT_EQ(CamelToSnakeCase("Camel__Case"), "camel__case");
  EXPECT_EQ(CamelToSnakeCase("CAMEL_CASE"), "c_a_m_e_l_c_a_s_e");
}

TEST(EnumTest, ScreamingSnakeToUpperCamelCase) {
  // TODO: Review this behavior.
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("CAMEL_CASE"), "CamelCase");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("NUMBER2020"), "Number2020");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("NUMBER_2020"), "Number2020");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("FOO_4040_BAR"), "Foo4040Bar");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("FOO_4040bar"), "Foo4040Bar");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("_CAMEL_CASE"), "CamelCase");

  // This function doesn't currently preserve prefix/suffix underscore,
  // while CamelToSnakeCase does.
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("CAMEL_CASE_"), "CamelCase");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("camel_case"), "CamelCase");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("CAMEL_CASE_TRIO"), "CamelCaseTrio");
  EXPECT_EQ(ScreamingSnakeToUpperCamelCase("UNDER_IN__MIDDLE"),
            "UnderInMiddle");
}

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
