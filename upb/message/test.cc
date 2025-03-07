// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/cleanup/cleanup.h"
#include "absl/numeric/bits.h"
#include "absl/strings/escaping.h"
#include "google/protobuf/test_messages_proto3.upb.h"
#include "upb/base/status.h"
#include "upb/base/status.hpp"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/json/decode.h"
#include "upb/json/encode.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/message/array.h"
#include "upb/message/compare.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/test.upb.h"
#include "upb/message/test.upb_minitable.h"
#include "upb/message/test.upbdefs.h"
#include "upb/message/value.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/reflection/def.h"
#include "upb/reflection/def.hpp"
#include "upb/reflection/message.h"
#include "upb/test/fuzz_util.h"
#include "upb/text/debug_string.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/types.h"

// Must be last
#include "upb/port/def.inc"

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
  EXPECT_TRUE(upb_JsonDecode(json.data(), json.size(), UPB_UPCAST(ext_msg),
                             m.ptr(), defpool.ptr(), 0, arena.ptr(),
                             status.ptr()))
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
  size_t json_size = upb_JsonEncode(UPB_UPCAST(ext_msg), m.ptr(), defpool.ptr(),
                                    0, nullptr, 0, status.ptr());
  char* json_buf =
      static_cast<char*>(upb_Arena_Malloc(arena.ptr(), json_size + 1));
  upb_JsonEncode(UPB_UPCAST(ext_msg), m.ptr(), defpool.ptr(), 0, json_buf,
                 json_size + 1, status.ptr());
  upb_test_TestExtensions* ext_msg3 = upb_test_TestExtensions_new(arena.ptr());
  EXPECT_TRUE(upb_JsonDecode(json_buf, json_size, UPB_UPCAST(ext_msg3), m.ptr(),
                             defpool.ptr(), 0, arena.ptr(), status.ptr()))
      << status.error_message();
  VerifyMessage(ext_msg3);

  // Test setters and mutable accessors
  upb_test_TestExtensions* ext_msg4 = upb_test_TestExtensions_new(arena.ptr());
  upb_test_TestExtensions_set_optional_int32_ext(ext_msg4, 123, arena.ptr());
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_int32(
      upb_test_mutable_optional_msg_ext(ext_msg4, arena.ptr()), 456);
  VerifyMessage(ext_msg4);
}

TEST(MessageTest, ExtensionsDeterministic) {
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
      "[upb_test.TestExtensions.Nested.repeated_int32_ext]": [],
      "[upb_test.optional_msg_ext]": {"optional_int32": 456}
  }
  )json";
  upb::Status status;
  EXPECT_TRUE(upb_JsonDecode(json.data(), json.size(), UPB_UPCAST(ext_msg),
                             m.ptr(), defpool.ptr(), 0, arena.ptr(),
                             status.ptr()))
      << status.error_message();

  VerifyMessage(ext_msg);

  size_t size;
  char* serialized =
      upb_test_TestExtensions_serialize(ext_msg, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
  ASSERT_GE(size, 0);

  size_t deterministic_size;
  char* deterministic_serialized = upb_test_TestExtensions_serialize_ex(
      ext_msg, kUpb_EncodeOption_Deterministic, arena.ptr(),
      &deterministic_size);
  ASSERT_TRUE(deterministic_serialized != nullptr);
  ASSERT_EQ(deterministic_size, size);
}

