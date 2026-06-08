// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/convert.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>

#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/test_messages_proto3.upb.h"
#include "google/protobuf/test_messages_proto3.upb_minitable.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.hpp"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/convert_test.pb.h"
#include "upb/message/convert_test.upb.h"
#include "upb/message/convert_test.upb_minitable.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/message/test.upb_minitable.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/message.h"
#include "upb/test/parse_text_proto.h"
#include "upb/wire/decode.h"

// Must be last to ensure UPB_PRIVATE is defined.
#include "upb/port/def.inc"

// We use the generated upb_MiniTables from test_messages_proto3.
#define TEST_MT &protobuf_0test_0messages__proto3__TestAllTypesProto3_msg_init

template <typename CppT, typename UpbT>
UpbT* ParseUpbTextProto(
    absl::string_view text_proto, const upb_MiniTable* mt,
    std::initializer_list<const upb_MiniTableExtension*> exts,
    upb::Arena* arena) {
  CppT cpp_msg = upb_test::ParseTextProtoOrDie(text_proto);
  std::string serialized = cpp_msg.SerializeAsString();
  upb_ExtensionRegistry* extreg = nullptr;
  if (exts.size() > 0) {
    extreg = upb_ExtensionRegistry_New(arena->ptr());
    for (const upb_MiniTableExtension* ext : exts) {
      upb_ExtensionRegistry_Add(extreg, ext);
    }
  }
  UpbT* upb_msg = (UpbT*)upb_Message_New(mt, arena->ptr());
  upb_DecodeStatus status =
      upb_Decode(serialized.data(), serialized.size(), UPB_UPCAST(upb_msg), mt,
                 extreg, 0, arena->ptr());
  EXPECT_EQ(status, kUpb_DecodeStatus_Ok);
  return upb_msg;
}

upb_test_convert_MessageWithExtension* ParseMessageWithExtension(
    absl::string_view text_proto,
    std::initializer_list<const upb_MiniTableExtension*> exts,
    upb::Arena* arena) {
  return ParseUpbTextProto<upb::test::convert::MessageWithExtension,
                           upb_test_convert_MessageWithExtension>(
      text_proto, &upb__test__convert__MessageWithExtension_msg_init, exts,
      arena);
}

TEST(ConvertTest, Identity) {
  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      ParseUpbTextProto<protobuf_test_messages::proto3::TestAllTypesProto3,
                        protobuf_test_messages_proto3_TestAllTypesProto3>(
          R"pb(
            optional_int32: 123 optional_string: "hello"
          )pb",
          TEST_MT, {}, &arena);

  const upb_MiniTable* mt = TEST_MT;

  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);
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
      ParseUpbTextProto<protobuf_test_messages::proto3::TestAllTypesProto3,
                        protobuf_test_messages_proto3_TestAllTypesProto3>(
          R"pb(
            optional_nested_message { a: 456 }
          )pb",
          TEST_MT, {}, &arena);

  const protobuf_test_messages_proto3_TestAllTypesProto3_NestedMessage* sub =
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_nested_message(
          msg);

  const upb_MiniTable* mt = TEST_MT;

  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);
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
  ASSERT_NE(dst_msg, nullptr);
  const protobuf_test_messages_proto3_TestAllTypesProto3* dst =
      (const protobuf_test_messages_proto3_TestAllTypesProto3*)dst_msg;

  EXPECT_EQ(
      789,
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_int32(dst));
}

TEST(ConvertTest, Demotion) {
  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      ParseUpbTextProto<protobuf_test_messages::proto3::TestAllTypesProto3,
                        protobuf_test_messages_proto3_TestAllTypesProto3>(
          R"pb(
            optional_int32: 999
          )pb",
          TEST_MT, {}, &arena);

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
      ParseUpbTextProto<protobuf_test_messages::proto3::TestAllTypesProto3,
                        protobuf_test_messages_proto3_TestAllTypesProto3>(
          R"pb(
            map_int32_int32 { key: 10 value: 20 }
          )pb",
          TEST_MT, {}, &arena);

  const upb_MiniTable* mt = TEST_MT;

  // Self-conversion should work (shallow).
  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);
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
      ParseUpbTextProto<upb::test::convert::MessageWithMapMessage,
                        upb_test_convert_MessageWithMapMessage>(
          R"pb(
            map_msg {
              key: 10
              value { f1: 123 }
            }
          )pb",
          &upb__test__convert__MessageWithMapMessage_msg_init, {}, &arena);

  upb_test_convert_MessageWithInt32* val;
  ASSERT_TRUE(
      upb_test_convert_MessageWithMapMessage_map_msg_get(msg, 10, &val));

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithMapMessage_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithMapMessageClone_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithMapMessageClone* dst =
      (const upb_test_convert_MessageWithMapMessageClone*)dst_msg;

  upb_test_convert_MessageWithInt32Clone* dst_val;
  EXPECT_TRUE(upb_test_convert_MessageWithMapMessageClone_map_msg_get(
      dst, 10, &dst_val));
  EXPECT_EQ(123, upb_test_convert_MessageWithInt32Clone_f1(dst_val));

  // It should be a deep copy, not the same pointer.
  EXPECT_NE((const void*)dst_val, (const void*)val);
}

