// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/status.h"
#include "upb/base/status.hpp"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/message/accessors.h"
#include "upb/message/accessors.hpp"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/message.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/unknown_fields.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.hpp"
#include "upb/mini_descriptor/internal/modifiers.h"
#include "upb/mini_descriptor/link.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/test/test.upb.h"
#include "upb/test/test.upb_minitable.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_test.upb_minitable.h"
#include "upb/wire/encode.h"
#include "upb/wire/test_util/field_types.h"
#include "upb/wire/test_util/make_mini_table.h"
#include "upb/wire/test_util/wire_message.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace test {

namespace {

std::vector<int> GetDecodeOptionsToTest() {
#if UPB_FASTTABLE
  return {0, kUpb_DecodeOption_DisableFastTable};
#else
  return {0};
#endif
}

#ifndef NDEBUG
std::string GetExpectedConsecutiveUnknownsTrace(int options) {
#if UPB_FASTTABLE
  if (!(options & kUpb_DecodeOption_DisableFastTable)) {
    return "D";
  }
#endif
  return "M";
}
#endif

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

std::string FilteredTrace(absl::string_view trace) {
  std::string filtered;
  for (char c : trace) {
    if (!absl::ascii_islower(c)) filtered.push_back(c);
  }
  return filtered;
}

TYPED_TEST(FieldTypeTest, DecodeOptionalMaxValue) {
  char trace_buf[64];
  using Value = typename TypeParam::Value;
  upb::Arena arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Scalar, arena.ptr());
  upb_Message* msg = upb_Message_New(mt, arena.ptr());
  std::string payload = ToBinaryPayload(wire_types::WireMessage{
      {1, TypeParam::WireValue(Value(TypeParam::kMax))}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetOptionalField<Value>(msg, field), TypeParam::kMax);
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
      {1, TypeParam::WireValue(Value(TypeParam::kMin))}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetOptionalField<Value>(msg, field), TypeParam::kMin);
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
      {1, TypeParam::WireValue(Value(TypeParam::kMax))}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetOptionalField<Value>(msg, field), TypeParam::kMax);
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
    value = TypeParam::kMax;
  }
  upb::Arena msg_arena;
  upb::Arena mt_arena;
  auto [mt, field] = MiniTable::MakeSingleFieldTable<TypeParam>(
      1, kUpb_DecodeFast_Repeated, mt_arena.ptr());
  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
  std::string payload = ToBinaryPayload(wire_types::WireMessage{
      {1, TypeParam::WireValue(Value(TypeParam::kZero))},
      {1, TypeParam::WireValue(Value(TypeParam::kMin))},
      {1, TypeParam::WireValue(Value(TypeParam::kMax))},
  });
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetRepeatedField<Value>(msg, field),
            (std::vector<Value>{Value(TypeParam::kZero), Value(TypeParam::kMin),
                                Value(TypeParam::kMax)}));
  EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)),
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
  std::string packed_value =
      ToBinaryPayload(TypeParam::WireValue(TypeParam::kZero)) +
      ToBinaryPayload(TypeParam::WireValue(TypeParam::kMin)) +
      ToBinaryPayload(TypeParam::WireValue(TypeParam::kMax));
  std::string payload = ToBinaryPayload(
      wire_types::WireMessage{{1, wire_types::Delimited{packed_value}}});
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  EXPECT_EQ(GetRepeatedField<Value>(msg, field),
            (std::vector<Value>{Value(TypeParam::kZero), Value(TypeParam::kMin),
                                Value(TypeParam::kMax)}));
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
      ToBinaryPayload(TypeParam::WireValue(TypeParam::kZero)) +
      ToBinaryPayload(TypeParam::WireValue(TypeParam::kMin)) +
      // For varint fields, this will be a multi-byte varint, such that
      // truncating the last byte will result in an invalid varint.
      ToBinaryPayloadWithLongVarints(TypeParam::WireValue(TypeParam::kMax), 2,
                                     2);
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
  EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)),
            ExpectedRepeatedFieldTrace(mt, field, 3));
}

TEST(RepeatedFieldTest, RepeatedMessageFallback) {
  Arena mt_arena;
  Arena msg_arena;

  auto [sub_mt, sub_field] =
      test::MiniTable::MakeSingleFieldTable<test::field_types::Int32>(
          1, kUpb_DecodeFast_Scalar, mt_arena.ptr());

  auto [mt, field] =
      test::MiniTable::MakeSingleFieldTable<test::field_types::Message>(
          1, kUpb_DecodeFast_Repeated, mt_arena.ptr());

  const upb_MiniTable* subs[1] = {sub_mt};
  bool linked =
      upb_MiniTable_Link(const_cast<upb_MiniTable*>(mt), subs, 1, nullptr, 0);
  ASSERT_TRUE(linked);

  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

  // Payload:
  // Element 1: tag 1, len 2, int32 value 5
  // Element 2: tag 1, len 2 (parsed as overlong 3-byte varint to trigger
  // fasttable fallback), int32 value 6
  std::string payload("\x0a\x02\x08\x05\x0a\x82\x80\x00\x08\x06", 10);
  upb_DecodeStatus result = upb_Decode(payload.data(), payload.size(), msg, mt,
                                       nullptr, 0, msg_arena.ptr());

  // Fasttable fallback used to drop the first element for repeated messages
  // because array size wasn't updated.
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

  const upb_Array* arr = upb_Message_GetArray(msg, field);
  ASSERT_NE(arr, nullptr);
  EXPECT_EQ(upb_Array_Size(arr), 2u);
}

TEST(RepeatedFieldTest, RepeatedMessageLongVarintSizeFastPath) {
  char trace_buf[64];
  Arena mt_arena;
  Arena msg_arena;

  auto [sub_mt, sub_field] =
      test::MiniTable::MakeSingleFieldTable<test::field_types::Int32>(
          1, kUpb_DecodeFast_Scalar, mt_arena.ptr());

  auto [mt, field] =
      test::MiniTable::MakeSingleFieldTable<test::field_types::Message>(
          1, kUpb_DecodeFast_Repeated, mt_arena.ptr());

  const upb_MiniTable* subs[1] = {sub_mt};
  bool linked =
      upb_MiniTable_Link(const_cast<upb_MiniTable*>(mt), subs, 1, nullptr, 0);
  ASSERT_TRUE(linked);

  upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

  // Payload:
  // Element 1: tag 1, len 2, int32 value 5
  // Element 2: tag 1, len 2 (parsed as overlong 3-byte varint), int32 value 6
  std::string payload("\x0a\x02\x08\x05\x0a\x82\x80\x00\x08\x06", 10);
  upb_DecodeStatus result =
      upb_DecodeWithTrace(payload.data(), payload.size(), msg, mt, nullptr, 0,
                          msg_arena.ptr(), trace_buf, sizeof(trace_buf));

  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
#if !defined(NDEBUG)
#if UPB_FASTTABLE
  EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)), "DDFFDFF");
#else
  EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)), "MMMM");
#endif
#endif
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
TEST(DecodeTest, EmptyMiniTableDecodedAsUnknown) {
  Arena mt_arena;
  Arena msg_arena;

  upb_MiniTable* empty_mt =
      (upb_MiniTable*)upb_Arena_Malloc(mt_arena.ptr(), sizeof(upb_MiniTable));
  memset(empty_mt, 0, sizeof(upb_MiniTable));
  empty_mt->UPB_PRIVATE(size) = sizeof(upb_Message);
  empty_mt->UPB_ONLYBITS(field_count) = 0;

  upb_Message* msg = upb_Message_New(empty_mt, msg_arena.ptr());

  // An arbitrary payload that should be parsed as unknown:
  // field 1, length-delimited, length 2, data="\x08\x05"
  std::string payload("\x0a\x02\x08\x05");

  upb_DecodeStatus result = upb_Decode(payload.data(), payload.size(), msg,
                                       empty_mt, nullptr, 0, msg_arena.ptr());

  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

  EXPECT_TRUE(upb_Message_HasUnknown(msg));

  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_MessageUnknown data;
  ASSERT_TRUE(upb_Message_NextUnknown2(msg, &data, &iter));
  ASSERT_EQ(data.type, kUpb_MessageUnknownType_StringView);
  EXPECT_EQ(absl::string_view(data.value.bytes.data, data.value.bytes.size),
            payload);
  EXPECT_FALSE(upb_Message_NextUnknown2(msg, &data, &iter));
}

