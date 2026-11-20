// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/enum.h"

#include <cstdint>
#include <limits>
#include <utility>

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
              ElementsAre(EnumValue(
                  "Val", 1, ElementsAre(std::make_pair("Alias", nullptr)))));
  EXPECT_THAT(
      EnumValues("Enum",
                 {{"ENUM_VAL", 1}, {"ENUM_ALIAS", 1}, {"ENUM_ALIAS2", 1}}),
      ElementsAre(EnumValue("Val", 1,
                            ElementsAre(std::make_pair("Alias", nullptr),
                                        std::make_pair("Alias2", nullptr)))));
  EXPECT_THAT(EnumValues("Enum", {{"ENUM_ENUM", 1}, {"ENUM", 1}}),
              ElementsAre(EnumValue("Enum", 1, IsEmpty())));
}

TEST(EnumTest, CollidingNormalizedNamesWithDistinctNumbers) {
  EXPECT_THAT(EnumValues("Status", {{"STATUS_OK", 1}, {"OK", 2}}),
              ElementsAre(EnumValue("Ok", 1), EnumValue("Ok_2", 2)));
  EXPECT_THAT(
      EnumValues("MyEnum",
                 {{"A1_B1_C", 1}, {"A1_B1C", 2}, {"A1B1_C", 3}, {"A1B1C", 4}}),
      ElementsAre(EnumValue("A1B1C", 1), EnumValue("A1B1C_2", 2),
                  EnumValue("A1B1C_3", 3), EnumValue("A1B1C_4", 4)));
  EXPECT_THAT(
      EnumValues("MyEnum",
                 {{"V1_NEG_1_A", -1},
                  {"V1_NEG_1A", -2},
                  {"V1_NEG_1_A_", std::numeric_limits<int32_t>::min()}}),
      ElementsAre(EnumValue("V1Neg1A", -1), EnumValue("V1Neg1A_Neg2", -2),
                  EnumValue("V1Neg1A_Neg2147483648",
                            std::numeric_limits<int32_t>::min())));
  EXPECT_THAT(EnumValues("Status", {{"STATUS_SELF", 1}, {"SELF", 2}}),
              ElementsAre(EnumValue("Self_", 1), EnumValue("Self_2", 2)));
  EXPECT_THAT(
      EnumValues("MyEnum",
                 {{"V1_A", 1}, {"OTHER", 2}, {"V1A", 2}, {"MY_ENUM_V1A", 2}}),
      ElementsAre(EnumValue("V1A", 1),
                  EnumValue("Other", 2,
                            ElementsAre(std::make_pair("V1A_2", nullptr)))));
}

}  // namespace
