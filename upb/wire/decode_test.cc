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
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/message/accessors.h"
#include "upb/message/accessors.hpp"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/message/unknown_fields.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.hpp"
#include "upb/mini_descriptor/internal/modifiers.h"
#include "upb/mini_descriptor/link.h"
#include "upb/mini_table/enum.h"
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
  std::string payload("\x10\x02\x18\x03", 4);

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
  std::string payload("\x10\x02\x18\x03", 4);

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
  std::string payload("\x2a\x02\x08\x7b", 4);

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
  std::string payload("\x13\x18\x7b\x14\x20\xc8\x03", 7);

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

TEST(DecodeTest, SetSubMessageMapValidation) {
  Arena arena;
  upb_Status status;
  upb_Status_Clear(&status);

  // 1. Parent message table with repeated field (valid for map), scalar field,
  // and oneof field.
  upb::MtDataEncoder parent_enc;
  parent_enc.StartMessage(kUpb_MessageModifier_IsExtendable);
  parent_enc.PutField(kUpb_FieldType_Message, 1, kUpb_FieldModifier_IsRepeated);
  parent_enc.PutField(kUpb_FieldType_Message, 2, 0);  // scalar message
  parent_enc.PutField(kUpb_FieldType_Message, 3, 0);  // oneof message
  parent_enc.PutField(kUpb_FieldType_Group, 4, 0);    // group field
  parent_enc.StartOneof();
  parent_enc.PutOneofField(3);
  upb_MiniTable* parent_mt = upb_MiniTable_Build(
      parent_enc.data().data(), parent_enc.data().size(), arena.ptr(), &status);
  ASSERT_TRUE(upb_Status_IsOk(&status)) << upb_Status_ErrorMessage(&status);
  upb_MiniTableField* repeated_field = const_cast<upb_MiniTableField*>(
      upb_MiniTable_GetFieldByIndex(parent_mt, 0));
  upb_MiniTableField* scalar_field = const_cast<upb_MiniTableField*>(
      upb_MiniTable_GetFieldByIndex(parent_mt, 1));
  upb_MiniTableField* oneof_field = const_cast<upb_MiniTableField*>(
      upb_MiniTable_GetFieldByIndex(parent_mt, 2));
  upb_MiniTableField* group_field = const_cast<upb_MiniTableField*>(
      upb_MiniTable_GetFieldByIndex(parent_mt, 3));

  // 2. Valid map entry table
  upb::MtDataEncoder map_enc;
  map_enc.EncodeMap(kUpb_FieldType_Int32, kUpb_FieldType_Message, 0, 0);
  upb_MiniTable* map_entry = upb_MiniTable_Build(
      map_enc.data().data(), map_enc.data().size(), arena.ptr(), &status);
  ASSERT_TRUE(upb_Status_IsOk(&status)) << upb_Status_ErrorMessage(&status);

  // 3. Submessage for value field
  auto [sub_mt, sub_field] =
      MiniTable::MakeSingleFieldTable<field_types::Int32>(
          1, kUpb_DecodeFast_Scalar, arena.ptr());

  // Valid linking: parent repeated field -> map_entry
  EXPECT_TRUE(
      upb_MiniTable_SetSubMessage(parent_mt, repeated_field, map_entry));
  EXPECT_TRUE(upb_MiniTableField_IsMap(repeated_field));

  // Valid repeated linking (idempotent call, e.g. from JS bridge):
  EXPECT_TRUE(
      upb_MiniTable_SetSubMessage(parent_mt, repeated_field, map_entry));
  EXPECT_TRUE(upb_MiniTableField_IsMap(repeated_field));

  // Valid linking: map_entry field 2 (value) -> sub_mt
  upb_MiniTableField* val_field = const_cast<upb_MiniTableField*>(
      upb_MiniTable_GetFieldByIndex(map_entry, 1));
  EXPECT_TRUE(upb_MiniTable_SetSubMessage(map_entry, val_field, sub_mt));

  // Invalid: linking map_entry to a scalar (non-repeated) field
  EXPECT_FALSE(upb_MiniTable_SetSubMessage(parent_mt, scalar_field, map_entry));

  // Invalid: linking map_entry to a oneof field
  EXPECT_FALSE(upb_MiniTable_SetSubMessage(parent_mt, oneof_field, map_entry));

  // Invalid: linking map_entry to a group field
  EXPECT_FALSE(upb_MiniTable_SetSubMessage(parent_mt, group_field, map_entry));

  // Invalid: map_entry field 1 (key) -> sub_mt (key cannot have submessage)
  upb_MiniTableField* key_field = const_cast<upb_MiniTableField*>(
      upb_MiniTable_GetFieldByIndex(map_entry, 0));
  EXPECT_FALSE(upb_MiniTable_SetSubMessage(map_entry, key_field, sub_mt));

  // Invalid: linking map_entry to an extension field
  upb::MtDataEncoder ext_enc;
  ext_enc.EncodeExtension(kUpb_FieldType_Message, 100,
                          kUpb_FieldModifier_IsRepeated);
  upb_MiniTableExtension* ext =
      upb_MiniTableExtension_Build(ext_enc.data().data(), ext_enc.data().size(),
                                   parent_mt, arena.ptr(), &status);
  ASSERT_TRUE(upb_Status_IsOk(&status)) << upb_Status_ErrorMessage(&status);
  EXPECT_FALSE(upb_MiniTableExtension_SetSubMessage(ext, map_entry));

  // Invalid: table_is_map && sub_is_map (nested map entry)
  EXPECT_FALSE(upb_MiniTable_SetSubMessage(map_entry, val_field, map_entry));
}

