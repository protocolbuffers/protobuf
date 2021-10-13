/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "src/google/protobuf/test_messages_proto3.upb.h"
#include "upb/def.hpp"
#include "upb/json_decode.h"
#include "upb/json_encode.h"
#include "upb/msg_test.upb.h"
#include "upb/msg_test.upbdefs.h"
#include "upb/upb.hpp"

void VerifyMessage(const upb_test_TestExtensions *ext_msg) {
  EXPECT_TRUE(upb_test_TestExtensions_has_optional_int32_ext(ext_msg));
  // EXPECT_FALSE(upb_test_TestExtensions_Nested_has_optional_int32_ext(ext_msg));
  EXPECT_TRUE(upb_test_has_optional_msg_ext(ext_msg));

  EXPECT_EQ(123, upb_test_TestExtensions_optional_int32_ext(ext_msg));
  const protobuf_test_messages_proto3_TestAllTypesProto3 *ext_submsg =
      upb_test_optional_msg_ext(ext_msg);
  EXPECT_TRUE(ext_submsg != nullptr);
  EXPECT_EQ(456,
            protobuf_test_messages_proto3_TestAllTypesProto3_optional_int32(
                ext_submsg));
}

TEST(MessageTest, Extensions) {
  upb::Arena arena;
  upb_test_TestExtensions *ext_msg = upb_test_TestExtensions_new(arena.ptr());

  EXPECT_FALSE(upb_test_TestExtensions_has_optional_int32_ext(ext_msg));
  // EXPECT_FALSE(upb_test_TestExtensions_Nested_has_optional_int32_ext(ext_msg));
  EXPECT_FALSE(upb_test_has_optional_msg_ext(ext_msg));

  upb::SymbolTable symtab;
  upb::MessageDefPtr m(upb_test_TestExtensions_getmsgdef(symtab.ptr()));
  EXPECT_TRUE(m.ptr() != nullptr);

  std::string json = R"json(
  {
      "[upb_test.TestExtensions.optional_int32_ext]": 123,
      "[upb_test.TestExtensions.Nested.repeated_int32_ext]": [2, 4, 6],
      "[upb_test.optional_msg_ext]": {"optional_int32": 456}
  }
  )json";
  upb::Status status;
  EXPECT_TRUE(upb_json_decode(json.data(), json.size(), ext_msg, m.ptr(),
                              symtab.ptr(), 0, arena.ptr(), status.ptr()))
      << status.error_message();

  VerifyMessage(ext_msg);

  // Test round-trip through binary format.
  size_t size;
  char *serialized = upb_test_TestExtensions_serialize(ext_msg, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
  ASSERT_GE(size, 0);

  upb_test_TestExtensions *ext_msg2 = upb_test_TestExtensions_parse_ex(
      serialized, size, upb_symtab_extreg(symtab.ptr()), 0, arena.ptr());
  VerifyMessage(ext_msg2);

  // Test round-trip through JSON format.
  size_t json_size =
      upb_json_encode(ext_msg, m.ptr(), symtab.ptr(), 0, NULL, 0, status.ptr());
  char *json_buf =
      static_cast<char *>(upb_arena_malloc(arena.ptr(), json_size + 1));
  upb_json_encode(ext_msg, m.ptr(), symtab.ptr(), 0, json_buf, json_size + 1,
                  status.ptr());
  upb_test_TestExtensions *ext_msg3 = upb_test_TestExtensions_new(arena.ptr());
  EXPECT_TRUE(upb_json_decode(json_buf, json_size, ext_msg3, m.ptr(),
                              symtab.ptr(), 0, arena.ptr(), status.ptr()))
      << status.error_message();
  VerifyMessage(ext_msg3);
}

void VerifyMessageSet(const upb_test_TestMessageSet *mset_msg) {
  bool has = upb_test_MessageSetMember_has_message_set_extension(mset_msg);
  EXPECT_TRUE(has);
  if (!has) return;
  const upb_test_MessageSetMember *member =
      upb_test_MessageSetMember_message_set_extension(mset_msg);
  EXPECT_TRUE(member != nullptr);
  EXPECT_TRUE(upb_test_MessageSetMember_has_optional_int32(member));
  EXPECT_EQ(234, upb_test_MessageSetMember_optional_int32(member));
}

