// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/* Test of mini table accessors.
 *
 * Messages are created and mutated using generated code, and then
 * accessed through reflective APIs exposed through mini table accessors.
 */

#include "upb/message/copy.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto2.upb.h"
#include "google/protobuf/test_messages_proto2.upb_minitable.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/internal/message.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

namespace {

// Proto2 test messages field numbers used for reflective access.
const uint32_t kFieldOptionalInt32 = 1;
const uint32_t kFieldOptionalString = 14;
const uint32_t kFieldOptionalNestedMessage = 18;

const char kTestStr1[] = "Hello1";
const char kTestStr2[] = "HelloWorld2";
const int32_t kTestInt32 = 567;
const int32_t kTestNestedInt32 = 123;
const int32_t kTestNestedInt64 = 123456789;

const upb_MiniTableField* find_proto2_field(int field_number) {
  return upb_MiniTable_FindFieldByNumber(
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      field_number);
}

TEST(GeneratedCode, DeepCloneMessageScalarAndString) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  const upb_MiniTableField* optional_int32_field =
      find_proto2_field(kFieldOptionalInt32);
  const upb_MiniTableField* optional_string_field =
      find_proto2_field(kFieldOptionalString);
  upb_Message_SetInt32(UPB_UPCAST(msg), optional_int32_field, kTestInt32,
                       nullptr);
  char* string_in_arena =
      (char*)upb_Arena_Malloc(source_arena, sizeof(kTestStr1));
  memcpy(string_in_arena, kTestStr1, sizeof(kTestStr1));
  upb_Message_SetString(
      UPB_UPCAST(msg), optional_string_field,
      upb_StringView_FromDataAndSize(string_in_arena, sizeof(kTestStr1) - 1),
      source_arena);
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* clone =
      (protobuf_test_messages_proto2_TestAllTypesProto2*)upb_Message_DeepClone(
          UPB_UPCAST(msg),
          &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
          arena);
  // After cloning overwrite values and destroy source arena for MSAN.
  memset(string_in_arena, 0, sizeof(kTestStr1));
  upb_Arena_Free(source_arena);
  EXPECT_TRUE(
      upb_Message_HasBaseField(UPB_UPCAST(clone), optional_int32_field));
  EXPECT_EQ(upb_Message_GetInt32(UPB_UPCAST(clone), optional_int32_field, 0),
            kTestInt32);
  EXPECT_TRUE(
      upb_Message_HasBaseField(UPB_UPCAST(clone), optional_string_field));
  EXPECT_EQ(upb_Message_GetString(UPB_UPCAST(clone), optional_string_field,
                                  upb_StringView_FromDataAndSize(nullptr, 0))
                .size,
            sizeof(kTestStr1) - 1);
  EXPECT_TRUE(upb_StringView_IsEqual(
      upb_Message_GetString(UPB_UPCAST(clone), optional_string_field,
                            upb_StringView_FromDataAndSize(nullptr, 0)),
      upb_StringView_FromString(kTestStr1)));
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, DeepCloneMessageSubMessage) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  const upb_MiniTableField* nested_message_field =
      find_proto2_field(kFieldOptionalNestedMessage);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* nested =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(
          source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(
      nested, kTestNestedInt32);
  upb_Message_SetMessage(UPB_UPCAST(msg), nested_message_field,
                         UPB_UPCAST(nested));
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* clone =
      (protobuf_test_messages_proto2_TestAllTypesProto2*)upb_Message_DeepClone(
          UPB_UPCAST(msg),
          &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
          arena);
  // After cloning overwrite values and destroy source arena for MSAN.
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(nested,
                                                                       0);
  upb_Arena_Free(source_arena);
  EXPECT_TRUE(
      upb_Message_HasBaseField(UPB_UPCAST(clone), nested_message_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage*
      cloned_nested =
          (protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage*)
              upb_Message_GetMessage(UPB_UPCAST(clone), nested_message_field);
  EXPECT_EQ(protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_a(
                cloned_nested),
            kTestNestedInt32);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, DeepCloneMessageArrayField) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  std::vector<int32_t> array_test_values = {3, 4, 5};
  for (int32_t value : array_test_values) {
    ASSERT_TRUE(
        protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(
            msg, value, source_arena));
  }
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* clone =
      (protobuf_test_messages_proto2_TestAllTypesProto2*)upb_Message_DeepClone(
          UPB_UPCAST(msg),
          &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
          arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_clear_repeated_sint32(msg);
  upb_Arena_Free(source_arena);
  size_t cloned_size = 0;
  const int32_t* cloned_values =
      protobuf_test_messages_proto2_TestAllTypesProto2_repeated_int32(
          clone, &cloned_size);
  EXPECT_EQ(cloned_size, array_test_values.size());
  int index = 0;
  for (int32_t value : array_test_values) {
    EXPECT_EQ(cloned_values[index++], value);
  }
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, DeepCloneMessageMapField) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_bool_set(
          msg, 0, true, source_arena));
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_set(
          msg, 12, 1200.5, source_arena));
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_string_string_set(
          msg, upb_StringView_FromString("key1"),
          upb_StringView_FromString("value1"), source_arena));
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* nested =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(
          source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(
      nested, kTestNestedInt32);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* nested2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(
          source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(
      nested2, kTestNestedInt64);
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_nested_message_set(
          msg, 1, nested2, source_arena));
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_string_nested_message_set(
          msg, upb_StringView_FromString("nestedkey1"), nested, source_arena));

  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* clone =
      (protobuf_test_messages_proto2_TestAllTypesProto2*)upb_Message_DeepClone(
          UPB_UPCAST(msg),
          &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
          arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(nested,
                                                                       0);
  upb_Arena_Free(source_arena);
  // Test map<int32, bool>.
  {
    int32_t key;
    bool value;
    size_t iter = kUpb_Map_Begin;

    ASSERT_TRUE(
        protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_bool_next(
            clone, &key, &value, &iter));
    EXPECT_EQ(key, 0);
    EXPECT_EQ(value, true);

    ASSERT_FALSE(
        protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_bool_next(
            clone, &key, &value, &iter));
  }

  // Test map<int32, double>.
  {
    int32_t key;
    double value;
    size_t iter = kUpb_Map_Begin;

    ASSERT_TRUE(
        protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_next(
            clone, &key, &value, &iter));
    EXPECT_EQ(key, 12);
    EXPECT_EQ(value, 1200.5);

    ASSERT_FALSE(
        protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_next(
            clone, &key, &value, &iter));
  }

  // Test map<int32, NestedMessage>.
  {
    int32_t key;
    const protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* value;
    size_t iter = kUpb_Map_Begin;
    ASSERT_TRUE(
        protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_nested_message_next(
            clone, &key, &value, &iter));
    EXPECT_EQ(key, 1);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(
        protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_a(value),
        kTestNestedInt64);
    ASSERT_FALSE(
        protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_nested_message_next(
            clone, &key, &value, &iter));
  }

  // Test map<string, string>.
  {
    upb_StringView key;
    upb_StringView value;
    size_t iter = kUpb_Map_Begin;

    ASSERT_TRUE(
        protobuf_test_messages_proto2_TestAllTypesProto2_map_string_string_next(
            clone, &key, &value, &iter));
    EXPECT_TRUE(upb_StringView_IsEqual(key, upb_StringView_FromString("key1")));
    EXPECT_TRUE(
        upb_StringView_IsEqual(value, upb_StringView_FromString("value1")));
  }

  // Test map<string, NestedMessage>.
  {
    upb_StringView key;
    const protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* value;
    size_t iter = kUpb_Map_Begin;
    ASSERT_TRUE(
        protobuf_test_messages_proto2_TestAllTypesProto2_map_string_nested_message_next(
            clone, &key, &value, &iter));
    EXPECT_TRUE(
        upb_StringView_IsEqual(key, upb_StringView_FromString("nestedkey1")));
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(
        protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_a(value),
        kTestNestedInt32);
  }

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, DeepCloneMessageExtensions) {
  // Alloc and fill in test message with extension.
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1*
      ext1 =
          protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_new(
              source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_set_str(
      ext1, upb_StringView_FromString(kTestStr1));
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_set_message_set_extension(
      msg, ext1, source_arena);
  // Create clone.
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect* clone =
      (protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect*)
          upb_Message_DeepClone(
              UPB_UPCAST(msg),
              &protobuf_0test_0messages__proto2__TestAllTypesProto2__MessageSetCorrect_msg_init,
              arena);

  // Mutate original extension.
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_set_str(
      ext1, upb_StringView_FromString(kTestStr2));
  upb_Arena_Free(source_arena);

  const protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1*
      cloned_ext =
          protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_message_set_extension(
              clone);
  ASSERT_NE(cloned_ext, nullptr);
  EXPECT_TRUE(upb_StringView_IsEqual(
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_str(
          cloned_ext),
      upb_StringView_FromString(kTestStr1)));
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, DeepCloneMessageWithUnknowns) {
  upb_Arena* source_arena = upb_Arena_New();
  upb_Arena* unknown_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_set(
          msg, 12, 1200.5, source_arena));
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_string_string_set(
          msg, upb_StringView_FromString("key1"),
          upb_StringView_FromString("value1"), source_arena));
  // Create unknown data.
  protobuf_test_messages_proto2_UnknownToTestAllTypes* unknown_source =
      protobuf_test_messages_proto2_UnknownToTestAllTypes_new(unknown_arena);
  protobuf_test_messages_proto2_UnknownToTestAllTypes_set_optional_bool(
      unknown_source, true);
  protobuf_test_messages_proto2_UnknownToTestAllTypes_set_optional_int32(
      unknown_source, 123);
  // Encode unknown message to bytes.
  size_t len;
  char* data;
  upb_Arena* encode_arena = upb_Arena_New();
  upb_EncodeStatus status = upb_Encode(
      UPB_UPCAST(unknown_source),
      &protobuf_0test_0messages__proto2__UnknownToTestAllTypes_msg_init,
      kUpb_EncodeOption_CheckRequired, encode_arena, &data, &len);
  ASSERT_EQ(status, kUpb_EncodeStatus_Ok);
  std::string unknown_data(data, len);
  // Add unknown data.
  UPB_PRIVATE(_upb_Message_AddUnknown)
  (UPB_UPCAST(msg), data, len, source_arena, false);
  // Create clone.
  upb_Arena* clone_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* clone =
      (protobuf_test_messages_proto2_TestAllTypesProto2*)upb_Message_DeepClone(
          UPB_UPCAST(msg),
          &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
          clone_arena);
  upb_Arena_Free(source_arena);
  upb_Arena_Free(unknown_arena);
  upb_Arena_Free(encode_arena);
  // Read unknown data from clone and verify.
  std::string cloned_unknown_data;
  upb_StringView unknown;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  while (upb_Message_NextUnknown(UPB_UPCAST(clone), &unknown, &iter)) {
    cloned_unknown_data.append(unknown.data, unknown.size);
  }
  EXPECT_EQ(unknown_data, cloned_unknown_data);
  upb_Arena_Free(clone_arena);
}

}  // namespace
