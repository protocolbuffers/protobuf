// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/convert.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <tuple>

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto3.upb.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.hpp"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/convert_test.upb.h"
#include "upb/message/convert_test.upb_minitable.h"
#include "upb/message/internal/message.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/test.upb_minitable.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"

// Must be last to ensure UPB_PRIVATE is defined.
#include "upb/port/def.inc"

// We use the generated upb_MiniTables from test_messages_proto3.
#define TEST_MT &protobuf_0test_0messages__proto3__TestAllTypesProto3_msg_init

class ConvertTest : public testing::TestWithParam<std::tuple<int, bool>> {};

INSTANTIATE_TEST_SUITE_P(
    ConvertOptions, ConvertTest,
    testing::Combine(
        testing::Values(kUpb_ConvertOption_Copy,  // conversion mode
                        kUpb_ConvertOption_Alias),
        testing::Bool()  // Whether to pass in a non-null
                         // destination message.
        ));

TEST_P(ConvertTest, Identity) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* src_msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena.ptr());

  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_int32(src_msg,
                                                                      123);
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_string(
      src_msg, upb_StringView_FromString("hello"));

  const upb_MiniTable* mt = TEST_MT;
  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, /*dst_mt=*/mt, UPB_UPCAST(src_msg), /*src_mt=*/mt,
      /*extreg=*/nullptr, convert_options, arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);
  const protobuf_test_messages_proto3_TestAllTypesProto3* dst =
      (const protobuf_test_messages_proto3_TestAllTypesProto3*)ret_dst_msg;

  EXPECT_EQ(
      123,
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_int32(dst));
  upb_StringView str =
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_string(dst);
  EXPECT_EQ(std::string("hello"), std::string(str.data, str.size));
}

TEST_P(ConvertTest, SubMessage) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* src_msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena.ptr());

  protobuf_test_messages_proto3_TestAllTypesProto3_NestedMessage* sub =
      protobuf_test_messages_proto3_TestAllTypesProto3_mutable_optional_nested_message(
          src_msg, arena.ptr());
  protobuf_test_messages_proto3_TestAllTypesProto3_NestedMessage_set_a(sub,
                                                                       456);

  const upb_MiniTable* mt = TEST_MT;
  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, /*dst_mt=*/mt, UPB_UPCAST(src_msg), /*src_mt=*/mt,
      /*extreg=*/nullptr, convert_options, arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);
  const protobuf_test_messages_proto3_TestAllTypesProto3* dst =
      (const protobuf_test_messages_proto3_TestAllTypesProto3*)ret_dst_msg;

  const protobuf_test_messages_proto3_TestAllTypesProto3_NestedMessage* dst_sub =
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_nested_message(
          dst);

  // Verify value
  EXPECT_EQ(456,
            protobuf_test_messages_proto3_TestAllTypesProto3_NestedMessage_a(
                dst_sub));

  if (convert_options & kUpb_ConvertOption_Alias) {
    // Verify pointer identity for aliasing.
    EXPECT_EQ(sub, dst_sub);
  } else {
    // Verify pointer inequality for deep copy.
    EXPECT_NE(sub, dst_sub);
  }
}

TEST_P(ConvertTest, ConvertField_UnknownsConversion) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* src_msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena.ptr());

  // 1. Create a serialized buffer with an unknown field.
  char buf[32];
  char* ptr = buf;
  // Tag: 1000 << 3 | 0 = 8000. Varint: 0xC0 0x3E
  *ptr++ = (char)0xC0;
  *ptr++ = (char)0x3E;
  // Value: 789 = 0x315. Varint: 0x95 0x06
  *ptr++ = (char)0x95;
  *ptr++ = (char)0x06;
  size_t len = ptr - buf;
  UPB_PRIVATE(_upb_Message_AddUnknown)(UPB_UPCAST(src_msg), buf, len,
                                       arena.ptr(), kUpb_AddUnknown_Copy);

  const upb_MiniTable* src_mt = TEST_MT;

  // Use a second, identically structured minitable so it doesn't just return
  // the identical message but performs the conversion.
  const upb_MiniTable* dst_mt =
      &upb_0test__EmptyMessage_msg_init;  // Demote everything to unknown
  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(src_msg), src_mt, /*extreg=*/nullptr,
      convert_options, arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  // Verify that the unknown field was converted.
  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  upb_Message_NextUnknown(ret_dst_msg, &data, &iter);

  size_t src_iter = kUpb_Message_UnknownBegin;
  upb_StringView src_data;
  upb_Message_NextUnknown(UPB_UPCAST(src_msg), &src_data, &src_iter);

  if (convert_options & kUpb_ConvertOption_Alias) {
    EXPECT_EQ(data.data, src_data.data);
  } else {
    EXPECT_NE(data.data, src_data.data);
    EXPECT_EQ(data.size, src_data.size);
    // Memory blocks are identical.
    EXPECT_EQ(0, memcmp(data.data, src_data.data, data.size));
  }
}

