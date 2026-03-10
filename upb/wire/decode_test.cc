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
#include <vector>

#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
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
struct TestValues {
  static constexpr T kZero = 0;
  static constexpr T kMin = std::numeric_limits<T>::min();
  static constexpr T kMax = std::numeric_limits<T>::max();
};

template <>
struct TestValues<std::string> {
  static constexpr absl::string_view kZero = "";
  static constexpr absl::string_view kMin = "a very minimum valued string!";
  static constexpr absl::string_view kMax = "a very maximum valued string!";
};

template <typename T>
std::optional<T> GetOptionalField(upb_Message* msg,
                                  const upb_MiniTableField* field) {
  if (upb_Message_HasBaseField(msg, field)) {
    return GetMessageBaseField<T>(msg, field, T{});
  } else {
    return std::nullopt;
  }
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

std::string ExpectedRepeatedFieldTrace(const upb_MiniTable* mt,
                                       const upb_MiniTableField* field,
                                       int count) {
#ifdef NDEBUG
  return "";
#else
  if (MiniTable::HasFastTableEntry(mt, field)) {
    // Fasttable repeated fields have a fast path where we bypass dispatch if
    // the same tag is encountered consecutively.
    return absl::StrCat("D", std::string(count, 'F'));
  } else {
    return std::string(count, 'M');
  }
#endif
}

TYPED_TEST(FieldTypeTest, DecodeOptionalMaxValue) {
  char trace_buf[64];
  using Value = typename TypeParam::Value;
  upb::Arena arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Scalar, arena.ptr());
  upb_Message* msg = upb_Message_New(mt, arena.ptr());
  std::string payload = ToBinaryPayload(wire_types::WireMessage{
      {1, TypeParam::WireValue(Value(TestValues<Value>::kMax))}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetOptionalField<Value>(msg, field), TestValues<Value>::kMax);
  EXPECT_EQ(absl::string_view(trace_buf), ExpectedSingleFieldTrace(mt, field));
}

TYPED_TEST(FieldTypeTest, DecodeOptionalMinValue) {
  char trace_buf[64];
  using Value = typename TypeParam::Value;
  upb::Arena arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Scalar, arena.ptr());
  upb_Message* msg = upb_Message_New(mt, arena.ptr());
  std::string payload = ToBinaryPayload(wire_types::WireMessage{
      {1, TypeParam::WireValue(Value(TestValues<Value>::kMin))}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetOptionalField<Value>(msg, field), TestValues<Value>::kMin);
  EXPECT_EQ(absl::string_view(trace_buf), ExpectedSingleFieldTrace(mt, field));
}

TYPED_TEST(FieldTypeTest, DecodeOneofMaxValue) {
  char trace_buf[64];
  using Value = typename TypeParam::Value;
  upb::Arena arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Oneof, arena.ptr());
  upb_Message* msg = upb_Message_New(mt, arena.ptr());
  std::string payload = ToBinaryPayload(wire_types::WireMessage{
      {1, TypeParam::WireValue(Value(TestValues<Value>::kMax))}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetOptionalField<Value>(msg, field), TestValues<Value>::kMax);
  EXPECT_EQ(absl::string_view(trace_buf), ExpectedSingleFieldTrace(mt, field));
}

TYPED_TEST(FieldTypeTest, DecodeRepeated) {
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
  upb::Arena msg_arena;
  upb::Arena mt_arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Repeated, mt_arena.ptr());
  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
  std::string payload = ToBinaryPayload(wire_types::WireMessage{
      {1, TypeParam::WireValue(Value(TestValues<Value>::kZero))},
      {1, TypeParam::WireValue(Value(TestValues<Value>::kMin))},
      {1, TypeParam::WireValue(Value(TestValues<Value>::kMax))},
  });
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetRepeatedField<Value>(msg, field),
            (std::vector<Value>{Value(TestValues<Value>::kZero),
                                Value(TestValues<Value>::kMin),
                                Value(TestValues<Value>::kMax)}));
  EXPECT_EQ(absl::string_view(trace_buf),
            ExpectedRepeatedFieldTrace(mt, field, 3));
}

template <typename T>
class PackedTest : public testing::Test {};

TYPED_TEST_SUITE(PackedTest, PackableFieldTypes);

TYPED_TEST(PackedTest, DecodePackedDataForPackedField) {
  char trace_buf[64];
  using Value = typename TypeParam::Value;
  upb::Arena msg_arena;
  upb::Arena mt_arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Packed, mt_arena.ptr());
  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
  std::string packed_value = ToBinaryPayload(TypeParam::WireValue(0)) +
                             ToBinaryPayload(TypeParam::WireValue(1 << 10)) +
                             ToBinaryPayload(TypeParam::WireValue(1 << 20));
  std::string payload = ToBinaryPayload(
      wire_types::WireMessage{{1, wire_types::Delimited{packed_value}}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetRepeatedField<Value>(msg, field),
            (std::vector<Value>{0, static_cast<Value>(1 << 10),
                                static_cast<Value>(1 << 20)}));
  EXPECT_EQ(absl::string_view(trace_buf), ExpectedSingleFieldTrace(mt, field));
}