TEST(ConvertTest, DeepConvertScalarMap) {
  upb::Arena arena;
  upb_test_convert_MessageWithMapInt32Int32* msg =
      ParseUpbTextProto<upb::test::convert::MessageWithMapInt32Int32,
                        upb_test_convert_MessageWithMapInt32Int32>(
          R"pb(
            m { key: 123 value: 456 }
          )pb",
          &upb__test__convert__MessageWithMapInt32Int32_msg_init, {}, &arena);

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithMapInt32Int32_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithMapInt32Int32Clone_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithMapInt32Int32Clone* dst =
      (const upb_test_convert_MessageWithMapInt32Int32Clone*)dst_msg;

  int32_t val;
  EXPECT_TRUE(
      upb_test_convert_MessageWithMapInt32Int32Clone_m_get(dst, 123, &val));
  EXPECT_EQ(456, val);
}

TEST(ConvertTest, ExtensionArrayShallowConversion) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_repeated_msg] { f1: 456 }
      )pb",
      {upb_test_convert_ext_field_repeated_msg_ext}, &arena);

  const upb_Array* ext_arr = upb_Message_GetExtensionArray(
      UPB_UPCAST(msg), upb_test_convert_ext_field_repeated_msg_ext);
  ASSERT_NE(ext_arr, nullptr);
  const upb_test_convert_MessageWithInt32* val =
      (const upb_test_convert_MessageWithInt32*)upb_Array_Get(ext_arr, 0)
          .msg_val;

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownRepeatedMsg_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

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
  upb_test_convert_MessageWithExtension* msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_repeated_msg] { f1: 789 }
      )pb",
      {upb_test_convert_ext_field_repeated_msg_ext}, &arena);

  const upb_Array* ext_arr = upb_Message_GetExtensionArray(
      UPB_UPCAST(msg), upb_test_convert_ext_field_repeated_msg_ext);
  ASSERT_NE(ext_arr, nullptr);
  const upb_test_convert_MessageWithInt32* val =
      (const upb_test_convert_MessageWithInt32*)upb_Array_Get(ext_arr, 0)
          .msg_val;

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownRepeatedMsgClone_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

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

TEST(ConvertTest, MismatchedExtensionFails) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_int32]: 12345
      )pb",
      {upb_test_convert_ext_field_int32_ext}, &arena);

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;

  // Convert to a message where field 1000 is an int64 instead of int32.
  // The type mismatch should cause it to skip setting the regular field,
  // but it should preserve the int32 wire data into the unknown fields.
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownInt64_msg_init;

  ASSERT_EQ(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                arena.ptr()),
            nullptr);
}

TEST(ConvertTest, ConvertField_IncompatibleCType) {
  upb::Arena arena;
  upb_test_convert_MessageWithString* msg =
      ParseUpbTextProto<upb::test::convert::MessageWithString,
                        upb_test_convert_MessageWithString>(
          R"pb(
            f1: "hello"
          )pb",
          &upb__test__convert__MessageWithString_msg_init, {}, &arena);

  const upb_MiniTable* src_mt = &upb__test__convert__MessageWithString_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__MessageWithInt32_msg_init;

  ASSERT_EQ(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                arena.ptr()),
            nullptr);
}

TEST(ConvertTest, ConvertField_ArrayIncompatibleCType) {
  upb::Arena arena;
  upb_test_convert_MessageWithRepeatedString* msg =
      ParseUpbTextProto<upb::test::convert::MessageWithRepeatedString,
                        upb_test_convert_MessageWithRepeatedString>(
          R"pb(
            r: "hello"
          )pb",
          &upb__test__convert__MessageWithRepeatedString_msg_init, {}, &arena);

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithRepeatedString_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithRepeatedInt32_msg_init;

  ASSERT_EQ(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                arena.ptr()),
            nullptr);
}

TEST(ConvertTest, ConvertField_TypeMismatch) {
  upb::Arena arena;
  upb_test_convert_MessageWithInt32* msg =
      ParseUpbTextProto<upb::test::convert::MessageWithInt32,
                        upb_test_convert_MessageWithInt32>(
          R"pb(
            f1: 12345
          )pb",
          &upb__test__convert__MessageWithInt32_msg_init, {}, &arena);

  const upb_MiniTable* src_mt = &upb__test__convert__MessageWithInt32_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__MessageWithInt64_msg_init;

  ASSERT_EQ(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                arena.ptr()),
            nullptr);
}

TEST(ConvertTest, ConvertField_ModeMismatch_ScalarToArray) {
  upb::Arena arena;
  upb_test_convert_MessageWithInt32* msg =
      ParseUpbTextProto<upb::test::convert::MessageWithInt32,
                        upb_test_convert_MessageWithInt32>(
          R"pb(
            f1: 12345
          )pb",
          &upb__test__convert__MessageWithInt32_msg_init, {}, &arena);

  const upb_MiniTable* src_mt = &upb__test__convert__MessageWithInt32_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithRepeatedInt32_msg_init;

  ASSERT_EQ(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                arena.ptr()),
            nullptr);
}

TEST(ConvertTest, ConvertField_ModeMismatch_ArrayToScalar) {
  upb::Arena arena;
  upb_test_convert_MessageWithRepeatedInt32* msg =
      ParseUpbTextProto<upb::test::convert::MessageWithRepeatedInt32,
                        upb_test_convert_MessageWithRepeatedInt32>(
          R"pb(
            r: 12345
          )pb",
          &upb__test__convert__MessageWithRepeatedInt32_msg_init, {}, &arena);

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithRepeatedInt32_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__MessageWithInt32_msg_init;

  ASSERT_EQ(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                arena.ptr()),
            nullptr);
}