TEST(DecodeTest, ConsecutiveUnknownFieldsWithoutAlias) {
  char trace_buf[64];
  Arena mt_arena;

  auto [mt, field] = MiniTable::MakeSingleFieldTable<field_types::Int32>(
      1, kUpb_DecodeFast_Scalar, mt_arena.ptr());

  // Field 2: tag 2, varint, value 2  -> \x10\x02
  // Field 3: tag 3, varint, value 3  -> \x18\x03
  absl::string_view payload("\x10\x02\x18\x03", 4);

  for (int options : GetDecodeOptionsToTest()) {
    Arena msg_arena;
    upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
    memset(trace_buf, 0, sizeof(trace_buf));

    upb_DecodeStatus result = upb_DecodeWithTrace(
        payload.data(), payload.size(), msg, mt, nullptr, options,
        msg_arena.ptr(), trace_buf, sizeof(trace_buf));

    ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

    EXPECT_TRUE(upb_Message_HasUnknown(msg));

    uintptr_t iter = kUpb_Message_UnknownBegin;
    upb_MessageUnknown data;

    // We expect them to be merged.
    ASSERT_TRUE(upb_Message_NextUnknown2(msg, &data, &iter));
    ASSERT_EQ(data.type, kUpb_MessageUnknownType_StringView);
    EXPECT_EQ(absl::string_view(data.value.bytes.data, data.value.bytes.size),
              payload);
    EXPECT_FALSE(upb_Message_NextUnknown2(msg, &data, &iter));

#ifndef NDEBUG
    // Assert that consecutive unknown fields optimization took effect, decoding
    // both unknown fields in a single step (trace "M" instead of "MM").
    EXPECT_EQ(absl::string_view(trace_buf),
              GetExpectedConsecutiveUnknownsTrace(options));
#endif
  }
}

TEST(DecodeTest, ConsecutiveUnknownFieldsWithAlias) {
  char trace_buf[64];
  Arena mt_arena;

  auto [mt, field] = MiniTable::MakeSingleFieldTable<field_types::Int32>(
      1, kUpb_DecodeFast_Scalar, mt_arena.ptr());

  // Field 2: tag 2, varint, value 2  -> \x10\x02
  // Field 3: tag 3, varint, value 3  -> \x18\x03
  absl::string_view payload("\x10\x02\x18\x03", 4);

  for (int extra_options : GetDecodeOptionsToTest()) {
    Arena msg_arena;
    upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
    memset(trace_buf, 0, sizeof(trace_buf));

    int options = extra_options | kUpb_DecodeOption_AliasString;
    upb_DecodeStatus result = upb_DecodeWithTrace(
        payload.data(), payload.size(), msg, mt, nullptr, options,
        msg_arena.ptr(), trace_buf, sizeof(trace_buf));

    ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

    EXPECT_TRUE(upb_Message_HasUnknown(msg));

    uintptr_t iter = kUpb_Message_UnknownBegin;
    upb_MessageUnknown data;

    ASSERT_TRUE(upb_Message_NextUnknown2(msg, &data, &iter));
    ASSERT_EQ(data.type, kUpb_MessageUnknownType_StringView);
    EXPECT_EQ(absl::string_view(data.value.bytes.data, data.value.bytes.size),
              payload);
    EXPECT_FALSE(upb_Message_NextUnknown2(msg, &data, &iter));

#ifndef NDEBUG
    EXPECT_EQ(absl::string_view(trace_buf),
              GetExpectedConsecutiveUnknownsTrace(options));
#endif
  }
}

TEST(DecodeTest, MaxDepthPayloadParsesSuccessfully) {
  upb::Arena mt_arena;
  upb::Arena msg_arena;

  // Construct recursive message to allow testing arbitrary depths.
  auto [mt, field] =
      test::MiniTable::MakeSingleFieldTable<test::field_types::Message>(
          1, kUpb_DecodeFast_Scalar, mt_arena.ptr());
  const upb_MiniTable* subs[1] = {mt};  // Submessage is of own type.
  bool linked =
      upb_MiniTable_Link(const_cast<upb_MiniTable*>(mt), subs, 1, nullptr, 0);
  ASSERT_TRUE(linked);

  // We'll set a small depth limit to make it easy to test.
  const int kMaxDepth = 10;
  int options = upb_Decode_LimitDepth(0, kMaxDepth);

  auto make_payload = [](int depth) {
    std::string payload;
    for (int i = 0; i < depth; ++i) {
      // field 1, delimited
      payload += '\n';
      // length (remaining payload)
      // Each level adds 2 bytes (tag + length byte).
      payload.push_back(static_cast<char>((depth - i - 1) * 2));
    }
    return payload;
  };

  // Test depth kMaxDepth - should succeed.
  {
    std::string payload = make_payload(kMaxDepth);
    upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
    upb_DecodeStatus result = upb_Decode(payload.data(), payload.size(), msg,
                                         mt, nullptr, options, msg_arena.ptr());
    EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
  }

  // Test depth kMaxDepth + 1 - should fail.
  {
    std::string payload = make_payload(kMaxDepth + 1);
    upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
    upb_DecodeStatus result = upb_Decode(payload.data(), payload.size(), msg,
                                         mt, nullptr, options, msg_arena.ptr());
    EXPECT_EQ(result, kUpb_DecodeStatus_MaxDepthExceeded)
        << upb_DecodeStatus_String(result);
  }
}

TEST(DecodeTest, DecodeNonCanonicalExtensionAsUnknown) {
  upb::Arena arena;

  // 1. Create base msg which starts empty
  upb_test_ModelWithExtensions* msg =
      upb_test_ModelWithExtensions_new(arena.ptr());

  // 2. Create parsed submessage ("World")
  upb_Message* extension1 =
      UPB_UPCAST(upb_test_ModelExtension1_new(arena.ptr()));
  upb_test_ModelExtension1_set_str((upb_test_ModelExtension1*)extension1,
                                   upb_StringView_FromString("World"));

  // 3. msg has a non-canonical extension A
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg), upb_test_ModelExtension1_model_ext_ext, &extension1,
      arena.ptr()));

  // Verify extension count is 0 before encoding/decoding.
  EXPECT_EQ((int)upb_Message_ExtensionCount(UPB_UPCAST(msg)), 0);

  // 5. Obtain encoded non-canonical extension A by serializing msg
  char* buf;
  size_t size;
  upb_EncodeStatus enc_status =
      upb_Encode(UPB_UPCAST(msg), &upb_0test__ModelWithExtensions_msg_init, 0,
                 arena.ptr(), &buf, &size);
  ASSERT_EQ(enc_status, kUpb_EncodeStatus_Ok);
  ASSERT_GT(size, 0u);

  // 6. Decode with extreg = nullptr (so the encoded extension A is decoded as
  // unknown bytes)
  upb_DecodeStatus dec_status = upb_Decode(
      buf, size, UPB_UPCAST(msg), &upb_0test__ModelWithExtensions_msg_init,
      /*extreg=*/nullptr, 0, arena.ptr());
  ASSERT_EQ(dec_status, kUpb_DecodeStatus_Ok);

  // 7. Verify that we end up with exactly one non-canonical extension A + one
  // unknown bytes block representing A
  int non_canonical_count = 0;
  int unknown_bytes_count = 0;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_MessageUnknown data;
  while (upb_Message_NextUnknown2(UPB_UPCAST(msg), &data, &iter)) {
    if (data.type == kUpb_MessageUnknownType_NonCanonicalExtension) {
      non_canonical_count++;
    } else if (data.type == kUpb_MessageUnknownType_StringView) {
      unknown_bytes_count++;
    }
  }
  EXPECT_EQ(non_canonical_count, 1);
  EXPECT_EQ(unknown_bytes_count, 1);

  // Verify extension APIs: there are zero canonical extensions.
  EXPECT_EQ((int)upb_Message_ExtensionCount(UPB_UPCAST(msg)), 0);
  uintptr_t ext_iter = kUpb_Message_ExtensionBegin;
  const upb_MiniTableExtension* ext_out = nullptr;
  upb_MessageValue val_out;
  EXPECT_FALSE(upb_Message_NextExtension(UPB_UPCAST(msg), &ext_out, &val_out,
                                         &ext_iter));
}

