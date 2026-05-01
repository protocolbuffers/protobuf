// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/naming_style.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status_matchers.h"

namespace google {
namespace protobuf {
namespace {

using ::absl_testing::IsOk;
using ::testing::Not;

TEST(NamingStyleTest, ContainsBadUnderscores) {
  EXPECT_TRUE(ContainsBadUnderscores("_foo"));
  EXPECT_TRUE(ContainsBadUnderscores("foo_"));
  EXPECT_TRUE(ContainsBadUnderscores("foo__bar"));
  EXPECT_TRUE(ContainsBadUnderscores("foo_1"));
  EXPECT_FALSE(ContainsBadUnderscores("foo_bar"));
  EXPECT_FALSE(ContainsBadUnderscores("foo_bar_baz"));
  EXPECT_FALSE(ContainsBadUnderscores(""));
}

TEST(NamingStyleTest, IsValidTitleCaseName) {
  EXPECT_THAT(IsValidTitleCaseName("Foo"), IsOk());
  EXPECT_THAT(IsValidTitleCaseName("FooBar"), IsOk());

  EXPECT_THAT(IsValidTitleCaseName("foo"), Not(IsOk()));
  EXPECT_THAT(IsValidTitleCaseName("Foo_Bar"), Not(IsOk()));
  EXPECT_THAT(IsValidTitleCaseName(""), Not(IsOk()));
}

TEST(NamingStyleTest, IsValidLowerSnakeCaseName) {
  EXPECT_THAT(IsValidLowerSnakeCaseName("foo"), IsOk());
  EXPECT_THAT(IsValidLowerSnakeCaseName("foo_bar"), IsOk());

  EXPECT_THAT(IsValidLowerSnakeCaseName("foo_bar_123"), Not(IsOk()));
  EXPECT_THAT(IsValidLowerSnakeCaseName("Foo"), Not(IsOk()));
  EXPECT_THAT(IsValidLowerSnakeCaseName("fooBar"), Not(IsOk()));
  EXPECT_THAT(IsValidLowerSnakeCaseName("foo__bar"), Not(IsOk()));
  EXPECT_THAT(IsValidLowerSnakeCaseName(""), Not(IsOk()));
}

TEST(NamingStyleTest, IsValidUpperSnakeCaseName) {
  EXPECT_THAT(IsValidUpperSnakeCaseName("FOO"), IsOk());
  EXPECT_THAT(IsValidUpperSnakeCaseName("FOO_BAR"), IsOk());

  EXPECT_THAT(IsValidUpperSnakeCaseName("FOO_BAR_123"), Not(IsOk()));
  EXPECT_THAT(IsValidUpperSnakeCaseName("foo"), Not(IsOk()));
  EXPECT_THAT(IsValidUpperSnakeCaseName("FOO_bar"), Not(IsOk()));
  EXPECT_THAT(IsValidUpperSnakeCaseName("FOO__BAR"), Not(IsOk()));
  EXPECT_THAT(IsValidUpperSnakeCaseName(""), Not(IsOk()));
}

}  // namespace
}  // namespace protobuf
}  // namespace google