TEST(ConvertTest, ConvertField_ModeMismatch_ScalarToMap) {
  upb::Arena arena;
  upb_test_convert_MessageWithInt32* msg =
      ParseUpbTextProto<upb::test::convert::MessageWithInt32,
                        upb_test_convert_MessageWithInt32>(
          R"pb(
            f1: 12345
          )pb",
          &upb__test__convert__MessageWithInt32_msg_init, {}, &arena);

  const upb_MiniTable* src_mt = &upb__test__convert__MessageWithInt32_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithMapInt32Int32_msg_init;

  ASSERT_EQ(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                arena.ptr()),
            nullptr);
}

TEST(ConvertTest, ConvertField_MapTypeMismatch) {
  upb::Arena arena;
  upb_test_convert_MessageWithMapInt32Int32* msg =
      ParseUpbTextProto<upb::test::convert::MessageWithMapInt32Int32,
                        upb_test_convert_MessageWithMapInt32Int32>(
          R"pb(
            m { key: 123 value: 456 }
          )pb",
          &upb__test__convert__MessageWithMapInt32Int32_msg_init, {}, &arena);

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithMapInt32Int32_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithMapInt32Int64_msg_init;

  ASSERT_EQ(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                arena.ptr()),
            nullptr);
}

TEST(ConvertTest, ConvertField_SingularMessageDeep) {
  upb::Arena arena;
  upb_test_convert_MessageWithMsg* msg =
      ParseUpbTextProto<upb::test::convert::MessageWithMsg,
                        upb_test_convert_MessageWithMsg>(
          R"pb(
            msg { f1: 123 }
          )pb",
          &upb__test__convert__MessageWithMsg_msg_init, {}, &arena);
  const upb_test_convert_MessageWithInt32* sub =
      upb_test_convert_MessageWithMsg_msg(msg);
  ASSERT_NE(sub, nullptr);

  const upb_MiniTable* src_mt = &upb__test__convert__MessageWithMsg_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithMsgClone_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

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
      ParseUpbTextProto<upb::test::convert::MessageWithRepeatedMsg,
                        upb_test_convert_MessageWithRepeatedMsg>(
          R"pb(
            msgs { f1: 123 }
          )pb",
          &upb__test__convert__MessageWithRepeatedMsg_msg_init, {}, &arena);
  size_t src_size;
  const upb_test_convert_MessageWithInt32* sub =
      upb_test_convert_MessageWithRepeatedMsg_msgs(msg, &src_size)[0];

  const upb_MiniTable* mt =
      &upb__test__convert__MessageWithRepeatedMsg_msg_init;

  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

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
      ParseUpbTextProto<upb::test::convert::MessageWithRepeatedMsg,
                        upb_test_convert_MessageWithRepeatedMsg>(
          R"pb(
            msgs { f1: 123 }
          )pb",
          &upb__test__convert__MessageWithRepeatedMsg_msg_init, {}, &arena);
  size_t src_size;
  const upb_test_convert_MessageWithInt32* sub =
      upb_test_convert_MessageWithRepeatedMsg_msgs(msg, &src_size)[0];

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithRepeatedMsg_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithRepeatedMsgClone_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

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
  upb_test_convert_MessageWithExtension* msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_int32]: 123
      )pb",
      {upb_test_convert_ext_field_int32_ext}, &arena);

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__MessageWithKnown_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithKnown* dst =
      (const upb_test_convert_MessageWithKnown*)dst_msg;
  EXPECT_TRUE(upb_test_convert_MessageWithKnown_has_known_field_int32(dst));
  EXPECT_EQ(123, upb_test_convert_MessageWithKnown_known_field_int32(dst));
}

TEST(ConvertTest, ConvertExtensions_SingularMessageShallow) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_msg] { f1: 123 }
      )pb",
      {upb_test_convert_ext_field_msg_ext}, &arena);
  const upb_Message* sub_msg = upb_Message_GetExtensionMessage(
      UPB_UPCAST(msg), upb_test_convert_ext_field_msg_ext, nullptr);
  ASSERT_NE(sub_msg, nullptr);
  const upb_test_convert_MessageWithInt32* sub =
      (const upb_test_convert_MessageWithInt32*)sub_msg;

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownMsg_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

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
  upb_test_convert_MessageWithExtension* msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_msg] { f1: 123 }
      )pb",
      {upb_test_convert_ext_field_msg_ext}, &arena);
  const upb_Message* sub_msg = upb_Message_GetExtensionMessage(
      UPB_UPCAST(msg), upb_test_convert_ext_field_msg_ext, nullptr);
  ASSERT_NE(sub_msg, nullptr);
  const upb_test_convert_MessageWithInt32* sub =
      (const upb_test_convert_MessageWithInt32*)sub_msg;

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownMsgClone_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

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
  upb_test_convert_MessageWithExtension* msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_int32]: 123
      )pb",
      {upb_test_convert_ext_field_int32_ext}, &arena);

  const upb_MiniTable* mt = &upb__test__convert__MessageWithExtension_msg_init;

  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

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
  upb_test_convert_MessageWithExtension* msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_int32]: 123
      )pb",
      {upb_test_convert_ext_field_int32_ext}, &arena);

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__AnotherMessageWithExtension_msg_init;

  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena.ptr());
  upb_ExtensionRegistry_Add(extreg,
                            upb_test_convert_another_ext_field_int32_ext);

  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, extreg, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

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
  upb_test_convert_MessageWithExtension* msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_int32]: 123
      )pb",
      {upb_test_convert_ext_field_int32_ext}, &arena);

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt = &upb_0test__EmptyMessage_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

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