TEST(DecodeTest, SetSubEnumMapValidation) {
  Arena arena;
  upb_Status status;
  upb_Status_Clear(&status);

  upb::MtDataEncoder map_enum_enc;
  map_enum_enc.EncodeMap(kUpb_FieldType_Int32, kUpb_FieldType_Enum, 0,
                         kUpb_FieldModifier_IsClosedEnum);
  upb_MiniTable* map_enum_entry =
      upb_MiniTable_Build(map_enum_enc.data().data(),
                          map_enum_enc.data().size(), arena.ptr(), &status);
  ASSERT_TRUE(upb_Status_IsOk(&status)) << upb_Status_ErrorMessage(&status);

  upb_MiniTableField* key_field = const_cast<upb_MiniTableField*>(
      upb_MiniTable_GetFieldByIndex(map_enum_entry, 0));
  upb_MiniTableField* val_field = const_cast<upb_MiniTableField*>(
      upb_MiniTable_GetFieldByIndex(map_enum_entry, 1));

  // Enum with 0
  upb::MtDataEncoder enum_with_0_enc;
  enum_with_0_enc.StartEnum();
  enum_with_0_enc.PutEnumValue(0);
  enum_with_0_enc.PutEnumValue(1);
  enum_with_0_enc.EndEnum();
  upb_MiniTableEnum* enum_with_0 = upb_MiniTableEnum_Build(
      enum_with_0_enc.data().data(), enum_with_0_enc.data().size(), arena.ptr(),
      &status);
  ASSERT_TRUE(upb_Status_IsOk(&status)) << upb_Status_ErrorMessage(&status);

  // Enum without 0
  upb::MtDataEncoder enum_without_0_enc;
  enum_without_0_enc.StartEnum();
  enum_without_0_enc.PutEnumValue(1);
  enum_without_0_enc.PutEnumValue(2);
  enum_without_0_enc.EndEnum();
  upb_MiniTableEnum* enum_without_0 = upb_MiniTableEnum_Build(
      enum_without_0_enc.data().data(), enum_without_0_enc.data().size(),
      arena.ptr(), &status);
  ASSERT_TRUE(upb_Status_IsOk(&status)) << upb_Status_ErrorMessage(&status);

  // Invalid: linking subenum to field 1 (key) of map entry
  EXPECT_FALSE(
      upb_MiniTable_SetSubEnum(map_enum_entry, key_field, enum_with_0));

  // Invalid: linking subenum lacking 0 to field 2 (value) of map entry
  EXPECT_FALSE(
      upb_MiniTable_SetSubEnum(map_enum_entry, val_field, enum_without_0));

  // Valid: linking subenum with 0 to field 2 (value) of map entry
  EXPECT_TRUE(upb_MiniTable_SetSubEnum(map_enum_entry, val_field, enum_with_0));
}

TEST(DecodeTest, BuildMalformedMapDescriptorValidation) {
  Arena arena;

  // Invalid: map mini-descriptor with 1 field (field_count != 2)
  {
    upb_Status status;
    upb_Status_Clear(&status);
    upb_MiniTable* bad_mt =
        upb_MiniTable_Build("M\x01", 2, arena.ptr(), &status);
    EXPECT_FALSE(upb_Status_IsOk(&status));
    EXPECT_EQ(bad_mt, nullptr);
  }

  // Invalid: map mini-descriptor with float key
  {
    upb_Status status;
    upb_Status_Clear(&status);
    upb::MtDataEncoder bad_enc;
    bad_enc.EncodeMap(kUpb_FieldType_Float, kUpb_FieldType_Int32, 0, 0);
    upb_MiniTable* bad_mt = upb_MiniTable_Build(
        bad_enc.data().data(), bad_enc.data().size(), arena.ptr(), &status);
    EXPECT_FALSE(upb_Status_IsOk(&status));
    EXPECT_EQ(bad_mt, nullptr);
  }

  // Invalid: map mini-descriptor with group value
  {
    upb_Status status;
    upb_Status_Clear(&status);
    upb::MtDataEncoder bad_enc;
    bad_enc.EncodeMap(kUpb_FieldType_Int32, kUpb_FieldType_Group, 0, 0);
    upb_MiniTable* bad_mt = upb_MiniTable_Build(
        bad_enc.data().data(), bad_enc.data().size(), arena.ptr(), &status);
    EXPECT_FALSE(upb_Status_IsOk(&status));
    EXPECT_EQ(bad_mt, nullptr);
  }
}

}  // namespace

}  // namespace test
}  // namespace upb
