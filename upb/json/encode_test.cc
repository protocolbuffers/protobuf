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

TEST(JsonTest, EncodeCustomEnumName) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());

  // ARMOR_GORGET has no custom name, should use default "ARMOR_GORGET"
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GORGET);
  EXPECT_EQ(R"({"armor":"ARMOR_GORGET"})", JsonEncodeKnight(knight, 0));

  // ARMOR_GREAT_HELM has custom name "gr8 helm"
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GREAT_HELM);
  EXPECT_EQ(R"({"armor":"gr8 helm"})", JsonEncodeKnight(knight, 0));

  // ARMOR_GAUNTLET has custom name "a\"b" (quote should be escaped)
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GAUNTLET);
  EXPECT_EQ("{\"armor\":\"a\\\"b\"}", JsonEncodeKnight(knight, 0));

  // ARMOR_COIF has an empty custom name ("")
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_COIF);
  EXPECT_EQ(R"({"armor":""})", JsonEncodeKnight(knight, 0));

  // ARMOR_PAULDRON has a tab and a newline
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_PAULDRON);
  EXPECT_EQ(R"({"armor":"p\taul\ndron"})", JsonEncodeKnight(knight, 0));

  // Int overrides always win.
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GREAT_HELM);
  EXPECT_EQ(R"({"armor":1})",
            JsonEncodeKnight(knight, upb_JsonEncode_FormatEnumsAsIntegers));
}
