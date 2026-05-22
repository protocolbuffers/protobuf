// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This test requires fasttable support to work properly.
// Run with `--//third_party/upb:fasttable_enabled=true`.

#include <cstdint>
#include <string>
#include <tuple>

#include <gtest/gtest.h>
#include "absl/strings/match.h"  // IWYU pragma: keep
#include "absl/strings/string_view.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/status.hpp"
#include "upb/base/string_view.h"
#include "upb/mem/arena.hpp"
#include "upb/message/accessors.h"
#include "upb/message/message.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.hpp"
#include "upb/mini_descriptor/link.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/test_util/field_types.h"
#include "upb/wire/test_util/make_mini_table.h"
#include "upb/wire/test_util/wire_message.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace test {
namespace {

struct UnknownFieldTestCase {
  uint32_t field_number;
  wire_types::WireValue value;
  std::string name;

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const UnknownFieldTestCase& tc) {
    sink.Append("{field_number: ");
    sink.Append(std::to_string(tc.field_number));
    sink.Append(", name: \"");
    sink.Append(tc.name);
    sink.Append("\"}");
  }
};

class UnknownFieldTest
    : public testing::TestWithParam<std::tuple<UnknownFieldTestCase, bool>> {};

TEST_P(UnknownFieldTest, UnknownFieldFastPath) {
  const auto& param = GetParam();
  const auto& test_case = std::get<0>(param);
  const bool extensible = std::get<1>(param);
  uint32_t unknown_field_num = test_case.field_number;
  char trace_buf[64];
  upb::Arena msg_arena;
  upb::Arena mt_arena;

  // Create a MiniTable with field 1. Any other field number will be unknown.
  auto [mt, field] =
      extensible
          ? MiniTable::MakeExtensibleSingleFieldTable<field_types::Int32>(
                1, kUpb_DecodeFast_Scalar, mt_arena.ptr())
          : MiniTable::MakeSingleFieldTable<field_types::Int32>(
                1, kUpb_DecodeFast_Scalar, mt_arena.ptr());
  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

  std::string payload = ToBinaryPayload(
      wire_types::WireMessage{{unknown_field_num, test_case.value}});

  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

  // Verify it was saved as an unknown field.
  ASSERT_TRUE(upb_Message_HasUnknown(msg));

  // Verify the contents of the unknown field.
  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_StringView unknown_data;
  std::string captured_unknown;
  while (upb_Message_NextUnknown(msg, &unknown_data, &iter)) {
    captured_unknown.append(unknown_data.data, unknown_data.size);
  }
  EXPECT_EQ(captured_unknown, payload);

#if !defined(NDEBUG) && UPB_FASTTABLE
  // Because it was parsed on the fast path and we natively support
  // >2 byte tags inline, we should not see fallback ('<')
  // or mini table ('M') in the trace output regardless of tag size.
  EXPECT_FALSE(absl::StrContains(trace_buf, "<"));
  EXPECT_FALSE(absl::StrContains(trace_buf, "M"));
#endif
}

INSTANTIATE_TEST_SUITE_P(
    AllWireTypes, UnknownFieldTest,
    testing::Combine(
        testing::Values(
            UnknownFieldTestCase{2, wire_types::Varint{123}, "Varint"},
            UnknownFieldTestCase{2, wire_types::Varint{255},
                                 "VarintHighBitValue"},
            UnknownFieldTestCase{2, wire_types::Delimited{"Hello World"},
                                 "Delimited"},
            UnknownFieldTestCase{3, wire_types::Varint{123},
                                 "VarintWithCollision"},
            UnknownFieldTestCase{100, wire_types::Fixed64{0x1234567890ABCDEF},
                                 "Fixed64"},
            UnknownFieldTestCase{16, wire_types::Fixed32{0x12345678},
                                 "Fixed32"},
            UnknownFieldTestCase{2048, wire_types::Varint{123}, "LargeTag"}),
        testing::Bool()),
    [](const testing::TestParamInfo<UnknownFieldTest::ParamType>& info) {
      const auto& test_case = std::get<0>(info.param);
      const bool extensible = std::get<1>(info.param);
      return test_case.name + (extensible ? "Extensible" : "Normal");
    });