TEST(MessageTest, ExtensionsEmpty) {
  upb::Arena arena;

  upb::DefPool defpool;
  upb::MessageDefPtr m(upb_test_TestExtensions_getmsgdef(defpool.ptr()));
  EXPECT_TRUE(m.ptr() != nullptr);

  for (int options : {0, int{kUpb_EncodeOption_Deterministic}}) {
    std::string json_with_empty = R"json(
  {
      "[upb_test.TestExtensions.optional_int32_ext]": 123,
      "[upb_test.TestExtensions.Nested.repeated_int32_ext]": []
  }
  )json";
    upb::Status status_empty;
    upb_test_TestExtensions* ext_msg_with_empty =
        upb_test_TestExtensions_new(arena.ptr());
    EXPECT_TRUE(upb_JsonDecode(json_with_empty.data(), json_with_empty.size(),
                               UPB_UPCAST(ext_msg_with_empty), m.ptr(),
                               defpool.ptr(), 0, arena.ptr(),
                               status_empty.ptr()))
        << status_empty.error_message();

    std::string json = R"json(
  {
      "[upb_test.TestExtensions.optional_int32_ext]": 123
  }
  )json";
    upb::Status status;
    upb_test_TestExtensions* ext_msg = upb_test_TestExtensions_new(arena.ptr());
    EXPECT_TRUE(upb_JsonDecode(json.data(), json.size(), UPB_UPCAST(ext_msg),
                               m.ptr(), defpool.ptr(), 0, arena.ptr(),
                               status.ptr()))
        << status.error_message();

    size_t size_with_empty;
    char* serialized = upb_test_TestExtensions_serialize_ex(
        ext_msg_with_empty, options, arena.ptr(), &size_with_empty);
    ASSERT_TRUE(serialized != nullptr);
    ASSERT_GE(size_with_empty, 0);

    size_t size;
    serialized = upb_test_TestExtensions_serialize_ex(ext_msg, options,
                                                      arena.ptr(), &size);
    ASSERT_TRUE(serialized != nullptr);
    // Presence or absence of an empty extension should not affect the
    // serialized output.
    ASSERT_EQ(size_with_empty, size);
  }
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
  EXPECT_TRUE(upb_JsonDecode(json.data(), json.size(), UPB_UPCAST(ext_msg),
                             m.ptr(), defpool.ptr(), 0, arena.ptr(),
                             status.ptr()))
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
  size_t json_size = upb_JsonEncode(UPB_UPCAST(ext_msg), m.ptr(), defpool.ptr(),
                                    0, nullptr, 0, status.ptr());
  char* json_buf =
      static_cast<char*>(upb_Arena_Malloc(arena.ptr(), json_size + 1));
  upb_JsonEncode(UPB_UPCAST(ext_msg), m.ptr(), defpool.ptr(), 0, json_buf,
                 json_size + 1, status.ptr());
  upb_test_TestMessageSet* ext_msg3 = upb_test_TestMessageSet_new(arena.ptr());
  EXPECT_TRUE(upb_JsonDecode(json_buf, json_size, UPB_UPCAST(ext_msg3), m.ptr(),
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

TEST(MessageTest, MessageSetSubmessageEncoding) {
  upb::Arena arena;

  // Create a normal extension message and use the set the doppelgänger message
  // set member extension on it. This will allow us to serialize as a normal
  // extension and then attempt to parse it as a message set.
  // This mimics the behavior of an encoder that is message set unaware.
  upb_test_TestExtensions* ext_msg = upb_test_TestExtensions_new(arena.ptr());
  upb_test_MessageSetMember* ext_member =
      upb_test_MessageSetMember_new(arena.ptr());
  upb_test_MessageSetMember_set_optional_int32(ext_member, 234);
  upb_test_MessageSetMember_set_doppelganger_message_set_extension(
      ext_msg, ext_member, arena.ptr());

  size_t size;
  char* serialized =
      upb_test_TestExtensions_serialize(ext_msg, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
  ASSERT_GE(size, 0);

  upb::DefPool defpool;
  upb::MessageDefPtr m(upb_test_TestMessageSet_getmsgdef(defpool.ptr()));
  EXPECT_TRUE(m.ptr() != nullptr);

  upb_test_TestMessageSet* message_set = upb_test_TestMessageSet_parse_ex(
      serialized, size, upb_DefPool_ExtensionRegistry(defpool.ptr()), 0,
      arena.ptr());
  ASSERT_TRUE(message_set != nullptr);

  VerifyMessageSet(message_set);
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

// On a 32 bit platform, upb_StringView has the same size as an int64 field, but
// lower alignment requirements - when selecting the representation for oneof,
// both size and alignment need to be considered.
TEST(MessageTest, OneOf32BitStringViewInt64Alignment) {
  upb::Arena arena;
  upb_StringView md = {nullptr, 0};
  uint32_t field_tag;
  {
    upb_DefPool* d = upb_DefPool_New();
    auto free = absl::MakeCleanup([d]() { upb_DefPool_Free(d); });
    const upb_MessageDef* def = upb_test_TestOneOfAlignment_getmsgdef(d);
    const upb_FieldDef* field_def =
        upb_MessageDef_FindFieldByName(def, "should_be_sixty_four_aligned");
    field_tag = upb_FieldDef_Number(field_def);
    EXPECT_TRUE(upb_MessageDef_MiniDescriptorEncode(def, arena.ptr(), &md));
  }
  upb_Status status;
  upb_Status_Clear(&status);
  const upb_MiniTable* table = _upb_MiniTable_Build(
      md.data, md.size, kUpb_MiniTablePlatform_32Bit, arena.ptr(), &status);
  if (!status.ok) {
    FAIL() << "Could not build minitable: " << status.msg;
  }
  const upb_MiniTableField* mtfield =
      upb_MiniTable_FindFieldByNumber(table, field_tag);
  uint16_t offset = UPB_PRIVATE(_upb_MiniTableField_Offset)(mtfield);
  size_t alignment = 1 << absl::countr_zero(offset);
  // Must align to at least 64 bit
  EXPECT_GE(alignment, 8);
}

TEST(MessageTest, DecodeRequiredFieldsTopLevelMessage) {
  upb::Arena arena;
  upb_test_TestRequiredFields* test_msg;
  upb_test_EmptyMessage* empty_msg;

  // Succeeds, because we did not request required field checks.
  test_msg = upb_test_TestRequiredFields_parse(nullptr, 0, arena.ptr());
  EXPECT_NE(nullptr, test_msg);

  // Fails, because required fields are missing.
  EXPECT_EQ(kUpb_DecodeStatus_MissingRequired,
            upb_Decode(nullptr, 0, UPB_UPCAST(test_msg),
                       &upb_0test__TestRequiredFields_msg_init, nullptr,
                       kUpb_DecodeOption_CheckRequired, arena.ptr()));

  upb_test_TestRequiredFields_set_required_int32(test_msg, 1);
  size_t size;
  char* serialized =
      upb_test_TestRequiredFields_serialize(test_msg, arena.ptr(), &size);
  ASSERT_TRUE(serialized != nullptr);
  EXPECT_NE(0, size);

  // Fails, but the code path is slightly different because the serialized
  // payload is not empty.
  EXPECT_EQ(kUpb_DecodeStatus_MissingRequired,
            upb_Decode(serialized, size, UPB_UPCAST(test_msg),
                       &upb_0test__TestRequiredFields_msg_init, nullptr,
                       kUpb_DecodeOption_CheckRequired, arena.ptr()));

  empty_msg = upb_test_EmptyMessage_new(arena.ptr());
  upb_test_TestRequiredFields_set_required_int32(test_msg, 1);
  upb_test_TestRequiredFields_set_required_int64(test_msg, 2);
  upb_test_TestRequiredFields_set_required_message(test_msg, empty_msg);

  // Succeeds, because required fields are present (though not in the input).
  EXPECT_EQ(kUpb_DecodeStatus_Ok,
            upb_Decode(nullptr, 0, UPB_UPCAST(test_msg),
                       &upb_0test__TestRequiredFields_msg_init, nullptr,
                       kUpb_DecodeOption_CheckRequired, arena.ptr()));

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
            upb_Decode(serialized, size, UPB_UPCAST(test_msg2),
                       &upb_0test__TestRequiredFields_msg_init, nullptr,
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
    upb_Message_SetFieldByDef(UPB_UPCAST(test_msg), f.ptr(), val, arena.ptr());
  }

  // Fails, field 63 still isn't set.
  serialized = upb_test_TestMaxRequiredFields_serialize_ex(
      test_msg, kUpb_EncodeOption_CheckRequired, arena.ptr(), &size);
  ASSERT_TRUE(serialized == nullptr);

  // Succeeds, all required fields are set.
  upb::FieldDefPtr f = m.FindFieldByNumber(62);
  ASSERT_TRUE(f);
  upb_Message_SetFieldByDef(UPB_UPCAST(test_msg), f.ptr(), val, arena.ptr());
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
  ASSERT_NE(nullptr, test_msg_extra2);
  ASSERT_TRUE(
      upb_test_TestMapFieldExtra_map_field_get(test_msg_extra2, 0, nullptr));
}

TEST(MessageTest, MapFieldDeterministicEncoding) {
  upb::Arena arena1;
  upb_test_TestMapField* test_msg1 = upb_test_TestMapField_new(arena1.ptr());
  for (int i = 0; i <= 1000; i++) {
    ASSERT_TRUE(upb_test_TestMapField_map_field_set(
        test_msg1, i, upb_test_TestMapField_ZERO, arena1.ptr()));
  }
  for (int i = 1001; i <= 2000; i++) {
    ASSERT_TRUE(upb_test_TestMapField_map_field_set(
        test_msg1, i, upb_test_TestMapField_ONE, arena1.ptr()));
  }
  for (int i = 2001; i <= 3000; i++) {
    ASSERT_TRUE(upb_test_TestMapField_map_field_set(
        test_msg1, i, upb_test_TestMapField_TWO, arena1.ptr()));
  }
  size_t size1;
  char* serialized1;
  upb_EncodeStatus status1 = upb_Encode(
      UPB_UPCAST(test_msg1), &upb_0test__TestMapField_msg_init,
      kUpb_EncodeOption_Deterministic, arena1.ptr(), &serialized1, &size1);
  ASSERT_EQ(status1, kUpb_EncodeStatus_Ok);
  ASSERT_NE(nullptr, serialized1);

  upb::Arena arena2;
  upb_test_TestMapField* test_msg2 = upb_test_TestMapField_new(arena2.ptr());
  // Add the same values in reverse order.
  for (int i = 3000; i >= 2001; i--) {
    ASSERT_TRUE(upb_test_TestMapField_map_field_set(
        test_msg2, i, upb_test_TestMapField_TWO, arena2.ptr()));
  }
  for (int i = 2000; i >= 1001; i--) {
    ASSERT_TRUE(upb_test_TestMapField_map_field_set(
        test_msg2, i, upb_test_TestMapField_ONE, arena2.ptr()));
  }
  for (int i = 1000; i >= 0; i--) {
    ASSERT_TRUE(upb_test_TestMapField_map_field_set(
        test_msg2, i, upb_test_TestMapField_ZERO, arena2.ptr()));
  }
  size_t size2;
  char* serialized2;
  upb_EncodeStatus status2 = upb_Encode(
      UPB_UPCAST(test_msg2), &upb_0test__TestMapField_msg_init,
      kUpb_EncodeOption_Deterministic, arena2.ptr(), &serialized2, &size2);
  ASSERT_EQ(status2, kUpb_EncodeStatus_Ok);
  ASSERT_NE(nullptr, serialized2);

  EXPECT_EQ(size1, size2);
  EXPECT_EQ(0, memcmp(serialized1, serialized2, size1));
}

TEST(MessageTest, Freeze) {
  const upb_MiniTable* m = &upb_0test__TestFreeze_msg_init;
  upb::Arena arena;

  {
    upb_test_TestFreeze* raw = upb_test_TestFreeze_new(arena.ptr());
    upb_Message* msg = UPB_UPCAST(raw);
    ASSERT_FALSE(upb_Message_IsFrozen(msg));
    upb_Message_Freeze(msg, m);
    ASSERT_TRUE(upb_Message_IsFrozen(msg));
  }
  {
    upb_test_TestFreeze* raw = upb_test_TestFreeze_new(arena.ptr());
    upb_Message* msg = UPB_UPCAST(raw);
    size_t size;
    upb_Array* arr = _upb_test_TestFreeze_array_int_mutable_upb_array(
        raw, &size, arena.ptr());
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(size, 0);
    ASSERT_FALSE(upb_Array_IsFrozen(arr));
    upb_Map* map =
        _upb_test_TestFreeze_map_int_mutable_upb_map(raw, arena.ptr());
    ASSERT_NE(map, nullptr);
    ASSERT_FALSE(upb_Map_IsFrozen(map));
    upb_test_TestFreeze* nest = upb_test_TestFreeze_new(arena.ptr());
    upb_test_set_nest(raw, nest, arena.ptr());
    ASSERT_FALSE(upb_Message_IsFrozen(UPB_UPCAST(nest)));

    upb_Message_Freeze(msg, m);
    ASSERT_TRUE(upb_Message_IsFrozen(msg));
    ASSERT_TRUE(upb_Array_IsFrozen(arr));
    ASSERT_TRUE(upb_Map_IsFrozen(map));
    ASSERT_TRUE(upb_Message_IsFrozen(UPB_UPCAST(nest)));
  }
  {
    upb_test_TestFreeze* raw = upb_test_TestFreeze_new(arena.ptr());
    upb_Message* msg = UPB_UPCAST(raw);
    size_t size;
    upb_Array* arr = _upb_test_TestFreeze_array_int_mutable_upb_array(
        raw, &size, arena.ptr());
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(size, 0);
    ASSERT_FALSE(upb_Array_IsFrozen(arr));
    upb_Map* map =
        _upb_test_TestFreeze_map_int_mutable_upb_map(raw, arena.ptr());
    ASSERT_NE(map, nullptr);
    ASSERT_FALSE(upb_Map_IsFrozen(map));
    upb_test_TestFreeze* nest = upb_test_TestFreeze_new(arena.ptr());
    upb_test_set_nest(raw, nest, arena.ptr());
    ASSERT_FALSE(upb_Message_IsFrozen(UPB_UPCAST(nest)));

    upb_Message_Freeze(UPB_UPCAST(nest), m);
    ASSERT_FALSE(upb_Message_IsFrozen(msg));
    ASSERT_FALSE(upb_Array_IsFrozen(arr));
    ASSERT_FALSE(upb_Map_IsFrozen(map));
    ASSERT_TRUE(upb_Message_IsFrozen(UPB_UPCAST(nest)));

    const upb_MiniTableField* fa = upb_MiniTable_FindFieldByNumber(m, 20);
    const upb_MiniTable* ma = upb_MiniTable_SubMessage(m, fa);
    upb_Array_Freeze(arr, ma);
    ASSERT_FALSE(upb_Message_IsFrozen(msg));
    ASSERT_TRUE(upb_Array_IsFrozen(arr));
    ASSERT_FALSE(upb_Map_IsFrozen(map));
    ASSERT_TRUE(upb_Message_IsFrozen(UPB_UPCAST(nest)));

    const upb_MiniTableField* fm = upb_MiniTable_FindFieldByNumber(m, 10);
    const upb_MiniTable* mm = upb_MiniTable_SubMessage(m, fm);
    upb_Map_Freeze(map, mm);
    ASSERT_FALSE(upb_Message_IsFrozen(msg));
    ASSERT_TRUE(upb_Array_IsFrozen(arr));
    ASSERT_TRUE(upb_Map_IsFrozen(map));
    ASSERT_TRUE(upb_Message_IsFrozen(UPB_UPCAST(nest)));
  }
}
