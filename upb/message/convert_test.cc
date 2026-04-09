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
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.hpp"
#include "upb/message/accessors.h"
#include "upb/message/convert_test.upb.h"
#include "upb/message/convert_test.upb_minitable.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/message/test.upb.h"
#include "upb/message/test.upb_minitable.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"
#include "upb/wire/internal/encode.h"

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
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, NULL, arena.ptr());
  EXPECT_TRUE(dst_msg != NULL);
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
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, NULL, arena.ptr());
  EXPECT_TRUE(dst_msg != NULL);
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
      upb_Decode(buf, len, msg_empty, empty_mt, NULL, 0, arena.ptr());
  EXPECT_EQ(kUpb_DecodeStatus_Ok, status);

  // 3. Convert `msg_empty` to `dst` (TestAllTypesProto3).
  // `dst` HAS field 1. So it should promote the unknown from `msg_empty`.

  const upb_MiniTable* mt = TEST_MT;

  const upb_Message* dst_msg =
      upb_Message_Convert(msg_empty, empty_mt, mt, NULL, arena.ptr());
  EXPECT_TRUE(dst_msg != NULL);
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
                                               empty_mt, NULL, arena.ptr());
  EXPECT_TRUE(dst != NULL);

  // Dst should have unknown field 1 with value 999.
  size_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  bool found = false;
  while (upb_Message_NextUnknown(dst, &data, &iter)) {
    if (data.size >= 3 && (uint8_t)data.data[0] == 8) {
      if ((uint8_t)data.data[1] == 0xE7 && (uint8_t)data.data[2] == 0x07) {
        found = true;
      }
    }
  }
  EXPECT_TRUE(found);
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
      upb_Message_Convert(UPB_UPCAST(msg), mt, mt, NULL, arena.ptr());
  EXPECT_TRUE(dst_msg != NULL);
  const protobuf_test_messages_proto3_TestAllTypesProto3* dst =
      (const protobuf_test_messages_proto3_TestAllTypesProto3*)dst_msg;

  int32_t val;
  EXPECT_TRUE(
      protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_get(
          dst, 10, &val));
  EXPECT_EQ(20, val);
}