TEST(UnknownFieldSpecialTest, UnknownVarintFollowedByUnknownGroup) {
  char trace_buf[64];
  upb::Arena arena;

  // Create a MiniTable with field 1. Any other field number will be unknown.
  auto [mt, field] = MiniTable::MakeSingleFieldTable<field_types::Int32>(
      1, kUpb_DecodeFast_Scalar, arena.ptr());
  upb_Message* msg = upb_Message_New(mt, arena.ptr());
  ASSERT_NE(msg, nullptr);

  // Payload: Unknown Varint (field 2), then Unknown Group (field 3).
  std::string payload = ToBinaryPayload(wire_types::WireMessage{
      {2, wire_types::Varint{123}},
      {3, wire_types::Group{{4, wire_types::Varint{456}}}},
  });

  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          arena.ptr(), trace_buf, sizeof(trace_buf));

  // We expect success, but due to the bug it might return Malformed.
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

TEST(UnknownFieldSpecialTest, UnknownVarintFollowedByEndGroupInGroup) {
  char trace_buf[64];
  upb::Arena arena;

  // 1. Create Child MiniTable (the message itself).
  auto [child_mt, child_field] =
      MiniTable::MakeSingleFieldTable<field_types::Int32>(
          1, kUpb_DecodeFast_Scalar, arena.ptr());

  // 2. Create Parent MiniTable with Group field (field 5).
  upb::MtDataEncoder parent_encoder;
  parent_encoder.StartMessage(0);
  parent_encoder.PutField(kUpb_FieldType_Group, 5, 0);  // Field 5, no modifiers
  upb::Status status;
  const upb_MiniTable* parent_mt = upb_MiniTable_Build(
      parent_encoder.data().data(), parent_encoder.data().size(), arena.ptr(),
      status.ptr());
  ASSERT_TRUE(status.ok()) << status.error_message();
  const upb_MiniTableField* parent_field =
      upb_MiniTable_GetFieldByIndex(parent_mt, 0);
  ASSERT_NE(parent_field, nullptr);

  // 3. Link Parent's Group field to Child MiniTable.
  upb_MiniTableField* group_field =
      const_cast<upb_MiniTableField*>(parent_field);
  ASSERT_TRUE(upb_MiniTable_SetSubMessage(const_cast<upb_MiniTable*>(parent_mt),
                                          group_field, child_mt));

  upb_Message* parent_msg = upb_Message_New(parent_mt, arena.ptr());
  ASSERT_NE(parent_msg, nullptr);

  // 4. Build Payload.
  // We want to test batching INSIDE the group.
  // The group is field 5.
  // Inside group: Unknown Varint (field 2, value 123).
  std::string payload = ToBinaryPayload(wire_types::WireMessage{
      {5, wire_types::Group{{2, wire_types::Varint{123}}}}});

  upb_DecodeStatus result = upb_DecodeWithTrace(
      payload.data(), payload.size(), parent_msg, parent_mt, nullptr, 0,
      arena.ptr(), trace_buf, sizeof(trace_buf));

  // It should be OK because EndGroup is expected.
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

  // 5. Verify the Child message has the Unknown Varint.
  const upb_Message* child_msg =
      upb_Message_GetMessage(parent_msg, group_field);
  ASSERT_NE(child_msg, nullptr);

  ASSERT_TRUE(upb_Message_HasUnknown(child_msg));

  // Verify contents.
  std::string captured_unknown;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_StringView unknown_data;
  while (upb_Message_NextUnknown(child_msg, &unknown_data, &iter)) {
    captured_unknown.append(unknown_data.data, unknown_data.size);
  }

  // The captured unknown should be just the varint part.
  // Field 2 Varint: (2 << 3) | 0 = 16 (0x10), value = 123 (0x7B)
  EXPECT_EQ(captured_unknown, "\x10\x7B");
}

}  // namespace
}  // namespace test
}  // namespace upb