TYPED_TEST(PackedTest, DecodeTruncatedPackedField) {
  char trace_buf[64];
  upb::Arena msg_arena;
  upb::Arena mt_arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Packed, mt_arena.ptr());
  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
  std::string packed_value =
      ToBinaryPayload(TypeParam::WireValue(0)) +
      ToBinaryPayload(TypeParam::WireValue(1 << 10)) +
      // For varint fields, this will be a multi-byte varint, such that
      // truncating the last byte will result in an invalid varint.
      ToBinaryPayloadWithLongVarints(TypeParam::WireValue(1 << 20), 2, 2);
  packed_value.resize(packed_value.size() - 1);  // Truncate the last byte.
  std::string payload = ToBinaryPayload(
      wire_types::WireMessage{{1, wire_types::Delimited{packed_value}}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Malformed)
      << upb_DecodeStatus_String(result);
}

TYPED_TEST(PackedTest, DecodeEmptyPackedField) {
  char trace_buf[64];
  using Value = typename TypeParam::Value;
  upb::Arena msg_arena;
  upb::Arena mt_arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Packed, mt_arena.ptr());
  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
  std::string payload =
      ToBinaryPayload(wire_types::WireMessage{{1, wire_types::Delimited{""}}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetRepeatedField<Value>(msg, field), (std::vector<Value>{}));
  EXPECT_EQ(absl::string_view(trace_buf), ExpectedSingleFieldTrace(mt, field));
}

TYPED_TEST(PackedTest, DecodePackedDataForUnpackedField) {
  // Schema says this is not a packed field, but we supply packed wire format.
  char trace_buf[64];
  using Value = typename TypeParam::Value;
  upb::Arena msg_arena;
  upb::Arena mt_arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Repeated, mt_arena.ptr());
  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
  std::string packed_value = ToBinaryPayload(TypeParam::WireValue(0)) +
                             ToBinaryPayload(TypeParam::WireValue(1 << 10)) +
                             ToBinaryPayload(TypeParam::WireValue(1 << 20));
  std::string payload = ToBinaryPayload(
      wire_types::WireMessage{{1, wire_types::Delimited{packed_value}}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetRepeatedField<Value>(msg, field),
            (std::vector<Value>{0, static_cast<Value>(1 << 10),
                                static_cast<Value>(1 << 20)}));
  // Even though there is a mismatch, we can still parse this fast.
  EXPECT_EQ(absl::string_view(trace_buf), ExpectedSingleFieldTrace(mt, field));
}

TYPED_TEST(PackedTest, DecodeUnpackedDataForPackedField) {
  // Schema says this is a packed field, but we supply unpacked wire format.
  char trace_buf[64];
  using Value = typename TypeParam::Value;
  upb::Arena msg_arena;
  upb::Arena mt_arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Packed, mt_arena.ptr());
  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
  std::string payload = ToBinaryPayload(wire_types::WireMessage{
      {1, TypeParam::WireValue(0)},
      {1, TypeParam::WireValue(1 << 10)},
      {1, TypeParam::WireValue(1 << 20)},
  });
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetRepeatedField<Value>(msg, field),
            (std::vector<Value>{0, static_cast<Value>(1 << 10),
                                static_cast<Value>(1 << 20)}));
  // Even though there is a mismatch, we can still parse this fast.
  EXPECT_EQ(absl::string_view(trace_buf),
            ExpectedRepeatedFieldTrace(mt, field, 3));
}

TEST(RepeatedFieldTest, LongRepeatedField) {
  auto trace_buf = std::make_unique<std::array<char, 1024>>();
  using TypeParam = field_types::Fixed64;
  using Value = typename TypeParam::Value;
  upb::Arena msg_arena;
  upb::Arena mt_arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Packed, mt_arena.ptr());
  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
  wire_types::WireMessage wire_msg;
  std::vector<Value> expected;
  for (int i = 0; i < 256; ++i) {
    wire_msg.push_back({1, TypeParam::WireValue(i)});
    expected.push_back(i);
  }
  std::string payload = ToBinaryPayload(wire_msg);
  upb_DecodeStatus result = upb_DecodeWithTrace(
      payload.data(), payload.size(), msg, mt, nullptr, 0, msg_arena.ptr(),
      trace_buf->data(), trace_buf->size());
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetRepeatedField<Value>(msg, field), expected);

  // We can't easily check the trace here because the large array size will
  // force reallocations that cause fallbacks to the MiniTable decoder.
}

TYPED_TEST(PackedTest, DecodeTruncatedPackedFieldMaxLen) {
  char trace_buf[64];
  upb::Arena msg_arena;
  upb::Arena mt_arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Packed, mt_arena.ptr());
  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
  // Malformed payload with the maximum allowed varint length but only one byte
  // of data.
  std::string payload = "\012\xff\xff\xff\xff\x07\000\000\000\000";
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Malformed)
      << upb_DecodeStatus_String(result);
}

TYPED_TEST(PackedTest, DecodeTruncatedPackedFieldShortLength) {
  char trace_buf[64];
  upb::Arena msg_arena;
  upb::Arena mt_arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Packed, mt_arena.ptr());
  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
  // Malformed payload with the maximum allowed varint length but only one byte
  // of data.
  std::string payload = "\012\001";
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Malformed)
      << upb_DecodeStatus_String(result);
}

}  // namespace

}  // namespace test
}  // namespace upb
