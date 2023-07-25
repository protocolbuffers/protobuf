/*
 * Copyright (c) 2009-2022, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/lex/atoi.h"

#include "gtest/gtest.h"
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
  EXPECT_EQ(NULL, upb_BufToUint64(u, u + strlen(u), &val));

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
  EXPECT_EQ(s + 4, upb_BufToInt64(s, s + 4, &val, NULL));
  EXPECT_EQ(val, 1234);
  EXPECT_EQ(s + 4, upb_BufToInt64(s, s + 5, &val, NULL));
  EXPECT_EQ(val, 1234);

  const char* t = "-42.6";
  EXPECT_EQ(t + 2, upb_BufToInt64(t, t + 2, &val, &neg));
  EXPECT_EQ(val, -4);
  EXPECT_EQ(neg, true);
  EXPECT_EQ(t + 3, upb_BufToInt64(t, t + 3, &val, NULL));
  EXPECT_EQ(val, -42);
  EXPECT_EQ(neg, true);
  EXPECT_EQ(t + 3, upb_BufToInt64(t, t + 5, &val, NULL));
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
    EXPECT_EQ(end, upb_BufToInt64(ptr, end, &val, NULL));
    EXPECT_EQ(val, values[i]);
  }
}
