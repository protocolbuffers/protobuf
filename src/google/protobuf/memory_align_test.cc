// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/memory_align.h"

#include <cstdint>

#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace internal {
namespace {

TEST(ArenaAlignDefault, Align) {
  auto align_default = ArenaAlignDefault();
  EXPECT_EQ(align_default.align, 8);
}

TEST(ArenaAlignDefault, Floor) {
  auto align_default = ArenaAlignDefault();
  EXPECT_EQ(align_default.Floor(0), 0);
  EXPECT_EQ(align_default.Floor(1), 0);
  EXPECT_EQ(align_default.Floor(7), 0);
  EXPECT_EQ(align_default.Floor(8), 8);
  EXPECT_EQ(align_default.Floor(9), 8);
  EXPECT_EQ(align_default.Floor(15), 8);
  EXPECT_EQ(align_default.Floor(16), 16);
}

TEST(ArenaAlignDefault, Ceil) {
  auto align_default = ArenaAlignDefault();
  EXPECT_EQ(align_default.Ceil(0), 0);
  EXPECT_EQ(align_default.Ceil(1), 8);
  EXPECT_EQ(align_default.Ceil(7), 8);
  EXPECT_EQ(align_default.Ceil(8), 8);
  EXPECT_EQ(align_default.Ceil(9), 16);
  EXPECT_EQ(align_default.Ceil(15), 16);
  EXPECT_EQ(align_default.Ceil(16), 16);
}

TEST(ArenaAlignDefault, Padded) {
  auto align_default = ArenaAlignDefault();
  EXPECT_EQ(align_default.Padded(0), 0);
  EXPECT_EQ(align_default.Padded(8), 8);
  EXPECT_EQ(align_default.Padded(64), 64);
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(align_default.Padded(1), ".*");
#endif  // GTEST_HAS_DEATH_TEST
}

TEST(ArenaAlignDefault, CeilPtr) {
  alignas(8) char p[17] = {0};
  auto align_default = ArenaAlignDefault();
  EXPECT_EQ(align_default.Ceil(p + 0), p + 0);
  EXPECT_EQ(align_default.Ceil(p + 1), p + 8);
  EXPECT_EQ(align_default.Ceil(p + 7), p + 8);
  EXPECT_EQ(align_default.Ceil(p + 8), p + 8);
  EXPECT_EQ(align_default.Ceil(p + 9), p + 16);
  EXPECT_EQ(align_default.Ceil(p + 15), p + 16);
  EXPECT_EQ(align_default.Ceil(p + 16), p + 16);
}

TEST(ArenaAlignDefault, CheckAligned) {
  alignas(8) char p[17] = {0};
  auto align_default = ArenaAlignDefault();
  EXPECT_EQ(align_default.CheckAligned(p + 0), p + 0);
  EXPECT_EQ(align_default.CheckAligned(p + 8), p + 8);
  EXPECT_EQ(align_default.CheckAligned(p + 16), p + 16);
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
  EXPECT_EQ(align_default.CeilDefaultAligned(p + 0), p + 0);
  EXPECT_EQ(align_default.CeilDefaultAligned(p + 8), p + 8);
  EXPECT_EQ(align_default.CeilDefaultAligned(p + 16), p + 16);
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

TEST(MemoryAlign, Of) {
  EXPECT_EQ(MemoryAlign::Of<void*>().align(), alignof(void*));
  EXPECT_EQ(MemoryAlign::Of<bool>().align(), alignof(bool));
  EXPECT_EQ(MemoryAlign::Of<char>().align(), alignof(char));
  EXPECT_EQ(MemoryAlign::Of<int8_t>().align(), alignof(int8_t));
  EXPECT_EQ(MemoryAlign::Of<int16_t>().align(), alignof(int16_t));
  EXPECT_EQ(MemoryAlign::Of<int32_t>().align(), alignof(int32_t));
  EXPECT_EQ(MemoryAlign::Of<int64_t>().align(), alignof(int64_t));
}

TEST(MemoryAlign, Align) {
  auto align_64 = ArenaAlignAs(64);
  EXPECT_EQ(align_64.align(), 64);
}

TEST(MemoryAlign, Floor) {
  auto align_64 = ArenaAlignAs(64);
  EXPECT_EQ(align_64.Floor(0), 0);
  EXPECT_EQ(align_64.Floor(1), 0);
  EXPECT_EQ(align_64.Floor(63), 0);
  EXPECT_EQ(align_64.Floor(64), 64);
  EXPECT_EQ(align_64.Floor(65), 64);
  EXPECT_EQ(align_64.Floor(127), 64);
  EXPECT_EQ(align_64.Floor(128), 128);
}

TEST(MemoryAlign, Ceil) {
  auto align_64 = ArenaAlignAs(64);
  EXPECT_EQ(align_64.Ceil(0), 0);
  EXPECT_EQ(align_64.Ceil(1), 64);
  EXPECT_EQ(align_64.Ceil(63), 64);
  EXPECT_EQ(align_64.Ceil(64), 64);
  EXPECT_EQ(align_64.Ceil(65), 128);
  EXPECT_EQ(align_64.Ceil(127), 128);
  EXPECT_EQ(align_64.Ceil(128), 128);
}

TEST(MemoryAlign, Padded) {
  auto align_64 = ArenaAlignAs(64);
  EXPECT_EQ(align_64.Padded(64), 64 + 64 - ArenaAlignDefault{}.align);
  EXPECT_EQ(align_64.Padded(128), 128 + 64 - ArenaAlignDefault{}.align);
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(align_64.Padded(2), ".*");
  EXPECT_DEBUG_DEATH(ArenaAlignAs(2).Padded(8), ".*");
#endif  // GTEST_HAS_DEATH_TEST
}

TEST(MemoryAlign, CeilPtr) {
  alignas(64) char p[129] = {0};
  auto align_64 = ArenaAlignAs(64);
  EXPECT_EQ(align_64.Ceil(p + 0), p);
  EXPECT_EQ(align_64.Ceil(p + 1), p + 64);
  EXPECT_EQ(align_64.Ceil(p + 63), p + 64);
  EXPECT_EQ(align_64.Ceil(p + 64), p + 64);
  EXPECT_EQ(align_64.Ceil(p + 65), p + 128);
  EXPECT_EQ(align_64.Ceil(p + 127), p + 128);
  EXPECT_EQ(align_64.Ceil(p + 128), p + 128);
}

TEST(MemoryAlign, CheckAligned) {
  alignas(128) char p[129] = {0};
  auto align_64 = ArenaAlignAs(64);
  EXPECT_EQ(align_64.CheckAligned(p + 0), p);
  EXPECT_EQ(align_64.CheckAligned(p + 64), p + 64);
  EXPECT_EQ(align_64.CheckAligned(p + 128), p + 128);
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

TEST(MemoryAlign, CeilDefaultAligned) {
  alignas(128) char p[129] = {0};
  auto align_64 = ArenaAlignAs(64);
  EXPECT_EQ(align_64.CeilDefaultAligned(p + 0), p);
  EXPECT_EQ(align_64.CeilDefaultAligned(p + 8), p + 64);
  EXPECT_EQ(align_64.CeilDefaultAligned(p + 56), p + 64);
  EXPECT_EQ(align_64.CeilDefaultAligned(p + 64), p + 64);
  EXPECT_EQ(align_64.CeilDefaultAligned(p + 72), p + 128);
  EXPECT_EQ(align_64.CeilDefaultAligned(p + 120), p + 128);
  EXPECT_EQ(align_64.CeilDefaultAligned(p + 128), p + 128);
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 1), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 7), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 63), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 65), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 127), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 129), ".*");
#endif  // GTEST_HAS_DEATH_TEST
}

TEST(MemoryAlign, IsAligned) {
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
