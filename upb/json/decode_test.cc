// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/json/decode.h"

#include <string>
#include <vector>

#include "google/protobuf/struct.upb.h"
#include <gtest/gtest.h>
#include "upb/base/status.hpp"
#include "upb/base/upcast.h"
#include "upb/json/test.upb.h"
#include "upb/json/test.upbdefs.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"

static upb_test_Box* JsonDecode(const char* json, upb_Arena* a) {
  upb::Status status;
  upb::DefPool defpool;
  upb::MessageDefPtr m(upb_test_Box_getmsgdef(defpool.ptr()));
  EXPECT_TRUE(m.ptr() != nullptr);

  upb_test_Box* box = upb_test_Box_new(a);
  int options = 0;
  bool ok = upb_JsonDecode(json, strlen(json), UPB_UPCAST(box), m.ptr(),
                           defpool.ptr(), options, a, status.ptr());
  return ok ? box : nullptr;
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
