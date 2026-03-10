// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/arena_align.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::testing::Eq;

TEST(ArenaAlignDefault, Align) {
  auto align_default = ArenaAlignDefault();
  EXPECT_THAT(align_default.align, Eq(8));
}

TEST(ArenaAlignDefault, Floor) {
  auto align_default = ArenaAlignDefault();
  EXPECT_THAT(align_default.Floor(0), Eq(0));
  EXPECT_THAT(align_default.Floor(1), Eq(0));
  EXPECT_THAT(align_default.Floor(7), Eq(0));
  EXPECT_THAT(align_default.Floor(8), Eq(8));
  EXPECT_THAT(align_default.Floor(9), Eq(8));
  EXPECT_THAT(align_default.Floor(15), Eq(8));
  EXPECT_THAT(align_default.Floor(16), Eq(16));
}

TEST(ArenaAlignDefault, Ceil) {
  auto align_default = ArenaAlignDefault();
  EXPECT_THAT(align_default.Ceil(0), Eq(0));
  EXPECT_THAT(align_default.Ceil(1), Eq(8));
  EXPECT_THAT(align_default.Ceil(7), Eq(8));
  EXPECT_THAT(align_default.Ceil(8), Eq(8));
  EXPECT_THAT(align_default.Ceil(9), Eq(16));
  EXPECT_THAT(align_default.Ceil(15), Eq(16));
  EXPECT_THAT(align_default.Ceil(16), Eq(16));
}

TEST(ArenaAlignDefault, Padded) {
  auto align_default = ArenaAlignDefault();
  EXPECT_THAT(align_default.Padded(0), Eq(0));
  EXPECT_THAT(align_default.Padded(8), Eq(8));
  EXPECT_THAT(align_default.Padded(64), Eq(64));
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(align_default.Padded(1), ".*");
#endif  // GTEST_HAS_DEATH_TEST
}

TEST(ArenaAlignDefault, CeilPtr) {
  alignas(8) char p[17] = {0};
  auto align_default = ArenaAlignDefault();
  EXPECT_THAT(align_default.Ceil(p + 0), Eq(p + 0));
  EXPECT_THAT(align_default.Ceil(p + 1), Eq(p + 8));
  EXPECT_THAT(align_default.Ceil(p + 7), Eq(p + 8));
  EXPECT_THAT(align_default.Ceil(p + 8), Eq(p + 8));
  EXPECT_THAT(align_default.Ceil(p + 9), Eq(p + 16));
  EXPECT_THAT(align_default.Ceil(p + 15), Eq(p + 16));
  EXPECT_THAT(align_default.Ceil(p + 16), Eq(p + 16));
}

TEST(ArenaAlignDefault, CheckAligned) {
  alignas(8) char p[17] = {0};
  auto align_default = ArenaAlignDefault();
  EXPECT_THAT(align_default.CheckAligned(p + 0), Eq(p + 0));
  EXPECT_THAT(align_default.CheckAligned(p + 8), Eq(p + 8));
  EXPECT_THAT(align_default.CheckAligned(p + 16), Eq(p + 16));
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(align_default.CheckAligned(p + 1), ".*");
  EXPECT_DEBUG_DEATH(align_default.CheckAligned(p + 7), ".*");
  EXPECT_DEBUG_DEATH(align_default.CheckAligned(p + 9), ".*");
  EXPECT_DEBUG_DEATH(align_default.CheckAligned(p + 15), ".*");
  EXPECT_DEBUG_DEATH(align_default.CheckAligned(p + 17), ".*");
#endif  // GTEST_HAS_DEATH_TEST
}

TEST(ArenaAlignDefault, CeilDefaultAligned) {
  alignas(8) char p[17] = {0};
  auto align_default = ArenaAlignDefault();
  EXPECT_THAT(align_default.CeilDefaultAligned(p + 0), Eq(p + 0));
  EXPECT_THAT(align_default.CeilDefaultAligned(p + 8), Eq(p + 8));
  EXPECT_THAT(align_default.CeilDefaultAligned(p + 16), Eq(p + 16));
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(align_default.CeilDefaultAligned(p + 1), ".*");
  EXPECT_DEBUG_DEATH(align_default.CeilDefaultAligned(p + 7), ".*");
  EXPECT_DEBUG_DEATH(align_default.CeilDefaultAligned(p + 9), ".*");
  EXPECT_DEBUG_DEATH(align_default.CeilDefaultAligned(p + 15), ".*");
  EXPECT_DEBUG_DEATH(align_default.CeilDefaultAligned(p + 17), ".*");
#endif  // GTEST_HAS_DEATH_TEST
}

TEST(ArenaAlignDefault, IsAligned) {
  auto align_default = ArenaAlignDefault();
  EXPECT_TRUE(align_default.IsAligned(0));
  EXPECT_FALSE(align_default.IsAligned(1));
  EXPECT_FALSE(align_default.IsAligned(7));
  EXPECT_TRUE(align_default.IsAligned(8));
  EXPECT_FALSE(align_default.IsAligned(9));
  EXPECT_FALSE(align_default.IsAligned(15));
  EXPECT_TRUE(align_default.IsAligned(16));
}

