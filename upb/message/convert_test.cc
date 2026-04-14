// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/convert.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto3.upb.h"
#include "google/protobuf/test_messages_proto3.upb_minitable.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.hpp"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/convert_test.upb.h"
#include "upb/message/convert_test.upb_minitable.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/message/test.upb_minitable.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"

// Must be last to ensure UPB_PRIVATE is defined.
#include "upb/port/def.inc"

// We use the generated upb_MiniTables from test_messages_proto3.
#define TEST_MT &protobuf_0test_0messages__proto3__TestAllTypesProto3_msg_init

TEST(ConvertTest, Identity) {
  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena.ptr());

  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_int32(msg, 123);
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_string(
      msg, upb_StringView_FromString("hello"));

  const upb_MiniTable* mt = TEST_MT;

  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);
  const protobuf_test_messages_proto3_TestAllTypesProto3* dst =
      (const protobuf_test_messages_proto3_TestAllTypesProto3*)dst_msg;

  EXPECT_EQ(
      123,
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_int32(dst));
  upb_StringView str =
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_string(dst);
  EXPECT_EQ(std::string("hello"), std::string(str.data, str.size));
}

TEST(ConvertTest, AliasSubMessage) {
  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena.ptr());

  protobuf_test_messages_proto3_TestAllTypesProto3_NestedMessage* sub =
      protobuf_test_messages_proto3_TestAllTypesProto3_mutable_optional_nested_message(
          msg, arena.ptr());
  protobuf_test_messages_proto3_TestAllTypesProto3_NestedMessage_set_a(sub,
                                                                       456);

  const upb_MiniTable* mt = TEST_MT;

  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);
  const protobuf_test_messages_proto3_TestAllTypesProto3* dst =
      (const protobuf_test_messages_proto3_TestAllTypesProto3*)dst_msg;

  const protobuf_test_messages_proto3_TestAllTypesProto3_NestedMessage* dst_sub =
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_nested_message(
          dst);

  // Verify value
  EXPECT_EQ(456,
            protobuf_test_messages_proto3_TestAllTypesProto3_NestedMessage_a(
                dst_sub));

  // Verify shallow copy (pointer identity)
  EXPECT_EQ(sub, dst_sub);
}

TEST(ConvertTest, UnknownPromotion) {
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

  const upb_Message* dst_msg =
      upb_Message_Convert(msg_empty, empty_mt, mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);
  const protobuf_test_messages_proto3_TestAllTypesProto3* dst =
      (const protobuf_test_messages_proto3_TestAllTypesProto3*)dst_msg;

  EXPECT_EQ(
      789,
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_int32(dst));
}

TEST(ConvertTest, Demotion) {
  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena.ptr());
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_int32(msg, 999);

  // Convert to Empty message. Field 1 should become unknown.
  const upb_MiniTable* empty_mt = &upb_0test__EmptyMessage_msg_init;

  const upb_Message* dst = upb_Message_Convert(UPB_UPCAST(msg), TEST_MT,
                                               empty_mt, nullptr, arena.ptr());
  EXPECT_NE(dst, nullptr);

  // Dst should have unknown field 1 with value 999.
  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(dst, &data, &iter));
  EXPECT_GE(data.size, 3);
  EXPECT_EQ((uint8_t)data.data[0], 8);
  EXPECT_EQ((uint8_t)data.data[1], 0xE7);
  EXPECT_EQ((uint8_t)data.data[2], 0x07);
  EXPECT_FALSE(upb_Message_NextUnknown(dst, &data, &iter));
}

TEST(ConvertTest, DeepConvertMap) {
  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena.ptr());

  protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_set(
      msg, 10, 20, arena.ptr());

  const upb_MiniTable* mt = TEST_MT;

  // Self-conversion should work (shallow).
  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);
  const protobuf_test_messages_proto3_TestAllTypesProto3* dst =
      (const protobuf_test_messages_proto3_TestAllTypesProto3*)dst_msg;

  int32_t val;
  EXPECT_TRUE(
      protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_get(
          dst, 10, &val));
  EXPECT_EQ(20, val);
}

TEST(ConvertTest, DeepConvertMapMessage) {
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

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithMapMessageClone* dst =
      (const upb_test_convert_MessageWithMapMessageClone*)dst_msg;

  upb_test_convert_MessageWithInt32Clone* dst_val;
  EXPECT_TRUE(upb_test_convert_MessageWithMapMessageClone_map_msg_get(
      dst, 10, &dst_val));
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32Clone_f1(dst_val));

  // It should be a deep copy, not the same pointer.
  EXPECT_NE((const void*)dst_val, (const void*)val);
}