TEST(DecodeTest, DecodeExtensionAsUnknownWithPreexistingUnknown) {
  upb::Arena arena;

  // 1. Create a temporary message to serialize the extension
  upb_test_ModelWithExtensions* tmp_msg =
      upb_test_ModelWithExtensions_new(arena.ptr());

  // 2. Create parsed submessage ("World")
  upb_Message* extension1 =
      UPB_UPCAST(upb_test_ModelExtension1_new(arena.ptr()));
  upb_test_ModelExtension1_set_str((upb_test_ModelExtension1*)extension1,
                                   upb_StringView_FromString("World"));

  // 3. Attach to tmp_msg as a non-canonical extension so we can serialize it to
  // get the bytes
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(tmp_msg), upb_test_ModelExtension1_model_ext_ext, &extension1,
      arena.ptr()));

  // 5. Obtain encoded extension A by serializing tmp_msg
  char* buf;
  size_t size;
  upb_EncodeStatus enc_status =
      upb_Encode(UPB_UPCAST(tmp_msg), &upb_0test__ModelWithExtensions_msg_init,
                 0, arena.ptr(), &buf, &size);
  ASSERT_EQ(enc_status, kUpb_EncodeStatus_Ok);
  ASSERT_GT(size, 0u);

  // 6. Create destination message and put the serialized bytes as an unknown
  // field on msg
  upb_test_ModelWithExtensions* msg =
      upb_test_ModelWithExtensions_new(arena.ptr());
  bool add_ok = UPB_PRIVATE(_upb_Message_AddUnknown)(
      UPB_UPCAST(msg), buf, size, arena.ptr(), kUpb_AddUnknown_Alias);
  ASSERT_TRUE(add_ok);

  // Verify extension count is 0 before decoding.
  EXPECT_EQ((int)upb_Message_ExtensionCount(UPB_UPCAST(msg)), 0);

  // 7. Decode with extreg = nullptr (so the encoded extension A is decoded as
  // unknown bytes)
  upb_DecodeStatus dec_status = upb_Decode(
      buf, size, UPB_UPCAST(msg), &upb_0test__ModelWithExtensions_msg_init,
      /*extreg=*/nullptr, 0, arena.ptr());
  ASSERT_EQ(dec_status, kUpb_DecodeStatus_Ok);

  // 8. Verify that we end up with exactly two unknown bytes blocks representing
  // A
  int non_canonical_count = 0;
  int unknown_bytes_count = 0;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_MessageUnknown data;
  while (upb_Message_NextUnknown2(UPB_UPCAST(msg), &data, &iter)) {
    if (data.type == kUpb_MessageUnknownType_NonCanonicalExtension) {
      non_canonical_count++;
    } else if (data.type == kUpb_MessageUnknownType_StringView) {
      unknown_bytes_count++;
    }
  }
  EXPECT_EQ(non_canonical_count, 0);
  EXPECT_EQ(unknown_bytes_count, 2);

  // Verify extension APIs: there are zero canonical extensions.
  EXPECT_EQ((int)upb_Message_ExtensionCount(UPB_UPCAST(msg)), 0);
  uintptr_t ext_iter = kUpb_Message_ExtensionBegin;
  const upb_MiniTableExtension* ext_out = nullptr;
  upb_MessageValue val_out;
  EXPECT_FALSE(upb_Message_NextExtension(UPB_UPCAST(msg), &ext_out, &val_out,
                                         &ext_iter));
}

TEST(DecodeTest, DecodeGroupFieldFromDelimitedWireFormatAsUnknown) {
  upb::Arena mt_arena;
  upb::Arena msg_arena;

  // 1. Create Parent MiniTable containing a repeated Group field directly.
  auto [parent_mt, parent_field] =
      test::MiniTable::MakeSingleFieldTable<test::field_types::Group>(
          5, kUpb_DecodeFast_Repeated, mt_arena.ptr());

  // 2. Build length-delimited wire payload for Group field 5:
  // Tag 5 Delimited = 42 (0x2a), length = 2, child field 1 = 123 ("\x08\x7b").
  absl::string_view payload("\x2a\x02\x08\x7b", 4);

  // 3. Parse the payload into Parent Message.
  upb_Message* parent_msg = upb_Message_New(parent_mt, msg_arena.ptr());
  upb_DecodeStatus result =
      upb_Decode(payload.data(), payload.size(), parent_msg, parent_mt, nullptr,
                 0, msg_arena.ptr());

  // 4. Verify parsing succeeded cleanly.
  ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

  // 5. Verify repeated Group field 5 was NOT populated as a known field.
  const upb_Array* arr = upb_Message_GetArray(parent_msg, parent_field);
  EXPECT_EQ(arr, nullptr);

  // 6. Verify the wire payload was instead preserved inside the Unknown field
  // set.
  EXPECT_TRUE(upb_Message_HasUnknown(parent_msg));

  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_MessageUnknown data;
  ASSERT_TRUE(upb_Message_NextUnknown2(parent_msg, &data, &iter));
  ASSERT_EQ(data.type, kUpb_MessageUnknownType_StringView);
  EXPECT_EQ(absl::string_view(data.value.bytes.data, data.value.bytes.size),
            payload);
  EXPECT_FALSE(upb_Message_NextUnknown2(parent_msg, &data, &iter));
}

TEST(DecodeTest, ConsecutiveUnknownFieldsWithGroup) {
  char trace_buf[64];
  Arena mt_arena;

  auto [mt, field] = MiniTable::MakeSingleFieldTable<field_types::Int32>(
      1, kUpb_DecodeFast_Scalar, mt_arena.ptr());

  // Field 2: StartGroup -> \x13
  //   Field 3: Varint, value 123 -> \x18\x7b
  // Field 2: EndGroup -> \x14
  // Field 4: Varint, value 456 -> \x20\xc8\x03
  absl::string_view payload("\x13\x18\x7b\x14\x20\xc8\x03", 7);

  for (int options : GetDecodeOptionsToTest()) {
    Arena msg_arena;
    upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
    memset(trace_buf, 0, sizeof(trace_buf));

    upb_DecodeStatus result = upb_DecodeWithTrace(
        payload.data(), payload.size(), msg, mt, nullptr, options,
        msg_arena.ptr(), trace_buf, sizeof(trace_buf));

    ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

    EXPECT_TRUE(upb_Message_HasUnknown(msg));

    uintptr_t iter = kUpb_Message_UnknownBegin;
    upb_MessageUnknown data;

    // We expect them to be merged.
    ASSERT_TRUE(upb_Message_NextUnknown2(msg, &data, &iter));
    ASSERT_EQ(data.type, kUpb_MessageUnknownType_StringView);
    EXPECT_EQ(absl::string_view(data.value.bytes.data, data.value.bytes.size),
              payload);
    EXPECT_FALSE(upb_Message_NextUnknown2(msg, &data, &iter));

#ifndef NDEBUG
    const char* expected = "M";
#if UPB_FASTTABLE
    if (!(options & kUpb_DecodeOption_DisableFastTable)) {
      expected = "D<M";
    }
#endif
    EXPECT_EQ(absl::string_view(trace_buf), expected);
#endif
  }
}

