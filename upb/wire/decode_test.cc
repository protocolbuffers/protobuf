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
#include "upb/base/status.hpp"
#include "upb/mem/arena.hpp"
#include "upb/message/accessors.h"
#include "upb/message/accessors.hpp"
#include "upb/message/message.h"
#include "upb/mini_descriptor/decode.h"
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

// ---------------------------------------------------------------------------
// Regression tests for https://github.com/protocolbuffers/protobuf/issues/26857
//
// `upb_MiniTable_Build()` does not populate sub-enum pointers for closed-enum
// fields — linking is a separate step via `upb_MiniTable_SetSubEnum()`.  Prior
// to the fix, the decoder would call `upb_MiniTableEnum_CheckValue()` with a
// NULL enum table, dereferencing `e->data[0]` (for values < 64) or
// `e->mask_limit` at offset 0x8 (for values >= 64), causing a SIGSEGV.  The
// fix routes any closed-enum value on an unlinked field through the
// unknown-field handler (kUpb_DecodeOp_UnknownField).
// ---------------------------------------------------------------------------

class UnlinkedClosedEnumTest : public testing::Test {
 protected:
  // Mini descriptor from the issue's 5-byte reproducer (first two bytes).
  // Builds a message with a single closed-enum field at tag 1, *unlinked*.
  static constexpr char kMiniDescriptor[] = "\x24\x34";
  static constexpr size_t kMiniDescriptorLen = 2;

  upb_MiniTable* BuildUnlinkedTable() {
    upb::Status status;
    upb_MiniTable* mt =
        upb_MiniTable_Build(kMiniDescriptor, kMiniDescriptorLen, arena_.ptr(),
                            status.ptr());
    EXPECT_NE(nullptr, mt) << status.error_message();
    return mt;
  }

  upb_DecodeStatus Decode(upb_MiniTable* mt, absl::string_view payload) {
    upb_Message* msg = upb_Message_New(mt, arena_.ptr());
    return upb_Decode(payload.data(), payload.size(), msg, mt, nullptr, 0,
                      arena_.ptr());
  }

  upb::Arena arena_;
};

// --- Scalar (varint) path --------------------------------------------------

// The canonical reproducer from the bug report: the wire portion of the
// 5-byte input `24 34 08 34 00`.  Pre-fix this crashes in
// `upb_MiniTableEnum_CheckValue()`.
TEST_F(UnlinkedClosedEnumTest, ExactReproducerDoesNotCrash) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // Three bytes: field 1 varint = 52 (0x34), then a trailing 0x00 byte
  // (tag 0, invalid).  The key assertion is that the call returns at all.
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x08\x34\x00", 3));
  EXPECT_TRUE(result == kUpb_DecodeStatus_Ok ||
              result == kUpb_DecodeStatus_Malformed)
      << upb_DecodeStatus_String(result);
}

// Clean varint with the reporter's exact value (52) and no trailing junk.
TEST_F(UnlinkedClosedEnumTest, ScalarVarintReporterValue52) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x08\x34", 2));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Value 0 — smallest possible varint.  Exercises the `val < 64` branch that
// pre-fix dereferences e->data[0] at NULL (address 0x0).
TEST_F(UnlinkedClosedEnumTest, ScalarVarintValueZero) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x08\x00", 2));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Small positive value (< 64).  Same NULL-deref path as value 0.
TEST_F(UnlinkedClosedEnumTest, ScalarVarintSmallValueUnder64) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x08\x05", 2));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Value at the 64-value boundary, which per the bug report shifts the crash
// from e->data[0] (offset 0) to e->mask_limit (offset 0x8).
TEST_F(UnlinkedClosedEnumTest, ScalarVarintValueAt64Boundary) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // 64 encodes as single-byte varint 0x40.
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x08\x40", 2));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Just below the boundary.
TEST_F(UnlinkedClosedEnumTest, ScalarVarintValueJustBelow64) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x08\x3F", 2));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Multi-byte varint value above 64 — exercises the >= 64 crash path plus
// the multi-byte varint decode.
TEST_F(UnlinkedClosedEnumTest, ScalarVarintLargeValue300) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // 300 = 0xAC 0x02 in varint encoding.
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x08\xAC\x02", 3));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Max int32 value.
TEST_F(UnlinkedClosedEnumTest, ScalarVarintInt32Max) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // INT32_MAX = 0x7FFFFFFF = varint 0xFF 0xFF 0xFF 0xFF 0x07.
  upb_DecodeStatus result = Decode(
      mt, absl::string_view("\x08\xFF\xFF\xFF\xFF\x07", 6));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Max uint64 — a 10-byte varint, tests the widest boundary value.