TEST(ConvertTest, ExtensionArrayShallowConversion) {
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
                           upb_test_convert_ext_field_repeated_msg_ext,
                           &ext_val, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownRepeatedMsg_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithKnownRepeatedMsg* dst =
      (const upb_test_convert_MessageWithKnownRepeatedMsg*)dst_msg;

  size_t size;
  const upb_test_convert_MessageWithInt32* const* dst_arr =
      upb_test_convert_MessageWithKnownRepeatedMsg_known_repeated_msg(dst,
                                                                      &size);
  EXPECT_EQ(1, size);
  EXPECT_EQ(456, upb_test_convert_MessageWithInt32_f1(dst_arr[0]));

  // Due to minitable identity match, elements are shallow-copied.
  EXPECT_EQ((const void*)dst_arr[0], (const void*)val);
}

TEST(ConvertTest, ExtensionArrayDeepConversion) {
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
                           upb_test_convert_ext_field_repeated_msg_ext,
                           &ext_val, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownRepeatedMsgClone_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithKnownRepeatedMsgClone* dst =
      (const upb_test_convert_MessageWithKnownRepeatedMsgClone*)dst_msg;

  size_t size;
  const upb_test_convert_MessageWithInt32Clone* const* dst_arr =
      upb_test_convert_MessageWithKnownRepeatedMsgClone_known_repeated_msg(
          dst, &size);
  EXPECT_EQ(1, size);
  EXPECT_EQ(789, upb_test_convert_MessageWithInt32Clone_f1(dst_arr[0]));

  // Deep conversion means the pointers are not equal.
  EXPECT_NE((const void*)dst_arr[0], (const void*)val);
}

#if !defined(NDEBUG)
TEST(ConvertTest, MismatchedExtensionFails) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  // Set extension field 1000 to an int32
  upb_MessageValue ext_val;
  ext_val.int32_val = 12345;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           upb_test_convert_ext_field_int32_ext, &ext_val,
                           arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;

  // Convert to a message where field 1000 is an int64 instead of int32.
  // The type mismatch should cause it to skip setting the regular field,
  // but it should preserve the int32 wire data into the unknown fields.
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownInt64_msg_init;

  EXPECT_DEATH(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                   arena.ptr()),
               "");
}

TEST(ConvertTest, ConvertField_IncompatibleCType) {
  upb::Arena arena;
  upb_test_convert_MessageWithString* msg =
      upb_test_convert_MessageWithString_new(arena.ptr());
  upb_test_convert_MessageWithString_set_f1(msg,
                                            upb_StringView_FromString("hello"));

  const upb_MiniTable* src_mt = &upb__test__convert__MessageWithString_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__MessageWithInt32_msg_init;

  EXPECT_DEATH(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                   arena.ptr()),
               "");
}

TEST(ConvertTest, ConvertField_ArrayIncompatibleCType) {
  upb::Arena arena;
  upb_test_convert_MessageWithRepeatedString* msg =
      upb_test_convert_MessageWithRepeatedString_new(arena.ptr());
  upb_StringView* arr =
      upb_test_convert_MessageWithRepeatedString_resize_r(msg, 1, arena.ptr());
  arr[0] = upb_StringView_FromString("hello");

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithRepeatedString_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithRepeatedInt32_msg_init;

  EXPECT_DEATH(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                   arena.ptr()),
               "");
}

TEST(ConvertTest, ConvertField_TypeMismatch) {
  upb::Arena arena;
  upb_test_convert_MessageWithInt32* msg =
      upb_test_convert_MessageWithInt32_new(arena.ptr());
  upb_test_convert_MessageWithInt32_set_f1(msg, 12345);

  const upb_MiniTable* src_mt = &upb__test__convert__MessageWithInt32_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__MessageWithInt64_msg_init;

  EXPECT_DEATH(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                   arena.ptr()),
               "");
}

TEST(ConvertTest, ConvertField_MapTypeMismatch) {
  upb::Arena arena;
  upb_test_convert_MessageWithMapInt32Int32* msg =
      upb_test_convert_MessageWithMapInt32Int32_new(arena.ptr());
  upb_test_convert_MessageWithMapInt32Int32_m_set(msg, 123, 456, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithMapInt32Int32_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithMapInt32Int64_msg_init;

  EXPECT_DEATH(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                   arena.ptr()),
               "");
}
#endif

