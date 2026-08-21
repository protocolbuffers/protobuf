// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/json/decode.h"

#include <cstring>
#include <string>
#include <vector>

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

static bool JsonDecodeGeneric(const char* json, upb_Message* msg,
                              const upb_MessageDef* (*getmsgdef)(upb_DefPool*),
                              upb_Arena* arena) {
  upb::Status status;
  upb::DefPool defpool;
  const upb_MessageDef* m = getmsgdef(defpool.ptr());
  EXPECT_TRUE(m != nullptr);

  int options = 0;
  return upb_JsonDecode(json, strlen(json), msg, m, defpool.ptr(), options,
                        arena, status.ptr());
}

static upb_test_Box* JsonDecode(const char* json, upb_Arena* a) {
  upb_test_Box* box = upb_test_Box_new(a);
  if (JsonDecodeGeneric(json, UPB_UPCAST(box), upb_test_Box_getmsgdef, a)) {
    return box;
  }
  return nullptr;
}

static json_enumval_custom_string_Knight* JsonDecodeKnight(const char* json,
                                                           upb_Arena* a) {
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a);
  if (JsonDecodeGeneric(json, UPB_UPCAST(knight),
                        json_enumval_custom_string_Knight_getmsgdef, a)) {
    return knight;
  }
  return nullptr;
}

struct FloatTest {
  const char* json;
  float f;
};

static const FloatTest kFloatTestsPass[] = {
    {R"({"f": 0})", 0},
    {R"({"f": 1})", 1},
    {R"({"f": 1.000000})", 1},
    {R"({"f": 1.5e1})", 15},
    {R"({"f": 15e-1})", 1.5},
    {R"({"f": -3.5})", -3.5},
    {R"({"f": 3.402823e38})", 3.402823e38},
    {R"({"f": -3.402823e38})", -3.402823e38},
    {R"({"f": 340282346638528859811704183484516925440.0})",
     340282346638528859811704183484516925440.0},
    {R"({"f": -340282346638528859811704183484516925440.0})",
     -340282346638528859811704183484516925440.0},
};

static const FloatTest kFloatTestsFail[] = {
    {R"({"f": 1z})", 0},
    {R"({"f": 3.4028236e+38})", 0},
    {R"({"f": -3.4028236e+38})", 0},
};

// Decode some floats.
TEST(JsonTest, DecodeFloats) {
  upb::Arena a;

  for (const auto& test : kFloatTestsPass) {
    upb_test_Box* box = JsonDecode(test.json, a.ptr());
    EXPECT_NE(box, nullptr);
    float f = upb_test_Box_f(box);
    EXPECT_EQ(f, test.f);
  }

  for (const auto& test : kFloatTestsFail) {
    upb_test_Box* box = JsonDecode(test.json, a.ptr());
    EXPECT_EQ(box, nullptr);
  }
}

TEST(JsonTest, DecodeConflictJsonName) {
  upb::Arena a;
  std::string json_string = R"({"value": 2})";
  upb_test_Box* box = JsonDecode(json_string.c_str(), a.ptr());
  EXPECT_EQ(2, upb_test_Box_new_value(box));
  EXPECT_EQ(0, upb_test_Box_value(box));
}

TEST(JsonTest, RejectsBadTrailingCharacters) {
  upb::Arena a;
  std::string json_string = R"({}abc)";
  upb_test_Box* box = JsonDecode(json_string.c_str(), a.ptr());
  EXPECT_EQ(box, nullptr);
}

TEST(JsonTest, AcceptsTrailingWhitespace) {
  upb::Arena a;
  std::string json_string = "{} \n \r\n \t\t";
  upb_test_Box* box = JsonDecode(json_string.c_str(), a.ptr());
  EXPECT_NE(box, nullptr);
}

