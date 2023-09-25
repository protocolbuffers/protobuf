// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/upb/jspb/decode.h"

#include <stdint.h>

#include <cstddef>
#include <string>

#include <gtest/gtest.h>
#include "upb/upb/base/status.hpp"
#include "upb/upb/base/string_view.h"
#include "upb/upb/jspb/test.upb.h"
#include "upb/upb/jspb/test.upb_minitable.h"
#include "upb/upb/mem/arena.h"
#include "upb/upb/mem/arena.hpp"
#include "upb/upb/mini_table/message.h"

static upb_test_Box* JspbDecode(const char* json, upb_Arena* a) {
  upb::Status status;
  const upb_MiniTable* t = &upb_test_Box_msg_init;

  upb_test_Box* box = upb_test_Box_new(a);
  int options = 0;
  bool ok = upb_JspbDecode(json, strlen(json), box, t, nullptr, options, a,
                           status.ptr());
  return ok ? box : nullptr;
}

struct FloatTest {
  const std::string json;
  float x;
};

// Decode some floats.
TEST(JspbTest, DecodeFloats) {
  upb::Arena a;
  const FloatTest float_tests[] = {
      {R"(  [0])", 0},
      {R"([1]  )", 1},
      {R"([1.000000])", 1},
      {R"([1.5e1])", 15},
      {R"([15e-1])", 1.5},
      {R"([-3.5])", -3.5},
      {R"([3.402823e38])", 3.402823e38},
      {R"([-3.402823e38])", -3.402823e38},
      {R"([340282346638528859811704183484516925440.0])",
       340282346638528859811704183484516925440.0},
      {R"([-340282346638528859811704183484516925440.0])",
       -340282346638528859811704183484516925440.0},
  };

  for (const auto& test : float_tests) {
    upb_test_Box* box = JspbDecode(test.json.c_str(), a.ptr());
    EXPECT_NE(box, nullptr);
    EXPECT_EQ(upb_test_Box_flt(box), test.x);
  }
}

struct TestCase {
  const std::string json;
  float flt;
  std::string str;
  int tag;
  bool b;
};

static std::string view_to_str(upb_StringView view) {
  return std::string(view.data, view.size);
}

TEST(JspbTest, RunTestCases) {
  upb::Arena a;

  const TestCase test_cases[] = {
      {R"([]   )", 0, "", 0},
      {R"([null, null, null, null, null])", 0, "", 0, false},
      {R"([null, null, "hello"])", 0, "hello", 0, false},
      {R"([null, null, null, 13])", 0, "", 13, false},
      {R"([null, null, null, 14])", 0, "", 0,
       false},  // Invalid closed enum val.
      {R"([null, null, null, null, 0])", 0, "", 0, false},
      {R"([null, null, null, null, 1])", 0, "", 0, true},
      {R"([null, null, null, null, false])", 0, "", 0, false},
      {R"([null, null, null, null, true])", 0, "", 0, true},
      {R"([{}])", 0, "", 0, false},
      {R"([{"1": 17}])", 17, "", 0, false},
  };

  for (const auto& test : test_cases) {
    upb_test_Box* box = JspbDecode(test.json.c_str(), a.ptr());
    EXPECT_NE(box, nullptr);
    EXPECT_EQ(upb_test_Box_flt(box), test.flt);
    EXPECT_EQ(view_to_str(upb_test_Box_str(box)), test.str);
    EXPECT_EQ(upb_test_Box_tag(box), test.tag);
    EXPECT_EQ(upb_test_Box_b(box), test.b);
  }
}

TEST(JspbTest, ShouldNotParseTest) {
  upb::Arena a;

  static const char* shouldNotParse[] = {
      "",
      "()",
      "1",
      "null",
      "{}",
      "[]]",
      "[[]",
      ",,,,,,#######,,,\021\021,,,,l,,",
      "[{}, null]",  // Sparse representation must be last.
      "[1, {}, null]",
      "[{}, {}]",
      "[1z]",             // Malformed number.
      "[3.4028236e+38]",  // Out of bounds literal values.
      "[-3.4028236e+38]",
  };

  for (const auto& test : shouldNotParse) {
    upb_test_Box* box = JspbDecode(test, a.ptr());
    EXPECT_EQ(box, nullptr);
  }
}

TEST(JspbTest, HasserFalse) {
  upb::Arena a;
  upb_test_Box* box = JspbDecode("[null]", a.ptr());
  EXPECT_NE(box, nullptr);
  EXPECT_FALSE(upb_test_Box_has_flt(box));
  EXPECT_FALSE(upb_test_Box_has_str(box));
}

TEST(JspbTest, RepeatedTest) {
  upb::Arena a;
  upb_test_Box* box = JspbDecode(R"([{"6": [13, 0, 1]}])", a.ptr());
  EXPECT_NE(box, nullptr);

  size_t size = 0;
  const int32_t* ptr = upb_test_Box_tags(box, &size);
  EXPECT_EQ(size, 3);
  EXPECT_EQ(ptr[0], 13);
  EXPECT_EQ(ptr[1], 0);
  EXPECT_EQ(ptr[2], 1);
}

TEST(JspbTest, SubMessageDenseTest) {
  upb::Arena a;
  upb_test_Box* box =
      JspbDecode(R"([null,null,null,null,null,null,[17]])", a.ptr());
  EXPECT_NE(box, nullptr);

  const upb_test_OtherMessage* msg = upb_test_Box_msg(box);
  EXPECT_NE(msg, nullptr);
  EXPECT_EQ(upb_test_OtherMessage_i(msg), 17);
}

TEST(JspbTest, SubMessageSparseTest) {
  upb::Arena a;
  upb_test_Box* box = JspbDecode(R"([{"7":[17]}])", a.ptr());
  EXPECT_NE(box, nullptr);

  const upb_test_OtherMessage* msg = upb_test_Box_msg(box);
  EXPECT_NE(msg, nullptr);
  EXPECT_EQ(upb_test_OtherMessage_i(msg), 17);
}

TEST(JspbTest, MapFieldTest) {
  upb::Arena a;
  upb_test_Box* box = JspbDecode(R"([{"9": [[50, 2], [150, 4]]}])", a.ptr());
  EXPECT_NE(box, nullptr);
  EXPECT_EQ(upb_test_Box_map_size(box), 2);

  int32_t first;
  EXPECT_TRUE(upb_test_Box_map_get(box, 50, &first));
  EXPECT_EQ(first, 2);

  int32_t second;
  EXPECT_TRUE(upb_test_Box_map_get(box, 150, &second));
  EXPECT_EQ(second, 4);
}