TEST(ConvertTest, ConvertExtensionToExtendableButUnknown) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_int32]: 123
      )pb",
      {upb_test_convert_ext_field_int32_ext}, &arena);

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__AnotherMessageWithExtension_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  // The destination message (AnotherMessageWithExtension) supports extensions,
  // but since we did not provide an extension registry containing field 1000 to
  // the conversion, the extension is treated as unknown/non-canonical in the
  // destination schemas. Therefore, it should be encoded as an unknown field
  // (field 1000 with value 123).
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

TEST(ConvertTest, OpenToClosedEnum) {
  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      ParseUpbTextProto<protobuf_test_messages::proto3::TestAllTypesProto3,
                        protobuf_test_messages_proto3_TestAllTypesProto3>(
          R"pb(
            optional_nested_enum: BAR
          )pb",
          TEST_MT, {}, &arena);

  const upb_MiniTable* src_mt =
      &protobuf_0test_0messages__proto3__TestAllTypesProto3_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__Proto2EnumMessage_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);
  const upb_test_convert_Proto2EnumMessage* dst =
      (const upb_test_convert_Proto2EnumMessage*)dst_msg;
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_BAR,
            upb_test_convert_Proto2EnumMessage_optional_nested_enum(dst));
}

TEST(ConvertTest, OpenToClosedEnum_InvalidValueInClosedEnum) {
  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      ParseUpbTextProto<protobuf_test_messages::proto3::TestAllTypesProto3,
                        protobuf_test_messages_proto3_TestAllTypesProto3>(
          R"pb(
            optional_nested_enum: 12345
          )pb",
          TEST_MT, {}, &arena);

  const upb_MiniTable* src_mt =
      &protobuf_0test_0messages__proto3__TestAllTypesProto3_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__Proto2EnumMessage_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_Proto2EnumMessage* dst =
      (const upb_test_convert_Proto2EnumMessage*)dst_msg;
  EXPECT_FALSE(
      upb_test_convert_Proto2EnumMessage_has_optional_nested_enum(dst));

  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(dst_msg, &data, &iter));
  EXPECT_GT(data.size, 0);

  protobuf_test_messages_proto3_TestAllTypesProto3* check_msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_parse(
          data.data, data.size, arena.ptr());
  ASSERT_NE(check_msg, nullptr);

  EXPECT_EQ(
      12345,
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_nested_enum(
          check_msg));
}

TEST(ConvertTest, OpenToClosedMapEnum) {
  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      ParseUpbTextProto<protobuf_test_messages::proto3::TestAllTypesProto3,
                        protobuf_test_messages_proto3_TestAllTypesProto3>(
          R"pb(
            map_string_nested_enum { key: "valid1" value: BAR }
            map_string_nested_enum { key: "valid2" value: BAZ }
          )pb",
          TEST_MT, {}, &arena);

  const upb_MiniTable* src_mt =
      &protobuf_0test_0messages__proto3__TestAllTypesProto3_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__Proto2EnumMessage_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);
  const upb_test_convert_Proto2EnumMessage* dst =
      (const upb_test_convert_Proto2EnumMessage*)dst_msg;

  // Valid enum should be present
  int val1;
  EXPECT_TRUE(upb_test_convert_Proto2EnumMessage_map_string_nested_enum_get(
      dst, upb_StringView_FromString("valid1"), &val1));
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_BAR, val1);
  int val2;
  EXPECT_TRUE(upb_test_convert_Proto2EnumMessage_map_string_nested_enum_get(
      dst, upb_StringView_FromString("valid2"), &val2));
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_BAZ, val2);
}

TEST(ConvertTest, OpenToClosedMapEnum_InvalidValueInClosedEnum) {
  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      ParseUpbTextProto<protobuf_test_messages::proto3::TestAllTypesProto3,
                        protobuf_test_messages_proto3_TestAllTypesProto3>(
          R"pb(
            map_string_nested_enum { key: "valid" value: BAR }
            map_string_nested_enum { key: "invalid" value: 12345 }
          )pb",
          TEST_MT, {}, &arena);

  const upb_MiniTable* src_mt =
      &protobuf_0test_0messages__proto3__TestAllTypesProto3_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__Proto2EnumMessage_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_Proto2EnumMessage* dst =
      (const upb_test_convert_Proto2EnumMessage*)dst_msg;
  int val;
  EXPECT_TRUE(upb_test_convert_Proto2EnumMessage_map_string_nested_enum_get(
      dst, upb_StringView_FromString("valid"), &val));
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_BAR, val);

  EXPECT_FALSE(upb_test_convert_Proto2EnumMessage_map_string_nested_enum_get(
      dst, upb_StringView_FromString("invalid"), &val));

  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(dst_msg, &data, &iter));
  EXPECT_GT(data.size, 0);

  // Verify that the unknown field contains the complete invalid map entry.
  // We can parse the unknown field data back into the source proto3 message,
  // which treats the enum as open, to verify both the key and the invalid value
  // were perfectly preserved.
  protobuf_test_messages_proto3_TestAllTypesProto3* check_msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_parse(
          data.data, data.size, arena.ptr());
  ASSERT_NE(check_msg, nullptr);

  int check_val;
  EXPECT_TRUE(
      protobuf_test_messages_proto3_TestAllTypesProto3_map_string_nested_enum_get(
          check_msg, upb_StringView_FromString("invalid"), &check_val));
  EXPECT_EQ(12345, check_val);

  // The "valid" map entry should NOT be in the unknown fields.
  EXPECT_FALSE(
      protobuf_test_messages_proto3_TestAllTypesProto3_map_string_nested_enum_get(
          check_msg, upb_StringView_FromString("valid"), &check_val));
}