// Regression: jsondec_base64_tablelookup() previously indexed a 256-byte
// signed-char table with table[(unsigned)ch], which let C integer
// promotion sign-extend bytes >= 0x80 into negative ints, producing
// OOB reads ~4 GiB past the table. Decoding a bytes-typed field whose
// JSON string contains high-bit-set bytes (e.g. the UTF-8 encoding of
// \u0080 = 0xC2 0x80) must fail gracefully without OOB-reading.
TEST(JsonTest, RejectsBase64WithHighBitBytes) {
  upb::Arena a;
  std::string json_string = R"({"data":"\u0080\u0080\u0080\u0080"})";
  upb_test_Box* box = JsonDecode(json_string.c_str(), a.ptr());
  EXPECT_EQ(box, nullptr);
}

// Test decoding when the enum value has no custom string option set, falling
// back to the default enum value name.
TEST(JsonTest, DecodeEnumMissingCustomStringOption) {
  upb::Arena a;
  std::string json = R"({"armor":"ARMOR_GORGET"})";
  json_enumval_custom_string_Knight* knight =
      JsonDecodeKnight(json.c_str(), a.ptr());
  EXPECT_NE(knight, nullptr);
  EXPECT_EQ(json_enumval_custom_string_ARMOR_GORGET,
            json_enumval_custom_string_Knight_armor(knight));
}

// Test decoding when a standard custom string option is provided.
TEST(JsonTest, DecodeEnumWithCustomString) {
  upb::Arena a;
  std::string json = R"({"armor":"gr8 helm"})";
  json_enumval_custom_string_Knight* knight =
      JsonDecodeKnight(json.c_str(), a.ptr());
  EXPECT_NE(knight, nullptr);
  EXPECT_EQ(json_enumval_custom_string_ARMOR_GREAT_HELM,
            json_enumval_custom_string_Knight_armor(knight));
}

// Test decoding when the custom string option contains escaped double quotes.
TEST(JsonTest, DecodeEnumCustomStringWithEscapedQuotes) {
  upb::Arena a;
  std::string json = "{\"armor\":\"a\\\"b\"}";
  json_enumval_custom_string_Knight* knight =
      JsonDecodeKnight(json.c_str(), a.ptr());
  EXPECT_NE(knight, nullptr);
  EXPECT_EQ(json_enumval_custom_string_ARMOR_GAUNTLET,
            json_enumval_custom_string_Knight_armor(knight));
}

// Test decoding when the custom string option is an empty string ("").
TEST(JsonTest, DecodeEnumEmptyCustomString) {
  upb::Arena a;
  std::string json = R"({"armor":""})";
  json_enumval_custom_string_Knight* knight =
      JsonDecodeKnight(json.c_str(), a.ptr());
  EXPECT_NE(knight, nullptr);
  EXPECT_EQ(json_enumval_custom_string_ARMOR_COIF,
            json_enumval_custom_string_Knight_armor(knight));
}

// Test decoding when the custom string option contains escaped control
// characters (tab and newline).
TEST(JsonTest, DecodeEnumCustomStringWithEscapedControlChars) {
  upb::Arena a;
  std::string json = R"({"armor":"p\taul\ndron"})";
  json_enumval_custom_string_Knight* knight =
      JsonDecodeKnight(json.c_str(), a.ptr());
  EXPECT_NE(knight, nullptr);
  EXPECT_EQ(json_enumval_custom_string_ARMOR_PAULDRON,
            json_enumval_custom_string_Knight_armor(knight));
}

// Test decoding an enum value passed as a raw integer literal.
TEST(JsonTest, DecodeEnumFromIntegerLiteral) {
  upb::Arena a;
  std::string json = R"({"armor":1})";
  json_enumval_custom_string_Knight* knight =
      JsonDecodeKnight(json.c_str(), a.ptr());
  EXPECT_NE(knight, nullptr);
  EXPECT_EQ(json_enumval_custom_string_ARMOR_GREAT_HELM,
            json_enumval_custom_string_Knight_armor(knight));
}