TEST_P(ConvertTest, UnknownPromotion) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;

  // 1. Create a serialized buffer with just the unknown field.
  // Field `optional_int32` has number 1.
  // Encode 789 as field 1.
  char buf[32];
  char* ptr = buf;
  // Tag: 1 << 3 | 0 (varint) = 8
  *ptr++ = 8;
  // Value: 789 = 0x315. Varint: 0x95 0x06
  *ptr++ = (char)0x95;
  *ptr++ = (char)0x06;
  size_t len = ptr - buf;

  // 2. Parse this buffer into Message with NO fields.
  const upb_MiniTable* empty_mt = &upb_0test__EmptyMessage_msg_init;

  upb_Message* msg_empty = upb_Message_New(empty_mt, arena.ptr());
  upb_DecodeStatus status =
      upb_Decode(buf, len, msg_empty, empty_mt, nullptr, 0, arena.ptr());
  EXPECT_EQ(kUpb_DecodeStatus_Ok, status);

  // 3. Convert `msg_empty` to `dst` (TestAllTypesProto3).
  // `dst` HAS field 1. So it should promote the unknown from `msg_empty`.

  const upb_MiniTable* mt = TEST_MT;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, mt, msg_empty, empty_mt, nullptr, convert_options, arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);
  const protobuf_test_messages_proto3_TestAllTypesProto3* dst =
      (const protobuf_test_messages_proto3_TestAllTypesProto3*)ret_dst_msg;

  EXPECT_EQ(
      789,
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_int32(dst));
}

TEST_P(ConvertTest, Demotion) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena.ptr());
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_int32(msg, 999);

  // Convert to Empty message. Field 1 should become unknown.
  const upb_MiniTable* empty_mt = &upb_0test__EmptyMessage_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(empty_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, empty_mt, UPB_UPCAST(msg), TEST_MT, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  // Dst should have unknown field 1 with value 999.
  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(ret_dst_msg, &data, &iter));
  EXPECT_GE(data.size, 3);
  EXPECT_EQ((uint8_t)data.data[0], 8);
  EXPECT_EQ((uint8_t)data.data[1], 0xE7);
  EXPECT_EQ((uint8_t)data.data[2], 0x07);
  EXPECT_FALSE(upb_Message_NextUnknown(ret_dst_msg, &data, &iter));
}

TEST_P(ConvertTest, DeepConvertMap) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena.ptr());

  protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_set(
      msg, 10, 20, arena.ptr());

  const upb_MiniTable* mt = TEST_MT;

  // Self-conversion should work (shallow).
  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, mt, UPB_UPCAST(msg), mt, nullptr, convert_options, arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);
  const protobuf_test_messages_proto3_TestAllTypesProto3* dst =
      (const protobuf_test_messages_proto3_TestAllTypesProto3*)ret_dst_msg;

  int32_t val;
  EXPECT_TRUE(
      protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_get(
          dst, 10, &val));
  EXPECT_EQ(20, val);
}

TEST_P(ConvertTest, DeepConvertMapMessage) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithMapMessage* msg =
      upb_test_convert_MessageWithMapMessage_new(arena.ptr());

  upb_test_convert_MessageWithInt32* val =
      upb_test_convert_MessageWithInt32_new(arena.ptr());
  upb_test_convert_MessageWithInt32_set_f1(val, 123);

  upb_test_convert_MessageWithMapMessage_map_msg_set(msg, 10, val, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithMapMessage_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithMapMessageClone_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithMapMessageClone* dst =
      (const upb_test_convert_MessageWithMapMessageClone*)ret_dst_msg;

  upb_test_convert_MessageWithInt32Clone* dst_val;
  EXPECT_TRUE(upb_test_convert_MessageWithMapMessageClone_map_msg_get(
      dst, 10, &dst_val));
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32Clone_f1(dst_val));

  // It should be a deep copy, not the same pointer.
  EXPECT_NE((const void*)dst_val, (const void*)val);
}

