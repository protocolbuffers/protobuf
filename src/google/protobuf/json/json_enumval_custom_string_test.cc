// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <iterator>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/json/json_enumval_custom_string.pb.h"


#ifndef EXPECT_OK
#define EXPECT_OK(x) EXPECT_TRUE(x.ok())
#endif  // EXPECT_OK

namespace google {
namespace protobuf {
namespace {

using ::absl_testing::StatusIs;
using json_enumval_custom_string::Armor;
using json_enumval_custom_string::EmptyOptionOrder;
using json_enumval_custom_string::Knight;
using json_enumval_custom_string::Weapon;
using ::testing::HasSubstr;

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

// Test that we accept the raw enum name as input even when a custom JSON
// enumval string option is set for Great Helm.
TEST(JsonEnumvalCustomStringTest, GreatHelmRawNameAccepted) {
  Knight msg;
  absl::Status status = json::JsonStringToMessage(
      R"json({"armor":"ARMOR_GREAT_HELM"})json", &msg);
  EXPECT_OK(status);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_GREAT_HELM);
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

// Test that regular case-insensitive payloads can still be parsed.
TEST(JsonEnumvalCustomStringTest, CaseInsensitiveGauntletRawNameParsing) {
  std::string json_res = "{\"armor\":\"armor_GAUNtlet\"}";
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

// Coif is an empty custom string, let's make sure that works.
TEST(JsonEnumvalCustomStringTest, CoifEmptySerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_COIF);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_COIF);
  std::string json_res{};
  absl::Status status = json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":""})json");
  // Roundtrip to make sure we can parse it back
  Knight msg2;
  absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
  EXPECT_OK(parse_status);
  EXPECT_EQ(msg2.armor(), Armor::ARMOR_COIF);
}

// Ensure newlines and tabs are properly escaped.
TEST(JsonEnumvalCustomStringTest, PauldronEscapingSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_PAULDRON);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_PAULDRON);
  std::string json_res{};
  absl::Status status = json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);

  EXPECT_EQ(json_res, R"json({"armor":"p\taul\ndron"})json");
  // Roundtrip to make sure we can parse it back
  Knight msg2;
  absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
  EXPECT_OK(parse_status);
  EXPECT_EQ(msg2.armor(), Armor::ARMOR_PAULDRON);
}

// Test that aliased enum values both serialize to the correct custom string.
TEST(JsonEnumvalCustomStringTest, AliasedSerialization) {
  {
    Knight msg;
    msg.set_armor(Armor::ARMOR_SABATON);
    EXPECT_EQ(msg.armor(), Armor::ARMOR_SABATON);

    std::string json_res{};
    absl::Status status = json::MessageToJsonString(msg, &json_res);
    EXPECT_OK(status);
    EXPECT_EQ(json_res, R"json({"armor":"sabaton"})json");

    Knight msg2;
    absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
    EXPECT_OK(parse_status);
    EXPECT_EQ(msg2.armor(), Armor::ARMOR_SABATON);
  }

  {
    Knight msg;
    msg.set_armor(Armor::ARMOR_SOLLERET);
    EXPECT_EQ(msg.armor(), Armor::ARMOR_SOLLERET);

    std::string json_res{};
    absl::Status status = json::MessageToJsonString(msg, &json_res);
    EXPECT_OK(status);
    EXPECT_EQ(json_res, R"json({"armor":"sabaton"})json");

    Knight msg2;
    absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
    EXPECT_OK(parse_status);
    EXPECT_EQ(msg2.armor(), Armor::ARMOR_SOLLERET);
  }
}

// Test that we accept the raw enum names as input even when a custom JSON
// enumval string option is set for aliased values.
TEST(JsonEnumvalCustomStringTest, AliasedRawNamesAccepted) {
  {
    Knight msg;
    absl::Status status =
        json::JsonStringToMessage(R"json({"armor":"ARMOR_SABATON"})json", &msg);
    EXPECT_OK(status);
    EXPECT_EQ(msg.armor(), Armor::ARMOR_SABATON);
  }
  {
    Knight msg;
    absl::Status status = json::JsonStringToMessage(
        R"json({"armor":"ARMOR_SOLLERET"})json", &msg);
    EXPECT_OK(status);
    EXPECT_EQ(msg.armor(), Armor::ARMOR_SOLLERET);
  }
}

