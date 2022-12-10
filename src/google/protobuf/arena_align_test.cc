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
#ifdef PROTOBUF_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(align_default.CheckAligned(p + 1), ".*");
  EXPECT_DEBUG_DEATH(align_default.CheckAligned(p + 7), ".*");
  EXPECT_DEBUG_DEATH(align_default.CheckAligned(p + 9), ".*");
  EXPECT_DEBUG_DEATH(align_default.CheckAligned(p + 15), ".*");
  EXPECT_DEBUG_DEATH(align_default.CheckAligned(p + 17), ".*");
#endif  // PROTOBUF_HAS_DEATH_TEST
}

TEST(ArenaAlignDefault, CeilDefaultAligned) {
  alignas(8) char p[17] = {0};
  auto align_default = ArenaAlignDefault();
  EXPECT_THAT(align_default.CeilDefaultAligned(p + 0), Eq(p + 0));
  EXPECT_THAT(align_default.CeilDefaultAligned(p + 8), Eq(p + 8));
  EXPECT_THAT(align_default.CeilDefaultAligned(p + 16), Eq(p + 16));
#ifdef PROTOBUF_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(align_default.CeilDefaultAligned(p + 1), ".*");
  EXPECT_DEBUG_DEATH(align_default.CeilDefaultAligned(p + 7), ".*");
  EXPECT_DEBUG_DEATH(align_default.CeilDefaultAligned(p + 9), ".*");
  EXPECT_DEBUG_DEATH(align_default.CeilDefaultAligned(p + 15), ".*");
  EXPECT_DEBUG_DEATH(align_default.CeilDefaultAligned(p + 17), ".*");
#endif  // PROTOBUF_HAS_DEATH_TEST
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
#ifdef PROTOBUF_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 1), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 7), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 8), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 56), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 63), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 65), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 72), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 120), ".*");
  EXPECT_DEBUG_DEATH(align_64.CheckAligned(p + 129), ".*");
#endif  // PROTOBUF_HAS_DEATH_TEST
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
#ifdef PROTOBUF_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 1), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 7), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 63), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 65), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 127), ".*");
  EXPECT_DEBUG_DEATH(align_64.CeilDefaultAligned(p + 129), ".*");
#endif  // PROTOBUF_HAS_DEATH_TEST
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
