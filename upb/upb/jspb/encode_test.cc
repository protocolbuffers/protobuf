// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/upb/jspb/encode.h"

#include <cstddef>
#include <string>

#include <gtest/gtest.h>
#include "upb/upb/base/status.hpp"
#include "upb/upb/jspb/test.upb.h"
#include "upb/upb/jspb/test.upb_minitable.h"
#include "upb/upb/mem/arena.h"
#include "upb/upb/mem/arena.hpp"
#include "upb/upb/mini_table/message.h"

static std::string JspbEncode(const upb_test_Box* msg) {
  upb::Arena a;
  upb::Status status;
  const upb_MiniTable* m = &upb_test_Box_msg_init;

  size_t jspb_size =
      upb_JspbEncode(msg, m, nullptr, 0, nullptr, 0, status.ptr());
  char* jspb_buf = (char*)upb_Arena_Malloc(a.ptr(), jspb_size + 1);

  size_t size =
      upb_JspbEncode(msg, m, nullptr, 0, jspb_buf, jspb_size + 1, status.ptr());
  EXPECT_EQ(size, jspb_size);
  return std::string(jspb_buf, jspb_size);
}

// Encode a single optional enum.
TEST(JspbTest, EncodeEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_set_tag(foo, upb_test_Z_BAR);

  EXPECT_EQ(R"([{"4":1}])", JspbEncode(foo));
}

// Encode a single optional negative enum.
TEST(JspbTest, EncodeNegativeEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_set_tag(foo, upb_test_Z_BAZ);
  EXPECT_EQ(R"([{"4":-2}])", JspbEncode(foo));
}

// Encode a single repeated enum.
TEST(JspbTest, EncodeRepeatedEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_add_tags(foo, upb_test_Z_BAT, a.ptr());

  EXPECT_EQ(R"([{"6":[13]}])", JspbEncode(foo));
}

TEST(JspbTest, EncodeMap) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_map_set(foo, 250, 1, a.ptr());
  upb_test_Box_map_set(foo, -350, -2, a.ptr());
  EXPECT_EQ(R"([{"9":[[-350,-2],[250,1]]}])", JspbEncode(foo));
}