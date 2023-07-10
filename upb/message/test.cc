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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "google/protobuf/test_messages_proto3.upb.h"
#include "upb/json/decode.h"
#include "upb/json/encode.h"
#include "upb/message/test.upb.h"
#include "upb/message/test.upbdefs.h"
#include "upb/reflection/def.hpp"
#include "upb/test/fuzz_util.h"
#include "upb/upb.hpp"
#include "upb/wire/decode.h"

// begin:google_only
// #include "testing/fuzzing/fuzztest.h"
// end:google_only

void VerifyMessage(const upb_test_TestExtensions* ext_msg) {
  EXPECT_TRUE(upb_test_TestExtensions_has_optional_int32_ext(ext_msg));
  // EXPECT_FALSE(upb_test_TestExtensions_Nested_has_optional_int32_ext(ext_msg));
  EXPECT_TRUE(upb_test_has_optional_msg_ext(ext_msg));

  EXPECT_EQ(123, upb_test_TestExtensions_optional_int32_ext(ext_msg));
  const protobuf_test_messages_proto3_TestAllTypesProto3* ext_submsg =
      upb_test_optional_msg_ext(ext_msg);
  EXPECT_TRUE(ext_submsg != nullptr);
  EXPECT_EQ(456,
            protobuf_test_messages_proto3_TestAllTypesProto3_optional_int32(
                ext_submsg));
}

TEST(MessageTest, Extensions) {
  upb::Arena arena;
  upb_test_TestExtensions* ext_msg = upb_test_TestExtensions_new(arena.ptr());

  EXPECT_FALSE(upb_test_TestExtensions_has_optional_int32_ext(ext_msg));
  // EXPECT_FALSE(upb_test_TestExtensions_Nested_has_optional_int32_ext(ext_msg));
  EXPECT_FALSE(upb_test_has_optional_msg_ext(ext_msg));

  upb::DefPool defpool;
  upb::MessageDefPtr m(upb_test_TestExtensions_getmsgdef(defpool.ptr()));
  EXPECT_TRUE(m.ptr() != nullptr);

  std::string json = R"json(
  {
      "[upb_test.TestExtensions.optional_int32_ext]": 123,
      "[upb_test.TestExtensions.Nested.repeated_int32_ext]": [2, 4, 6],
      "[upb_test.optional_msg_ext]": {"optional_int32": 456}
  }
  )json";
  upb::Status status;
  EXPECT_TRUE(upb_JsonDecode(json.data(), json.size(), ext_msg, m.ptr(),
                             defpool.ptr(), 0, arena.ptr(), status.ptr()))
      << status.error_message();

  VerifyMessage(ext_msg);

  // Test round-trip through binary format.
  size_t size;
  char* serialized =
      upb_test_TestExtensions_serialize(ext_msg, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
  ASSERT_GE(size, 0);

  upb_test_TestExtensions* ext_msg2 = upb_test_TestExtensions_parse_ex(
      serialized, size, upb_DefPool_ExtensionRegistry(defpool.ptr()), 0,
      arena.ptr());
  VerifyMessage(ext_msg2);

  // Test round-trip through JSON format.
  size_t json_size = upb_JsonEncode(ext_msg, m.ptr(), defpool.ptr(), 0, nullptr,
                                    0, status.ptr());
  char* json_buf =
      static_cast<char*>(upb_Arena_Malloc(arena.ptr(), json_size + 1));
  upb_JsonEncode(ext_msg, m.ptr(), defpool.ptr(), 0, json_buf, json_size + 1,
                 status.ptr());
  upb_test_TestExtensions* ext_msg3 = upb_test_TestExtensions_new(arena.ptr());
  EXPECT_TRUE(upb_JsonDecode(json_buf, json_size, ext_msg3, m.ptr(),
                             defpool.ptr(), 0, arena.ptr(), status.ptr()))
      << status.error_message();
  VerifyMessage(ext_msg3);
}

void VerifyMessageSet(const upb_test_TestMessageSet* mset_msg) {
  ASSERT_TRUE(mset_msg != nullptr);
  bool has = upb_test_MessageSetMember_has_message_set_extension(mset_msg);
  EXPECT_TRUE(has);
  if (!has) return;
  const upb_test_MessageSetMember* member =
      upb_test_MessageSetMember_message_set_extension(mset_msg);
  EXPECT_TRUE(member != nullptr);
  EXPECT_TRUE(upb_test_MessageSetMember_has_optional_int32(member));
  EXPECT_EQ(234, upb_test_MessageSetMember_optional_int32(member));
}