// Test that custom enumval option string containing numeric digits (e.g., "8")
// serializes properly and can be parsed back as a string, and is also parsed
// correctly as an integer input.
TEST(JsonEnumvalCustomStringTest, NumericStringCustomOption) {
  {
    Knight msg;
    msg.set_armor(Armor::ARMOR_HACHI_MAI_DO);
    EXPECT_EQ(msg.armor(), Armor::ARMOR_HACHI_MAI_DO);

    std::string json_res{};
    absl::Status status = json::MessageToJsonString(msg, &json_res);
    EXPECT_OK(status);
    EXPECT_EQ(json_res, R"json({"armor":"8"})json");

    Knight msg2;
    absl::Status parse_status = json::JsonStringToMessage(json_res, &msg2);
    EXPECT_OK(parse_status);
    EXPECT_EQ(msg2.armor(), Armor::ARMOR_HACHI_MAI_DO);
  }
  {
    // Verify that the integer input 8 also parses successfully.
    Knight msg;
    absl::Status status =
        json::JsonStringToMessage(R"json({"armor":8})json", &msg);
    EXPECT_OK(status);
    EXPECT_EQ(msg.armor(), Armor::ARMOR_HACHI_MAI_DO);
  }
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

// TODO: b/534418787 - Investigate this behavior further, especially in
// conformance tests.
TEST(JsonEnumvalCustomStringTest, ParserSingleElementArrayEnumName) {
  Knight msg;
  absl::Status status = json::JsonStringToMessage(
      R"json({"armor":["ARMOR_GREAT_HELM"]})json", &msg);
  EXPECT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument,
                               HasSubstr("invalid JSON")));
}

TEST(JsonEnumvalCustomStringTest, EmptyJsonOptionUsesDefaultIdentifier) {
  {
    Knight msg;
    msg.set_armor(Armor::ARMOR_VAMBRACE);
    std::string json_res;
    EXPECT_OK(json::MessageToJsonString(msg, &json_res));
    EXPECT_EQ(json_res, R"json({"armor":"ARMOR_VAMBRACE"})json");

    Knight parsed;
    EXPECT_OK(json::JsonStringToMessage(json_res, &parsed));
    EXPECT_EQ(parsed.armor(), Armor::ARMOR_VAMBRACE);
  }
  {
    Knight msg;
    msg.set_armor(Armor::ARMOR_BRACERS);
    std::string json_res;
    EXPECT_OK(json::MessageToJsonString(msg, &json_res));
    EXPECT_EQ(json_res, R"json({"armor":"ARMOR_BRACERS"})json");

    Knight parsed;
    EXPECT_OK(json::JsonStringToMessage(json_res, &parsed));
    EXPECT_EQ(parsed.armor(), Armor::ARMOR_BRACERS);
  }
  {
    Knight msg;
    msg.set_weapon(Weapon::WEAPON_SWORD);
    std::string json_res;
    EXPECT_OK(json::MessageToJsonString(msg, &json_res));
    EXPECT_EQ(json_res, R"json({"weapon":"WEAPON_SWORD"})json");

    Knight parsed;
    EXPECT_OK(json::JsonStringToMessage(json_res, &parsed));
    EXPECT_EQ(parsed.weapon(), Weapon::WEAPON_SWORD);
  }
  {
    Knight msg;
    msg.set_empty_option_order(EmptyOptionOrder::VAL_EMPTY_OPT);
    std::string json_res;
    EXPECT_OK(json::MessageToJsonString(msg, &json_res));
    EXPECT_EQ(json_res, R"json({"emptyOptionOrder":"VAL_EMPTY_OPT"})json");

    Knight parsed;
    EXPECT_OK(json::JsonStringToMessage(json_res, &parsed));
    EXPECT_EQ(parsed.empty_option_order(), EmptyOptionOrder::VAL_EMPTY_OPT);
  }
}

TEST(JsonEnumvalCustomStringTest, EmptyJsonOptionCaseInsensitiveParsing) {
  json::ParseOptions parse_options;
  parse_options.case_insensitive_enum_parsing = true;

  Knight msg;
  EXPECT_OK(json::JsonStringToMessage(
      R"json({"armor":"armor_VAMbrace","weapon":"WeApOn_SwOrD","emptyOptionOrder":"val_empty_opt"})json",
      &msg, parse_options));
  EXPECT_EQ(msg.armor(), Armor::ARMOR_VAMBRACE);
  EXPECT_EQ(msg.weapon(), Weapon::WEAPON_SWORD);
  EXPECT_EQ(msg.empty_option_order(), EmptyOptionOrder::VAL_EMPTY_OPT);
}