TEST_F(UnlinkedClosedEnumTest, ScalarVarintUint64Max) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  upb_DecodeStatus result = Decode(
      mt,
      absl::string_view("\x08\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01", 11));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Value that looks like a negative int32 when interpreted as signed.
TEST_F(UnlinkedClosedEnumTest, ScalarVarintNegativeInt32Representation) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // -1 as int32 sent on the wire (sign-extended to 10-byte varint).
  upb_DecodeStatus result = Decode(
      mt,
      absl::string_view("\x08\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01", 11));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Truncated varint — must still return a defined status (not crash).  The
// NULL-deref fix should not alter malformed-input handling.
TEST_F(UnlinkedClosedEnumTest, ScalarVarintTruncated) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // Tag 0x08 then a varint byte with continuation bit set, but no next byte.
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x08\x80", 2));
  EXPECT_EQ(result, kUpb_DecodeStatus_Malformed)
      << upb_DecodeStatus_String(result);
}

// Tag only, no value bytes — malformed, must not crash.
TEST_F(UnlinkedClosedEnumTest, ScalarVarintTagOnly) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x08", 1));
  EXPECT_EQ(result, kUpb_DecodeStatus_Malformed)
      << upb_DecodeStatus_String(result);
}

// --- Repeated, non-packed path --------------------------------------------

// Plan edge case #7: three back-to-back occurrences of the unlinked
// closed-enum field on the wire.  Each one independently enters the
// vulnerable varint path, so all three must be routed to unknown-fields
// without crashing.
TEST_F(UnlinkedClosedEnumTest, RepeatedNonPackedMultipleValues) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // Three occurrences: value 5 (<64), value 64 (=boundary), value 300
  // (multi-byte, >64).  Covers both crash addresses pre-fix.
  upb_DecodeStatus result = Decode(
      mt, absl::string_view("\x08\x05\x08\x40\x08\xAC\x02", 7));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Same field number, many occurrences — stresses the per-element iteration
// of the guard.
TEST_F(UnlinkedClosedEnumTest, RepeatedNonPackedManyOccurrences) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  std::string payload;
  for (int i = 0; i < 100; ++i) {
    payload.push_back('\x08');  // tag: field 1, wire type varint
    payload.push_back(static_cast<char>(i));  // single-byte varint
  }
  upb_DecodeStatus result = Decode(mt, payload);
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// --- Packed path (the second crash site) ----------------------------------

// Minimal packed payload: field 1, wire type 2 (LEN), length 1, one varint.
// Exercises `_upb_Decoder_DecodeEnumPacked` directly.
TEST_F(UnlinkedClosedEnumTest, PackedSingleElementSmall) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x0A\x01\x05", 3));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Packed, single element >= 64 — the other crash address.
TEST_F(UnlinkedClosedEnumTest, PackedSingleElementLarge) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // Length 2: varint 300 (0xAC 0x02).
  upb_DecodeStatus result =
      Decode(mt, absl::string_view("\x0A\x02\xAC\x02", 4));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Packed, element exactly at the 64 boundary.
TEST_F(UnlinkedClosedEnumTest, PackedSingleElementAt64Boundary) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x0A\x01\x40", 3));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Packed, element zero.
TEST_F(UnlinkedClosedEnumTest, PackedSingleElementZero) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x0A\x01\x00", 3));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Empty packed array — loop body never executes.  Still must succeed, and is
// important as a negative control (this case never crashed even pre-fix,
// since the inner deref was never reached).
TEST_F(UnlinkedClosedEnumTest, PackedEmpty) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x0A\x00", 2));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Packed array of varied values straddling the 64 boundary.
TEST_F(UnlinkedClosedEnumTest, PackedMixedBoundaryValues) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // Length 6, values: 5, 64, 1, 127, 0, 63.
  upb_DecodeStatus result = Decode(
      mt, absl::string_view("\x0A\x06\x05\x40\x01\x7F\x00\x3F", 8));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Long packed array — stresses the per-element NULL check inside the
