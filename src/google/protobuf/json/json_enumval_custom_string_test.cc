// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/json/json_enumval_custom_string.pb.h"

#ifndef EXPECT_OK
#define EXPECT_OK(x) EXPECT_TRUE(x.ok())
#endif  // EXPECT_OK

namespace google {
namespace protobuf {
namespace {

using json_enumval_custom_string::Armor;
using json_enumval_custom_string::Knight;

// Gorget does not have a custom json enumval string set, so it defaults to
// the original enumval: ARMOR_GORGET.
TEST(JsonEnumvalCustomStringTest, GorgetDefaultSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GORGET);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_GORGET);

  std::string json_res{};
  absl::Status status = json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":"ARMOR_GORGET"})json");
}

// The great helm does have a custom json enumval, so let's confirm.
TEST(JsonEnumvalCustomStringTest, GreatHelmSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GREAT_HELM);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_GREAT_HELM);

  std::string json_res{};
  absl::Status status = json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":"gr8 helm"})json");
}

// The gauntlet has a double quote in the middle of its custom json enumval,
// let's make sure special characters are escaped properly.
TEST(JsonEnumvalCustomStringTest, GauntletSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GAUNTLET);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_GAUNTLET);
  std::string json_res{};
  absl::Status status = json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, "{\"armor\":\"a\\\"b\"}");
  // Roundtrip to make sure we can parse it back
  Knight msg2;
  absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
  EXPECT_OK(parse_status);
  EXPECT_EQ(msg2.armor(), Armor::ARMOR_GAUNTLET);
}

// Test the intersection of case-insensitive and custom json enumval.
TEST(JsonEnumvalCustomStringTest, CaseInsensitiveGauntletParsing) {
  std::string json_res = "{\"armor\":\"A\\\"b\"}";
  json::ParseOptions parse_options;
  parse_options.case_insensitive_enum_parsing = true;

  Knight msg2;
  absl::Status parse_status =
      json::JsonStringToMessage(json_res, &msg2, parse_options);

  EXPECT_OK(parse_status);
  EXPECT_EQ(msg2.armor(), Armor::ARMOR_GAUNTLET);
}

// Test quotes surrounding the custom json enumval.
TEST(JsonEnumvalCustomStringTest, DoubleQuoteEnumSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_PLATE);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_PLATE);
  std::string json_res{};
  absl::Status status = json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, "{\"armor\":\"\\\"plate\\\"\"}");
  // Roundtrip to make sure we can parse it back
  Knight msg2;
  absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
  EXPECT_OK(parse_status);
  EXPECT_EQ(msg2.armor(), Armor::ARMOR_PLATE);
}

// Int overrides always win.
TEST(JsonEnumvalCustomStringTest, GreatHelmIntOverride) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GREAT_HELM);
  std::string json_res;
  json::PrintOptions print_options;
  print_options.always_print_enums_as_ints = true;
  absl::Status status =
      json::MessageToJsonString(msg, &json_res, print_options);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":1})json");
}
}  // namespace
}  // namespace protobuf
}  // namespace google
