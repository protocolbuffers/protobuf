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
#include <string>

#include <gtest/gtest.h>
#include "absl/base/macros.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/status.hpp"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.hpp"
#include "upb/mini_descriptor/internal/modifiers.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
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
  int size = upb_DecodeFast_BuildTable(mt, nullptr, table);

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

// Builds a MiniTable laid out for the 64-bit platform regardless of the host's
// pointer size. The table is not linked.
upb_MiniTable* Build64BitMiniTable(const std::string& data, upb_Arena* arena) {
  upb::Status status;
  upb_MiniTable* mt =
      _upb_MiniTable_Build(data.data(), data.size(),
                           kUpb_MiniTablePlatform_64Bit, arena, status.ptr());
  EXPECT_NE(mt, nullptr) << status.error_message();
  return mt;
}

upb_MiniTable* Build64BitMapEntry(upb_FieldType key_type,
                                  upb_FieldType val_type, upb_Arena* arena) {
  upb::MtDataEncoder e;
  EXPECT_TRUE(e.EncodeMap(key_type, val_type, 0, 0));
  return Build64BitMiniTable(e.data(), arena);
}

TEST(SelectTest, MapFieldsIdentifiedByMapEntriesNotLinks) {
  upb::Arena arena;
  // In a MiniDescriptor (and an unlinked MiniTable), a map field is a repeated
  // message field.
  upb::MtDataEncoder e;
  ASSERT_TRUE(e.StartMessage(0));
  for (uint32_t num = 1; num <= 3; num++) {
    ASSERT_TRUE(
        e.PutField(kUpb_FieldType_Message, num, kUpb_FieldModifier_IsRepeated));
  }
  upb_MiniTable* mt = Build64BitMiniTable(e.data(), arena.ptr());
  ASSERT_NE(mt, nullptr);
  ASSERT_EQ(upb_MiniTable_FieldCount(mt), 3);

  // Field 1 is a map<int32, uint32>, field 2 a map<bool, uint32>, and field 3
  // a plain repeated message field.
  const upb_MiniTable* map_entries[3] = {
      Build64BitMapEntry(kUpb_FieldType_Int32, kUpb_FieldType_UInt32,
                         arena.ptr()),
      Build64BitMapEntry(kUpb_FieldType_Bool, kUpb_FieldType_UInt32,
                         arena.ptr()),
      nullptr,
  };
  for (int i = 0; i < 3; i++) {
    ASSERT_EQ(upb_MiniTableField_Number(upb_MiniTable_GetFieldByIndex(mt, i)),
              i + 1);
  }

  // Field N has a 1-byte tag and lands in slot N.
  upb_DecodeFast_TableEntry table[32];
  upb_DecodeFast_BuildTable(mt, map_entries, table);
  EXPECT_EQ(table[1].function_idx, kUpb_DecodeFast_IntMap_Tag1Byte);
  // Maps with bool keys are not supported by the fast decoder, and the field
  // must not be treated as a repeated message field either.
  EXPECT_NE(table[2].function_idx, kUpb_DecodeFast_IntMap_Tag1Byte);
  EXPECT_NE(table[2].function_idx, kUpb_DecodeFast_StrMap_Tag1Byte);
  EXPECT_NE(upb_DecodeFastData_GetFieldNumber(table[2].function_data), 2u);
  EXPECT_EQ(upb_DecodeFast_GetType(table[3].function_idx),
            kUpb_DecodeFast_Message);
  EXPECT_EQ(upb_DecodeFast_GetCardinality(table[3].function_idx),
            kUpb_DecodeFast_Repeated);
}

}  // namespace
}  // namespace test
}  // namespace upb