TEST(MessageTest, MessageSet) {
  upb::Arena arena;
  upb_test_TestMessageSet* ext_msg = upb_test_TestMessageSet_new(arena.ptr());

  EXPECT_FALSE(upb_test_MessageSetMember_has_message_set_extension(ext_msg));

  upb::DefPool defpool;
  upb::MessageDefPtr m(upb_test_TestMessageSet_getmsgdef(defpool.ptr()));
  EXPECT_TRUE(m.ptr() != nullptr);

  std::string json = R"json(
  {
      "[upb_test.MessageSetMember]": {"optional_int32": 234}
  }
  )json";
  upb::Status status;
  EXPECT_TRUE(upb_JsonDecode(json.data(), json.size(), ext_msg, m.ptr(),
                             defpool.ptr(), 0, arena.ptr(), status.ptr()))
      << status.error_message();

  VerifyMessageSet(ext_msg);

  // Test round-trip through binary format.
  size_t size;
  char* serialized =
      upb_test_TestMessageSet_serialize(ext_msg, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
  ASSERT_GE(size, 0);

  upb_test_TestMessageSet* ext_msg2 = upb_test_TestMessageSet_parse_ex(
      serialized, size, upb_DefPool_ExtensionRegistry(defpool.ptr()), 0,
      arena.ptr());
  VerifyMessageSet(ext_msg2);

  // Test round-trip through JSON format.
  size_t json_size = upb_JsonEncode(ext_msg, m.ptr(), defpool.ptr(), 0, nullptr,
                                    0, status.ptr());
  char* json_buf =
      static_cast<char*>(upb_Arena_Malloc(arena.ptr(), json_size + 1));
  upb_JsonEncode(ext_msg, m.ptr(), defpool.ptr(), 0, json_buf, json_size + 1,
                 status.ptr());
  upb_test_TestMessageSet* ext_msg3 = upb_test_TestMessageSet_new(arena.ptr());
  EXPECT_TRUE(upb_JsonDecode(json_buf, json_size, ext_msg3, m.ptr(),
                             defpool.ptr(), 0, arena.ptr(), status.ptr()))
      << status.error_message();
  VerifyMessageSet(ext_msg3);
}