TEST_P(ConvertTest, ExtensionArrayConversion) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_test_convert_MessageWithInt32* val =
      upb_test_convert_MessageWithInt32_new(arena.ptr());
  upb_test_convert_MessageWithInt32_set_f1(val, 456);

  upb_Array* ext_arr = upb_Array_New(arena.ptr(), kUpb_CType_Message);
  upb_Array_Resize(ext_arr, 1, arena.ptr());
  upb_MessageValue elem_val;
  elem_val.msg_val = (const upb_Message*)val;
  upb_Array_Set(ext_arr, 0, elem_val);
  upb_MessageValue ext_val;
  ext_val.array_val = ext_arr;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           &upb_test_convert_ext_field_repeated_msg_ext,
                           &ext_val, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownRepeatedMsg_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithKnownRepeatedMsg* dst =
      (const upb_test_convert_MessageWithKnownRepeatedMsg*)ret_dst_msg;

  size_t size;
  const upb_test_convert_MessageWithInt32* const* dst_arr =
      upb_test_convert_MessageWithKnownRepeatedMsg_known_repeated_msg(dst,
                                                                      &size);
  EXPECT_EQ(1, size);
  EXPECT_EQ(456, upb_test_convert_MessageWithInt32_f1(dst_arr[0]));

  if (convert_options & kUpb_ConvertOption_Alias) {
    EXPECT_EQ((const void*)dst_arr[0], (const void*)val);
  } else {
    EXPECT_NE((const void*)dst_arr[0], (const void*)val);
  }
}

TEST_P(ConvertTest, ExtensionArrayDeepConversion) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_test_convert_MessageWithInt32* val =
      upb_test_convert_MessageWithInt32_new(arena.ptr());
  upb_test_convert_MessageWithInt32_set_f1(val, 789);

  upb_Array* ext_arr = upb_Array_New(arena.ptr(), kUpb_CType_Message);
  upb_Array_Resize(ext_arr, 1, arena.ptr());
  upb_MessageValue elem_val;
  elem_val.msg_val = (const upb_Message*)val;
  upb_Array_Set(ext_arr, 0, elem_val);
  upb_MessageValue ext_val;
  ext_val.array_val = ext_arr;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           &upb_test_convert_ext_field_repeated_msg_ext,
                           &ext_val, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownRepeatedMsgClone_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithKnownRepeatedMsgClone* dst =
      (const upb_test_convert_MessageWithKnownRepeatedMsgClone*)ret_dst_msg;

  size_t size;
  const upb_test_convert_MessageWithInt32Clone* const* dst_arr =
      upb_test_convert_MessageWithKnownRepeatedMsgClone_known_repeated_msg(
          dst, &size);
  EXPECT_EQ(1, size);
  EXPECT_EQ(789, upb_test_convert_MessageWithInt32Clone_f1(dst_arr[0]));

  // Deep conversion means the pointers are not equal.
  EXPECT_NE((const void*)dst_arr[0], (const void*)val);
}

TEST_P(ConvertTest, SkippedMismatchedExtensionIsPreservedAsUnknown) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  // Set extension field 1000 to an int32
  upb_MessageValue ext_val;
  ext_val.int32_val = 12345;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           &upb_test_convert_ext_field_int32_ext, &ext_val,
                           arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;

  // Convert to a message where field 1000 is an int64 instead of int32.
  // The type mismatch should cause it to skip setting the regular field,
  // but it should preserve the int32 wire data into the unknown fields.
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownInt64_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithKnownInt64* dst =
      (const upb_test_convert_MessageWithKnownInt64*)ret_dst_msg;

  // The field should NOT be set as a regular field because of the mismatch
  EXPECT_FALSE(
      upb_test_convert_MessageWithKnownInt64_has_known_field_int64(dst));

  // The value MUST be preserved in the unknown fields
  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(UPB_UPCAST(dst), &data, &iter));
  EXPECT_GE(data.size, 4);
  EXPECT_EQ((uint8_t)data.data[0], 0xC0);
  EXPECT_EQ((uint8_t)data.data[1], 0x3E);
  EXPECT_EQ((uint8_t)data.data[2], 0xB9);
  EXPECT_EQ((uint8_t)data.data[3], 0x60);
  EXPECT_FALSE(upb_Message_NextUnknown(UPB_UPCAST(dst), &data, &iter));
}

