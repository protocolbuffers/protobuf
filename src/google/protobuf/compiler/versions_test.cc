// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/versions.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/test_textproto.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace internal {
namespace {
TEST(ParseProtobufVersionTest, EmptyVersionString) {
  EXPECT_DEATH({ ParseProtobufVersion(""); }, "version cannot be empty.");
}

TEST(ParseProtobufVersionTest, MissingVersionSegment) {
  EXPECT_DEATH({ ParseProtobufVersion("3.26-dev"); },
               "version string must provide major, minor and micro numbers.");
}

TEST(ParseProtobufVersionTest, RedundantVersionSuffix) {
  EXPECT_DEATH({ ParseProtobufVersion("3.26-dev-rc1"); },
               "version cannot have more than one suffix annotated by \"-\".");
}

TEST(ParseProtobufVersionTest, FullVersionWithRCSuffix) {
  EXPECT_THAT(ParseProtobufVersion("3.26.2-rc1"),
              EqualsProto(R"pb(major: 3 minor: 26 patch: 2 suffix: "-rc1")pb"));
}

TEST(ParseProtobufVersionTest, FullVersionWithoutSuffix) {
  EXPECT_THAT(ParseProtobufVersion("3.26.2"),
              EqualsProto(R"pb(major: 3 minor: 26 patch: 2)pb"));
}

TEST(ParseProtobufVersionTest, VersionWithDevSuffix) {
  EXPECT_THAT(ParseProtobufVersion("3.26.0-dev"),
              EqualsProto(R"pb(major: 3 minor: 26 patch: 0 suffix: "-dev")pb"));
}
}  // namespace
}  // namespace internal
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