// Test decoding by raw enum name even when a custom string option is defined.
TEST(JsonTest, DecodeEnumRawNameWhenCustomStringDefined) {
  upb::Arena a;
  std::string json = R"({"armor":"ARMOR_GREAT_HELM"})";
  json_enumval_custom_string_Knight* knight =
      JsonDecodeKnight(json.c_str(), a.ptr());
  EXPECT_NE(knight, nullptr);
  EXPECT_EQ(json_enumval_custom_string_ARMOR_GREAT_HELM,
            json_enumval_custom_string_Knight_armor(knight));
}

// Test decoding when the custom string option is enclosed within double quotes.
TEST(JsonTest, DecodeEnumCustomStringEnclosedInQuotes) {
  upb::Arena a;
  std::string json = "{\"armor\":\"\\\"plate\\\"\"}";
  json_enumval_custom_string_Knight* knight =
      JsonDecodeKnight(json.c_str(), a.ptr());
  EXPECT_NE(knight, nullptr);
  EXPECT_EQ(json_enumval_custom_string_ARMOR_PLATE,
            json_enumval_custom_string_Knight_armor(knight));
}

// Test decoding a custom string that maps to aliased enum values.
TEST(JsonTest, DecodeEnumAliasedCustomString) {
  upb::Arena a;
  std::string json = R"({"armor":"sabaton"})";
  json_enumval_custom_string_Knight* knight =
      JsonDecodeKnight(json.c_str(), a.ptr());
  EXPECT_NE(knight, nullptr);
  EXPECT_EQ(json_enumval_custom_string_ARMOR_SABATON,
            json_enumval_custom_string_Knight_armor(knight));
  EXPECT_EQ(json_enumval_custom_string_ARMOR_SOLLERET,
            json_enumval_custom_string_Knight_armor(knight));
}

// Test decoding when the custom string option consists of numeric digits ("8").
TEST(JsonTest, DecodeEnumNumericCustomString) {
  upb::Arena a;
  std::string json = R"({"armor":"8"})";
  json_enumval_custom_string_Knight* knight =
      JsonDecodeKnight(json.c_str(), a.ptr());
  EXPECT_NE(knight, nullptr);
  EXPECT_EQ(json_enumval_custom_string_ARMOR_HACHI_MAI_DO,
            json_enumval_custom_string_Knight_armor(knight));
}

// Test decoding an enum value whose custom string option is identical to its
// original enum value name. Also verify that normal name lookup by original
// name is not marked as JSON-only.
TEST(JsonTest, DecodeEnumCustomStringSameAsName) {
  upb::Arena a;
  std::string json = R"({"armor":"ARMOR_GREAVES"})";
  json_enumval_custom_string_Knight* knight =
      JsonDecodeKnight(json.c_str(), a.ptr());
  EXPECT_NE(knight, nullptr);
  EXPECT_EQ(json_enumval_custom_string_ARMOR_GREAVES,
            json_enumval_custom_string_Knight_armor(knight));

  upb::DefPool defpool;
  const upb_MessageDef* m =
      json_enumval_custom_string_Knight_getmsgdef(defpool.ptr());
  ASSERT_NE(m, nullptr);
  const upb_FieldDef* f = upb_MessageDef_FindFieldByName(m, "armor");
  ASSERT_NE(f, nullptr);
  const upb_EnumDef* e = upb_FieldDef_EnumSubDef(f);
  ASSERT_NE(e, nullptr);
  const upb_EnumValueDef* ev = upb_EnumDef_FindValueByName(e, "ARMOR_GREAVES");
  ASSERT_NE(ev, nullptr);
  EXPECT_EQ(upb_EnumValueDef_Number(ev),
            json_enumval_custom_string_ARMOR_GREAVES);
}

// Test that decoding fails on case mismatch against a custom string.
TEST(JsonTest, DecodeEnumCustomStringCaseMismatchFails) {
  upb::Arena a;
  std::string json = "{\"armor\":\"A\\\"b\"}";
  EXPECT_EQ(JsonDecodeKnight(json.c_str(), a.ptr()), nullptr);
}