TEST(ConvertTest, OpenToClosedRepeatedEnum) {
  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      ParseUpbTextProto<protobuf_test_messages::proto3::TestAllTypesProto3,
                        protobuf_test_messages_proto3_TestAllTypesProto3>(
          R"pb(
            repeated_nested_enum: FOO repeated_nested_enum: BAR
          )pb",
          TEST_MT, {}, &arena);

  const upb_MiniTable* src_mt =
      &protobuf_0test_0messages__proto3__TestAllTypesProto3_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__Proto2EnumMessage_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);
  const upb_test_convert_Proto2EnumMessage* dst =
      (const upb_test_convert_Proto2EnumMessage*)dst_msg;
  size_t count;
  const int* values =
      upb_test_convert_Proto2EnumMessage_repeated_nested_enum(dst, &count);
  EXPECT_EQ(2, count);
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_FOO, values[0]);
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_BAR, values[1]);
}

TEST(ConvertTest, OpenToClosedRepeatedEnum_ContainsInvalidValue) {
  upb::Arena arena;
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      ParseUpbTextProto<protobuf_test_messages::proto3::TestAllTypesProto3,
                        protobuf_test_messages_proto3_TestAllTypesProto3>(
          R"pb(
            repeated_nested_enum: 12345 repeated_nested_enum: BAR
          )pb",
          TEST_MT, {}, &arena);

  const upb_MiniTable* src_mt =
      &protobuf_0test_0messages__proto3__TestAllTypesProto3_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__Proto2EnumMessage_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_Proto2EnumMessage* dst =
      (const upb_test_convert_Proto2EnumMessage*)dst_msg;
  size_t count;
  const int* values =
      upb_test_convert_Proto2EnumMessage_repeated_nested_enum(dst, &count);
  ASSERT_EQ(1, count);
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_BAR, values[0]);

  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(dst_msg, &data, &iter));
  EXPECT_GT(data.size, 0);

  protobuf_test_messages_proto3_TestAllTypesProto3* check_msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_parse(
          data.data, data.size, arena.ptr());
  ASSERT_NE(check_msg, nullptr);

  size_t check_count;
  const int* check_values =
      protobuf_test_messages_proto3_TestAllTypesProto3_repeated_nested_enum(
          check_msg, &check_count);
  EXPECT_EQ(1, check_count);
  EXPECT_EQ(12345, check_values[0]);
}

TEST(ConvertTest, OpenToClosedExtensionEnum) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_MessageValue ext_val;
  ext_val.int32_val = upb_test_convert_Proto2EnumMessage_BAR;
  upb_Message_SetExtension(UPB_UPCAST(msg), upb_test_convert_ext_enum_ext,
                           &ext_val, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownEnum_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);
  const upb_test_convert_MessageWithKnownEnum* dst =
      (const upb_test_convert_MessageWithKnownEnum*)dst_msg;

  EXPECT_TRUE(upb_test_convert_MessageWithKnownEnum_has_ext_enum(dst));
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_BAR,
            upb_test_convert_MessageWithKnownEnum_ext_enum(dst));
}

TEST(ConvertTest, OpenToClosedExtensionEnum_InvalidValue) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_MessageValue ext_val;
  ext_val.int32_val = 12345;  // Invalid value.
  upb_Message_SetExtension(UPB_UPCAST(msg), upb_test_convert_ext_enum_ext,
                           &ext_val, arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownEnum_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithKnownEnum* dst =
      (const upb_test_convert_MessageWithKnownEnum*)dst_msg;
  EXPECT_FALSE(upb_test_convert_MessageWithKnownEnum_has_ext_enum(dst));

  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(dst_msg, &data, &iter));
  EXPECT_GT(data.size, 0);

  // We cannot parse it back using the extension registry because the extension
  // itself is a closed enum, so the parser would just put it into unknown
  // fields again. Instead, we verify the raw bytes.
  // Tag: 2000 << 3 | 0 (varint) = 16000 = 0x3E80 -> 0x80 0x7D
  // Value: 12345 = 0x3039 -> 0xB9 0x60
  EXPECT_EQ(data.size, 4);
  EXPECT_EQ((uint8_t)data.data[0], 0x80);
  EXPECT_EQ((uint8_t)data.data[1], 0x7D);
  EXPECT_EQ((uint8_t)data.data[2], 0xB9);
  EXPECT_EQ((uint8_t)data.data[3], 0x60);
}