TEST(MessageTest, UnknownMessageSet) {
  static const char data[] = "ABCDE";
  upb_StringView data_view = upb_StringView_FromString(data);
  upb::Arena arena;
  upb_test_FakeMessageSet* fake = upb_test_FakeMessageSet_new(arena.ptr());

  // Add a MessageSet item that is unknown (there is no matching extension in
  // the .proto file)
  upb_test_FakeMessageSet_Item* item =
      upb_test_FakeMessageSet_add_item(fake, arena.ptr());
  upb_test_FakeMessageSet_Item_set_type_id(item, 12345);
  upb_test_FakeMessageSet_Item_set_message(item, data_view);

  // Set unknown fields inside the message set to test that we can skip them.
  upb_test_FakeMessageSet_Item_set_unknown_varint(item, 12345678);
  upb_test_FakeMessageSet_Item_set_unknown_fixed32(item, 12345678);
  upb_test_FakeMessageSet_Item_set_unknown_fixed64(item, 12345678);
  upb_test_FakeMessageSet_Item_set_unknown_bytes(item, data_view);
  upb_test_FakeMessageSet_Item_mutable_unknowngroup(item, arena.ptr());

  // Round trip through a true MessageSet where this item_id is unknown.
  size_t size;
  char* serialized =
      upb_test_FakeMessageSet_serialize(fake, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
  ASSERT_GE(size, 0);

  upb::DefPool defpool;
  upb::MessageDefPtr m(upb_test_TestMessageSet_getmsgdef(defpool.ptr()));
  EXPECT_TRUE(m.ptr() != nullptr);
  upb_test_TestMessageSet* message_set = upb_test_TestMessageSet_parse_ex(
      serialized, size, upb_DefPool_ExtensionRegistry(defpool.ptr()), 0,
      arena.ptr());
  ASSERT_TRUE(message_set != nullptr);

  char* serialized2 =
      upb_test_TestMessageSet_serialize(message_set, arena.ptr(), &size);
  ASSERT_TRUE(serialized2 != nullptr);
  ASSERT_GE(size, 0);

  // Parse back into a fake MessageSet and verify that the unknown MessageSet
  // item was preserved in full (both type_id and message).
  upb_test_FakeMessageSet* fake2 =
      upb_test_FakeMessageSet_parse(serialized2, size, arena.ptr());
  ASSERT_TRUE(fake2 != nullptr);

  const upb_test_FakeMessageSet_Item* const* items =
      upb_test_FakeMessageSet_item(fake2, &size);
  ASSERT_EQ(1, size);
  EXPECT_EQ(12345, upb_test_FakeMessageSet_Item_type_id(items[0]));
  EXPECT_TRUE(upb_StringView_IsEqual(
      data_view, upb_test_FakeMessageSet_Item_message(items[0])));

  // The non-MessageSet unknown fields should have been discarded.
  EXPECT_FALSE(upb_test_FakeMessageSet_Item_has_unknown_varint(items[0]));
  EXPECT_FALSE(upb_test_FakeMessageSet_Item_has_unknown_fixed32(items[0]));
  EXPECT_FALSE(upb_test_FakeMessageSet_Item_has_unknown_fixed64(items[0]));
  EXPECT_FALSE(upb_test_FakeMessageSet_Item_has_unknown_bytes(items[0]));
  EXPECT_FALSE(upb_test_FakeMessageSet_Item_has_unknowngroup(items[0]));
}

TEST(MessageTest, Proto2Enum) {
  upb::Arena arena;
  upb_test_Proto2FakeEnumMessage* fake_msg =
      upb_test_Proto2FakeEnumMessage_new(arena.ptr());

  upb_test_Proto2FakeEnumMessage_set_optional_enum(fake_msg, 999);

  int32_t* vals = upb_test_Proto2FakeEnumMessage_resize_repeated_enum(
      fake_msg, 6, arena.ptr());
  vals[0] = upb_test_Proto2EnumMessage_ZERO;
  vals[1] = 7;  // Unknown small.
  vals[2] = upb_test_Proto2EnumMessage_SMALL;
  vals[3] = 888;  // Unknown large.
  vals[4] = upb_test_Proto2EnumMessage_LARGE;
  vals[5] = upb_test_Proto2EnumMessage_NEGATIVE;

  vals = upb_test_Proto2FakeEnumMessage_resize_packed_enum(fake_msg, 6,
                                                           arena.ptr());
  vals[0] = upb_test_Proto2EnumMessage_ZERO;
  vals[1] = 7;  // Unknown small.
  vals[2] = upb_test_Proto2EnumMessage_SMALL;
  vals[3] = 888;  // Unknown large.
  vals[4] = upb_test_Proto2EnumMessage_LARGE;
  vals[5] = upb_test_Proto2EnumMessage_NEGATIVE;

  size_t size;
  char* pb =
      upb_test_Proto2FakeEnumMessage_serialize(fake_msg, arena.ptr(), &size);

  // Parsing as enums puts unknown values into unknown fields.
  upb_test_Proto2EnumMessage* enum_msg =
      upb_test_Proto2EnumMessage_parse(pb, size, arena.ptr());
  ASSERT_TRUE(enum_msg != nullptr);

  EXPECT_EQ(false, upb_test_Proto2EnumMessage_has_optional_enum(enum_msg));
  const int32_t* vals_const =
      upb_test_Proto2EnumMessage_repeated_enum(enum_msg, &size);
  EXPECT_EQ(4, size);  // Two unknown values moved to the unknown field set.

  // Parsing back into the fake message shows the original data, except the
  // repeated enum is rearranged.
  pb = upb_test_Proto2EnumMessage_serialize(enum_msg, arena.ptr(), &size);
  upb_test_Proto2FakeEnumMessage* fake_msg2 =
      upb_test_Proto2FakeEnumMessage_parse(pb, size, arena.ptr());
  ASSERT_TRUE(fake_msg2 != nullptr);

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

  vals_const = upb_test_Proto2FakeEnumMessage_repeated_enum(fake_msg2, &size);
  EXPECT_EQ(6, size);
  EXPECT_THAT(std::vector<int32_t>(vals_const, vals_const + size),
              ::testing::ElementsAreArray(expected));

  vals_const = upb_test_Proto2FakeEnumMessage_packed_enum(fake_msg2, &size);
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

TEST(MessageTest, DecodeRequiredFieldsTopLevelMessage) {
  upb::Arena arena;
  upb_test_TestRequiredFields* test_msg;
  upb_test_EmptyMessage* empty_msg;

  // Succeeds, because we did not request required field checks.
  test_msg = upb_test_TestRequiredFields_parse(nullptr, 0, arena.ptr());
  EXPECT_NE(nullptr, test_msg);

  // Fails, because required fields are missing.
  EXPECT_EQ(
      kUpb_DecodeStatus_MissingRequired,
      upb_Decode(nullptr, 0, test_msg, &upb_test_TestRequiredFields_msg_init,
                 nullptr, kUpb_DecodeOption_CheckRequired, arena.ptr()));

  upb_test_TestRequiredFields_set_required_int32(test_msg, 1);
  size_t size;
  char* serialized =
      upb_test_TestRequiredFields_serialize(test_msg, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
  EXPECT_NE(0, size);

  // Fails, but the code path is slightly different because the serialized
  // payload is not empty.
  EXPECT_EQ(kUpb_DecodeStatus_MissingRequired,
            upb_Decode(serialized, size, test_msg,
                       &upb_test_TestRequiredFields_msg_init, nullptr,
                       kUpb_DecodeOption_CheckRequired, arena.ptr()));

  empty_msg = upb_test_EmptyMessage_new(arena.ptr());
  upb_test_TestRequiredFields_set_required_int32(test_msg, 1);
  upb_test_TestRequiredFields_set_required_int64(test_msg, 2);
  upb_test_TestRequiredFields_set_required_message(test_msg, empty_msg);

  // Succeeds, because required fields are present (though not in the input).
  EXPECT_EQ(
      kUpb_DecodeStatus_Ok,
      upb_Decode(nullptr, 0, test_msg, &upb_test_TestRequiredFields_msg_init,
                 nullptr, kUpb_DecodeOption_CheckRequired, arena.ptr()));

  // Serialize a complete payload.
  serialized =
      upb_test_TestRequiredFields_serialize(test_msg, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
  EXPECT_NE(0, size);

  upb_test_TestRequiredFields* test_msg2 = upb_test_TestRequiredFields_parse_ex(
      serialized, size, nullptr, kUpb_DecodeOption_CheckRequired, arena.ptr());
  EXPECT_NE(nullptr, test_msg2);

  // When we add an incomplete sub-message, this is not flagged by the parser.
  // This makes parser checking unsuitable for MergeFrom().
  upb_test_TestRequiredFields_set_optional_message(
      test_msg2, upb_test_TestRequiredFields_new(arena.ptr()));
  EXPECT_EQ(kUpb_DecodeStatus_Ok,
            upb_Decode(serialized, size, test_msg2,
                       &upb_test_TestRequiredFields_msg_init, nullptr,
                       kUpb_DecodeOption_CheckRequired, arena.ptr()));
}

TEST(MessageTest, DecodeRequiredFieldsSubMessage) {
  upb::Arena arena;
  upb_test_TestRequiredFields* test_msg =
      upb_test_TestRequiredFields_new(arena.ptr());
  upb_test_SubMessageHasRequired* sub_msg =
      upb_test_SubMessageHasRequired_new(arena.ptr());
  upb_test_EmptyMessage* empty_msg = upb_test_EmptyMessage_new(arena.ptr());

  upb_test_SubMessageHasRequired_set_optional_message(sub_msg, test_msg);
  size_t size;
  char* serialized =
      upb_test_SubMessageHasRequired_serialize(sub_msg, arena.ptr(), &size);
  EXPECT_NE(0, size);

  // No parse error when parsing normally.
  EXPECT_NE(nullptr, upb_test_SubMessageHasRequired_parse(serialized, size,
                                                          arena.ptr()));

  // Parse error when verifying required fields, due to incomplete sub-message.
  EXPECT_EQ(nullptr, upb_test_SubMessageHasRequired_parse_ex(
                         serialized, size, nullptr,
                         kUpb_DecodeOption_CheckRequired, arena.ptr()));

  upb_test_TestRequiredFields_set_required_int32(test_msg, 1);
  upb_test_TestRequiredFields_set_required_int64(test_msg, 2);
  upb_test_TestRequiredFields_set_required_message(test_msg, empty_msg);

  serialized =
      upb_test_SubMessageHasRequired_serialize(sub_msg, arena.ptr(), &size);
  EXPECT_NE(0, size);

  // No parse error; sub-message now is complete.
  EXPECT_NE(nullptr, upb_test_SubMessageHasRequired_parse_ex(
                         serialized, size, nullptr,
                         kUpb_DecodeOption_CheckRequired, arena.ptr()));
}

TEST(MessageTest, EncodeRequiredFields) {
  upb::Arena arena;
  upb_test_TestRequiredFields* test_msg =
      upb_test_TestRequiredFields_new(arena.ptr());

  // Succeeds, we didn't ask for required field checking.
  size_t size;
  char* serialized =
      upb_test_TestRequiredFields_serialize_ex(test_msg, 0, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
  EXPECT_EQ(size, 0);

  // Fails, we asked for required field checking but the required field is
  // missing.
  serialized = upb_test_TestRequiredFields_serialize_ex(
      test_msg, kUpb_EncodeOption_CheckRequired, arena.ptr(), &size);
  ASSERT_TRUE(serialized == nullptr);

  // Fails, some required fields are present but not others.
  upb_test_TestRequiredFields_set_required_int32(test_msg, 1);
  serialized = upb_test_TestRequiredFields_serialize_ex(
      test_msg, kUpb_EncodeOption_CheckRequired, arena.ptr(), &size);
  ASSERT_TRUE(serialized == nullptr);

  // Succeeds, all required fields are set.
  upb_test_EmptyMessage* empty_msg = upb_test_EmptyMessage_new(arena.ptr());
  upb_test_TestRequiredFields_set_required_int64(test_msg, 2);
  upb_test_TestRequiredFields_set_required_message(test_msg, empty_msg);
  serialized = upb_test_TestRequiredFields_serialize_ex(
      test_msg, kUpb_EncodeOption_CheckRequired, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
}

TEST(MessageTest, MaxRequiredFields) {
  upb::Arena arena;
  upb_test_TestMaxRequiredFields* test_msg =
      upb_test_TestMaxRequiredFields_new(arena.ptr());

  // Fails, we asked for required field checking but the required field is
  // missing.
  size_t size;
  char* serialized = upb_test_TestMaxRequiredFields_serialize_ex(
      test_msg, kUpb_EncodeOption_CheckRequired, arena.ptr(), &size);
  ASSERT_TRUE(serialized == nullptr);

  upb::DefPool defpool;
  upb::MessageDefPtr m(upb_test_TestMaxRequiredFields_getmsgdef(defpool.ptr()));
  upb_MessageValue val;
  val.int32_val = 1;
  for (int i = 1; i <= 61; i++) {
    upb::FieldDefPtr f = m.FindFieldByNumber(i);
    ASSERT_TRUE(f);
    upb_Message_SetFieldByDef(test_msg, f.ptr(), val, arena.ptr());
  }

  // Fails, field 63 still isn't set.
  serialized = upb_test_TestMaxRequiredFields_serialize_ex(
      test_msg, kUpb_EncodeOption_CheckRequired, arena.ptr(), &size);
  ASSERT_TRUE(serialized == nullptr);

  // Succeeds, all required fields are set.
  upb::FieldDefPtr f = m.FindFieldByNumber(62);
  ASSERT_TRUE(f);
  upb_Message_SetFieldByDef(test_msg, f.ptr(), val, arena.ptr());
  serialized = upb_test_TestMaxRequiredFields_serialize_ex(
      test_msg, kUpb_EncodeOption_CheckRequired, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
}

TEST(MessageTest, MapField) {
  upb::Arena arena;
  upb_test_TestMapFieldExtra* test_msg_extra =
      upb_test_TestMapFieldExtra_new(arena.ptr());

  ASSERT_TRUE(upb_test_TestMapFieldExtra_map_field_set(
      test_msg_extra, 0, upb_test_TestMapFieldExtra_THREE, arena.ptr()));

  size_t size;
  char* serialized = upb_test_TestMapFieldExtra_serialize_ex(
      test_msg_extra, 0, arena.ptr(), &size);
  ASSERT_NE(nullptr, serialized);
  ASSERT_NE(0, size);

  upb_test_TestMapField* test_msg =
      upb_test_TestMapField_parse(serialized, size, arena.ptr());
  ASSERT_NE(nullptr, test_msg);

  ASSERT_FALSE(upb_test_TestMapField_map_field_get(test_msg, 0, nullptr));
  serialized =
      upb_test_TestMapField_serialize_ex(test_msg, 0, arena.ptr(), &size);
  ASSERT_NE(0, size);
  // parse into second instance
  upb_test_TestMapFieldExtra* test_msg_extra2 =
      upb_test_TestMapFieldExtra_parse(serialized, size, arena.ptr());
  ASSERT_TRUE(
      upb_test_TestMapFieldExtra_map_field_get(test_msg_extra2, 0, nullptr));
}

// begin:google_only
//
// static void DecodeEncodeArbitrarySchemaAndPayload(
//     const upb::fuzz::MiniTableFuzzInput& input, std::string_view proto_payload,
//     int decode_options, int encode_options) {
// // Lexan does not have setenv
// #ifndef _MSC_VER
//   setenv("FUZZTEST_STACK_LIMIT", "262144", 1);
// #endif
//   // The value of 80 used here is empirical and intended to roughly represent
//   // the tiny 64K stack size used by the test framework. We still see the
//   // occasional stack overflow at 90, so far 80 has worked 100% of the time.
//   decode_options = upb_Decode_LimitDepth(decode_options, 80);
//   encode_options = upb_Encode_LimitDepth(encode_options, 80);
//
//   upb::Arena arena;
//   upb_ExtensionRegistry* exts;
//   const upb_MiniTable* mini_table =
//       upb::fuzz::BuildMiniTable(input, &exts, arena.ptr());
//   if (!mini_table) return;
//   upb_Message* msg = upb_Message_New(mini_table, arena.ptr());
//   upb_Decode(proto_payload.data(), proto_payload.size(), msg, mini_table, exts,
//              decode_options, arena.ptr());
//   char* ptr;
//   size_t size;
//   upb_Encode(msg, mini_table, encode_options, arena.ptr(), &ptr, &size);
// }
// FUZZ_TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayload);
//
// TEST(FuzzTest, DecodeUnknownProto2EnumExtension) {
//   DecodeEncodeArbitrarySchemaAndPayload(
//       {{"\256\354Rt\216\3271\234", "\243\243\267\207\336gV\366w"},
//        {"z"},
//        "}\212\304d\371\363\341\2329\325B\264\377?\215\223\201\201\226y\201%"
//        "\321\363\255;",
//        {}},
//       "\010", -724543908, -591643538);
// }
//
// TEST(FuzzTest, DecodeExtensionEnsurePresenceInitialized) {
//   DecodeEncodeArbitrarySchemaAndPayload(
//       {{"\031", "S", "\364", "", "", "j", "\303", "", "\224", "\277"},
//        {},
//        "_C-\236$*)C0C>",
//        {4041515984, 2147483647, 1929379871, 0, 3715937258, 4294967295}},
//       "\010\002", 342248070, -806315555);
// }
//
// TEST(FuzzTest, DecodeExtendMessageSetWithNonMessage) {
//   DecodeEncodeArbitrarySchemaAndPayload(
//       {{"\n"}, {""}, ".\244", {}}, "\013\032\005\212a#\365\336\020\001\226",
//       14803219, 670718349);
// }
//
// TEST(FuzzTest, DecodeExtendMessageSetWithNonMessage2) {
//   DecodeEncodeArbitrarySchemaAndPayload({{"\n", "G", "\n", "\274", ""},
//                                          {"", "\030"},
//                                          "_@",
//                                          {4294967295, 2147483647}},
//                                         std::string("\013\032\000\220", 4),
//                                         279975758, 1647495141);
// }
//
// TEST(FuzzTest, DecodeExtendMessageSetWithNonMessage3) {
//   DecodeEncodeArbitrarySchemaAndPayload(
//       {{"\n"}, {"B", ""}, "\212:b", {11141121}},
//       "\013\032\004\357;7\363\020\001\346\240\200\201\271", 399842149,
//       -452966025);
// }
//
// TEST(FuzzTest, DecodeExtendMessageSetWithNonMessage4) {
//   DecodeEncodeArbitrarySchemaAndPayload(
//       {{"\n", "3\340", "\354"}, {}, "B}G", {4294967295, 4082331310}},
//       "\013\032\004\244B\331\255\020\001\220\224\243\350\t", -561523015,
//       1683327312);
// }
//
// TEST(FuzzTest, DecodeExtendMessageSetWithNonMessage5) {
//   DecodeEncodeArbitrarySchemaAndPayload(
//       {{"\n"}, {""}, "kB", {0}},
//       "x\203\251\006\013\032\002S\376\010\273\'\020\014\365\207\244\234",
//       -696925610, -654590577);
// }
//
// TEST(FuzzTest, ExtendMessageSetWithEmptyExtension) {
//   DecodeEncodeArbitrarySchemaAndPayload({{"\n"}, {}, "_", {}}, std::string(), 0,
//                                         0);
// }
//
// TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegression) {
//   DecodeEncodeArbitrarySchemaAndPayload(
//       {{"\320", "\320", "\320", "\320", "\320", "%2%%%%%"},
//        {"", "", "", "", "", "", "", "", "", "", "", "",
//         "", "", "", "", "", "", "", "", "", "", ""},
//        "\226\226\226\226\226\226\350\351\350\350\350\350\350\350\350\314",
//        {4026531839}},
//       std::string("\n\n\n\n\272\n======@@%%%%%%%%%%%%%%%@@@(("
//                   "qqqqqqqq5555555555qqqqqffq((((((((((((\335@@>"
//                   "\ru\360ncppppxxxxxxxxx\025\025\025xxxxxppppppp<="
//                   "\2165\275\275\315\217\361\010\t\000\016\013in\n\n\n\256\263",
//                   130),
//       901979906, 65537);
// }
//
// // This test encodes a map field with extra cruft.
// TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegressionInvalidMap) {
//   DecodeEncodeArbitrarySchemaAndPayload({{"%%%%///////"}, {}, "", {}},
//                                         std::string("\035|", 2), 65536, 3);
// }
//
// // This test found a case where presence was unset for a mini table field.
// TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegressionMsan) {
//   DecodeEncodeArbitrarySchemaAndPayload({{"%-#^#"}, {}, "", {}}, std::string(),
//                                         -1960166338, 16809991);
// }
//
// // This test encodes a map containing a msg wrapping another, empty msg.
// TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegressionMapMap) {
//   DecodeEncodeArbitrarySchemaAndPayload(
//       {{"%#G"}, {}, "", {}}, std::string("\022\002\022\000", 4), 0, 0);
// }
//
// TEST(FuzzTest, GroupMap) {
//   // Groups should not be allowed as maps, but we previously failed to prevent
//   // this.
//   DecodeEncodeArbitrarySchemaAndPayload(
//       {.mini_descriptors = {"$$FF$", "%-C"},
//        .enum_mini_descriptors = {},
//        .extensions = "",
//        .links = {1}},
//       std::string(
//           "\023\020\030\233\000\204\330\372#\000`"
//           "a\000\000\001\000\000\000ccccccc\030s\273sssssssss\030\030\030\030"
//           "\030\030\030\030\215\215\215\215\215\215\215\215\030\030\232\253\253"
//           "\232*\334\227\273\231\207\373\t\0051\305\265\335\224\226"),
//       0, 0);
// }
//
// TEST(FuzzTest, MapUnknownFieldSpanBuffers) {
//   DecodeEncodeArbitrarySchemaAndPayload(
//       {{"$   3", "%# "}, {}, "", {1}},
//       std::string(
//           "\"\002\010\000\000\000\000\000\000\000\000\000\000\000\000\000\000",
//           17),
//       0, 0);
// }
//
// // Another test for mismatched submsg types.
// TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayloadRegression22) {
//   DecodeEncodeArbitrarySchemaAndPayload(
//       {{"$2222222222222222222222", "%,&"}, {}, "", {1}},
//       std::string("\035\170\170\170\051\263\001\030\000\035\357\357\340\021\035"
//                   "\025\331\035\035\035\035\035\035\035\035",
//                   25),
//       0, 0);
// }
//
// TEST(FuzzTest, ExtensionWithoutExt) {
//   DecodeEncodeArbitrarySchemaAndPayload({{"$ 3", "", "%#F"}, {}, "", {2, 1}},
//                                         std::string("\022\002\010\000", 4), 0,
//                                         0);
// }
//
// TEST(FuzzTest, MapFieldVerify) {
//   DecodeEncodeArbitrarySchemaAndPayload({{"%  ^!"}, {}, "", {}}, "", 0, 0);
// }
//
// TEST(FuzzTest, TooManyRequiredFields) {
//   DecodeEncodeArbitrarySchemaAndPayload(
//       {{"$ N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N "
//         "N N N N N N N N N N N N N N N N N N N N N N N N N N N N N N"},
//        {},
//        "",
//        {}},
//       "", 0, 4);
// }
//
// end:google_only
