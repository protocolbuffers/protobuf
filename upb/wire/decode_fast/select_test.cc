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
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/test/test.upb_minitable.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/data.h"
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

TEST(SelectTest, TryFillEntryForCollidingMapField) {
  const upb_MiniTable* mt = &upb_0test__ModelWithCollidingMaps_msg_init;
  const upb_MiniTableField* scalar_field =
      upb_MiniTable_FindFieldByNumber(mt, 60);
  const upb_MiniTableField* colliding_scalar_field =
      upb_MiniTable_FindFieldByNumber(mt, 76);
  const upb_MiniTableField* map_field =
      upb_MiniTable_FindFieldByNumber(mt, 108);
  const upb_MiniTableField* bool_map_field =
      upb_MiniTable_FindFieldByNumber(mt, 124);
  ASSERT_NE(scalar_field, nullptr);
  ASSERT_NE(colliding_scalar_field, nullptr);
  ASSERT_NE(map_field, nullptr);
  ASSERT_NE(bool_map_field, nullptr);

  upb_DecodeFast_TableEntry table[32];
  int size = upb_DecodeFast_BuildTable(mt, table);

  // Fields 60, 76, 108, and 124 all map to slot 28; field 60 wins the slot.
  // Only repeated/map fields that are fast-compatible have
  // kUpb_LabelFlags_IsFastTableCollision set.
  EXPECT_EQ(size, 32);
  EXPECT_EQ(upb_DecodeFastData_GetFieldNumber(table[28].function_data), 60u);
  EXPECT_EQ(mt->UPB_PRIVATE(ext) & kUpb_ExtMode_AllFastFieldsAssigned, 0);
  EXPECT_FALSE(
      UPB_PRIVATE(_upb_MiniTableField_IsFastTableCollision)(scalar_field));
  EXPECT_FALSE(UPB_PRIVATE(_upb_MiniTableField_IsFastTableCollision)(
      colliding_scalar_field));
  EXPECT_TRUE(UPB_PRIVATE(_upb_MiniTableField_IsFastTableCollision)(map_field));
  EXPECT_FALSE(
      UPB_PRIVATE(_upb_MiniTableField_IsFastTableCollision)(bool_map_field));

  bool supported_tag_size = false;
  upb_DecodeFast_TableEntry map_entry;
  ASSERT_TRUE(upb_DecodeFast_TryFillEntry(mt, map_field, &supported_tag_size,
                                          &map_entry));
  EXPECT_TRUE(supported_tag_size);
  EXPECT_EQ(map_entry.function_idx, kUpb_DecodeFast_IntMap_Tag2Byte);
  EXPECT_EQ(upb_DecodeFastData_GetFieldNumber(map_entry.function_data), 108u);
  EXPECT_EQ(upb_DecodeFastData_GetTableSlot(map_entry.function_data), 28);
}

}  // namespace
}  // namespace test
}  // namespace upb