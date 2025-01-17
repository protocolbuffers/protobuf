#include "upb/base/string_view.h"

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

}  // namespace