TEST(MessageTest, MessageSet) {
  upb::Arena arena;
  upb_test_TestMessageSet *ext_msg = upb_test_TestMessageSet_new(arena.ptr());

  EXPECT_FALSE(upb_test_MessageSetMember_has_message_set_extension(ext_msg));

  upb::SymbolTable symtab;
  upb::MessageDefPtr m(upb_test_TestMessageSet_getmsgdef(symtab.ptr()));
  EXPECT_TRUE(m.ptr() != nullptr);

  std::string json = R"json(
  {
      "[upb_test.MessageSetMember]": {"optional_int32": 234}
  }
  )json";
  upb::Status status;
  EXPECT_TRUE(upb_json_decode(json.data(), json.size(), ext_msg, m.ptr(),
                              symtab.ptr(), 0, arena.ptr(), status.ptr()))
      << status.error_message();

  VerifyMessageSet(ext_msg);

  // Test round-trip through binary format.
  size_t size;
  char *serialized = upb_test_TestMessageSet_serialize(ext_msg, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
  ASSERT_GE(size, 0);

  upb_test_TestMessageSet *ext_msg2 = upb_test_TestMessageSet_parse_ex(
      serialized, size, upb_symtab_extreg(symtab.ptr()), 0, arena.ptr());
  VerifyMessageSet(ext_msg2);

  // Test round-trip through JSON format.
  size_t json_size =
      upb_json_encode(ext_msg, m.ptr(), symtab.ptr(), 0, NULL, 0, status.ptr());
  char *json_buf =
      static_cast<char *>(upb_arena_malloc(arena.ptr(), json_size + 1));
  upb_json_encode(ext_msg, m.ptr(), symtab.ptr(), 0, json_buf, json_size + 1,
                  status.ptr());
  upb_test_TestMessageSet *ext_msg3 = upb_test_TestMessageSet_new(arena.ptr());
  EXPECT_TRUE(upb_json_decode(json_buf, json_size, ext_msg3, m.ptr(),
                              symtab.ptr(), 0, arena.ptr(), status.ptr()))
      << status.error_message();
  VerifyMessageSet(ext_msg3);
}

TEST(MessageTest, Proto2Enum) {
  upb::Arena arena;
  upb_test_Proto2FakeEnumMessage *fake_msg = upb_test_Proto2FakeEnumMessage_new(arena.ptr());

  upb_test_Proto2FakeEnumMessage_set_optional_enum(fake_msg, 999);

  int32_t *vals = upb_test_Proto2FakeEnumMessage_resize_repeated_enum(
      fake_msg, 6, arena.ptr());
  vals[0] = upb_test_Proto2EnumMessage_ZERO;
  vals[1] = 7;  // Unknown small.
  vals[2] = upb_test_Proto2EnumMessage_SMALL;
  vals[3] = 888;  // Unknown large.
  vals[4] = upb_test_Proto2EnumMessage_LARGE;
  vals[5] = upb_test_Proto2EnumMessage_NEGATIVE;

  vals = upb_test_Proto2FakeEnumMessage_resize_packed_enum(
      fake_msg, 6, arena.ptr());
  vals[0] = upb_test_Proto2EnumMessage_ZERO;
  vals[1] = 7;  // Unknown small.
  vals[2] = upb_test_Proto2EnumMessage_SMALL;
  vals[3] = 888;  // Unknown large.
  vals[4] = upb_test_Proto2EnumMessage_LARGE;
  vals[5] = upb_test_Proto2EnumMessage_NEGATIVE;

  size_t size;
  char *pb =
      upb_test_Proto2FakeEnumMessage_serialize(fake_msg, arena.ptr(), &size);

  // Parsing as enums puts unknown values into unknown fields.
  upb_test_Proto2EnumMessage *enum_msg =
      upb_test_Proto2EnumMessage_parse(pb, size, arena.ptr());
  ASSERT_TRUE(enum_msg != nullptr);

  EXPECT_EQ(false, upb_test_Proto2EnumMessage_has_optional_enum(enum_msg));
  const int32_t *vals_const =
      upb_test_Proto2EnumMessage_repeated_enum(enum_msg, &size);
  EXPECT_EQ(4, size);  // Two unknown values moved to the unknown field set.

  // Parsing back into the fake message shows the original data, except the
  // repeated enum is rearranged.
  pb = upb_test_Proto2EnumMessage_serialize(enum_msg, arena.ptr(), &size);
  upb_test_Proto2FakeEnumMessage *fake_msg2 =
      upb_test_Proto2FakeEnumMessage_parse(pb, size, arena.ptr());

  EXPECT_EQ(true, upb_test_Proto2FakeEnumMessage_has_optional_enum(fake_msg2));
  EXPECT_EQ(999, upb_test_Proto2FakeEnumMessage_optional_enum(fake_msg2));

  int32_t expected[] = {
      upb_test_Proto2EnumMessage_ZERO,
      upb_test_Proto2EnumMessage_SMALL,
      upb_test_Proto2EnumMessage_LARGE,
      upb_test_Proto2EnumMessage_NEGATIVE,
      7,
      888,
  };

  vals_const =
      upb_test_Proto2FakeEnumMessage_repeated_enum(fake_msg2, &size);
  EXPECT_EQ(6, size);
  EXPECT_THAT(std::vector<int32_t>(vals_const, vals_const + size),
              ::testing::ElementsAreArray(expected));

  vals_const =
      upb_test_Proto2FakeEnumMessage_packed_enum(fake_msg2, &size);
  EXPECT_EQ(6, size);
  EXPECT_THAT(std::vector<int32_t>(vals_const, vals_const + size),
              ::testing::ElementsAreArray(expected));
}

TEST(MessageTest, TestBadUTF8) {
  upb::Arena arena;
  std::string serialized("r\x03\xed\xa0\x81");
  EXPECT_EQ(nullptr, protobuf_test_messages_proto3_TestAllTypesProto3_parse(
                         serialized.data(), serialized.size(), arena.ptr()));
}