TEST(DecodeTest, MessageSetConsecutiveUnknowns) {
  Arena mt_arena;

  const upb_MiniTable* mset_mt = &upb_0decode_0test__TestMessageSet_msg_init;
  const upb_MiniTableExtension* ext = upb_decode_test_ext_message_set_ext;

  upb_ExtensionRegistry* reg = upb_ExtensionRegistry_New(mt_arena.ptr());
  ASSERT_TRUE(reg != nullptr);
  EXPECT_EQ(upb_ExtensionRegistry_Add(reg, ext),
            kUpb_ExtensionRegistryStatus_Ok);

  // 5. Construct the payload.
  // Field 10 (Varint, value 1) is unknown: \x50\x01
  // Field 1 (StartGroup, representing the MessageSet Item)
  //   Inside the group:
  //   Field 2 (type_id = 2000): \x10\xd0\x0f
  //   Field 3 (message: empty message, length 0): \x1a\x00
  // Field 1 (EndGroup): \x0c
  std::string payload("\x50\x01\x0b\x10\xd0\x0f\x1a\x00\x0c", 9);

  for (int options : GetDecodeOptionsToTest()) {
    Arena msg_arena;
    upb_Message* msg = upb_Message_New(mset_mt, msg_arena.ptr());
    ASSERT_TRUE(msg != nullptr);

    // Parse the payload.
    upb_DecodeStatus result =
        upb_Decode(payload.data(), payload.size(), msg, mset_mt, reg, options,
                   msg_arena.ptr());

    ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

    // Check if the extension was successfully parsed.
    EXPECT_TRUE(upb_Message_HasExtension(msg, ext));
  }
}

TEST(DecodeTest, FieldZeroRejected) {
  Arena mt_arena;

  // 1. Empty message with field 0 varint payload.
  {
    upb_MiniTable* empty_mt =
        (upb_MiniTable*)upb_Arena_Malloc(mt_arena.ptr(), sizeof(upb_MiniTable));
    memset(empty_mt, 0, sizeof(upb_MiniTable));
    empty_mt->UPB_PRIVATE(size) = sizeof(upb_Message);
    empty_mt->UPB_ONLYBITS(field_count) = 0;

    std::string payload("\x00\x00", 2);
    for (int options : GetDecodeOptionsToTest()) {
      Arena msg_arena;
      upb_Message* msg = upb_Message_New(empty_mt, msg_arena.ptr());
      upb_DecodeStatus result =
          upb_Decode(payload.data(), payload.size(), msg, empty_mt, nullptr,
                     options, msg_arena.ptr());
      EXPECT_EQ(result, kUpb_DecodeStatus_Malformed);
    }
  }

  // 2. Field 0 varint inside unknown group.
  {
    auto [mt, field] = MiniTable::MakeSingleFieldTable<field_types::Int32>(
        1, kUpb_DecodeFast_Scalar, mt_arena.ptr());

    // Field 2 (StartGroup) containing Field 0 varint.
    std::string payload("\x13\x00\x00\x14", 4);
    for (int options : GetDecodeOptionsToTest()) {
      Arena msg_arena;
      upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
      upb_DecodeStatus result =
          upb_Decode(payload.data(), payload.size(), msg, mt, nullptr, options,
                     msg_arena.ptr());
      EXPECT_EQ(result, kUpb_DecodeStatus_Malformed);
    }
  }

  // 3. Field 0 varint inside MessageSet item.
  {
    const upb_MiniTable* mset_mt = &upb_0decode_0test__TestMessageSet_msg_init;
    // Field 1 (StartGroup for MessageSet Item) containing Field 0 varint.
    std::string payload("\x0b\x00\x00\x0c", 4);
    for (int options : GetDecodeOptionsToTest()) {
      Arena msg_arena;
      upb_Message* msg = upb_Message_New(mset_mt, msg_arena.ptr());
      upb_DecodeStatus result =
          upb_Decode(payload.data(), payload.size(), msg, mset_mt, nullptr,
                     options, msg_arena.ptr());
      EXPECT_EQ(result, kUpb_DecodeStatus_Malformed);
    }
  }
}

TEST(DecodeTest, UnlinkedSubMessageFastTableSlotCollision) {
  Arena mt_arena;

  // Build a message where:
  // - Field 16 is an unlinked submessage (slot 16)
  // - Field 32 is a bool field that collides on the same fasttable slot (slot
  // 16)
  upb::MtDataEncoder e;
  e.StartMessage(0);
  e.PutField(kUpb_FieldType_Message, 16, 0);
  e.PutField(kUpb_FieldType_Bool, 32, 0);

  upb_Status status;
  upb_Status_Clear(&status);
  const upb_MiniTable* mt = upb_MiniTable_Build(
      e.data().data(), e.data().size(), mt_arena.ptr(), &status);
  ASSERT_TRUE(upb_Status_IsOk(&status)) << upb_Status_ErrorMessage(&status);

  const upb_MiniTableField* bool_field =
      upb_MiniTable_FindFieldByNumber(mt, 32);
  ASSERT_NE(bool_field, nullptr);

  // Field 32 (tag 256, varint: 0x80, 0x02), value = 1 (true)
  std::string payload("\x80\x02\x01");
  for (int options : GetDecodeOptionsToTest()) {
    Arena msg_arena;
    upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());
    upb_DecodeStatus result = upb_Decode(payload.data(), payload.size(), msg,
                                         mt, nullptr, options, msg_arena.ptr());
    EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
    EXPECT_TRUE(upb_Message_GetBool(msg, bool_field, false));
    EXPECT_FALSE(upb_Message_HasUnknown(msg));
  }
}

TEST(DecodeTest, DecodeMapInt32Int32SingleAndConsecutive) {
  const upb_MiniTable* mt = &upb_0test__ModelWithMaps_msg_init;

  Arena enc_arena;
  upb_test_ModelWithMaps* src_single =
      upb_test_ModelWithMaps_new(enc_arena.ptr());
  upb_test_ModelWithMaps_map_ii_set(src_single, 10, 100, enc_arena.ptr());
  size_t single_size;
  char* single_buf = upb_test_ModelWithMaps_serialize(
      src_single, enc_arena.ptr(), &single_size);
  ASSERT_NE(single_buf, nullptr);
  absl::string_view single_payload(single_buf, single_size);

  upb_test_ModelWithMaps* src_multi =
      upb_test_ModelWithMaps_new(enc_arena.ptr());
  upb_test_ModelWithMaps_map_ii_set(src_multi, 10, 100, enc_arena.ptr());
  upb_test_ModelWithMaps_map_ii_set(src_multi, 20, 200, enc_arena.ptr());
  upb_test_ModelWithMaps_map_ii_set(src_multi, 30, 300, enc_arena.ptr());
  size_t multi_size;
  char* multi_buf =
      upb_test_ModelWithMaps_serialize(src_multi, enc_arena.ptr(), &multi_size);
  ASSERT_NE(multi_buf, nullptr);
  absl::string_view multi_payload(multi_buf, multi_size);

  for (int options : GetDecodeOptionsToTest()) {
    // Single entry test
    {
      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_test_ModelWithMaps* msg = upb_test_ModelWithMaps_new(msg_arena.ptr());
      upb_DecodeStatus result = upb_DecodeWithTrace(
          single_payload.data(), single_payload.size(), UPB_UPCAST(msg), mt,
          nullptr, options, msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      ASSERT_EQ(result, kUpb_DecodeStatus_Ok)
          << upb_DecodeStatus_String(result);

      const upb_Map* map = _upb_test_ModelWithMaps_map_ii_upb_map(msg);
      ASSERT_NE(map, nullptr);
      EXPECT_EQ(upb_Map_Size(map), 1);
      upb_MessageValue val;
      EXPECT_TRUE(upb_Map_Get(map, upb_MessageValue{.int32_val = 10}, &val));
      EXPECT_EQ(val.int32_val, 100);

#ifndef NDEBUG
      std::string expected_trace = "MMM";
#if UPB_FASTTABLE
      if (!(options & kUpb_DecodeOption_DisableFastTable)) {
        expected_trace = "DF";
      }
#endif
      EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)), expected_trace);
#endif
    }

    // Consecutive entries test (loopback)
    {
      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_test_ModelWithMaps* msg = upb_test_ModelWithMaps_new(msg_arena.ptr());
      upb_DecodeStatus result = upb_DecodeWithTrace(
          multi_payload.data(), multi_payload.size(), UPB_UPCAST(msg), mt,
          nullptr, options, msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      ASSERT_EQ(result, kUpb_DecodeStatus_Ok)
          << upb_DecodeStatus_String(result);

      const upb_Map* map = _upb_test_ModelWithMaps_map_ii_upb_map(msg);
      ASSERT_NE(map, nullptr);
      EXPECT_EQ(upb_Map_Size(map), 3);
      upb_MessageValue val;
      EXPECT_TRUE(upb_Map_Get(map, upb_MessageValue{.int32_val = 10}, &val));
      EXPECT_EQ(val.int32_val, 100);
      EXPECT_TRUE(upb_Map_Get(map, upb_MessageValue{.int32_val = 20}, &val));
      EXPECT_EQ(val.int32_val, 200);
      EXPECT_TRUE(upb_Map_Get(map, upb_MessageValue{.int32_val = 30}, &val));
      EXPECT_EQ(val.int32_val, 300);

#ifndef NDEBUG
      std::string expected_trace = "MMMMMMMMM";
#if UPB_FASTTABLE
      if (!(options & kUpb_DecodeOption_DisableFastTable)) {
        expected_trace = "DFFF";
      }
#endif
      EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)), expected_trace);