TEST(ConvertTest, OpenToClosedExtensionRepeatedEnum) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_Array* ext_arr = upb_Array_New(arena.ptr(), kUpb_CType_Enum);
  upb_MessageValue elem_val;
  elem_val.int32_val = upb_test_convert_Proto2EnumMessage_FOO;
  upb_Array_Append(ext_arr, elem_val, arena.ptr());
  elem_val.int32_val = upb_test_convert_Proto2EnumMessage_BAR;
  upb_Array_Append(ext_arr, elem_val, arena.ptr());

  upb_MessageValue ext_val;
  ext_val.array_val = ext_arr;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           upb_test_convert_ext_repeated_enum_ext, &ext_val,
                           arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownEnum_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);
  const upb_test_convert_MessageWithKnownEnum* dst =
      (const upb_test_convert_MessageWithKnownEnum*)dst_msg;

  size_t count;
  const int* values =
      upb_test_convert_MessageWithKnownEnum_ext_repeated_enum(dst, &count);
  EXPECT_EQ(2, count);
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_FOO, values[0]);
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_BAR, values[1]);
}

TEST(ConvertTest, OpenToClosedExtensionRepeatedEnum_InvalidValue) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg =
      upb_test_convert_MessageWithExtension_new(arena.ptr());

  upb_Array* ext_arr = upb_Array_New(arena.ptr(), kUpb_CType_Enum);
  upb_MessageValue elem_val;
  elem_val.int32_val = 12345;  // Invalid value.
  upb_Array_Append(ext_arr, elem_val, arena.ptr());
  elem_val.int32_val = upb_test_convert_Proto2EnumMessage_BAR;
  upb_Array_Append(ext_arr, elem_val, arena.ptr());

  upb_MessageValue ext_val;
  ext_val.array_val = ext_arr;
  upb_Message_SetExtension(UPB_UPCAST(msg),
                           upb_test_convert_ext_repeated_enum_ext, &ext_val,
                           arena.ptr());

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__MessageWithKnownEnum_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithKnownEnum* dst =
      (const upb_test_convert_MessageWithKnownEnum*)dst_msg;
  size_t count = 0;
  const int* values =
      upb_test_convert_MessageWithKnownEnum_ext_repeated_enum(dst, &count);
  EXPECT_EQ(1, count);
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_BAR, values[0]);

  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(dst_msg, &data, &iter));
  EXPECT_GT(data.size, 0);

  // We cannot parse it back using the extension registry because the extension
  // itself is a closed enum, so the parser would just put it into unknown
  // fields again. Instead, we verify the raw bytes.
  // Tag: 2001 << 3 | 0 (varint) = 16008 = 0x3E88 -> 0x88 0x7D
  // Value: 12345 = 0x3039 -> 0xB9 0x60
  EXPECT_EQ(data.size, 4);
  EXPECT_EQ((uint8_t)data.data[0], 0x88);
  EXPECT_EQ((uint8_t)data.data[1], 0x7D);
  EXPECT_EQ((uint8_t)data.data[2], 0xB9);
  EXPECT_EQ((uint8_t)data.data[3], 0x60);
}

TEST(ConvertTest, ExtensionToMapMismatch) {
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
      &upb__test__convert__MessageWithMapAt1000_msg_init;

  // Return NULL when the destination mini table has a map, which is not
  // compatible with the source extension.
  ASSERT_EQ(upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, nullptr,
                                arena.ptr()),
            nullptr);
}

TEST(ConvertTest, OneofPromotion) {
  upb::Arena arena;
  upb_test_convert_SrcWithoutOneof* msg =
      upb_test_convert_SrcWithoutOneof_new(arena.ptr());
  upb_test_convert_SrcWithoutOneof_set_oneof_int32(msg, 123);
  upb_test_convert_SrcWithoutOneof_set_oneof_string(
      msg, upb_StringView_FromString("abc"));

  const upb_MiniTable* src_mt = &upb__test__convert__SrcWithoutOneof_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__SrcWithOneof_msg_init;

  const upb_Message* dst_msg = upb_Message_Convert(
      UPB_UPCAST(msg), src_mt, dst_mt, nullptr, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  // Since both fields in the source message are set (oneof_int32=123 and
  // oneof_string="abc") and are moved into a oneof in the destination schema,
  // only one of them can be set at the end. In this case, because fields are
  // converted in descending order, the higher field number 2 (oneof_string)
  // is converted first, and then field number 1 (oneof_int32) is ignored
  // because the oneof already has a field set. This matches the roundtrip
  // encoding-then-decoding path where field 2 is serialized last and wins.
  const upb_test_convert_SrcWithOneof* dst =
      (const upb_test_convert_SrcWithOneof*)dst_msg;
  EXPECT_EQ(upb_test_convert_SrcWithOneof_my_oneof_case(dst),
            upb_test_convert_SrcWithOneof_my_oneof_oneof_string);
  upb_StringView str = upb_test_convert_SrcWithOneof_oneof_string(dst);
  EXPECT_EQ(std::string("abc"), std::string(str.data, str.size));
}

TEST(ConvertTest, ConvertExtensions_LookupMessageExtensionInRegistry) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* src_msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_msg] { f1: 456 }
      )pb",
      {upb_test_convert_ext_field_msg_ext}, &arena);

  const upb_MiniTable* mt1 = &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* mt2 =
      &upb__test__convert__AnotherMessageWithExtension_msg_init;

  // Convert to mt2 without registry -> becomes unknown field.
  const upb_Message* intermediate_msg =
      upb_Message_Convert(UPB_UPCAST(src_msg), mt1, mt2, nullptr, arena.ptr());
  ASSERT_NE(intermediate_msg, nullptr);

  // Now convert back to mt1 with registry -> promoted back to extension.
  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena.ptr());
  upb_ExtensionRegistry_Add(extreg, upb_test_convert_ext_field_msg_ext);

  const upb_Message* dst_msg =
      upb_Message_Convert(intermediate_msg, mt2, mt1, extreg, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithExtension* dst =
      (const upb_test_convert_MessageWithExtension*)dst_msg;

  EXPECT_TRUE(upb_Message_HasExtension(UPB_UPCAST(dst),
                                       upb_test_convert_ext_field_msg_ext));
  const upb_Message* out_sub = upb_Message_GetExtensionMessage(
      UPB_UPCAST(dst), upb_test_convert_ext_field_msg_ext, nullptr);
  ASSERT_NE(out_sub, nullptr);
  EXPECT_EQ(456, upb_test_convert_MessageWithInt32_f1(
                     (const upb_test_convert_MessageWithInt32*)out_sub));
}

