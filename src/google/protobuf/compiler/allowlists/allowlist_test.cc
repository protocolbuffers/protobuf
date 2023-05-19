// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "google/protobuf/compiler/allowlists/allowlist.h"

#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace internal {
namespace {

TEST(AllowlistTest, Smoke) {
  static const auto kList = MakeAllowlist({
      "bar",
      "baz",
      "foo",
  });

  EXPECT_TRUE(kList.Allows("bar"));
  EXPECT_TRUE(kList.Allows("baz"));
  EXPECT_TRUE(kList.Allows("foo"));
  EXPECT_FALSE(kList.Allows("barf"));
  EXPECT_FALSE(kList.Allows("baq"));
  EXPECT_FALSE(kList.Allows("bak"));
  EXPECT_FALSE(kList.Allows("foob"));
}

TEST(AllowlistTest, Empty) {
  static const auto kList = MakeAllowlist({});

  EXPECT_FALSE(kList.Allows("bar"));
  EXPECT_FALSE(kList.Allows("baz"));
  EXPECT_FALSE(kList.Allows("foo"));
  EXPECT_FALSE(kList.Allows("barf"));
  EXPECT_FALSE(kList.Allows("baq"));
  EXPECT_FALSE(kList.Allows("bak"));
  EXPECT_FALSE(kList.Allows("foob"));
}

TEST(AllowlistTest, Prefix) {
  static const auto kList = MakeAllowlist(
      {
          "bar",
          "baz",
          "foo",
      },
      AllowlistFlags::kMatchPrefix);

  EXPECT_TRUE(kList.Allows("bar"));
  EXPECT_TRUE(kList.Allows("baz"));
  EXPECT_TRUE(kList.Allows("foo"));
  EXPECT_TRUE(kList.Allows("barf"));
  EXPECT_TRUE(kList.Allows("foon"));
  EXPECT_TRUE(kList.Allows("bazaar"));
  EXPECT_FALSE(kList.Allows("baq"));
  EXPECT_FALSE(kList.Allows("bbr"));
  EXPECT_FALSE(kList.Allows("fbar"));
  EXPECT_FALSE(kList.Allows("ba"));
  EXPECT_FALSE(kList.Allows("fon"));
  EXPECT_FALSE(kList.Allows("fop"));
}

TEST(AllowlistTest, Oss) {
  static const auto kList = MakeAllowlist(
      {
          "bar",
          "baz",
          "foo",
      },
      AllowlistFlags::kAllowAllInOss);

  EXPECT_TRUE(kList.Allows("bar"));
  EXPECT_TRUE(kList.Allows("baz"));
  EXPECT_TRUE(kList.Allows("foo"));
  EXPECT_TRUE(kList.Allows("barf"));
  EXPECT_TRUE(kList.Allows("baq"));
  EXPECT_TRUE(kList.Allows("bak"));
  EXPECT_TRUE(kList.Allows("foob"));
}

#if GTEST_HAS_DEATH_TEST && !defined(NDEBUG)
TEST(AllowlistTest, Unsorted) {
  EXPECT_DEATH(MakeAllowlist({"foo", "bar"}), "Allowlist must be sorted!");
}
#endif

}  // namespace
}  // namespace internal
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