#endif
    }
  }
}

TEST(DecodeTest, DecodeMapStringStringSingleAndConsecutive) {
  const upb_MiniTable* mt = &upb_0test__ModelWithMaps_msg_init;

  Arena enc_arena;
  upb_test_ModelWithMaps* src_single =
      upb_test_ModelWithMaps_new(enc_arena.ptr());
  upb_test_ModelWithMaps_map_ss_set(src_single, upb_StringView_FromString("k1"),
                                    upb_StringView_FromString("v1"),
                                    enc_arena.ptr());
  size_t single_size;
  char* single_buf = upb_test_ModelWithMaps_serialize(
      src_single, enc_arena.ptr(), &single_size);
  ASSERT_NE(single_buf, nullptr);
  absl::string_view single_payload(single_buf, single_size);

  upb_test_ModelWithMaps* src_multi =
      upb_test_ModelWithMaps_new(enc_arena.ptr());
  upb_test_ModelWithMaps_map_ss_set(src_multi, upb_StringView_FromString("k1"),
                                    upb_StringView_FromString("v1"),
                                    enc_arena.ptr());
  upb_test_ModelWithMaps_map_ss_set(src_multi, upb_StringView_FromString("k2"),
                                    upb_StringView_FromString("v2"),
                                    enc_arena.ptr());
  size_t multi_size;
  char* multi_buf =
      upb_test_ModelWithMaps_serialize(src_multi, enc_arena.ptr(), &multi_size);
  ASSERT_NE(multi_buf, nullptr);
  absl::string_view multi_payload(multi_buf, multi_size);

  for (int options : GetDecodeOptionsToTest()) {
    // Single entry test
    {
      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_test_ModelWithMaps* msg = upb_test_ModelWithMaps_new(msg_arena.ptr());
      upb_DecodeStatus result = upb_DecodeWithTrace(
          single_payload.data(), single_payload.size(), UPB_UPCAST(msg), mt,
          nullptr, options, msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      ASSERT_EQ(result, kUpb_DecodeStatus_Ok)
          << upb_DecodeStatus_String(result);

      const upb_Map* map = _upb_test_ModelWithMaps_map_ss_upb_map(msg);
      ASSERT_NE(map, nullptr);
      EXPECT_EQ(upb_Map_Size(map), 1);
      upb_MessageValue val;
      EXPECT_TRUE(upb_Map_Get(
          map, upb_MessageValue{.str_val = upb_StringView_FromString("k1")},
          &val));
      EXPECT_EQ(absl::string_view(val.str_val.data, val.str_val.size), "v1");

#ifndef NDEBUG
      std::string expected_trace = "MMM";
#if UPB_FASTTABLE
      if (!(options & kUpb_DecodeOption_DisableFastTable)) {
        expected_trace = "DF";
      }
#endif
      EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)), expected_trace);
#endif
    }

    // Consecutive entries test (loopback)
    {
      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_test_ModelWithMaps* msg = upb_test_ModelWithMaps_new(msg_arena.ptr());
      upb_DecodeStatus result = upb_DecodeWithTrace(
          multi_payload.data(), multi_payload.size(), UPB_UPCAST(msg), mt,
          nullptr, options, msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      ASSERT_EQ(result, kUpb_DecodeStatus_Ok)
          << upb_DecodeStatus_String(result);

      const upb_Map* map = _upb_test_ModelWithMaps_map_ss_upb_map(msg);
      ASSERT_NE(map, nullptr);
      EXPECT_EQ(upb_Map_Size(map), 2);
      upb_MessageValue val;
      EXPECT_TRUE(upb_Map_Get(
          map, upb_MessageValue{.str_val = upb_StringView_FromString("k1")},
          &val));
      EXPECT_EQ(absl::string_view(val.str_val.data, val.str_val.size), "v1");
      EXPECT_TRUE(upb_Map_Get(
          map, upb_MessageValue{.str_val = upb_StringView_FromString("k2")},
          &val));
      EXPECT_EQ(absl::string_view(val.str_val.data, val.str_val.size), "v2");

#ifndef NDEBUG
      std::string expected_trace = "MMMMMM";
#if UPB_FASTTABLE
      if (!(options & kUpb_DecodeOption_DisableFastTable)) {
        expected_trace = "DFF";
      }
#endif
      EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)), expected_trace);
#endif
    }
  }
}

TEST(DecodeTest, DecodeMapInt32Message) {
  const upb_MiniTable* mt = &upb_0test__ModelWithMaps_msg_init;

  Arena enc_arena;
  upb_test_ModelWithMaps* src = upb_test_ModelWithMaps_new(enc_arena.ptr());
  upb_test_ModelWithExtensions* submsg =
      upb_test_ModelWithExtensions_new(enc_arena.ptr());
  upb_test_ModelWithExtensions_set_random_int32(submsg, 123);
  upb_test_ModelWithMaps_map_im_set(src, 10, submsg, enc_arena.ptr());

  size_t size;
  char* buf = upb_test_ModelWithMaps_serialize(src, enc_arena.ptr(), &size);
  ASSERT_NE(buf, nullptr);
  absl::string_view payload(buf, size);

  for (int options : GetDecodeOptionsToTest()) {
    Arena msg_arena;
    char trace_buf[64] = {0};
    upb_test_ModelWithMaps* msg = upb_test_ModelWithMaps_new(msg_arena.ptr());
    upb_DecodeStatus result = upb_DecodeWithTrace(
        payload.data(), payload.size(), UPB_UPCAST(msg), mt, nullptr, options,
        msg_arena.ptr(), trace_buf, sizeof(trace_buf));
    ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

    const upb_Map* map = _upb_test_ModelWithMaps_map_im_upb_map(msg);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(upb_Map_Size(map), 1);
    upb_MessageValue val;
    EXPECT_TRUE(upb_Map_Get(map, upb_MessageValue{.int32_val = 10}, &val));
    ASSERT_NE(val.msg_val, nullptr);
    const upb_test_ModelWithExtensions* decoded_sub =
        (const upb_test_ModelWithExtensions*)val.msg_val;
    EXPECT_EQ(upb_test_ModelWithExtensions_random_int32(decoded_sub), 123);

#ifndef NDEBUG
    std::string expected_trace = "MMMM";
#if UPB_FASTTABLE
    if (!(options & kUpb_DecodeOption_DisableFastTable)) {
      expected_trace = "DDFF";
    }
#endif
    EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)), expected_trace);