TEST(ConvertTest, ConvertExtensions_LookupRepeatedMessageExtensionInRegistry) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* src_msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_repeated_msg] { f1: 789 }
      )pb",
      {upb_test_convert_ext_field_repeated_msg_ext}, &arena);

  const upb_MiniTable* mt1 = &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* mt2 =
      &upb__test__convert__AnotherMessageWithExtension_msg_init;

  // Convert to mt2 without registry -> becomes unknown field.
  const upb_Message* intermediate_msg =
      upb_Message_Convert(UPB_UPCAST(src_msg), mt1, mt2, nullptr, arena.ptr());
  ASSERT_NE(intermediate_msg, nullptr);

  // Now convert back to mt1 with registry -> promoted back to repeated
  // extension.
  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena.ptr());
  upb_ExtensionRegistry_Add(extreg,
                            upb_test_convert_ext_field_repeated_msg_ext);

  const upb_Message* dst_msg =
      upb_Message_Convert(intermediate_msg, mt2, mt1, extreg, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithExtension* dst =
      (const upb_test_convert_MessageWithExtension*)dst_msg;

  const upb_Array* out_arr = upb_Message_GetExtensionArray(
      UPB_UPCAST(dst), upb_test_convert_ext_field_repeated_msg_ext);
  ASSERT_NE(out_arr, nullptr);
  ASSERT_EQ(1, upb_Array_Size(out_arr));
  const upb_Message* out_elem = upb_Array_Get(out_arr, 0).msg_val;
  EXPECT_EQ(789, upb_test_convert_MessageWithInt32_f1(
                     (const upb_test_convert_MessageWithInt32*)out_elem));
}

TEST(ConvertTest, ConvertExtensions_LookupEnumExtensionInRegistry) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* src_msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_enum]: BAR
      )pb",
      {upb_test_convert_ext_enum_ext}, &arena);

  const upb_MiniTable* mt1 = &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* mt2 =
      &upb__test__convert__AnotherMessageWithExtension_msg_init;

  // Convert to mt2 without registry -> becomes unknown field.
  const upb_Message* intermediate_msg =
      upb_Message_Convert(UPB_UPCAST(src_msg), mt1, mt2, nullptr, arena.ptr());
  ASSERT_NE(intermediate_msg, nullptr);

  // Convert back to mt1 with registry -> promoted back to enum extension.
  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena.ptr());
  upb_ExtensionRegistry_Add(extreg, upb_test_convert_ext_enum_ext);

  const upb_Message* dst_msg =
      upb_Message_Convert(intermediate_msg, mt2, mt1, extreg, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithExtension* dst =
      (const upb_test_convert_MessageWithExtension*)dst_msg;

  EXPECT_TRUE(
      upb_Message_HasExtension(UPB_UPCAST(dst), upb_test_convert_ext_enum_ext));
  int32_t out_val = upb_Message_GetExtensionInt32(
      UPB_UPCAST(dst), upb_test_convert_ext_enum_ext, 0);
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_BAR, out_val);
}

TEST(ConvertTest, ConvertExtensions_LookupRepeatedEnumExtensionInRegistry) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* src_msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_repeated_enum]: FOO
        [upb.test.convert.ext_repeated_enum]: BAZ
      )pb",
      {upb_test_convert_ext_repeated_enum_ext}, &arena);

  const upb_MiniTable* mt1 = &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* mt2 =
      &upb__test__convert__AnotherMessageWithExtension_msg_init;

  // Convert to mt2 without registry -> becomes unknown field.
  const upb_Message* intermediate_msg =
      upb_Message_Convert(UPB_UPCAST(src_msg), mt1, mt2, nullptr, arena.ptr());
  ASSERT_NE(intermediate_msg, nullptr);

  // Convert back to mt1 with registry -> promoted back to repeated enum
  // extension.
  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena.ptr());
  upb_ExtensionRegistry_Add(extreg, upb_test_convert_ext_repeated_enum_ext);

  const upb_Message* dst_msg =
      upb_Message_Convert(intermediate_msg, mt2, mt1, extreg, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithExtension* dst =
      (const upb_test_convert_MessageWithExtension*)dst_msg;

  const upb_Array* out_arr = upb_Message_GetExtensionArray(
      UPB_UPCAST(dst), upb_test_convert_ext_repeated_enum_ext);
  ASSERT_NE(out_arr, nullptr);
  ASSERT_EQ(2, upb_Array_Size(out_arr));
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_FOO,
            upb_Array_Get(out_arr, 0).int32_val);
  EXPECT_EQ(upb_test_convert_Proto2EnumMessage_BAZ,
            upb_Array_Get(out_arr, 1).int32_val);
}