TEST_P(ConvertTest, ConvertField_TypeMismatch) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithInt32* msg =
      upb_test_convert_MessageWithInt32_new(arena.ptr());
  upb_test_convert_MessageWithInt32_set_f1(msg, 12345);

  const upb_MiniTable* src_mt = &upb__test__convert__MessageWithInt32_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__MessageWithInt64_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithInt64* dst =
      (const upb_test_convert_MessageWithInt64*)ret_dst_msg;

  // The type mismatch between int32 and int64 causes upb_Message_ConvertField
  // to return false, dropping the field and promoting it to an unknown field.
  EXPECT_FALSE(upb_test_convert_MessageWithInt64_has_f1(dst));

  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(UPB_UPCAST(dst), &data, &iter));
  EXPECT_GE(data.size, 3);
  EXPECT_EQ((uint8_t)data.data[0], 0x08);
  EXPECT_EQ((uint8_t)data.data[1], 0xB9);
  EXPECT_EQ((uint8_t)data.data[2], 0x60);
  EXPECT_FALSE(upb_Message_NextUnknown(UPB_UPCAST(dst), &data, &iter));
}

TEST_P(ConvertTest, ConvertField_MapTypeMismatch) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithMapInt32Int32* msg =
      upb_test_convert_MessageWithMapInt32Int32_new(arena.ptr());
  upb_test_convert_MessageWithMapInt32Int32_m_set(msg, 123, 456, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithMapInt32Int32_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithMapInt32Int64_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithMapInt32Int64* dst =
      (const upb_test_convert_MessageWithMapInt32Int64*)ret_dst_msg;

  // Since the map value types differ (int32 vs int64), the field shouldn't
  // be converted as a map, but instead demoted to an unknown field.
  size_t map_size = upb_test_convert_MessageWithMapInt32Int64_m_size(dst);
  EXPECT_EQ(map_size, 0);

  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(UPB_UPCAST(dst), &data, &iter));
  EXPECT_GE(data.size, 1);
  EXPECT_EQ((uint8_t)data.data[0], 0x0A);
  EXPECT_FALSE(upb_Message_NextUnknown(UPB_UPCAST(dst), &data, &iter));
}

TEST_P(ConvertTest, ConvertField_SingularMessageDeep) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithMsg* msg =
      upb_test_convert_MessageWithMsg_new(arena.ptr());
  upb_test_convert_MessageWithInt32* sub =
      upb_test_convert_MessageWithInt32_new(arena.ptr());
  upb_test_convert_MessageWithInt32_set_f1(sub, 123);
  upb_test_convert_MessageWithMsg_set_msg(msg, sub);

  const upb_MiniTable* src_mt = &upb__test__convert__MessageWithMsg_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithMsgClone_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;

  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithMsgClone* dst =
      (const upb_test_convert_MessageWithMsgClone*)ret_dst_msg;

  const upb_test_convert_MessageWithInt32Clone* dst_sub =
      upb_test_convert_MessageWithMsgClone_msg(dst);
  EXPECT_NE(dst_sub, nullptr);
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32Clone_f1(dst_sub));

  // Verify deep copy logic occurred correctly
  EXPECT_NE((const void*)dst_sub, (const void*)sub);
}

TEST_P(ConvertTest, ConvertField_ArrayMessageShallow) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithRepeatedMsg* msg =
      upb_test_convert_MessageWithRepeatedMsg_new(arena.ptr());
  upb_test_convert_MessageWithInt32* sub =
      upb_test_convert_MessageWithInt32_new(arena.ptr());
  upb_test_convert_MessageWithInt32_set_f1(sub, 123);

  upb_test_convert_MessageWithInt32** arr =
      upb_test_convert_MessageWithRepeatedMsg_resize_msgs(msg, 1, arena.ptr());
  arr[0] = sub;

  const upb_MiniTable* mt =
      &upb__test__convert__MessageWithRepeatedMsg_msg_init;

  // Use a registry to bypass the identical-minitable short-circuit, so it goes
  // through the field conversion logic where the deep copy takes place.
  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena.ptr());

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, mt, UPB_UPCAST(msg), mt, extreg, convert_options, arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithRepeatedMsg* dst =
      (const upb_test_convert_MessageWithRepeatedMsg*)ret_dst_msg;

  size_t size;
  const upb_test_convert_MessageWithInt32* const* dst_arr =
      upb_test_convert_MessageWithRepeatedMsg_msgs(dst, &size);
  EXPECT_EQ(1, size);
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32_f1(dst_arr[0]));

  if (convert_options & kUpb_ConvertOption_Alias) {
    EXPECT_EQ((const void*)dst_arr[0], (const void*)sub);
  } else {
    EXPECT_NE((const void*)dst_arr[0], (const void*)sub);
  }
}

