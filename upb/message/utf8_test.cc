// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdlib.h>

#include <gtest/gtest.h>
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/message/utf8_test.upb.h"
#include "upb/message/utf8_test.upb_minitable.h"
#include "upb/message/utf8_test_proto2.upb.h"
#include "upb/message/utf8_test_proto2.upb_minitable.h"
#include "upb/wire/decode.h"

namespace {

const char bad_utf8[] = "\xff";

static char* GetBadUtf8Payload(upb_Arena* arena, size_t* size) {
  upb_test_TestUtf8Bytes* msg = upb_test_TestUtf8Bytes_new(arena);
  upb_test_TestUtf8Bytes_set_data(msg, upb_StringView_FromString(bad_utf8));
  char* data = upb_test_TestUtf8Bytes_serialize(msg, arena, size);
  EXPECT_TRUE(data != nullptr);
  return data;
}

TEST(Utf8Test, BytesFieldDoesntValidate) {
  upb::Arena arena;
  size_t size;
  char* data = GetBadUtf8Payload(arena.ptr(), &size);
  upb_test_TestUtf8Bytes* msg2 =
      upb_test_TestUtf8Bytes_parse(data, size, arena.ptr());

  // Parse succeeds, because the bytes field does not validate UTF-8.
  ASSERT_TRUE(msg2 != nullptr);
}

TEST(Utf8Test, Proto3FieldValidates) {
  upb::Arena arena;
  size_t size;
  char* data = GetBadUtf8Payload(arena.ptr(), &size);

  upb_test_TestUtf8Proto3String* msg =
      upb_test_TestUtf8Proto3String_new(arena.ptr());

  upb_DecodeStatus status = upb_Decode(
      data, size, UPB_UPCAST(msg), &upb_0test__TestUtf8Proto3String_msg_init,
      nullptr, 0, arena.ptr());

  // Parse fails, because proto3 string fields validate UTF-8.
  ASSERT_EQ(kUpb_DecodeStatus_BadUtf8, status);
}

TEST(Utf8Test, RepeatedProto3FieldValidates) {
  upb::Arena arena;
  size_t size;
  char* data = GetBadUtf8Payload(arena.ptr(), &size);

  upb_test_TestUtf8RepeatedProto3String* msg =
      upb_test_TestUtf8RepeatedProto3String_new(arena.ptr());

  upb_DecodeStatus status =
      upb_Decode(data, size, UPB_UPCAST(msg),
                 &upb_0test__TestUtf8RepeatedProto3String_msg_init, nullptr, 0,
                 arena.ptr());

  // Parse fails, because proto3 string fields validate UTF-8.
  ASSERT_EQ(kUpb_DecodeStatus_BadUtf8, status);
}

TEST(Utf8Test, Proto2BytesValidates) {
  upb::Arena arena;
  size_t size;
  char* data = GetBadUtf8Payload(arena.ptr(), &size);

  upb_test_TestUtf8Proto2Bytes* msg =
      upb_test_TestUtf8Proto2Bytes_new(arena.ptr());

  upb_DecodeStatus status;
  status = upb_Decode(data, size, UPB_UPCAST(msg),
                      &upb_0test__TestUtf8Proto2Bytes_msg_init, nullptr, 0,
                      arena.ptr());

  // Parse succeeds, because proto2 bytes fields don't validate UTF-8.
  ASSERT_EQ(kUpb_DecodeStatus_Ok, status);
}

TEST(Utf8Test, Proto2RepeatedBytesValidates) {
  upb::Arena arena;
  size_t size;
  char* data = GetBadUtf8Payload(arena.ptr(), &size);

  upb_test_TestUtf8RepeatedProto2Bytes* msg =
      upb_test_TestUtf8RepeatedProto2Bytes_new(arena.ptr());

  upb_DecodeStatus status;
  status = upb_Decode(data, size, UPB_UPCAST(msg),
                      &upb_0test__TestUtf8RepeatedProto2Bytes_msg_init, nullptr,
                      0, arena.ptr());

  // Parse succeeds, because proto2 bytes fields don't validate UTF-8.
  ASSERT_EQ(kUpb_DecodeStatus_Ok, status);
}

TEST(Utf8Test, Proto2StringValidates) {
  upb::Arena arena;
  size_t size;
  char* data = GetBadUtf8Payload(arena.ptr(), &size);

  upb_test_TestUtf8Proto2String* msg =
      upb_test_TestUtf8Proto2String_new(arena.ptr());

  upb_DecodeStatus status;
  status = upb_Decode(data, size, UPB_UPCAST(msg),
                      &upb_0test__TestUtf8Proto2String_msg_init, nullptr, 0,
                      arena.ptr());

  // Parse succeeds, because proto2 string fields don't validate UTF-8.
  ASSERT_EQ(kUpb_DecodeStatus_Ok, status);
}

TEST(Utf8Test, Proto2FieldFailsValidation) {
  upb::Arena arena;
  size_t size;
  char* data = GetBadUtf8Payload(arena.ptr(), &size);

  upb_test_TestUtf8Proto2String* msg =
      upb_test_TestUtf8Proto2String_new(arena.ptr());

  upb_DecodeStatus status;
  status = upb_Decode(data, size, UPB_UPCAST(msg),
                      &upb_0test__TestUtf8Proto2String_msg_init, nullptr, 0,
                      arena.ptr());

  // Parse fails, because we pass in kUpb_DecodeOption_AlwaysValidateUtf8 to
  // force validation of proto2 string fields.
  status = upb_Decode(data, size, UPB_UPCAST(msg),
                      &upb_0test__TestUtf8Proto2String_msg_init, nullptr,
                      kUpb_DecodeOption_AlwaysValidateUtf8, arena.ptr());
  ASSERT_EQ(kUpb_DecodeStatus_BadUtf8, status);
}

TEST(Utf8Test, Proto2RepeatedFieldFailsValidation) {
  upb::Arena arena;
  size_t size;
  char* data = GetBadUtf8Payload(arena.ptr(), &size);

  upb_test_TestUtf8RepeatedProto2String* msg =
      upb_test_TestUtf8RepeatedProto2String_new(arena.ptr());

  upb_DecodeStatus status;
  status = upb_Decode(data, size, UPB_UPCAST(msg),
                      &upb_0test__TestUtf8RepeatedProto2String_msg_init,
                      nullptr, 0, arena.ptr());

  // Parse fails, because we pass in kUpb_DecodeOption_AlwaysValidateUtf8 to
  // force validation of proto2 string fields.
  status =
      upb_Decode(data, size, UPB_UPCAST(msg),
                 &upb_0test__TestUtf8RepeatedProto2String_msg_init, nullptr,
                 kUpb_DecodeOption_AlwaysValidateUtf8, arena.ptr());
  ASSERT_EQ(kUpb_DecodeStatus_BadUtf8, status);
}

}  // namespace
