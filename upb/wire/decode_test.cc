// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode.h"

#include <limits>
#include <optional>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "upb/mem/arena.hpp"
#include "upb/message/accessors.h"
#include "upb/message/accessors.hpp"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/test_util/field_types.h"
#include "upb/wire/test_util/make_mini_table.h"
#include "upb/wire/test_util/wire_message.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace test {

namespace {

template <typename T>
std::optional<T> GetOptionalField(upb_Message* msg, const upb_MiniTable* mt,
                                  int field_number) {
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(mt, field_number);
  ABSL_CHECK(field != nullptr);
  if (!upb_Message_HasBaseField(msg, field)) {
    return std::nullopt;
  }
  return GetMessageBaseField<T>(msg, field, T{});
}

template <typename T>
class FieldTypeTest : public testing::Test {};

TYPED_TEST_SUITE(FieldTypeTest, FieldTypes);

std::string ExpectedSingleFieldTrace(const upb_MiniTable* mt,
                                     const upb_MiniTableField* field) {
#ifdef NDEBUG
  return "";
#else
  return MiniTable::HasFastTableEntry(mt, field) ? "DF" : "M";
#endif
}

TYPED_TEST(FieldTypeTest, DecodeOptionalMaxValue) {
  char trace_buf[64];
  using Value = typename TypeParam::Value;
  Value value;
  if constexpr (std::is_same_v<Value, std::string>) {
    for (int i = 0; i < 1000; ++i) {
      value.append("hello world! ");
    }
  } else {
    value = std::numeric_limits<Value>::max();
  }
  upb::Arena arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Scalar, arena.ptr());
  upb_Message* msg = upb_Message_New(mt, arena.ptr());
  std::string payload = ToBinaryPayload(
      wire_types::WireMessage{{1, TypeParam::WireValue(value)}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetOptionalField<Value>(msg, mt, 1), value);
  EXPECT_EQ(absl::string_view(trace_buf), ExpectedSingleFieldTrace(mt, field));
}

TYPED_TEST(FieldTypeTest, DecodeOptionalMinValue) {
  char trace_buf[64];
  using Value = typename TypeParam::Value;
  Value value;
  if constexpr (!std::is_same_v<Value, std::string>) {
    value = std::numeric_limits<Value>::max();
  }
  upb::Arena arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Scalar, arena.ptr());
  upb_Message* msg = upb_Message_New(mt, arena.ptr());
  std::string payload = ToBinaryPayload(
      wire_types::WireMessage{{1, TypeParam::WireValue(value)}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetOptionalField<Value>(msg, mt, 1), value);
  EXPECT_EQ(absl::string_view(trace_buf), ExpectedSingleFieldTrace(mt, field));
}

TYPED_TEST(FieldTypeTest, DecodeOneofMaxValue) {
  char trace_buf[64];
  using Value = typename TypeParam::Value;
  Value value;
  if constexpr (std::is_same_v<Value, std::string>) {
    for (int i = 0; i < 1000; ++i) {
      value.append("hello world! ");
    }
  } else {
    value = std::numeric_limits<Value>::max();
  }
  upb::Arena arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Oneof, arena.ptr());
  upb_Message* msg = upb_Message_New(mt, arena.ptr());
  std::string payload = ToBinaryPayload(
      wire_types::WireMessage{{1, TypeParam::WireValue(value)}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetOptionalField<Value>(msg, mt, 1), value);
  EXPECT_EQ(absl::string_view(trace_buf), ExpectedSingleFieldTrace(mt, field));
}

}  // namespace

}  // namespace test
}  // namespace upb
