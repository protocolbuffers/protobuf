// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode_fast/select.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <gtest/gtest.h>
#include "absl/base/macros.h"
#include "upb/mem/arena.hpp"
#include "upb/mini_table/internal/message.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/test_util/field_types.h"
#include "upb/wire/test_util/make_mini_table.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace test {
namespace {

TEST(SelectTest, FunctionIndicesMatch) {
  // Verify that UPB_DECODEFAST_FUNCTION_IDX() and UPB_DECODEFAST_FUNCTIONS()
  // use the same ordering and the indexes correctly match.
#define IDX(type, card, size)                                                 \
  UPB_DECODEFAST_FUNCTION_IDX(kUpb_DecodeFast_##type, kUpb_DecodeFast_##card, \
                              kUpb_DecodeFast_##size),
  static const uint32_t numbers[] = {UPB_DECODEFAST_FUNCTIONS(IDX)};
  for (size_t i = 0; i < ABSL_ARRAYSIZE(numbers); i++) {
    EXPECT_EQ(numbers[i], i) << upb_DecodeFast_GetFunctionName(i) << " vs "
                             << upb_DecodeFast_GetFunctionName(numbers[i]);
  }
#undef IDX
}

TEST(SelectTest, UnknownSlotsAssignedForNonExtendable) {
  upb::Arena mt_arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<field_types::Int32>(
      1, kUpb_DecodeFast_Scalar, mt_arena.ptr());

  upb_DecodeFast_TableEntry table[32];
  int size = upb_DecodeFast_BuildTable(mt, table);

  // Field 1 maps to slot 1: (1 << 3) | 0 (varint wire type) = 8.
  // Bits 3-7 are 00001, so slot 1.
  // Table size will be 2 (power of 2 >= max+1).
  EXPECT_EQ(size, 2);
  EXPECT_NE(table[1].function_idx, UINT32_MAX);
  EXPECT_NE(table[1].function_idx, kUpb_DecodeFast_Unknown);
  EXPECT_EQ(table[0].function_idx, kUpb_DecodeFast_Unknown);

  // The all_fields_assigned bit should be set in ext since the message
  // is non-extendable and all fields were placed in the fasttable.
  EXPECT_NE(mt->UPB_PRIVATE(ext) & kUpb_ExtMode_AllFastFieldsAssigned, 0);
}

}  // namespace
}  // namespace test
}  // namespace upb