// hot loop.
TEST_F(UnlinkedClosedEnumTest, PackedManyElements) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  std::string body;
  for (int i = 0; i < 128; ++i) body.push_back(static_cast<char>(i));
  std::string payload;
  payload.push_back('\x0A');  // tag
  payload.push_back('\x7F');  // length 127
  // Match the declared length (127 bytes of body).
  payload.append(body.data(), 127);
  upb_DecodeStatus result = Decode(mt, payload);
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Packed with multi-byte varint elements.
TEST_F(UnlinkedClosedEnumTest, PackedMultiByteVarints) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // Length 6: three 2-byte varints 128, 200, 300.
  upb_DecodeStatus result = Decode(
      mt, absl::string_view("\x0A\x06\x80\x01\xC8\x01\xAC\x02", 8));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Packed payload whose declared length extends past the input — must be
// detected as malformed, not crash.
TEST_F(UnlinkedClosedEnumTest, PackedTruncatedDeclaredLength) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // Claims length 5, only 1 byte supplied.
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x0A\x05\x05", 3));
  EXPECT_EQ(result, kUpb_DecodeStatus_Malformed)
      << upb_DecodeStatus_String(result);
}

// Packed payload whose last varint element has the continuation bit set but
// fits within the declared packed length.  The key assertion is that decoding
// does not crash on the unlinked-enum path; the precise status is whatever
// upb's varint reader returns for this shape of input.
TEST_F(UnlinkedClosedEnumTest, PackedTruncatedLastVarint) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // Length 2, body = 0x05, 0x80 (continuation bit set, no next byte).
  upb_DecodeStatus result =
      Decode(mt, absl::string_view("\x0A\x02\x05\x80", 4));
  EXPECT_TRUE(result == kUpb_DecodeStatus_Ok ||
              result == kUpb_DecodeStatus_Malformed)
      << upb_DecodeStatus_String(result);
}

// --- Mixed & miscellaneous -------------------------------------------------

// Both packed and non-packed occurrences of the same field in one payload.
TEST_F(UnlinkedClosedEnumTest, MixedPackedAndNonPacked) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // Non-packed value, then packed array of two values, then another
  // non-packed value.
  upb_DecodeStatus result = Decode(
      mt,
      absl::string_view("\x08\x05\x0A\x02\x40\x01\x08\xAC\x02", 9));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Completely empty payload.  The crash site is never reached; this is a
// negative control confirming the guard doesn't regress the trivial case.
TEST_F(UnlinkedClosedEnumTest, EmptyPayload) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  upb_Message* msg = upb_Message_New(mt, arena_.ptr());
  upb_DecodeStatus result =
      upb_Decode(nullptr, 0, msg, mt, nullptr, 0, arena_.ptr());
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// Decode twice against the same unlinked table — no state corruption /
// caching of a bad pointer from the first call into the second.
TEST_F(UnlinkedClosedEnumTest, DecodeTwiceOnSameTable) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  EXPECT_EQ(Decode(mt, absl::string_view("\x08\x05", 2)), kUpb_DecodeStatus_Ok);
  EXPECT_EQ(Decode(mt, absl::string_view("\x0A\x02\x40\x01", 4)),
            kUpb_DecodeStatus_Ok);
}

// Wrong wire type for the enum field (fixed32 = wire type 5) — the
// closed-enum check isn't reached since the wire type doesn't match the
// field, so no crash pre-fix and no behavior change post-fix.  Confirms the
// fix is scoped to the varint path.
TEST_F(UnlinkedClosedEnumTest, WrongWireTypeSkipsEnumCheck) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // Field 1, wire type 5 (fixed32), 4 bytes of value.
  upb_DecodeStatus result =
      Decode(mt, absl::string_view("\x0D\x00\x00\x00\x00", 5));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

// A payload that is valid-looking for an unrelated field — the closed-enum
// field guard should not fire on other fields.
TEST_F(UnlinkedClosedEnumTest, UnrelatedUnknownFieldIsUnaffected) {
  upb_MiniTable* mt = BuildUnlinkedTable();
  ASSERT_NE(mt, nullptr);
  // Field 2 varint = 7 (not the closed-enum field).  Must decode as unknown
  // without entering the vulnerable path.
  upb_DecodeStatus result = Decode(mt, absl::string_view("\x10\x07", 2));
  EXPECT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
}

}  // namespace

}  // namespace test
}  // namespace upb