#endif
  }
}

TEST(DecodeTest, DecodeMapUnknownFieldFallback) {
  const upb_MiniTable* mt = &upb_0test__ModelWithMaps_msg_init;

  // Field 4 (tag 0x22):
  // Entry with key=10 (\x08\x0a), val=100 (\x10\x64), and unknown field 3
  // (\x18\x03) len 6 -> \x22\x06\x08\x0a\x10\x64\x18\x03
  absl::string_view payload("\x22\x06\x08\x0a\x10\x64\x18\x03", 8);

  for (int options : GetDecodeOptionsToTest()) {
    Arena msg_arena;
    char trace_buf[64] = {0};
    upb_test_ModelWithMaps* msg = upb_test_ModelWithMaps_new(msg_arena.ptr());
    upb_DecodeStatus result = upb_DecodeWithTrace(
        payload.data(), payload.size(), UPB_UPCAST(msg), mt, nullptr, options,
        msg_arena.ptr(), trace_buf, sizeof(trace_buf));
    ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

    const upb_Map* map = _upb_test_ModelWithMaps_map_ii_upb_map(msg);
    // When a map entry has unknown fields, the entry is not added to the map.
    EXPECT_TRUE(map == nullptr || upb_Map_Size(map) == 0);

    // The unknown field inside the map entry is retained in message unknowns.
    EXPECT_TRUE(upb_Message_HasUnknown(UPB_UPCAST(msg)));

#ifndef NDEBUG
    std::string expected_trace = "MMMM";
#if UPB_FASTTABLE
    if (!(options & kUpb_DecodeOption_DisableFastTable)) {
      expected_trace = "D<MMMM";
    }
#endif
    EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)), expected_trace);
#endif
  }
}

static const upb_MiniTable* CreateDynamicMapTable(
    int field_number, upb_FieldType key_type, upb_FieldType val_type,
    upb_Arena* arena, const upb_MiniTable* val_submsg = nullptr) {
  uint64_t key_mod =
      (key_type == kUpb_FieldType_String) ? kUpb_FieldModifier_ValidateUtf8 : 0;
  uint64_t val_mod =
      (val_type == kUpb_FieldType_String) ? kUpb_FieldModifier_ValidateUtf8 : 0;
  upb::MtDataEncoder entry_enc;
  entry_enc.EncodeMap(key_type, val_type, key_mod, val_mod);
  upb::Status status;
  upb_MiniTable* entry_table = upb_MiniTable_Build(
      entry_enc.data().data(), entry_enc.data().size(), arena, status.ptr());
  UPB_ASSERT(status.ok());
  if (val_submsg) {
    upb_MiniTableField* val_f = const_cast<upb_MiniTableField*>(
        upb_MiniTable_GetFieldByIndex(entry_table, 1));
    bool ok = upb_MiniTable_SetSubMessage(entry_table, val_f, val_submsg);
    UPB_ASSERT(ok);
  }

  upb::MtDataEncoder msg_enc;
  msg_enc.StartMessage(0);
  msg_enc.PutField(kUpb_FieldType_Message, field_number,
                   kUpb_FieldModifier_IsRepeated);
  upb_MiniTable* msg_table = upb_MiniTable_Build(
      msg_enc.data().data(), msg_enc.data().size(), arena, status.ptr());
  UPB_ASSERT(status.ok());

  upb_MiniTableField* map_f = const_cast<upb_MiniTableField*>(
      upb_MiniTable_GetFieldByIndex(msg_table, 0));
  bool ok = upb_MiniTable_SetSubMessage(msg_table, map_f, entry_table);
  UPB_ASSERT(ok);

  return msg_table;
}

template <typename K, typename V>
std::string SerializeDynamicMap(const upb_MiniTable* mt, K key, V val,
                                upb_Arena* arena) {
  upb_Message* src = upb_Message_New(mt, arena);
  const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(mt, 0);
  const upb_MiniTable* entry_mt = upb_MiniTable_GetSubMessageTable(f);
  upb_Map* map = upb_Message_GetOrCreateMutableMap(src, entry_mt, f, arena);
  upb_MessageValue kv{}, vv{};
  if constexpr (std::is_same_v<K, int32_t>)
    kv.int32_val = key;
  else if constexpr (std::is_same_v<K, int64_t>)
    kv.int64_val = key;
  else if constexpr (std::is_same_v<K, uint32_t>)
    kv.uint32_val = key;
  else if constexpr (std::is_same_v<K, uint64_t>)
    kv.uint64_val = key;
  else if constexpr (std::is_same_v<K, upb_StringView>)
    kv.str_val = key;

  if constexpr (std::is_same_v<V, int32_t>)
    vv.int32_val = val;
  else if constexpr (std::is_same_v<V, int64_t>)
    vv.int64_val = val;
  else if constexpr (std::is_same_v<V, uint32_t>)
    vv.uint32_val = val;
  else if constexpr (std::is_same_v<V, uint64_t>)
    vv.uint64_val = val;
  else if constexpr (std::is_same_v<V, float>)
    vv.float_val = val;
  else if constexpr (std::is_same_v<V, double>)
    vv.double_val = val;
  else if constexpr (std::is_same_v<V, bool>)
    vv.bool_val = val;
  else if constexpr (std::is_same_v<V, upb_StringView>)
    vv.str_val = val;
  else if constexpr (std::is_same_v<V, const upb_Message*> ||
                     std::is_same_v<V, upb_Message*>)
    vv.msg_val = val;

  upb_Map_Set(map, kv, vv, arena);

  char* buf;
  size_t size;
  upb_EncodeStatus status = upb_Encode(src, mt, 0, arena, &buf, &size);
  UPB_ASSERT(status == kUpb_EncodeStatus_Ok);
  return std::string(buf, size);
}

TEST(DecodeTest, DecodeMapTag2Byte) {
  for (int options : GetDecodeOptionsToTest()) {
    // IntMap Tag2Byte (field 16)
    {
      Arena mt_arena;
      const upb_MiniTable* mt = CreateDynamicMapTable(
          16, kUpb_FieldType_Int32, kUpb_FieldType_Int32, mt_arena.ptr());
      std::string payload =
          SerializeDynamicMap<int32_t, int32_t>(mt, 10, 100, mt_arena.ptr());
      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

      upb_DecodeStatus result = upb_DecodeWithTrace(
          payload.data(), payload.size(), msg, mt, nullptr, options,
          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      ASSERT_EQ(result, kUpb_DecodeStatus_Ok)
          << upb_DecodeStatus_String(result);

      const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(mt, 0);
      const upb_Map* map = upb_Message_GetMap(msg, f);
      ASSERT_NE(map, nullptr);
      EXPECT_EQ(upb_Map_Size(map), 1);
      upb_MessageValue val;
      EXPECT_TRUE(upb_Map_Get(map, upb_MessageValue{.int32_val = 10}, &val));
      EXPECT_EQ(val.int32_val, 100);

#ifndef NDEBUG
      std::string expected_trace = "MMM";
#if UPB_FASTTABLE
      if (!(options & kUpb_DecodeOption_DisableFastTable)) {
        expected_trace = "DF";
      }
#endif
      EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)), expected_trace);
#endif
    }

    // StrMap Tag2Byte (field 17)
    {
      Arena mt_arena;
      const upb_MiniTable* mt = CreateDynamicMapTable(
          17, kUpb_FieldType_String, kUpb_FieldType_String, mt_arena.ptr());
      std::string payload = SerializeDynamicMap<upb_StringView, upb_StringView>(
          mt, upb_StringView_FromString("k1"), upb_StringView_FromString("v1"),
          mt_arena.ptr());
      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

      upb_DecodeStatus result = upb_DecodeWithTrace(
          payload.data(), payload.size(), msg, mt, nullptr, options,
          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      ASSERT_EQ(result, kUpb_DecodeStatus_Ok)
          << upb_DecodeStatus_String(result);

      const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(mt, 0);
      const upb_Map* map = upb_Message_GetMap(msg, f);
      ASSERT_NE(map, nullptr);
      EXPECT_EQ(upb_Map_Size(map), 1);
      upb_MessageValue val;
      EXPECT_TRUE(upb_Map_Get(
          map, upb_MessageValue{.str_val = upb_StringView_FromString("k1")},
          &val));
      EXPECT_EQ(absl::string_view(val.str_val.data, val.str_val.size), "v1");

#ifndef NDEBUG
      std::string expected_trace = "MMM";
#if UPB_FASTTABLE
      if (!(options & kUpb_DecodeOption_DisableFastTable)) {
        expected_trace = "DF";
      }
#endif
      EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)), expected_trace);
