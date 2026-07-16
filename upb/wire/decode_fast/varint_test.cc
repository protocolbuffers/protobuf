// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This test requires fasttable support to work properly.
// Run with `--//upb:fasttable_enabled=true`.

#include <cstdint>
#include <string>

#include <gtest/gtest.h>
#include "upb/mem/arena.hpp"
#include "upb/message/message.h"
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

TEST(DecodeFastVarintTest, RejectsSizeAboveInt32Max) {
  upb::Arena arena;
  const upb_MiniTable* mt =
      MiniTable::MakeSingleFieldTable<field_types::String>(
          1, kUpb_DecodeFast_Scalar, arena.ptr())
          .first;

  // Field 1, wire type 2, followed by the size varint 2^32, which is above the
  // INT32_MAX cap that upb_WireReader_ReadSize() enforces.
  const std::string payload("\x0a\x80\x80\x80\x80\x10", 6);

  upb_Message* msg = upb_Message_New(mt, arena.ptr());
  EXPECT_EQ(upb_Decode(payload.data(), payload.size(), msg, mt, nullptr, 0,
                       arena.ptr()),
            kUpb_DecodeStatus_Malformed);
}

TEST(DecodeFastVarintTest, KeepsFullEnumValueInUnknownFields) {
  upb::Arena arena;
  const upb_MiniTable* mt =
      MiniTable::MakeSingleFieldTable<field_types::ClosedEnum>(
          1, kUpb_DecodeFast_Scalar, arena.ptr())
          .first;

  // The low 32 bits are outside the test enum's range, so the value is
  // unrecognized and lands in the unknown fields, where all 64 bits of it must
  // survive.
  const uint64_t value = (uint64_t{1} << 32) | (1 << 21);
  const std::string payload =
      ToBinaryPayload(wire_types::WireMessage{{1, wire_types::Varint(value)}});

  upb_Message* msg = upb_Message_New(mt, arena.ptr());
  ASSERT_EQ(upb_Decode(payload.data(), payload.size(), msg, mt, nullptr, 0,
                       arena.ptr()),
            kUpb_DecodeStatus_Ok);
  ASSERT_TRUE(upb_Message_HasUnknown(msg));

  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_StringView unknown_data;
  std::string captured_unknown;
  while (upb_Message_NextUnknown(msg, &unknown_data, &iter)) {
    captured_unknown.append(unknown_data.data, unknown_data.size);
  }
  EXPECT_EQ(captured_unknown, payload);
}

}  // namespace
}  // namespace test
}  // namespace upb
