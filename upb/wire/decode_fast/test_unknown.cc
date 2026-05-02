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
#include "upb/base/string_view.h"
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

#if !defined(NDEBUG) && defined(UPB_ENABLE_FASTTABLE)
  // Because it was parsed on the fast path, we should not see fallback ('<' )
  // or mini table ('M') in the trace output for tags that fit in 1 or 2 bytes.
  if (unknown_field_num < 2048) {
    EXPECT_FALSE(absl::StrContains(trace_buf, "<"));
    EXPECT_FALSE(absl::StrContains(trace_buf, "M"));
  } else {
    // Large tags are expected to fall back to the MiniTable decoder.
    EXPECT_TRUE(absl::StrContains(trace_buf, "<"));
    EXPECT_TRUE(absl::StrContains(trace_buf, "M"));
  }
#endif
}

INSTANTIATE_TEST_SUITE_P(
    AllWireTypes, UnknownFieldTest,
    testing::Combine(
        testing::Values(
            UnknownFieldTestCase{2, wire_types::Varint{123}, "Varint"},
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

}  // namespace
}  // namespace test
}  // namespace upb