TEST(ConvertTest, ConvertExtensions_RegistryProvidedButUnneeded) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_int32]: 12345
      )pb",
      {upb_test_convert_ext_field_int32_ext}, &arena);

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt = &upb__test__convert__MessageWithKnown_msg_init;

  // Provide an extension registry, even though dst_mt has field 1000 as a
  // regular field.
  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena.ptr());
  upb_ExtensionRegistry_Add(extreg, upb_test_convert_ext_field_int32_ext);

  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, extreg, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithKnown* dst =
      (const upb_test_convert_MessageWithKnown*)dst_msg;
  EXPECT_TRUE(upb_test_convert_MessageWithKnown_has_known_field_int32(dst));
  EXPECT_EQ(12345, upb_test_convert_MessageWithKnown_known_field_int32(dst));
}

TEST(ConvertTest, ConvertExtensions_RegistryMissingExtensionDemotesToUnknown) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_int32]: 123
      )pb",
      {upb_test_convert_ext_field_int32_ext}, &arena);

  const upb_MiniTable* src_mt =
      &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* dst_mt =
      &upb__test__convert__AnotherMessageWithExtension_msg_init;

  // Create an empty extension registry that does not contain field 1000.
  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena.ptr());

  const upb_Message* dst_msg =
      upb_Message_Convert(UPB_UPCAST(msg), src_mt, dst_mt, extreg, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_AnotherMessageWithExtension* dst =
      (const upb_test_convert_AnotherMessageWithExtension*)dst_msg;

  // It should NOT have the extension set, because it wasn't in the registry.
  EXPECT_FALSE(upb_Message_HasExtension(
      UPB_UPCAST(dst), upb_test_convert_another_ext_field_int32_ext));

  // Dst should have unknown field 1000 with value 123.
  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  ASSERT_TRUE(upb_Message_NextUnknown(dst_msg, &data, &iter));
  EXPECT_EQ(data.size, 3);
  EXPECT_EQ((uint8_t)data.data[0], 0xC0);
  EXPECT_EQ((uint8_t)data.data[1], 0x3E);
  EXPECT_EQ((uint8_t)data.data[2], 0x7B);
}

TEST(ConvertTest, ConvertExtensions_LookupMultipleExtensionsInRegistry) {
  upb::Arena arena;
  upb_test_convert_MessageWithExtension* src_msg = ParseMessageWithExtension(
      R"pb(
        [upb.test.convert.ext_field_int32]: 123
        [upb.test.convert.ext_field_msg] { f1: 456 }
      )pb",
      {upb_test_convert_ext_field_int32_ext,
       upb_test_convert_ext_field_msg_ext},
      &arena);

  const upb_MiniTable* mt1 = &upb__test__convert__MessageWithExtension_msg_init;
  const upb_MiniTable* mt2 =
      &upb__test__convert__AnotherMessageWithExtension_msg_init;

  // Convert to mt2 without registry -> 1000 and 1002 become unknown fields.
  const upb_Message* intermediate_msg =
      upb_Message_Convert(UPB_UPCAST(src_msg), mt1, mt2, nullptr, arena.ptr());
  ASSERT_NE(intermediate_msg, nullptr);

  // Convert back to mt1 with registry containing both extensions.
  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena.ptr());
  upb_ExtensionRegistry_Add(extreg, upb_test_convert_ext_field_int32_ext);
  upb_ExtensionRegistry_Add(extreg, upb_test_convert_ext_field_msg_ext);

  const upb_Message* dst_msg =
      upb_Message_Convert(intermediate_msg, mt2, mt1, extreg, arena.ptr());
  ASSERT_NE(dst_msg, nullptr);

  const upb_test_convert_MessageWithExtension* dst =
      (const upb_test_convert_MessageWithExtension*)dst_msg;

  EXPECT_TRUE(upb_Message_HasExtension(UPB_UPCAST(dst),
                                       upb_test_convert_ext_field_int32_ext));
  EXPECT_EQ(123, upb_Message_GetExtensionInt32(
                     UPB_UPCAST(dst), upb_test_convert_ext_field_int32_ext, 0));

  EXPECT_TRUE(upb_Message_HasExtension(UPB_UPCAST(dst),
                                       upb_test_convert_ext_field_msg_ext));
  const upb_Message* out_sub = upb_Message_GetExtensionMessage(
      UPB_UPCAST(dst), upb_test_convert_ext_field_msg_ext, nullptr);
  ASSERT_NE(out_sub, nullptr);
  EXPECT_EQ(456, upb_test_convert_MessageWithInt32_f1(
                     (const upb_test_convert_MessageWithInt32*)out_sub));
}