TEST_P(ConvertTest, ConvertField_ArrayMessageDeep) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithRepeatedMsg* msg =
      upb_test_convert_MessageWithRepeatedMsg_new(arena.ptr());
  upb_test_convert_MessageWithInt32* sub =
      upb_test_convert_MessageWithInt32_new(arena.ptr());
  upb_test_convert_MessageWithInt32_set_f1(sub, 123);

  upb_test_convert_MessageWithInt32** arr =
      upb_test_convert_MessageWithRepeatedMsg_resize_msgs(msg, 1, arena.ptr());
  arr[0] = sub;

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithRepeatedMsg_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithRepeatedMsgClone_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithRepeatedMsgClone* dst =
      (const upb_test_convert_MessageWithRepeatedMsgClone*)ret_dst_msg;

  size_t size;
  const upb_test_convert_MessageWithInt32Clone* const* dst_arr =
      upb_test_convert_MessageWithRepeatedMsgClone_msgs(dst, &size);
  EXPECT_EQ(1, size);
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32Clone_f1(dst_arr[0]));

  // Deep copy expected
  EXPECT_NE((const void*)dst_arr[0], (const void*)sub);
}

TEST_P(ConvertTest, ConvertExtensions_ScalarMatch) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_MessageValue ext_val;
  ext_val.int32_val = 123;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           &upb_test_convert_ext_field_int32_ext, &ext_val,
                           arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__MessageWithKnown_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithKnown* dst =
      (const upb_test_convert_MessageWithKnown*)ret_dst_msg;
  EXPECT_TRUE(upb_test_convert_MessageWithKnown_has_known_field_int32(dst));
  EXPECT_EQ(123, upb_test_convert_MessageWithKnown_known_field_int32(dst));
}

TEST_P(ConvertTest, ConvertExtensions_SingularMessageConversion) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_test_convert_MessageWithInt32* sub =
      upb_test_convert_MessageWithInt32_new(arena.ptr());
  upb_test_convert_MessageWithInt32_set_f1(sub, 123);

  upb_MessageValue ext_val;
  ext_val.msg_val = UPB_UPCAST(sub);
  upb_Message_SetExtension(UPB_UPCAST(msg), &upb_test_convert_ext_field_msg_ext,
                           &ext_val, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownMsg_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithKnownMsg* dst =
      (const upb_test_convert_MessageWithKnownMsg*)ret_dst_msg;
  const upb_test_convert_MessageWithInt32* dst_sub =
      upb_test_convert_MessageWithKnownMsg_known_msg(dst);
  EXPECT_NE(dst_sub, nullptr);
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32_f1(dst_sub));

  if (convert_options & kUpb_ConvertOption_Alias) {
    EXPECT_EQ((const void*)dst_sub, (const void*)sub);
  } else {
    EXPECT_NE((const void*)dst_sub, (const void*)sub);
  }
}

TEST_P(ConvertTest, ConvertExtensions_SingularMessageDeep) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_test_convert_MessageWithInt32* sub =
      upb_test_convert_MessageWithInt32_new(arena.ptr());
  upb_test_convert_MessageWithInt32_set_f1(sub, 123);

  upb_MessageValue ext_val;
  ext_val.msg_val = UPB_UPCAST(sub);
  upb_Message_SetExtension(UPB_UPCAST(msg), &upb_test_convert_ext_field_msg_ext,
                           &ext_val, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownMsgClone_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithKnownMsgClone* dst =
      (const upb_test_convert_MessageWithKnownMsgClone*)ret_dst_msg;
  const upb_test_convert_MessageWithInt32Clone* dst_sub =
      upb_test_convert_MessageWithKnownMsgClone_known_msg(dst);
  EXPECT_NE(dst_sub, nullptr);
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32Clone_f1(dst_sub));

  // Deep copy expected because of mismatched minitables
  EXPECT_NE((const void*)dst_sub, (const void*)sub);
}