TEST(ArenaAlign, Align) {
  auto align_64 = ArenaAlignAs(64);
  EXPECT_THAT(align_64.align, Eq(64));
}

TEST(ArenaAlign, Floor) {
  auto align_64 = ArenaAlignAs(64);
  EXPECT_THAT(align_64.Floor(0), Eq(0));
  EXPECT_THAT(align_64.Floor(1), Eq(0));
  EXPECT_THAT(align_64.Floor(63), Eq(0));
  EXPECT_THAT(align_64.Floor(64), Eq(64));
  EXPECT_THAT(align_64.Floor(65), Eq(64));
  EXPECT_THAT(align_64.Floor(127), Eq(64));
  EXPECT_THAT(align_64.Floor(128), Eq(128));
}

TEST(ArenaAlign, Ceil) {
  auto align_64 = ArenaAlignAs(64);
  EXPECT_THAT(align_64.Ceil(0), Eq(0));
  EXPECT_THAT(align_64.Ceil(1), Eq(64));
  EXPECT_THAT(align_64.Ceil(63), Eq(64));
  EXPECT_THAT(align_64.Ceil(64), Eq(64));
  EXPECT_THAT(align_64.Ceil(65), Eq(128));
  EXPECT_THAT(align_64.Ceil(127), Eq(128));
  EXPECT_THAT(align_64.Ceil(128), Eq(128));
}

TEST(ArenaAlign, Padded) {
  auto align_64 = ArenaAlignAs(64);
  EXPECT_THAT(align_64.Padded(64), Eq(64 + 64 - ArenaAlignDefault::align));
  EXPECT_THAT(align_64.Padded(128), Eq(128 + 64 - ArenaAlignDefault::align));
#if GTEST_HAS_DEATH_TEST
  // TODO: there are direct callers of AllocateAligned() that violate
  // `size` being a multiple of `align`: that should be an error / assert.
  //  EXPECT_DEBUG_DEATH(align_64.Padded(16), ".*");
  EXPECT_DEBUG_DEATH(ArenaAlignAs(2).Padded(8), ".*");
#endif  // GTEST_HAS_DEATH_TEST
}

TEST(ArenaAlign, CeilPtr) {
  alignas(64) char p[129] = {0};
  auto align_64 = ArenaAlignAs(64);
  EXPECT_THAT(align_64.Ceil(p + 0), Eq(p));
  EXPECT_THAT(align_64.Ceil(p + 1), Eq(p + 64));
  EXPECT_THAT(align_64.Ceil(p + 63), Eq(p + 64));
  EXPECT_THAT(align_64.Ceil(p + 64), Eq(p + 64));
  EXPECT_THAT(align_64.Ceil(p + 65), Eq(p + 128));
  EXPECT_THAT(align_64.Ceil(p + 127), Eq(p + 128));
  EXPECT_THAT(align_64.Ceil(p + 128), Eq(p + 128));
}

TEST(ArenaAlign, CheckAligned) {
  alignas(128) char p[129] = {0};
  auto align_64 = ArenaAlignAs(64);
  EXPECT_THAT(align_64.CheckAligned(p + 0), Eq(p));
  EXPECT_THAT(align_64.CheckAligned(p + 64), Eq(p + 64));
  EXPECT_THAT(align_64.CheckAligned(p + 128), Eq(p + 128));
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 1), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 7), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 8), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 56), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 63), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 65), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 72), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 120), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 129), ".*");
#endif  // GTEST_HAS_DEATH_TEST
}

TEST(ArenaAlign, CeilDefaultAligned) {
  alignas(128) char p[129] = {0};
  auto align_64 = ArenaAlignAs(64);
  EXPECT_THAT(align_64.CeilDefaultAligned(p + 0), Eq(p));
  EXPECT_THAT(align_64.CeilDefaultAligned(p + 8), Eq(p + 64));
  EXPECT_THAT(align_64.CeilDefaultAligned(p + 56), Eq(p + 64));
  EXPECT_THAT(align_64.CeilDefaultAligned(p + 64), Eq(p + 64));
  EXPECT_THAT(align_64.CeilDefaultAligned(p + 72), Eq(p + 128));
  EXPECT_THAT(align_64.CeilDefaultAligned(p + 120), Eq(p + 128));
  EXPECT_THAT(align_64.CeilDefaultAligned(p + 128), Eq(p + 128));
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 1), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 7), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 63), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 65), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 127), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 129), ".*");
#endif  // GTEST_HAS_DEATH_TEST
}

TEST(ArenaAlign, IsAligned) {
  auto align_64 = ArenaAlignAs(64);
  EXPECT_TRUE(align_64.IsAligned(0));
  EXPECT_FALSE(align_64.IsAligned(1));
  EXPECT_FALSE(align_64.IsAligned(63));
  EXPECT_TRUE(align_64.IsAligned(64));
  EXPECT_FALSE(align_64.IsAligned(65));
  EXPECT_FALSE(align_64.IsAligned(127));
  EXPECT_TRUE(align_64.IsAligned(128));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