// Test that decoding fails on case mismatch against a raw enum name.
TEST(JsonTest, DecodeEnumRawNameCaseMismatchFails) {
  upb::Arena a;
  std::string json = R"({"armor":"armor_GAUNtlet"})";
  EXPECT_EQ(JsonDecodeKnight(json.c_str(), a.ptr()), nullptr);
}

// Test that decoding fails on an unknown enum value string.
TEST(JsonTest, DecodeEnumUnknownStringFails) {
  upb::Arena a;
  std::string json = R"({"armor":"UNKNOWN_1"})";
  EXPECT_EQ(JsonDecodeKnight(json.c_str(), a.ptr()), nullptr);
}

// Test that decoding fails when the JSON payload is an array.
TEST(JsonTest, DecodeEnumInvalidArrayPayloadFails) {
  upb::Arena a;
  std::string json = R"({"armor":["gr8 helm"]})";
  EXPECT_EQ(JsonDecodeKnight(json.c_str(), a.ptr()), nullptr);
}

// Test that decoding fails when the JSON payload is a boolean.
TEST(JsonTest, DecodeEnumInvalidBooleanPayloadFails) {
  upb::Arena a;
  std::string json = R"({"armor":true})";
  EXPECT_EQ(JsonDecodeKnight(json.c_str(), a.ptr()), nullptr);
}

#include "upb/mem/alloc.h"

struct FailAfterAlloc {
  upb_alloc alloc;
  int limit;
  int count;
};

static void* FailAfterAllocFunc(upb_alloc* alloc, void* ptr, size_t oldsize,
                                size_t size, size_t* actual_size) {
  FailAfterAlloc* self = reinterpret_cast<FailAfterAlloc*>(alloc);
  if (size > 0) {
    if (self->count >= self->limit) {
      return nullptr;
    }
    self->count++;
  }
  return upb_alloc_global.func(alloc, ptr, oldsize, size, actual_size);
}

static bool TryDecodeWithLimit(const char* json, int limit) {
  FailAfterAlloc allocator = {{&FailAfterAllocFunc}, limit, 0};
  upb_Arena* a = upb_Arena_Init(nullptr, 0, &allocator.alloc);
  if (!a) {
    return false;
  }
  upb::Status status;
  upb::DefPool defpool;
  upb_test_Box* box = upb_test_Box_new(a);
  bool ok = false;
  if (box) {
    upb::MessageDefPtr m(upb_test_Box_getmsgdef(defpool.ptr()));
    int options = 0;
    ok = upb_JsonDecode(json, strlen(json), UPB_UPCAST(box), m.ptr(),
                        defpool.ptr(), options, a, status.ptr());
  }
  upb_Arena_Free(a);
  return ok;
}

static void TestJsonAllocationFailure(const char* json) {
  int limit = 0;
  bool success = false;
  for (; limit < 1000; ++limit) {
    if (TryDecodeWithLimit(json, limit)) {
      success = true;
      break;
    }
  }
  EXPECT_TRUE(success) << "JSON failed to decode even with 1000 allocations: "
                       << json;

  for (int i = 0; i < limit; ++i) {
    TryDecodeWithLimit(json, i);
  }
}

TEST(JsonTest, AllocationFailureFieldMask) {
  TestJsonAllocationFailure(R"({"mask_val": "foo,bar,baz"})");
}

TEST(JsonTest, AllocationFailureListValue) {
  TestJsonAllocationFailure(R"({"val": [1, 2, 3]})");
}

TEST(JsonTest, AllocationFailureStruct) {
  TestJsonAllocationFailure(R"({"val": {"a": 1, "b": 2}})");
}

TEST(JsonTest, AllocationFailureAny) {
  TestJsonAllocationFailure(
      R"({"any_val": {"@type": "type.googleapis.com/google.protobuf.Value", "value": "foo"}})");
}