TEST_P(ConvertTest, ConvertExtensions_RemainsExtension) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_MessageValue ext_val;
  ext_val.int32_val = 123;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           &upb_test_convert_ext_field_int32_ext, &ext_val,
                           arena.ptr());

  const upb_MiniTable* mt = &upb__test__convert__MessageWithExtension_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, mt, UPB_UPCAST(msg), mt, nullptr, convert_options, arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_MessageWithExtension* dst =
      (const upb_test_convert_MessageWithExtension*)ret_dst_msg;

  // Extension should still be perfectly preserved on the destination message.
  EXPECT_TRUE(upb_Message_HasExtension(UPB_UPCAST(dst),
                                       &upb_test_convert_ext_field_int32_ext));

  int32_t out_val = upb_Message_GetExtensionInt32(
      UPB_UPCAST(dst), &upb_test_convert_ext_field_int32_ext, 0);
  EXPECT_EQ(123, out_val);
}

TEST_P(ConvertTest, ConvertExtensions_LookupExtensionInRegistry) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_MessageValue ext_val;
  ext_val.int32_val = 123;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           &upb_test_convert_ext_field_int32_ext, &ext_val,
                           arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__AnotherMessageWithExtension_msg_init;

  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena.ptr());
  upb_ExtensionRegistry_Add(extreg,
                            &upb_test_convert_another_ext_field_int32_ext);

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, extreg, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  const upb_test_convert_AnotherMessageWithExtension* dst =
      (const upb_test_convert_AnotherMessageWithExtension*)ret_dst_msg;

  // Extension should be found via registry and converted.
  EXPECT_TRUE(upb_Message_HasExtension(
      UPB_UPCAST(dst), &upb_test_convert_another_ext_field_int32_ext));

  int32_t out_val = upb_Message_GetExtensionInt32(
      UPB_UPCAST(dst), &upb_test_convert_another_ext_field_int32_ext, 0);
  EXPECT_EQ(123, out_val);
}

TEST_P(ConvertTest, ConvertExtensionToNonExtendable) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_MessageValue ext_val;
  ext_val.int32_val = 123;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           &upb_test_convert_ext_field_int32_ext, &ext_val,
                           arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt = &upb_0test__EmptyMessage_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(dst_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, dst_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  // Dst should have unknown field 1000 with value 123.
  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(ret_dst_msg, &data, &iter));
  EXPECT_EQ(data.size, 3);
  EXPECT_EQ((uint8_t)data.data[0], 0xC0);
  EXPECT_EQ((uint8_t)data.data[1], 0x3E);
  EXPECT_EQ((uint8_t)data.data[2], 0x7B);
  EXPECT_FALSE(upb_Message_NextUnknown(ret_dst_msg, &data, &iter));
}

TEST_P(ConvertTest, OneofDemotion) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  upb_test_convert_SrcWithOneof* msg =
      upb_test_convert_SrcWithOneof_new(arena.ptr());
  // Set both fields of the oneof. The last one should win and the first should
  // be dropped.
  upb_test_convert_SrcWithOneof_set_oneof_int32(msg, 54321);
  upb_StringView str = upb_StringView_FromString("test");
  upb_test_convert_SrcWithOneof_set_oneof_string(msg, str);

  // Convert to Empty message. Field 2 should become unknown.
  const upb_MiniTable* empty_mt = &upb_0test__EmptyMessage_msg_init;
  const upb_MiniTable* src_mt = &upb__test__convert__SrcWithOneof_msg_init;

  upb_Message* dst_msg =
      pass_dst_msg ? upb_Message_New(empty_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg = UPB_PRIVATE(_upb_Message_Convert)(
      dst_msg, empty_mt, UPB_UPCAST(msg), src_mt, nullptr, convert_options,
      arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);

  // Dst should have unknown field 2 with value "test".
  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  EXPECT_TRUE(upb_Message_NextUnknown(ret_dst_msg, &data, &iter));
  // "test" string field 2.
  // Tag 2, type 2: 2<<3 | 2 = 18
  EXPECT_EQ(data.size, 6);
  EXPECT_EQ((uint8_t)data.data[0], 18);
  EXPECT_EQ((uint8_t)data.data[1], 4);
  EXPECT_EQ((uint8_t)data.data[2], 't');
  EXPECT_EQ((uint8_t)data.data[3], 'e');
  EXPECT_EQ((uint8_t)data.data[4], 's');
  EXPECT_EQ((uint8_t)data.data[5], 't');
  EXPECT_FALSE(upb_Message_NextUnknown(ret_dst_msg, &data, &iter));

  // Now convert back and verify.
  upb_Message* dst2_msg =
      pass_dst_msg ? upb_Message_New(src_mt, arena.ptr()) : nullptr;
  const upb_Message* ret_dst_msg2 =
      UPB_PRIVATE(_upb_Message_Convert)(dst2_msg, src_mt, ret_dst_msg, empty_mt,
                                        nullptr, convert_options, arena.ptr());
  EXPECT_NE(ret_dst_msg2, nullptr);
  const upb_test_convert_SrcWithOneof* dst2 =
      (const upb_test_convert_SrcWithOneof*)ret_dst_msg2;
  EXPECT_EQ(upb_test_convert_SrcWithOneof_my_oneof_case(dst2),
            upb_test_convert_SrcWithOneof_my_oneof_oneof_string);
  upb_StringView str2 = upb_test_convert_SrcWithOneof_oneof_string(dst2);
  EXPECT_EQ(std::string("test"), std::string(str2.data, str2.size));
}