#endif
    }
  }
}

TEST(DecodeTest, DecodeMapZigZag) {
  Arena mt_arena;
  const upb_MiniTable* mt = CreateDynamicMapTable(
      1, kUpb_FieldType_SInt32, kUpb_FieldType_SInt32, mt_arena.ptr());
  std::string payload =
      SerializeDynamicMap<int32_t, int32_t>(mt, -10, -100, mt_arena.ptr());

  for (int options : GetDecodeOptionsToTest()) {
    Arena msg_arena;
    char trace_buf[64] = {0};
    upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

    upb_DecodeStatus result = upb_DecodeWithTrace(
        payload.data(), payload.size(), msg, mt, nullptr, options,
        msg_arena.ptr(), trace_buf, sizeof(trace_buf));
    ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(mt, 0);
    const upb_Map* map = upb_Message_GetMap(msg, f);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(upb_Map_Size(map), 1);
    upb_MessageValue val;
    EXPECT_TRUE(upb_Map_Get(map, upb_MessageValue{.int32_val = -10}, &val));
    EXPECT_EQ(val.int32_val, -100);
  }
}

TEST(DecodeTest, DecodeMapFixed32AndFloat) {
  Arena mt_arena;
  const upb_MiniTable* mt = CreateDynamicMapTable(
      1, kUpb_FieldType_Fixed32, kUpb_FieldType_Float, mt_arena.ptr());
  std::string payload =
      SerializeDynamicMap<uint32_t, float>(mt, 123, 1.5f, mt_arena.ptr());

  for (int options : GetDecodeOptionsToTest()) {
    Arena msg_arena;
    char trace_buf[64] = {0};
    upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

    upb_DecodeStatus result = upb_DecodeWithTrace(
        payload.data(), payload.size(), msg, mt, nullptr, options,
        msg_arena.ptr(), trace_buf, sizeof(trace_buf));
    ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(mt, 0);
    const upb_Map* map = upb_Message_GetMap(msg, f);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(upb_Map_Size(map), 1);
    upb_MessageValue val;
    EXPECT_TRUE(upb_Map_Get(map, upb_MessageValue{.uint32_val = 123}, &val));
    EXPECT_EQ(val.float_val, 1.5f);
  }
}

TEST(DecodeTest, DecodeMapFixed64AndDouble) {
  Arena mt_arena;
  const upb_MiniTable* mt = CreateDynamicMapTable(
      1, kUpb_FieldType_Fixed64, kUpb_FieldType_Double, mt_arena.ptr());
  std::string payload =
      SerializeDynamicMap<uint64_t, double>(mt, 456, 2.5, mt_arena.ptr());

  for (int options : GetDecodeOptionsToTest()) {
    Arena msg_arena;
    char trace_buf[64] = {0};
    upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

    upb_DecodeStatus result = upb_DecodeWithTrace(
        payload.data(), payload.size(), msg, mt, nullptr, options,
        msg_arena.ptr(), trace_buf, sizeof(trace_buf));
    ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(mt, 0);
    const upb_Map* map = upb_Message_GetMap(msg, f);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(upb_Map_Size(map), 1);
    upb_MessageValue val;
    EXPECT_TRUE(upb_Map_Get(map, upb_MessageValue{.uint64_val = 456}, &val));
    EXPECT_EQ(val.double_val, 2.5);
  }
}

