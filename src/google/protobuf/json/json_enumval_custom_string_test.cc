// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <iterator>
#include <memory>
#include <string>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/json/json_enumval_custom_string.pb.h"
#include "google/protobuf/message.h"


#ifndef EXPECT_OK
#define EXPECT_OK(x) EXPECT_TRUE(x.ok())
#endif  // EXPECT_OK

namespace google {
namespace protobuf {
namespace {

using ::absl_testing::StatusIs;
using json_enumval_custom_string::Armor;
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

class DynamicEmptyOptionSchema {
 public:
  DynamicEmptyOptionSchema() {
    const FieldDescriptor* json_ext =
        DescriptorPool::generated_pool()->FindExtensionByName(
            "pb.enumvalue.json");
    ABSL_CHECK_NE(json_ext, nullptr);
    const FieldDescriptor* string_field =
        json_ext->message_type()->FindFieldByName("string");
    ABSL_CHECK_NE(string_field, nullptr);

    FileDescriptorProto file;
    file.set_name("dynamic_empty_json_option.proto");
    file.set_package("json_enumval_custom_string_dynamic");
    file.set_edition(Edition::EDITION_2026);
    file.add_dependency(json_ext->file()->name());

    const auto add_val = [&](EnumDescriptorProto* enm, absl::string_view name,
                             int number, bool has_json_opt,
                             absl::optional<absl::string_view> custom_str) {
      EnumValueDescriptorProto* ev = enm->add_value();
      ev->set_name(name);
      ev->set_number(number);
      if (has_json_opt) {
        EnumValueOptions* opts = ev->mutable_options();
        Message* sub = opts->GetReflection()->MutableMessage(opts, json_ext);
        if (custom_str.has_value()) {
          sub->GetReflection()->SetString(sub, string_field,
                                          std::string(*custom_str));
        }
      }
    };

    // DynamicArmor: explicit empty string (5) followed by multiple empty
    // options (10, 11) and aliased empty option + omitted (12).
    EnumDescriptorProto* armor = file.add_enum_type();
    armor->set_name("DynamicArmor");
    armor->mutable_options()->set_allow_alias(true);
    add_val(armor, "ARMOR_UNKNOWN", 0, false, absl::nullopt);
    add_val(armor, "ARMOR_COIF", 5, true, "");
    add_val(armor, "ARMOR_VAMBRACE", 10, true, absl::nullopt);
    add_val(armor, "ARMOR_BRACERS", 11, true, absl::nullopt);
    add_val(armor, "ARMOR_CUIRASS", 12, true, absl::nullopt);
    add_val(armor, "ARMOR_BREASTPLATE", 12, false, absl::nullopt);

    // DynamicWeapon: all values have empty (pb.enumvalue.json) = {} (including
    // default 0) and no explicit empty string.
    EnumDescriptorProto* weapon = file.add_enum_type();
    weapon->set_name("DynamicWeapon");
    add_val(weapon, "WEAPON_UNKNOWN", 0, true, absl::nullopt);
    add_val(weapon, "WEAPON_SWORD", 1, true, absl::nullopt);
    add_val(weapon, "WEAPON_MACE", 2, true, absl::nullopt);

    // EmptyOptionOrder: empty option (1) declared BEFORE explicit empty string
    // (2).
    EnumDescriptorProto* order = file.add_enum_type();
    order->set_name("EmptyOptionOrder");
    add_val(order, "VAL_UNSPECIFIED", 0, false, absl::nullopt);
    add_val(order, "VAL_EMPTY_OPT", 1, true, absl::nullopt);
    add_val(order, "VAL_EMPTY_STR", 2, true, "");

    DescriptorProto* msg = file.add_message_type();
    msg->set_name("DynamicKnight");
    FieldDescriptorProto* f1 = msg->add_field();
    f1->set_name("armor");
    f1->set_number(1);
    f1->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
    f1->set_type(FieldDescriptorProto::TYPE_ENUM);
    f1->set_type_name("DynamicArmor");

    FieldDescriptorProto* f2 = msg->add_field();
    f2->set_name("weapon");
    f2->set_number(2);
    f2->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
    f2->set_type(FieldDescriptorProto::TYPE_ENUM);
    f2->set_type_name("DynamicWeapon");

    FieldDescriptorProto* f3 = msg->add_field();
    f3->set_name("empty_option_order");
    f3->set_number(3);
    f3->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
    f3->set_type(FieldDescriptorProto::TYPE_ENUM);
    f3->set_type_name("EmptyOptionOrder");

    const FileDescriptor* built = pool_.BuildFile(file);
    ABSL_CHECK_NE(built, nullptr);
    holder_desc_ = built->FindMessageTypeByName("DynamicKnight");
    ABSL_CHECK_NE(holder_desc_, nullptr);
    armor_field_ = holder_desc_->FindFieldByName("armor");
    weapon_field_ = holder_desc_->FindFieldByName("weapon");
    order_field_ = holder_desc_->FindFieldByName("empty_option_order");
  }

