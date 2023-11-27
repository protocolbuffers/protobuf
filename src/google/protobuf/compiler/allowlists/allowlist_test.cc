// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
