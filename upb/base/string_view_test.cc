// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/base/string_view.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>

namespace {

TEST(upb_StringView, Compare_Eq) {
  std::string s1("12345");
  std::string s2("12345");

  upb_StringView h1 = upb_StringView_FromDataAndSize(s1.data(), s1.size());
  upb_StringView h2 = upb_StringView_FromDataAndSize(s2.data(), s2.size());

  ASSERT_EQ(upb_StringView_Compare(h1, h2), 0);
}

TEST(upb_StringView, Compare_Eq_Shorter) {
  std::string s1("1234");  // s1 is shorter.
  std::string s2("12345");

  upb_StringView h1 = upb_StringView_FromDataAndSize(s1.data(), s1.size());
  upb_StringView h2 = upb_StringView_FromDataAndSize(s2.data(), s2.size());

  ASSERT_LT(upb_StringView_Compare(h1, h2), 0);
}

TEST(upb_StringView, Compare_Eq_Longer) {
  std::string s1("123456");  // s1 is longer.
  std::string s2("12345");

  upb_StringView h1 = upb_StringView_FromDataAndSize(s1.data(), s1.size());
  upb_StringView h2 = upb_StringView_FromDataAndSize(s2.data(), s2.size());

  ASSERT_GT(upb_StringView_Compare(h1, h2), 0);
}

TEST(upb_StringView, Compare_Less) {
  std::string s1("12245");  // 2 < 3
  std::string s2("12345");

  upb_StringView h1 = upb_StringView_FromDataAndSize(s1.data(), s1.size());
  upb_StringView h2 = upb_StringView_FromDataAndSize(s2.data(), s2.size());

  ASSERT_LT(upb_StringView_Compare(h1, h2), 0);
}

TEST(upb_StringView, Compare_Greater) {
  std::string s1("12445");  // 4 > 3
  std::string s2("12345");

  upb_StringView h1 = upb_StringView_FromDataAndSize(s1.data(), s1.size());
  upb_StringView h2 = upb_StringView_FromDataAndSize(s2.data(), s2.size());

  ASSERT_GT(upb_StringView_Compare(h1, h2), 0);
}

TEST(upb_StringView, Compare_Greater_Shorter) {
  std::string s1("1244");  // s1 is shorter but 4 > 3.
  std::string s2("12345");

  upb_StringView h1 = upb_StringView_FromDataAndSize(s1.data(), s1.size());
  upb_StringView h2 = upb_StringView_FromDataAndSize(s2.data(), s2.size());

  ASSERT_GT(upb_StringView_Compare(h1, h2), 0);
}

int FormatPrecision(int precision, const char* data) {
  (void)data;
  return precision;
}

TEST(upb_StringView, FormatArgsKeepSmallSize) {
  std::string s("12345");
  upb_StringView h = upb_StringView_FromDataAndSize(s.data(), s.size());

  EXPECT_EQ(FormatPrecision(UPB_STRINGVIEW_ARGS(h)), 5);
}

// The precision of "%.*s" is an int.  A larger size must not wrap around to a
// negative precision, because printf() then reads up to the first NUL byte,
// past the end of the view.
TEST(upb_StringView, FormatArgsClampLargeSize) {
  std::string s("12345");
  upb_StringView h =
      upb_StringView_FromDataAndSize(s.data(), (size_t)INT_MAX + 1);
  EXPECT_EQ(FormatPrecision(UPB_STRINGVIEW_ARGS(h)), INT_MAX);

  h.size = SIZE_MAX;
  EXPECT_EQ(FormatPrecision(UPB_STRINGVIEW_ARGS(h)), INT_MAX);
}

}  // namespace
