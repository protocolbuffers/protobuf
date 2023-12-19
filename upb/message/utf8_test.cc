// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdlib.h>

#include <gtest/gtest.h>
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/message/utf8_test.upb.h"
#include "upb/message/utf8_test.upb_minitable.h"
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

  upb_DecodeStatus status =
      upb_Decode(data, size, msg, &upb_0test__TestUtf8Proto3String_msg_init,
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

  upb_DecodeStatus status = upb_Decode(
      data, size, msg, &upb_0test__TestUtf8RepeatedProto3String_msg_init,
      nullptr, 0, arena.ptr());

  // Parse fails, because proto3 string fields validate UTF-8.
  ASSERT_EQ(kUpb_DecodeStatus_BadUtf8, status);
}

// begin:google_only
// TEST(Utf8Test, Proto3MixedFieldValidates) {
//   upb::Arena arena;
//   size_t size;
//   char* data = GetBadUtf8Payload(arena.ptr(), &size);
//
//   upb_test_TestUtf8Proto3StringMixed* msg =
//       upb_test_TestUtf8Proto3StringMixed_new(arena.ptr());
//
//   upb_DecodeStatus status = upb_Decode(
//       data, size, msg, &upb_0test__TestUtf8Proto3StringMixed_msg_init, nullptr,
//       0, arena.ptr());
//
//   // Parse fails, because proto3 string fields validate UTF-8.
//   ASSERT_EQ(kUpb_DecodeStatus_BadUtf8, status);
// }
//
// TEST(Utf8Test, EnforceUtf8Options) {
//   upb::Arena arena;
//   size_t size;
//   char* data = GetBadUtf8Payload(arena.ptr(), &size);
//   upb_test_TestUtf8Proto3StringEnforceUtf8False* msg2 =
//       upb_test_TestUtf8Proto3StringEnforceUtf8False_parse(data, size,
//                                                           arena.ptr());
//
//   // Parse succeeds, because enforce_utf8=false inhibits utf-8 validation.
//   ASSERT_TRUE(msg2 != nullptr);
// }
//
// TEST(Utf8Test, RepeatedEnforceUtf8Options) {
//   upb::Arena arena;
//   size_t size;
//   char* data = GetBadUtf8Payload(arena.ptr(), &size);
//   upb_test_TestUtf8RepeatedProto3StringEnforceUtf8False* msg2 =
//       upb_test_TestUtf8RepeatedProto3StringEnforceUtf8False_parse(data, size,
//                                                                   arena.ptr());
//
//   // Parse succeeds, because enforce_utf8=false inhibits utf-8 validation.
//   ASSERT_TRUE(msg2 != nullptr);
// }
//
// TEST(Utf8Test, EnforceUtf8OptionsMixed) {
//   upb::Arena arena;
//   size_t size;
//   char* data = GetBadUtf8Payload(arena.ptr(), &size);
//   upb_test_TestUtf8Proto3StringEnforceUtf8FalseMixed* msg2 =
//       upb_test_TestUtf8Proto3StringEnforceUtf8FalseMixed_parse(data, size,
//                                                                arena.ptr());
//
//   // Parse succeeds, because enforce_utf8=false inhibits utf-8 validation.
//   ASSERT_TRUE(msg2 != nullptr);
// }
// end:google_only

}  // namespace
