// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_descriptor/build_enum.h"

#include <stdint.h>

#include <string>

#include <gtest/gtest.h>
#include "upb/base/status.h"
#include "upb/mem/arena.h"
#include "upb/mini_descriptor/internal/base92.h"
#include "upb/mini_descriptor/internal/wire_constants.h"
#include "upb/mini_table/enum.h"

namespace {

void AppendSkip(std::string* out, uint32_t value) {
  const int min_skip = _upb_FromBase92(kUpb_EncodedValue_MinSkip);
  for (int shift = 0; shift < 32; shift += 5) {
    const int chunk = static_cast<int>((value >> shift) & 31u);
    out->push_back(_upb_ToBase92(static_cast<int8_t>(min_skip + chunk)));
  }
}

std::string MakeHighCountEnumDescriptor(uint32_t target_value) {
  std::string desc(1, static_cast<char>(kUpb_EncodedVersion_EnumV1));
  const char mask31 = _upb_ToBase92(31);
  // 410 * 65536 * 5 = 134,348,800 (> UINT32_MAX / 32), while base wraps back
  // to 0 each cycle so mask_limit stays at 327,680 (10,240 words).
  desc.reserve(1 + 410 * (65536 + 7) + 8);
  for (int cycle = 0; cycle < 410; ++cycle) {
    desc.append(65536, mask31);
    if (cycle + 1 < 410) {
      AppendSkip(&desc, 0u - 327680u);
    }
  }
  AppendSkip(&desc, target_value - 327680u);
  desc.push_back(_upb_ToBase92(1));
  return desc;
}

TEST(BuildEnumTest, DenseSmallAndSparseHighValuesSucceed) {
  std::string desc(1, static_cast<char>(kUpb_EncodedVersion_EnumV1));
  desc.push_back(_upb_ToBase92(0b00101));  // 0 and 2
  AppendSkip(&desc, UINT32_MAX - 5u);      // base advances from 5 to UINT32_MAX
  desc.push_back(_upb_ToBase92(1));        // UINT32_MAX

  upb_Arena* arena = upb_Arena_New();
  ASSERT_NE(arena, nullptr);
  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTableEnum* table =
      upb_MiniTableEnum_Build(desc.data(), desc.size(), arena, &status);
  ASSERT_NE(table, nullptr) << upb_Status_ErrorMessage(&status);
  EXPECT_TRUE(upb_MiniTableEnum_CheckValue(table, 0));
  EXPECT_FALSE(upb_MiniTableEnum_CheckValue(table, 1));
  EXPECT_TRUE(upb_MiniTableEnum_CheckValue(table, 2));
  EXPECT_TRUE(upb_MiniTableEnum_CheckValue(table, UINT32_MAX));
  EXPECT_FALSE(upb_MiniTableEnum_CheckValue(table, UINT32_MAX - 1u));
  upb_Arena_Free(arena);
}

TEST(BuildEnumTest, DenseMaskCountHighBoundaryUint32MaxMinus31UsesSparseTail) {
  const std::string desc = MakeHighCountEnumDescriptor(UINT32_MAX - 31u);
  upb_Arena* arena = upb_Arena_New();
  ASSERT_NE(arena, nullptr);
  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTableEnum* table =
      upb_MiniTableEnum_Build(desc.data(), desc.size(), arena, &status);
  ASSERT_NE(table, nullptr) << upb_Status_ErrorMessage(&status);
  EXPECT_TRUE(upb_MiniTableEnum_CheckValue(table, 0));
  EXPECT_TRUE(upb_MiniTableEnum_CheckValue(table, 327679u));
  EXPECT_FALSE(upb_MiniTableEnum_CheckValue(table, 327680u));
  EXPECT_TRUE(upb_MiniTableEnum_CheckValue(table, UINT32_MAX - 31u));
  EXPECT_FALSE(upb_MiniTableEnum_CheckValue(table, UINT32_MAX));
  upb_Arena_Free(arena);
}

TEST(BuildEnumTest, DenseMaskCountUint32MaxUsesSparseTail) {
  const std::string desc = MakeHighCountEnumDescriptor(UINT32_MAX);
  upb_Arena* arena = upb_Arena_New();
  ASSERT_NE(arena, nullptr);
  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTableEnum* table =
      upb_MiniTableEnum_Build(desc.data(), desc.size(), arena, &status);
  ASSERT_NE(table, nullptr) << upb_Status_ErrorMessage(&status);
  EXPECT_TRUE(upb_MiniTableEnum_CheckValue(table, 0));
  EXPECT_TRUE(upb_MiniTableEnum_CheckValue(table, 327679u));
  EXPECT_FALSE(upb_MiniTableEnum_CheckValue(table, 327680u));
  EXPECT_FALSE(upb_MiniTableEnum_CheckValue(table, UINT32_MAX - 31u));
  EXPECT_TRUE(upb_MiniTableEnum_CheckValue(table, UINT32_MAX));
  upb_Arena_Free(arena);
}

}  // namespace