TEST_P(ConvertTest, DeepConvertMapStringKey) {
  int convert_options = std::get<0>(GetParam());
  bool pass_dst_msg = std::get<1>(GetParam());

  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena.ptr());

  upb_StringView key = upb_StringView_FromString("key");
  upb_StringView val = upb_StringView_FromString("val");
  protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_set(
      msg, key, val, arena.ptr());

  const upb_MiniTable* mt = TEST_MT;
  upb_Message* dst_msg_in =
      pass_dst_msg ? upb_Message_New(mt, arena.ptr()) : nullptr;
  // Use a registry to bypass the identical-minitable short-circuit for copy
  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena.ptr());
  const upb_Message* ret_dst_msg =
      UPB_PRIVATE(_upb_Message_Convert)(dst_msg_in, mt, UPB_UPCAST(msg), mt,
                                        extreg, convert_options, arena.ptr());
  EXPECT_NE(ret_dst_msg, nullptr);
  const protobuf_test_messages_proto3_TestAllTypesProto3* dst =
      (const protobuf_test_messages_proto3_TestAllTypesProto3*)ret_dst_msg;

  const upb_MiniTableField* field = upb_MiniTable_FindFieldByNumber(mt, 69);
  ASSERT_NE(field, nullptr);
  const upb_Map* src_map = upb_Message_GetMap(UPB_UPCAST(msg), field);
  const upb_Map* dst_map = upb_Message_GetMap(UPB_UPCAST(dst), field);
  ASSERT_NE(src_map, nullptr);
  ASSERT_NE(dst_map, nullptr);
  EXPECT_EQ(upb_Map_Size(src_map), 1);
  EXPECT_EQ(upb_Map_Size(dst_map), 1);

  upb_StringView out_val;
  ASSERT_TRUE(
      protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_get(
          dst, key, &out_val));
  EXPECT_EQ(std::string(out_val.data, out_val.size), "val");

  size_t src_iter = kUpb_Map_Begin;
  upb_MessageValue src_key, src_val_msg;
  ASSERT_TRUE(upb_Map_Next(src_map, &src_key, &src_val_msg, &src_iter));

  size_t dst_iter = kUpb_Map_Begin;
  upb_MessageValue dst_key, dst_val_msg;
  ASSERT_TRUE(upb_Map_Next(dst_map, &dst_key, &dst_val_msg, &dst_iter));

  if (convert_options & kUpb_ConvertOption_Alias) {
    EXPECT_EQ(dst_key.str_val.data, src_key.str_val.data);
    EXPECT_EQ(dst_val_msg.str_val.data, src_val_msg.str_val.data);
  } else {
    EXPECT_NE(dst_key.str_val.data, src_key.str_val.data);
    EXPECT_NE(dst_val_msg.str_val.data, src_val_msg.str_val.data);
  }
  EXPECT_EQ(std::string(dst_key.str_val.data, dst_key.str_val.size), "key");
  EXPECT_EQ(std::string(dst_val_msg.str_val.data, dst_val_msg.str_val.size),
            "val");
  EXPECT_FALSE(upb_Map_Next(dst_map, &dst_key, &dst_val_msg, &dst_iter));
}