TEST(JsonEnumvalCustomStringTest,
     EmptyStringResolvesToExplicitEmptyCustomStringNotEmptyOption) {
  json::ParseOptions caseless_options;
  caseless_options.case_insensitive_enum_parsing = true;

  // ARMOR_COIF (5) precedes ARMOR_VAMBRACE (10) / ARMOR_BRACERS (11), catching
  // last-wins map overwrites.
  {
    Knight strict_msg;
    EXPECT_OK(
        json::JsonStringToMessage(R"json({"armor":""})json", &strict_msg));
    EXPECT_EQ(strict_msg.armor(), Armor::ARMOR_COIF);

    Knight caseless_msg;
    EXPECT_OK(json::JsonStringToMessage(R"json({"armor":""})json",
                                        &caseless_msg, caseless_options));
    EXPECT_EQ(caseless_msg.armor(), Armor::ARMOR_COIF);
  }

  // VAL_EMPTY_OPT (1) precedes VAL_EMPTY_STR (2), catching first-wins map
  // hijacking.
  {
    Knight strict_msg;
    EXPECT_OK(json::JsonStringToMessage(R"json({"emptyOptionOrder":""})json",
                                        &strict_msg));
    EXPECT_EQ(strict_msg.empty_option_order(), EmptyOptionOrder::VAL_EMPTY_STR);

    std::string json_res;
    EXPECT_OK(json::MessageToJsonString(strict_msg, &json_res));
    EXPECT_EQ(json_res, R"json({"emptyOptionOrder":""})json");

    Knight caseless_msg;
    EXPECT_OK(json::JsonStringToMessage(R"json({"emptyOptionOrder":""})json",
                                        &caseless_msg, caseless_options));
    EXPECT_EQ(caseless_msg.empty_option_order(),
              EmptyOptionOrder::VAL_EMPTY_STR);
  }
}

TEST(JsonEnumvalCustomStringTest, EmptyJsonOptionRejectsEmptyStringInput) {
  Knight strict_msg;
  EXPECT_THAT(
      json::JsonStringToMessage(R"json({"weapon":""})json", &strict_msg),
      StatusIs(absl::StatusCode::kInvalidArgument));

  json::ParseOptions caseless_options;
  caseless_options.case_insensitive_enum_parsing = true;
  Knight caseless_msg;
  EXPECT_THAT(json::JsonStringToMessage(R"json({"weapon":""})json",
                                        &caseless_msg, caseless_options),
              StatusIs(absl::StatusCode::kInvalidArgument));

  json::ParseOptions lenient_options;
  lenient_options.ignore_unknown_fields = true;
  Knight lenient_msg;
  EXPECT_OK(json::JsonStringToMessage(R"json({"weapon":""})json", &lenient_msg,
                                      lenient_options));
  EXPECT_FALSE(lenient_msg.has_weapon());

  Knight default_zero_msg;
  default_zero_msg.set_weapon(Weapon::WEAPON_UNKNOWN);
  std::string json_res;
  EXPECT_OK(json::MessageToJsonString(default_zero_msg, &json_res));
  EXPECT_EQ(json_res, R"json({"weapon":"WEAPON_UNKNOWN"})json");
}

TEST(JsonEnumvalCustomStringTest, AliasedEmptyOptionAndOmitted) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_CUIRASS);
  std::string json_res;
  EXPECT_OK(json::MessageToJsonString(msg, &json_res));
  EXPECT_EQ(json_res, R"json({"armor":"ARMOR_CUIRASS"})json");

  for (absl::string_view payload :
       {R"json({"armor":"ARMOR_CUIRASS"})json",
        R"json({"armor":"ARMOR_BREASTPLATE"})json"}) {
    Knight parsed;
    EXPECT_OK(json::JsonStringToMessage(payload, &parsed));
    EXPECT_EQ(parsed.armor(), Armor::ARMOR_CUIRASS);
  }
}

}  // namespace
}  // namespace protobuf
}  // namespace google
