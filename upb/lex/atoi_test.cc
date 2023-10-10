// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/lex/atoi.h"

#include <string.h>

#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"

TEST(AtoiTest, Uint64) {
  uint64_t val;

  const char* s = "1234z";
  EXPECT_EQ(s + 1, upb_BufToUint64(s, s + 1, &val));
  EXPECT_EQ(val, 1);
  EXPECT_EQ(s + 4, upb_BufToUint64(s, s + 4, &val));
  EXPECT_EQ(val, 1234);
  EXPECT_EQ(s + 4, upb_BufToUint64(s, s + 5, &val));
  EXPECT_EQ(val, 1234);

  const char* t = "42.6";
  EXPECT_EQ(t + 1, upb_BufToUint64(t, t + 1, &val));
  EXPECT_EQ(val, 4);
  EXPECT_EQ(t + 2, upb_BufToUint64(t, t + 2, &val));
  EXPECT_EQ(val, 42);
  EXPECT_EQ(t + 2, upb_BufToUint64(t, t + 3, &val));
  EXPECT_EQ(val, 42);

  // Integer overflow
  const char* u = "1000000000000000000000000000000";
  EXPECT_EQ(nullptr, upb_BufToUint64(u, u + strlen(u), &val));

  // Not an integer
  const char* v = "foobar";
  EXPECT_EQ(v, upb_BufToUint64(v, v + strlen(v), &val));

  const uint64_t values[] = {
      std::numeric_limits<uint64_t>::max(),
      std::numeric_limits<uint64_t>::min(),
  };
  for (size_t i = 0; i < ABSL_ARRAYSIZE(values); i++) {
    std::string v = absl::StrCat(values[i]);
    const char* ptr = v.c_str();
    const char* end = ptr + strlen(ptr);
    EXPECT_EQ(end, upb_BufToUint64(ptr, end, &val));
    EXPECT_EQ(val, values[i]);
  }
}

TEST(AtoiTest, Int64) {
  int64_t val;
  bool neg;

  const char* s = "1234z";
  EXPECT_EQ(s + 1, upb_BufToInt64(s, s + 1, &val, &neg));
  EXPECT_EQ(val, 1);
  EXPECT_EQ(neg, false);
  EXPECT_EQ(s + 4, upb_BufToInt64(s, s + 4, &val, nullptr));
  EXPECT_EQ(val, 1234);
  EXPECT_EQ(s + 4, upb_BufToInt64(s, s + 5, &val, nullptr));
  EXPECT_EQ(val, 1234);

  const char* t = "-42.6";
  EXPECT_EQ(t + 2, upb_BufToInt64(t, t + 2, &val, &neg));
  EXPECT_EQ(val, -4);
  EXPECT_EQ(neg, true);
  EXPECT_EQ(t + 3, upb_BufToInt64(t, t + 3, &val, nullptr));
  EXPECT_EQ(val, -42);
  EXPECT_EQ(neg, true);
  EXPECT_EQ(t + 3, upb_BufToInt64(t, t + 5, &val, nullptr));
  EXPECT_EQ(val, -42);

  const int64_t values[] = {
      std::numeric_limits<int32_t>::max(),
      std::numeric_limits<int32_t>::min(),
      std::numeric_limits<int64_t>::max(),
      std::numeric_limits<int64_t>::min(),
  };
  for (size_t i = 0; i < ABSL_ARRAYSIZE(values); i++) {
    std::string v = absl::StrCat(values[i]);
    const char* ptr = v.c_str();
    const char* end = ptr + strlen(ptr);
    EXPECT_EQ(end, upb_BufToInt64(ptr, end, &val, nullptr));
    EXPECT_EQ(val, values[i]);
  }
}
