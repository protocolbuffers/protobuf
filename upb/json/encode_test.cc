// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/json/encode.h"

#include <cstddef>
#include <string>

#include "google/protobuf/struct.upb.h"
#include <gtest/gtest.h>
#include "google/protobuf/json/json_enumval_custom_string.upb.h"
#include "google/protobuf/json/json_enumval_custom_string.upbdefs.h"
#include "upb/base/status.hpp"
#include "upb/base/upcast.h"
#include "upb/json/test.upb.h"
#include "upb/json/test.upbdefs.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/message/message.h"
#include "upb/reflection/def.h"
#include "upb/reflection/def.hpp"

static std::string JsonEncodeGeneric(
    const upb_Message* msg, const upb_MessageDef* (*getmsgdef)(upb_DefPool*),
    int options) {
  upb::Arena a;
  upb::Status status;
  upb::DefPool defpool;
  const upb_MessageDef* m = getmsgdef(defpool.ptr());
  EXPECT_TRUE(m != nullptr);

  size_t json_size =
      upb_JsonEncode(msg, m, defpool.ptr(), options, nullptr, 0, status.ptr());
  char* json_buf = (char*)upb_Arena_Malloc(a.ptr(), json_size + 1);

  size_t size = upb_JsonEncode(msg, m, defpool.ptr(), options, json_buf,
                               json_size + 1, status.ptr());
  EXPECT_EQ(size, json_size);
  return std::string(json_buf, json_size);
}

static std::string JsonEncode(const upb_test_Box* msg, int options) {
  return JsonEncodeGeneric(UPB_UPCAST(msg), upb_test_Box_getmsgdef, options);
}

static std::string JsonEncodeKnight(
    const json_enumval_custom_string_Knight* msg, int options) {
  return JsonEncodeGeneric(
      UPB_UPCAST(msg), json_enumval_custom_string_Knight_getmsgdef, options);
}

// Encode a single optional enum.
TEST(JsonTest, EncodeEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_set_first_tag(foo, upb_test_Z_BAR);

  EXPECT_EQ(R"({"firstTag":"Z_BAR"})", JsonEncode(foo, 0));
  EXPECT_EQ(R"({"firstTag":1})",
            JsonEncode(foo, upb_JsonEncode_FormatEnumsAsIntegers));
}

// Encode a single optional negative enum.
TEST(JsonTest, EncodeNegativeEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_set_last_tag(foo, upb_test_Z_BAZ);

  EXPECT_EQ(R"({"lastTag":"Z_BAZ"})", JsonEncode(foo, 0));
  EXPECT_EQ(R"({"lastTag":-2})",
            JsonEncode(foo, upb_JsonEncode_FormatEnumsAsIntegers));
}

// Encode a single repeated enum.
TEST(JsonTest, EncodeRepeatedEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_add_more_tags(foo, upb_test_Z_BAT, a.ptr());

  EXPECT_EQ(R"({"moreTags":["Z_BAT"]})", JsonEncode(foo, 0));
  EXPECT_EQ(R"({"moreTags":[13]})",
            JsonEncode(foo, upb_JsonEncode_FormatEnumsAsIntegers));
}

// Special case: encode null enum.
TEST(JsonTest, EncodeNullEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  google_protobuf_Value_set_null_value(upb_test_Box_mutable_val(foo, a.ptr()),
                                       google_protobuf_NULL_VALUE);

  EXPECT_EQ(R"({"val":null})", JsonEncode(foo, 0));
  EXPECT_EQ(R"({"val":null})",
            JsonEncode(foo, upb_JsonEncode_FormatEnumsAsIntegers));
}

TEST(JsonTest, EncodeConflictJsonName) {
  upb::Arena a;
  upb_test_Box* box = upb_test_Box_new(a.ptr());
  upb_test_Box_set_value(box, 2);
  EXPECT_EQ(R"({"old_value":2})", JsonEncode(box, 0));

  upb_test_Box* new_box = upb_test_Box_new(a.ptr());
  upb_test_Box_set_new_value(new_box, 2);
  EXPECT_EQ(R"({"value":2})", JsonEncode(new_box, 0));
}

// Test encoding when the enum value has no custom string option set, verifying
// it falls back to the default enum value name ("ARMOR_GORGET").
TEST(JsonTest, EncodeEnumMissingCustomStringOption) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GORGET);
  EXPECT_EQ(R"({"armor":"ARMOR_GORGET"})", JsonEncodeKnight(knight, 0));
}

// Test encoding when a standard custom string option is defined ("gr8 helm").
TEST(JsonTest, EncodeEnumWithCustomString) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GREAT_HELM);
  EXPECT_EQ(R"({"armor":"gr8 helm"})", JsonEncodeKnight(knight, 0));
}

// Test encoding when the custom string option contains double quotes requiring
// escaping ("a\"b").
TEST(JsonTest, EncodeEnumCustomStringWithEscapedQuotes) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GAUNTLET);
  EXPECT_EQ("{\"armor\":\"a\\\"b\"}", JsonEncodeKnight(knight, 0));
}

// Test encoding when the custom string option is enclosed within double quotes
// ("\"plate\"").
TEST(JsonTest, EncodeEnumCustomStringEnclosedInQuotes) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_PLATE);
  EXPECT_EQ("{\"armor\":\"\\\"plate\\\"\"}", JsonEncodeKnight(knight, 0));
}

// Test encoding when the custom string option is an empty string ("").
TEST(JsonTest, EncodeEnumEmptyCustomString) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_COIF);
  EXPECT_EQ(R"({"armor":""})", JsonEncodeKnight(knight, 0));
}

// Test encoding when the custom string option contains control characters (tab
// and newline) requiring escaping ("p\taul\ndron").
TEST(JsonTest, EncodeEnumCustomStringWithEscapedControlChars) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_PAULDRON);
  EXPECT_EQ(R"({"armor":"p\taul\ndron"})", JsonEncodeKnight(knight, 0));
}

// Test encoding an enum value that has aliased values sharing a custom string
// option ("sabaton").
TEST(JsonTest, EncodeEnumAliasedCustomString) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_SABATON);
  EXPECT_EQ(R"({"armor":"sabaton"})", JsonEncodeKnight(knight, 0));
}

// Test encoding when the custom string option consists of numeric digits ("8").
TEST(JsonTest, EncodeEnumNumericCustomString) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_HACHI_MAI_DO);
  EXPECT_EQ(R"({"armor":"8"})", JsonEncodeKnight(knight, 0));
}

// Test encoding an enum value whose custom string option is identical to its
// original enum value name.
TEST(JsonTest, EncodeEnumCustomStringSameAsName) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GREAVES);
  EXPECT_EQ(R"({"armor":"ARMOR_GREAVES"})", JsonEncodeKnight(knight, 0));
}

// Test encoding when upb_JsonEncode_FormatEnumsAsIntegers is set, verifying
// integer format option overrides the custom string option.
TEST(JsonTest, EncodeEnumWithIntegerFormatOverride) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GREAT_HELM);
  EXPECT_EQ(R"({"armor":1})",
            JsonEncodeKnight(knight, upb_JsonEncode_FormatEnumsAsIntegers));
}