  DynamicEmptyOptionSchema(const DynamicEmptyOptionSchema&) = delete;
  DynamicEmptyOptionSchema& operator=(const DynamicEmptyOptionSchema&) = delete;

  std::unique_ptr<Message> NewMessage() {
    return std::unique_ptr<Message>(
        factory_.GetPrototype(holder_desc_)->New());
  }

  const FieldDescriptor* armor_field() const { return armor_field_; }
  const FieldDescriptor* weapon_field() const { return weapon_field_; }
  const FieldDescriptor* order_field() const { return order_field_; }

 private:
  DescriptorPool pool_{DescriptorPool::generated_pool()};
  DynamicMessageFactory factory_{&pool_};
  const Descriptor* holder_desc_ = nullptr;
  const FieldDescriptor* armor_field_ = nullptr;
  const FieldDescriptor* weapon_field_ = nullptr;
  const FieldDescriptor* order_field_ = nullptr;
};

void SetEnumValue(Message* msg, const FieldDescriptor* field, int number) {
  msg->GetReflection()->SetEnumValue(msg, field, number);
}

int GetEnumValue(const Message& msg, const FieldDescriptor* field) {
  return msg.GetReflection()->GetEnumValue(msg, field);
}

bool HasField(const Message& msg, const FieldDescriptor* field) {
  return msg.GetReflection()->HasField(msg, field);
}

TEST(JsonEnumvalCustomStringTest, EmptyJsonOptionUsesDefaultIdentifier) {
  DynamicEmptyOptionSchema schema;
  {
    std::unique_ptr<Message> msg = schema.NewMessage();
    SetEnumValue(msg.get(), schema.armor_field(), 10);
    std::string json_res;
    EXPECT_OK(json::MessageToJsonString(*msg, &json_res));
    EXPECT_EQ(json_res, R"json({"armor":"ARMOR_VAMBRACE"})json");

    std::unique_ptr<Message> parsed = schema.NewMessage();
    EXPECT_OK(json::JsonStringToMessage(json_res, parsed.get()));
    EXPECT_EQ(GetEnumValue(*parsed, schema.armor_field()), 10);
  }
  {
    std::unique_ptr<Message> msg = schema.NewMessage();
    SetEnumValue(msg.get(), schema.armor_field(), 11);
    std::string json_res;
    EXPECT_OK(json::MessageToJsonString(*msg, &json_res));
    EXPECT_EQ(json_res, R"json({"armor":"ARMOR_BRACERS"})json");

    std::unique_ptr<Message> parsed = schema.NewMessage();
    EXPECT_OK(json::JsonStringToMessage(json_res, parsed.get()));
    EXPECT_EQ(GetEnumValue(*parsed, schema.armor_field()), 11);
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
    std::unique_ptr<Message> msg = schema.NewMessage();
    SetEnumValue(msg.get(), schema.order_field(), 1);
    std::string json_res;
    EXPECT_OK(json::MessageToJsonString(*msg, &json_res));
    EXPECT_EQ(json_res, R"json({"emptyOptionOrder":"VAL_EMPTY_OPT"})json");

    std::unique_ptr<Message> parsed = schema.NewMessage();
    EXPECT_OK(json::JsonStringToMessage(json_res, parsed.get()));
    EXPECT_EQ(GetEnumValue(*parsed, schema.order_field()), 1);
  }
}

TEST(JsonEnumvalCustomStringTest, EmptyJsonOptionCaseInsensitiveParsing) {
  DynamicEmptyOptionSchema schema;
  json::ParseOptions parse_options;
  parse_options.case_insensitive_enum_parsing = true;

  std::unique_ptr<Message> msg = schema.NewMessage();
  EXPECT_OK(json::JsonStringToMessage(
      R"json({"armor":"armor_VAMbrace","weapon":"WeApOn_SwOrD","emptyOptionOrder":"val_empty_opt"})json",
      msg.get(), parse_options));
  EXPECT_EQ(GetEnumValue(*msg, schema.armor_field()), 10);
  EXPECT_EQ(GetEnumValue(*msg, schema.weapon_field()), 1);
  EXPECT_EQ(GetEnumValue(*msg, schema.order_field()), 1);
}

TEST(JsonEnumvalCustomStringTest,
     EmptyStringResolvesToExplicitEmptyCustomStringNotEmptyOption) {
  DynamicEmptyOptionSchema schema;
  json::ParseOptions caseless_options;
  caseless_options.case_insensitive_enum_parsing = true;

  // ARMOR_COIF (5) precedes ARMOR_VAMBRACE (10) / ARMOR_BRACERS (11), catching
  // last-wins map overwrites.
  {
    std::unique_ptr<Message> strict_msg = schema.NewMessage();
    EXPECT_OK(json::JsonStringToMessage(R"json({"armor":""})json",
                                        strict_msg.get()));
    EXPECT_EQ(GetEnumValue(*strict_msg, schema.armor_field()), 5);

    std::unique_ptr<Message> caseless_msg = schema.NewMessage();
    EXPECT_OK(json::JsonStringToMessage(
        R"json({"armor":""})json", caseless_msg.get(), caseless_options));
    EXPECT_EQ(GetEnumValue(*caseless_msg, schema.armor_field()), 5);
  }

  // VAL_EMPTY_OPT (1) precedes VAL_EMPTY_STR (2), catching first-wins map
  // hijacking.
  {
    std::unique_ptr<Message> strict_msg = schema.NewMessage();
    EXPECT_OK(json::JsonStringToMessage(R"json({"emptyOptionOrder":""})json",
                                        strict_msg.get()));
    EXPECT_EQ(GetEnumValue(*strict_msg, schema.order_field()), 2);

    std::string json_res;
    EXPECT_OK(json::MessageToJsonString(*strict_msg, &json_res));
    EXPECT_EQ(json_res, R"json({"emptyOptionOrder":""})json");

    std::unique_ptr<Message> caseless_msg = schema.NewMessage();
    EXPECT_OK(json::JsonStringToMessage(R"json({"emptyOptionOrder":""})json",
                                        caseless_msg.get(), caseless_options));
    EXPECT_EQ(GetEnumValue(*caseless_msg, schema.order_field()), 2);
  }
}

TEST(JsonEnumvalCustomStringTest, EmptyJsonOptionRejectsEmptyStringInput) {
  DynamicEmptyOptionSchema schema;
  std::unique_ptr<Message> strict_msg = schema.NewMessage();
  EXPECT_THAT(
      json::JsonStringToMessage(R"json({"weapon":""})json", strict_msg.get()),
      StatusIs(absl::StatusCode::kInvalidArgument));

  json::ParseOptions caseless_options;
  caseless_options.case_insensitive_enum_parsing = true;
  std::unique_ptr<Message> caseless_msg = schema.NewMessage();
  EXPECT_THAT(json::JsonStringToMessage(R"json({"weapon":""})json",
                                        caseless_msg.get(), caseless_options),
              StatusIs(absl::StatusCode::kInvalidArgument));

  json::ParseOptions lenient_options;
  lenient_options.ignore_unknown_fields = true;
  std::unique_ptr<Message> lenient_msg = schema.NewMessage();
  EXPECT_OK(json::JsonStringToMessage(R"json({"weapon":""})json",
                                      lenient_msg.get(), lenient_options));
  EXPECT_FALSE(HasField(*lenient_msg, schema.weapon_field()));

  std::unique_ptr<Message> default_zero_msg = schema.NewMessage();
  SetEnumValue(default_zero_msg.get(), schema.weapon_field(), 0);
  std::string json_res;
  EXPECT_OK(json::MessageToJsonString(*default_zero_msg, &json_res));
  EXPECT_EQ(json_res, R"json({"weapon":"WEAPON_UNKNOWN"})json");
}

TEST(JsonEnumvalCustomStringTest, AliasedEmptyOptionAndOmitted) {
  Knight msg;
  msg.set_weapon(Weapon::WEAPON_SWORD);
  std::string json_res;
  EXPECT_OK(json::MessageToJsonString(msg, &json_res));
  EXPECT_EQ(json_res, R"json({"weapon":"WEAPON_SWORD"})json");

  for (absl::string_view payload :
       {R"json({"weapon":"WEAPON_SWORD"})json",
        R"json({"weapon":"WEAPON_BLADE"})json"}) {
    Knight parsed;
    EXPECT_OK(json::JsonStringToMessage(payload, &parsed));
    EXPECT_EQ(parsed.weapon(), Weapon::WEAPON_SWORD);
  }

  DynamicEmptyOptionSchema schema;
  std::unique_ptr<Message> dyn_msg = schema.NewMessage();
  SetEnumValue(dyn_msg.get(), schema.armor_field(), 12);
  json_res.clear();
  EXPECT_OK(json::MessageToJsonString(*dyn_msg, &json_res));
  EXPECT_EQ(json_res, R"json({"armor":"ARMOR_CUIRASS"})json");

  for (absl::string_view payload :
       {R"json({"armor":"ARMOR_CUIRASS"})json",
        R"json({"armor":"ARMOR_BREASTPLATE"})json"}) {
    std::unique_ptr<Message> parsed = schema.NewMessage();
    EXPECT_OK(json::JsonStringToMessage(payload, parsed.get()));
    EXPECT_EQ(GetEnumValue(*parsed, schema.armor_field()), 12);
  }
}

}  // namespace
}  // namespace protobuf
}  // namespace google
