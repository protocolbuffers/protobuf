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

namespace google {
namespace protobuf {
namespace {

using ::testing::IsEmpty;
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
  {
    std::string error;
    EXPECT_TRUE(IsValidTitleCaseName("Foo", &error));
    EXPECT_THAT(error, IsEmpty());
  }

  {
    std::string error;
    EXPECT_TRUE(IsValidTitleCaseName("FooBar", &error));
    EXPECT_THAT(error, IsEmpty());
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidTitleCaseName("foo", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidTitleCaseName("Foo_Bar", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidTitleCaseName("", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }
}

TEST(NamingStyleTest, IsValidLowerSnakeCaseName) {
  {
    std::string error;
    EXPECT_TRUE(IsValidLowerSnakeCaseName("foo", &error));
    EXPECT_THAT(error, IsEmpty());
  }

  {
    std::string error;
    EXPECT_TRUE(IsValidLowerSnakeCaseName("foo_bar", &error));
    EXPECT_THAT(error, IsEmpty());
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidLowerSnakeCaseName("foo_bar_123", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidLowerSnakeCaseName("Foo", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidLowerSnakeCaseName("fooBar", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidLowerSnakeCaseName("foo__bar", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidLowerSnakeCaseName("", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }
}

TEST(NamingStyleTest, IsValidUpperSnakeCaseName) {
  {
    std::string error;
    EXPECT_TRUE(IsValidUpperSnakeCaseName("FOO", &error));
    EXPECT_THAT(error, IsEmpty());
  }

  {
    std::string error;
    EXPECT_TRUE(IsValidUpperSnakeCaseName("FOO_BAR", &error));
    EXPECT_THAT(error, IsEmpty());
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidUpperSnakeCaseName("FOO_BAR_123", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidUpperSnakeCaseName("foo", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidUpperSnakeCaseName("FOO_bar", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidUpperSnakeCaseName("FOO__BAR", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }

  {
    std::string error;
    EXPECT_FALSE(IsValidUpperSnakeCaseName("", &error));
    EXPECT_THAT(error, Not(IsEmpty()));
  }
}

}  // namespace
}  // namespace protobuf
}  // namespace google