TEST(ConvertTest, ConvertField_SingularMessageDeep) {
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

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithMsgClone* dst =
      (const upb_test_convert_MessageWithMsgClone*)dst_msg;

  const upb_test_convert_MessageWithInt32Clone* dst_sub =
      upb_test_convert_MessageWithMsgClone_msg(dst);
  EXPECT_NE(dst_sub, nullptr);
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32Clone_f1(dst_sub));

  // Verify deep copy logic occurred correctly
  EXPECT_NE((const void*)dst_sub, (const void*)sub);
}

TEST(ConvertTest, ConvertField_ArrayMessageShallow) {
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

  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithRepeatedMsg* dst =
      (const upb_test_convert_MessageWithRepeatedMsg*)dst_msg;

  size_t size;
  const upb_test_convert_MessageWithInt32* const* dst_arr =
      upb_test_convert_MessageWithRepeatedMsg_msgs(dst, &size);
  EXPECT_EQ(1, size);
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32_f1(dst_arr[0]));

  // Shallow copy because the destination MiniTable array element is identical
  EXPECT_EQ((const void*)dst_arr[0], (const void*)sub);
}

TEST(ConvertTest, ConvertField_ArrayMessageDeep) {
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

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithRepeatedMsgClone* dst =
      (const upb_test_convert_MessageWithRepeatedMsgClone*)dst_msg;

  size_t size;
  const upb_test_convert_MessageWithInt32Clone* const* dst_arr =
      upb_test_convert_MessageWithRepeatedMsgClone_msgs(dst, &size);
  EXPECT_EQ(1, size);
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32Clone_f1(dst_arr[0]));

  // Deep copy expected
  EXPECT_NE((const void*)dst_arr[0], (const void*)sub);
}

TEST(ConvertTest, ConvertExtensions_ScalarMatch) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_MessageValue ext_val;
  ext_val.int32_val = 123;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           upb_test_convert_ext_field_int32_ext, &ext_val,
                           arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__MessageWithKnown_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithKnown* dst =
      (const upb_test_convert_MessageWithKnown*)dst_msg;
  EXPECT_TRUE(upb_test_convert_MessageWithKnown_has_known_field_int32(dst));
  EXPECT_EQ(123, upb_test_convert_MessageWithKnown_known_field_int32(dst));
}

TEST(ConvertTest, ConvertExtensions_SingularMessageShallow) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_test_convert_MessageWithInt32* sub =
      upb_test_convert_MessageWithInt32_new(arena.ptr());
  upb_test_convert_MessageWithInt32_set_f1(sub, 123);

  upb_MessageValue ext_val;
  ext_val.msg_val = UPB_UPCAST(sub);
  upb_Message_SetExtension(UPB_UPCAST(msg), upb_test_convert_ext_field_msg_ext,
                           &ext_val, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownMsg_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithKnownMsg* dst =
      (const upb_test_convert_MessageWithKnownMsg*)dst_msg;
  const upb_test_convert_MessageWithInt32* dst_sub =
      upb_test_convert_MessageWithKnownMsg_known_msg(dst);
  EXPECT_NE(dst_sub, nullptr);
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32_f1(dst_sub));

  // Shallow copy expected since we are using the exact same minitable
  // underlying message
  EXPECT_EQ((const void*)dst_sub, (const void*)sub);
}

TEST(ConvertTest, ConvertExtensions_SingularMessageDeep) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_test_convert_MessageWithInt32* sub =
      upb_test_convert_MessageWithInt32_new(arena.ptr());
  upb_test_convert_MessageWithInt32_set_f1(sub, 123);

  upb_MessageValue ext_val;
  ext_val.msg_val = UPB_UPCAST(sub);
  upb_Message_SetExtension(UPB_UPCAST(msg), upb_test_convert_ext_field_msg_ext,
                           &ext_val, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownMsgClone_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithKnownMsgClone* dst =
      (const upb_test_convert_MessageWithKnownMsgClone*)dst_msg;
  const upb_test_convert_MessageWithInt32Clone* dst_sub =
      upb_test_convert_MessageWithKnownMsgClone_known_msg(dst);
  EXPECT_NE(dst_sub, nullptr);
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32Clone_f1(dst_sub));

  // Deep copy expected because of mismatched minitables
  EXPECT_NE((const void*)dst_sub, (const void*)sub);
}

TEST(ConvertTest, ConvertExtensions_RemainsExtension) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_MessageValue ext_val;
  ext_val.int32_val = 123;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           upb_test_convert_ext_field_int32_ext, &ext_val,
                           arena.ptr());

  const upb_MiniTable* mt = &upb__test__convert__MessageWithExtension_msg_init;

  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithExtension* dst =
      (const upb_test_convert_MessageWithExtension*)dst_msg;

  // Extension should still be perfectly preserved on the destination message.
  EXPECT_TRUE(upb_Message_HasExtension(UPB_UPCAST(dst),
                                       upb_test_convert_ext_field_int32_ext));

  int32_t out_val = upb_Message_GetExtensionInt32(
      UPB_UPCAST(dst), upb_test_convert_ext_field_int32_ext, 0);
  EXPECT_EQ(123, out_val);
}

