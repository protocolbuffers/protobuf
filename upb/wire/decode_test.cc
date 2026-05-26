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
#include <limits>
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
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.hpp"
#include "upb/mini_descriptor/link.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/test/test.upb.h"
#include "upb/test/test.upb_minitable.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/encode.h"
#include "upb/wire/test_util/field_types.h"
#include "upb/wire/test_util/make_mini_table.h"
#include "upb/wire/test_util/wire_message.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace test {

namespace {

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
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(msg, &data, &iter));
  EXPECT_EQ(absl::string_view(data.data, data.size), payload);
  EXPECT_FALSE(upb_Message_NextUnknown(msg, &data, &iter));
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

  // 1. Build custom different mini-table for the submessage layout ("extension
  // A").
  upb::MtDataEncoder e;
  e.StartMessage(0);
  e.PutField(kUpb_FieldType_String, 25, 0);

  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTable* custom_sub_table = upb_MiniTable_Build(
      e.data().data(), e.data().size(), arena.ptr(), &status);
  ASSERT_TRUE(status.ok);

  upb_MiniTableExtension custom_ext = *upb_test_ModelExtension1_model_ext_ext;
  upb_MiniTableExtension_SetSubMessage(&custom_ext, custom_sub_table);

  // 2. Create base msg which starts empty
  upb_test_ModelWithExtensions* msg =
      upb_test_ModelWithExtensions_new(arena.ptr());

  // 3. Create parsed submessage ("World") under custom_sub_table
  upb_Message* extension1 = _upb_Message_New(custom_sub_table, arena.ptr());
  upb_MessageValue val_str;
  val_str.str_val = upb_StringView_FromString("World");
  const upb_MiniTableField* custom_f =
      upb_MiniTable_GetFieldByIndex(custom_sub_table, 0);
  upb_Message_SetString(extension1, custom_f, val_str.str_val, arena.ptr());

  // 4. msg has a non-canonical extension A
  UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg), &custom_ext, &extension1, arena.ptr());

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
    } else if (data.type == kUpb_MessageUnknownType_Bytes) {
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

  // 1. Build custom different mini-table for the submessage layout ("extension
  // A").
  upb::MtDataEncoder e;
  e.StartMessage(0);
  e.PutField(kUpb_FieldType_String, 25, 0);

  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTable* custom_sub_table = upb_MiniTable_Build(
      e.data().data(), e.data().size(), arena.ptr(), &status);
  ASSERT_TRUE(status.ok);

  upb_MiniTableExtension custom_ext = *upb_test_ModelExtension1_model_ext_ext;
  upb_MiniTableExtension_SetSubMessage(&custom_ext, custom_sub_table);

  // 2. Create a temporary message to serialize the extension
  upb_test_ModelWithExtensions* tmp_msg =
      upb_test_ModelWithExtensions_new(arena.ptr());

  // 3. Create parsed submessage ("World") under custom_sub_table
  upb_Message* extension1 = _upb_Message_New(custom_sub_table, arena.ptr());
  upb_MessageValue val_str;
  val_str.str_val = upb_StringView_FromString("World");
  const upb_MiniTableField* custom_f =
      upb_MiniTable_GetFieldByIndex(custom_sub_table, 0);
  upb_Message_SetString(extension1, custom_f, val_str.str_val, arena.ptr());

  // 4. Attach to tmp_msg as a non-canonical extension so we can serialize it to
  // get the bytes
  UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(tmp_msg), &custom_ext, &extension1, arena.ptr());

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
    } else if (data.type == kUpb_MessageUnknownType_Bytes) {
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
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(parent_msg, &data, &iter));
  EXPECT_EQ(absl::string_view(data.data, data.size), payload);
  EXPECT_FALSE(upb_Message_NextUnknown(parent_msg, &data, &iter));
}

}  // namespace

}  // namespace test
}  // namespace upb