TEST(DecodeTest, DecodeMapInt64AndBool) {
  // Int64 -> Int64
  {
    Arena mt_arena;
    const upb_MiniTable* mt = CreateDynamicMapTable(
        1, kUpb_FieldType_Int64, kUpb_FieldType_Int64, mt_arena.ptr());
    std::string payload =
        SerializeDynamicMap<int64_t, int64_t>(mt, 10, 20, mt_arena.ptr());

    for (int options : GetDecodeOptionsToTest()) {
      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

      upb_DecodeStatus result = upb_DecodeWithTrace(
          payload.data(), payload.size(), msg, mt, nullptr, options,
          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      ASSERT_EQ(result, kUpb_DecodeStatus_Ok)
          << upb_DecodeStatus_String(result);

      const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(mt, 0);
      const upb_Map* map = upb_Message_GetMap(msg, f);
      ASSERT_NE(map, nullptr);
      EXPECT_EQ(upb_Map_Size(map), 1);
      upb_MessageValue val;
      EXPECT_TRUE(upb_Map_Get(map, upb_MessageValue{.int64_val = 10}, &val));
      EXPECT_EQ(val.int64_val, 20);
    }
  }

  // Int32 -> Bool
  {
    Arena mt_arena;
    const upb_MiniTable* mt = CreateDynamicMapTable(
        1, kUpb_FieldType_Int32, kUpb_FieldType_Bool, mt_arena.ptr());
    std::string payload =
        SerializeDynamicMap<int32_t, bool>(mt, 10, true, mt_arena.ptr());

    for (int options : GetDecodeOptionsToTest()) {
      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

      upb_DecodeStatus result = upb_DecodeWithTrace(
          payload.data(), payload.size(), msg, mt, nullptr, options,
          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      ASSERT_EQ(result, kUpb_DecodeStatus_Ok)
          << upb_DecodeStatus_String(result);

      const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(mt, 0);
      const upb_Map* map = upb_Message_GetMap(msg, f);
      ASSERT_NE(map, nullptr);
      EXPECT_EQ(upb_Map_Size(map), 1);
      upb_MessageValue val;
      EXPECT_TRUE(upb_Map_Get(map, upb_MessageValue{.int32_val = 10}, &val));
      EXPECT_EQ(val.bool_val, true);
    }
  }
}

TEST(DecodeTest, DecodeMapStringBytes) {
  Arena mt_arena;
  const upb_MiniTable* mt = CreateDynamicMapTable(
      1, kUpb_FieldType_String, kUpb_FieldType_Bytes, mt_arena.ptr());
  std::string payload = SerializeDynamicMap<upb_StringView, upb_StringView>(
      mt, upb_StringView_FromString("k1"),
      upb_StringView_FromDataAndSize("\xff\xfe", 2), mt_arena.ptr());

  for (int options : GetDecodeOptionsToTest()) {
    Arena msg_arena;
    char trace_buf[64] = {0};
    upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

    upb_DecodeStatus result = upb_DecodeWithTrace(
        payload.data(), payload.size(), msg, mt, nullptr, options,
        msg_arena.ptr(), trace_buf, sizeof(trace_buf));
    ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);

    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(mt, 0);
    const upb_Map* map = upb_Message_GetMap(msg, f);
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(upb_Map_Size(map), 1);
    upb_MessageValue val;
    EXPECT_TRUE(upb_Map_Get(
        map, upb_MessageValue{.str_val = upb_StringView_FromString("k1")},
        &val));
    EXPECT_EQ(absl::string_view(val.str_val.data, val.str_val.size),
              "\xff\xfe");
  }
}

TEST(DecodeTest, DecodeMapBadUtf8KeyAndValue) {
  Arena mt_arena;
  const upb_MiniTable* mt = CreateDynamicMapTable(
      1, kUpb_FieldType_String, kUpb_FieldType_String, mt_arena.ptr());

  for (int options : GetDecodeOptionsToTest()) {
    // Bad UTF-8 key
    {
      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

      // key = \xff\xfe, val = "v1"
      absl::string_view payload("\x0a\x08\x0a\x02\xff\xfe\x12\x02v1", 10);
      upb_DecodeStatus result = upb_DecodeWithTrace(
          payload.data(), payload.size(), msg, mt, nullptr, options,
          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      EXPECT_EQ(result, kUpb_DecodeStatus_BadUtf8);
    }

    // Bad UTF-8 value
    {
      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

      // key = "k1", val = \xff\xfe
      absl::string_view payload("\x0a\x08\x0a\x02k1\x12\x02\xff\xfe", 10);
      upb_DecodeStatus result = upb_DecodeWithTrace(
          payload.data(), payload.size(), msg, mt, nullptr, options,
          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      EXPECT_EQ(result, kUpb_DecodeStatus_BadUtf8);
    }
  }
}

TEST(DecodeTest, DecodeMapOutOfOrderTagsAndFallback) {
  const upb_MiniTable* mt = &upb_0test__ModelWithMaps_msg_init;

  for (int options : GetDecodeOptionsToTest()) {
    // Out-of-order tags: value (tag 2) before key (tag 1)
    {
      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_test_ModelWithMaps* msg = upb_test_ModelWithMaps_new(msg_arena.ptr());

      // Field 4 (tag 0x22):
      // Entry: val=100 (\x10\x64), key=10 (\x08\x0a) -> len 4 ->
      // \x22\x04\x10\x64\x08\x0a
      absl::string_view payload("\x22\x04\x10\x64\x08\x0a", 6);
      upb_DecodeStatus result = upb_DecodeWithTrace(
          payload.data(), payload.size(), UPB_UPCAST(msg), mt, nullptr, options,
          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      ASSERT_EQ(result, kUpb_DecodeStatus_Ok)
          << upb_DecodeStatus_String(result);

      const upb_Map* map = _upb_test_ModelWithMaps_map_ii_upb_map(msg);
      ASSERT_NE(map, nullptr);
      EXPECT_EQ(upb_Map_Size(map), 1);
      upb_MessageValue val;
      EXPECT_TRUE(upb_Map_Get(map, upb_MessageValue{.int32_val = 10}, &val));
      EXPECT_EQ(val.int32_val, 100);

#ifndef NDEBUG
      std::string expected_trace = "MMM";
#if UPB_FASTTABLE
      if (!(options & kUpb_DecodeOption_DisableFastTable)) {
        expected_trace = "D<MMM";
      }
#endif
      EXPECT_EQ(FilteredTrace(absl::string_view(trace_buf)), expected_trace);
#endif
    }

    // Empty map entry (len 0)
    {
      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_test_ModelWithMaps* msg = upb_test_ModelWithMaps_new(msg_arena.ptr());

      absl::string_view payload("\x22\x00", 2);
      upb_DecodeStatus result = upb_DecodeWithTrace(
          payload.data(), payload.size(), UPB_UPCAST(msg), mt, nullptr, options,
          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      ASSERT_EQ(result, kUpb_DecodeStatus_Ok)
          << upb_DecodeStatus_String(result);
    }
  }
}

TEST(DecodeTest, DecodeMapRequiredFieldsCheck) {
  const upb_MiniTable* mt = &upb_0test__ModelWithMaps_msg_init;

  for (int extra_options : GetDecodeOptionsToTest()) {
    int options = extra_options | kUpb_DecodeOption_CheckRequired;
    // Valid submessage containing required field
    {
      Arena enc_arena;
      upb_test_ModelWithMaps* src = upb_test_ModelWithMaps_new(enc_arena.ptr());
      upb_test_ModelWithRequiredFields* req =
          upb_test_ModelWithRequiredFields_new(enc_arena.ptr());
      upb_test_ModelWithRequiredFields_set_id(req, 1);
      upb_test_ModelWithMaps_map_im_required_set(src, 10, req, enc_arena.ptr());
      size_t size;
      char* buf = upb_test_ModelWithMaps_serialize(src, enc_arena.ptr(), &size);
      ASSERT_NE(buf, nullptr);
      absl::string_view payload(buf, size);

      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_test_ModelWithMaps* msg = upb_test_ModelWithMaps_new(msg_arena.ptr());
      upb_DecodeStatus result = upb_DecodeWithTrace(
          payload.data(), payload.size(), UPB_UPCAST(msg), mt, nullptr, options,
          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      ASSERT_EQ(result, kUpb_DecodeStatus_Ok)
          << upb_DecodeStatus_String(result);

      const upb_Map* map = _upb_test_ModelWithMaps_map_im_required_upb_map(msg);
      ASSERT_NE(map, nullptr);
      EXPECT_EQ(upb_Map_Size(map), 1);
    }

    // Invalid submessage missing required field
    {
      Arena enc_arena;
      upb_test_ModelWithMaps* src = upb_test_ModelWithMaps_new(enc_arena.ptr());
      upb_test_ModelWithRequiredFields* req =
          upb_test_ModelWithRequiredFields_new(enc_arena.ptr());
      // id is not set
      upb_test_ModelWithMaps_map_im_required_set(src, 10, req, enc_arena.ptr());
      size_t size;
      char* buf = upb_test_ModelWithMaps_serialize(src, enc_arena.ptr(), &size);
      ASSERT_NE(buf, nullptr);
      absl::string_view payload(buf, size);

      Arena msg_arena;
      char trace_buf[64] = {0};
      upb_test_ModelWithMaps* msg = upb_test_ModelWithMaps_new(msg_arena.ptr());
      upb_DecodeStatus result = upb_DecodeWithTrace(
          payload.data(), payload.size(), UPB_UPCAST(msg), mt, nullptr, options,
          msg_arena.ptr(), trace_buf, sizeof(trace_buf));
      EXPECT_EQ(result, kUpb_DecodeStatus_MissingRequired);
    }
  }
}

TEST(DecodeTest, DecodeMapMaxDepthExceeded) {
  Arena mt_arena;
  auto [sub_mt, sub_field] =
      MiniTable::MakeSingleFieldTable<field_types::Message>(
          1, kUpb_DecodeFast_Scalar, mt_arena.ptr());
  bool linked = upb_MiniTable_SetSubMessage(
      const_cast<upb_MiniTable*>(sub_mt),
      const_cast<upb_MiniTableField*>(sub_field), sub_mt);
  UPB_ASSERT(linked);

  const upb_MiniTable* mt = CreateDynamicMapTable(
      1, kUpb_FieldType_Int32, kUpb_FieldType_Message, mt_arena.ptr(), sub_mt);

  Arena enc_arena;
  upb_Message* sub3 = upb_Message_New(sub_mt, enc_arena.ptr());
  upb_Message* sub2 = upb_Message_New(sub_mt, enc_arena.ptr());
  upb_Message_SetMessage(sub2, sub_field, sub3);
  upb_Message* sub1 = upb_Message_New(sub_mt, enc_arena.ptr());
  upb_Message_SetMessage(sub1, sub_field, sub2);

  std::string payload =
      SerializeDynamicMap<int32_t, upb_Message*>(mt, 10, sub1, enc_arena.ptr());

  for (int options : GetDecodeOptionsToTest()) {
    Arena msg_arena;
    char trace_buf[64] = {0};
    upb_Message* msg = upb_Message_New(mt, msg_arena.ptr());

    int decode_options = upb_Decode_LimitDepth(options, 2);
    upb_DecodeStatus result = upb_DecodeWithTrace(
        payload.data(), payload.size(), msg, mt, nullptr, decode_options,
        msg_arena.ptr(), trace_buf, sizeof(trace_buf));
    EXPECT_EQ(result, kUpb_DecodeStatus_MaxDepthExceeded);
  }
}

}  // namespace

}  // namespace test
}  // namespace upb