TEST(ConvertTest, ConvertExtensions_LookupExtensionInRegistry) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_MessageValue ext_val;
  ext_val.int32_val = 123;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           upb_test_convert_ext_field_int32_ext, &ext_val,
                           arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__AnotherMessageWithExtension_msg_init;

  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena.ptr());
  upb_ExtensionRegistry_Add(extreg,
                            upb_test_convert_another_ext_field_int32_ext);

  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, extreg, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);

  const upb_test_convert_AnotherMessageWithExtension* dst =
      (const upb_test_convert_AnotherMessageWithExtension*)dst_msg;

  // Extension should be found via registry and converted.
  EXPECT_TRUE(upb_Message_HasExtension(
      UPB_UPCAST(dst), upb_test_convert_another_ext_field_int32_ext));

  int32_t out_val = upb_Message_GetExtensionInt32(
      UPB_UPCAST(dst), upb_test_convert_another_ext_field_int32_ext, 0);
  EXPECT_EQ(123, out_val);
}

TEST(ConvertTest, ConvertExtensionToNonExtendable) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_MessageValue ext_val;
  ext_val.int32_val = 123;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           upb_test_convert_ext_field_int32_ext, &ext_val,
                           arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt = &upb_0test__EmptyMessage_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  EXPECT_NE(dst_msg, nullptr);

  // Dst should have unknown field 1000 with value 123.
  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(dst_msg, &data, &iter));
  EXPECT_EQ(data.size, 3);
  EXPECT_EQ((uint8_t)data.data[0], 0xC0);
  EXPECT_EQ((uint8_t)data.data[1], 0x3E);
  EXPECT_EQ((uint8_t)data.data[2], 0x7B);
  EXPECT_FALSE(upb_Message_NextUnknown(dst_msg, &data, &iter));
}

TEST(ConvertTest, OneofDemotion) {
  upb::Arena arena;
  upb_test_convert_SrcWithOneof* msg =
      upb_test_convert_SrcWithOneof_new(arena.ptr());
  // Set both fields of the oneof. The last one should win and the first should
  // be dropped.
  upb_test_convert_SrcWithOneof_set_oneof_int32(msg, 54321);
  upb_test_convert_SrcWithOneof_set_oneof_string(
      msg, upb_StringView_FromString("test"));

  // Convert to Empty message. Field 2 should become unknown.
  const upb_MiniTable* empty_mt = &upb_0test__EmptyMessage_msg_init;
  const upb_MiniTable* src_mt = &upb__test__convert__SrcWithOneof_msg_init;

  const upb_Message* dst = upb_Message_Convert(UPB_UPCAST(msg), src_mt,
                                               empty_mt, nullptr, arena.ptr());
  EXPECT_NE(dst, nullptr);

  // Dst should have unknown field 2 with value "test".
  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  EXPECT_TRUE(upb_Message_NextUnknown(dst, &data, &iter));
  // "test" string field 2.
  // Tag 2, type 2: 2<<3 | 2 = 18
  EXPECT_EQ(data.size, 6);
  EXPECT_EQ((uint8_t)data.data[0], 18);
  EXPECT_EQ((uint8_t)data.data[1], 4);
  EXPECT_EQ((uint8_t)data.data[2], 't');
  EXPECT_EQ((uint8_t)data.data[3], 'e');
  EXPECT_EQ((uint8_t)data.data[4], 's');
  EXPECT_EQ((uint8_t)data.data[5], 't');
  EXPECT_FALSE(upb_Message_NextUnknown(dst, &data, &iter));

  // Now convert back and verify.
  const upb_Message* dst2_msg =
      upb_Message_Convert(dst, empty_mt, src_mt, nullptr, arena.ptr());
  EXPECT_NE(dst2_msg, nullptr);
  const upb_test_convert_SrcWithOneof* dst2 =
      (const upb_test_convert_SrcWithOneof*)dst2_msg;
  EXPECT_EQ(upb_test_convert_SrcWithOneof_my_oneof_case(dst2),
            upb_test_convert_SrcWithOneof_my_oneof_oneof_string);
  upb_StringView str = upb_test_convert_SrcWithOneof_oneof_string(dst2);
  EXPECT_EQ(std::string("test"), std::string(str.data, str.size));
}
