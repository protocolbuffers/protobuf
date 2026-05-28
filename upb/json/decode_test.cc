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
  const std::string json;
  float f;
};

static const std::vector<FloatTest> FloatTestsPass = {
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

static const std::vector<FloatTest> FloatTestsFail = {
    {R"({"f": 1z})", 0},
    {R"({"f": 3.4028236e+38})", 0},
    {R"({"f": -3.4028236e+38})", 0},
};

// Decode some floats.
TEST(JsonTest, DecodeFloats) {
  upb::Arena a;

  for (const auto& test : FloatTestsPass) {
    upb_test_Box* box = JsonDecode(test.json.c_str(), a.ptr());
    EXPECT_NE(box, nullptr);
    float f = upb_test_Box_f(box);
    EXPECT_EQ(f, test.f);
  }

  for (const auto& test : FloatTestsFail) {
    upb_test_Box* box = JsonDecode(test.json.c_str(), a.ptr());
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

TEST(JsonTest, DecodeCustomEnumName) {
  upb::Arena a;

  // ARMOR_GORGET has no custom name, should use default "ARMOR_GORGET"
  {
    std::string json = R"({"armor":"ARMOR_GORGET"})";
    json_enumval_custom_string_Knight* knight =
        JsonDecodeKnight(json.c_str(), a.ptr());
    EXPECT_NE(knight, nullptr);
    EXPECT_EQ(json_enumval_custom_string_ARMOR_GORGET,
              json_enumval_custom_string_Knight_armor(knight));
  }

  // ARMOR_GREAT_HELM has custom name "gr8 helm"
  {
    std::string json = R"({"armor":"gr8 helm"})";
    json_enumval_custom_string_Knight* knight =
        JsonDecodeKnight(json.c_str(), a.ptr());
    EXPECT_NE(knight, nullptr);
    EXPECT_EQ(json_enumval_custom_string_ARMOR_GREAT_HELM,
              json_enumval_custom_string_Knight_armor(knight));
  }

  // ARMOR_GAUNTLET has custom name "a\"b" (quote should be escaped)
  {
    std::string json = "{\"armor\":\"a\\\"b\"}";
    json_enumval_custom_string_Knight* knight =
        JsonDecodeKnight(json.c_str(), a.ptr());
    EXPECT_NE(knight, nullptr);
    EXPECT_EQ(json_enumval_custom_string_ARMOR_GAUNTLET,
              json_enumval_custom_string_Knight_armor(knight));
  }

  // ARMOR_COIF has an empty custom name ("")
  {
    std::string json = R"({"armor":""})";
    json_enumval_custom_string_Knight* knight =
        JsonDecodeKnight(json.c_str(), a.ptr());
    EXPECT_NE(knight, nullptr);
    EXPECT_EQ(json_enumval_custom_string_ARMOR_COIF,
              json_enumval_custom_string_Knight_armor(knight));
  }

  // ARMOR_PAULDRON has a tab and a newline
  {
    std::string json = R"({"armor":"p\taul\ndron"})";
    json_enumval_custom_string_Knight* knight =
        JsonDecodeKnight(json.c_str(), a.ptr());
    EXPECT_NE(knight, nullptr);
    EXPECT_EQ(json_enumval_custom_string_ARMOR_PAULDRON,
              json_enumval_custom_string_Knight_armor(knight));
  }

  // Ints are always valid enumvals
  {
    std::string json = R"({"armor":1})";
    json_enumval_custom_string_Knight* knight =
        JsonDecodeKnight(json.c_str(), a.ptr());
    EXPECT_NE(knight, nullptr);
    EXPECT_EQ(json_enumval_custom_string_ARMOR_GREAT_HELM,
              json_enumval_custom_string_Knight_armor(knight));
  }
}
