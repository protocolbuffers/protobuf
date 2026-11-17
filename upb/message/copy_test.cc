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
#include <string_view>
#include <vector>

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto2.upb.h"
#include "google/protobuf/test_messages_proto2.upb_minitable.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/status.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/unknown_fields.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.hpp"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/test/test.upb.h"
#include "upb/test/test.upb_minitable.h"
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
  ASSERT_TRUE(upb_Message_SetInt32(UPB_UPCAST(msg), optional_int32_field,
                                   kTestInt32, nullptr));
  char* string_in_arena =
      (char*)upb_Arena_Malloc(source_arena, sizeof(kTestStr1));
  memcpy(string_in_arena, kTestStr1, sizeof(kTestStr1));
  ASSERT_TRUE(upb_Message_SetString(
      UPB_UPCAST(msg), optional_string_field,
      upb_StringView_FromDataAndSize(string_in_arena, sizeof(kTestStr1) - 1),
      source_arena));
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
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      UPB_UPCAST(msg), data, len, source_arena, kUpb_AddUnknown_Copy));
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
  upb_MessageUnknown unknown;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  while (upb_Message_NextUnknown2(UPB_UPCAST(clone), &unknown, &iter)) {
    ASSERT_EQ(unknown.type, kUpb_MessageUnknownType_StringView);
    cloned_unknown_data.append(unknown.value.bytes.data,
                               unknown.value.bytes.size);
  }
  EXPECT_EQ(unknown_data, cloned_unknown_data);
  upb_Arena_Free(clone_arena);
}

TEST(GeneratedCode, ShallowCopyMessage) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(
      msg, kTestInt32);
  char* string_in_arena = (char*)upb_Arena_Malloc(arena, sizeof(kTestStr1));
  memcpy(string_in_arena, kTestStr1, sizeof(kTestStr1));
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_string(
      msg,
      upb_StringView_FromDataAndSize(string_in_arena, sizeof(kTestStr1) - 1));

  protobuf_test_messages_proto2_TestAllTypesProto2* dst =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  EXPECT_TRUE(upb_Message_ShallowCopy(
      UPB_UPCAST(dst), UPB_UPCAST(msg),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, arena));

  EXPECT_EQ(
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(dst),
      kTestInt32);
  upb_StringView dst_str =
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_string(dst);
  EXPECT_EQ(dst_str.size, sizeof(kTestStr1) - 1);
  EXPECT_EQ(dst_str.data, string_in_arena);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, ShallowCopyIncludesExtensions) {
  upb_Arena* source_arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg =
      upb_test_ModelWithExtensions_new(source_arena);
  upb_test_ModelExtension1* ext1 = upb_test_ModelExtension1_new(source_arena);
  upb_test_ModelExtension1_set_str(ext1, upb_StringView_FromString(kTestStr1));
  upb_test_ModelExtension1_set_model_ext(msg, ext1, source_arena);

  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* dst = upb_test_ModelWithExtensions_new(arena);

  EXPECT_TRUE(upb_Message_ShallowCopy(UPB_UPCAST(dst), UPB_UPCAST(msg),
                                      &upb_0test__ModelWithExtensions_msg_init,
                                      arena));

  // Modifying extension in dst should NOT affect msg because extensions are
  // copied.
  upb_test_ModelExtension1* ext2 = upb_test_ModelExtension1_new(arena);
  upb_test_ModelExtension1_set_str(ext2, upb_StringView_FromString(kTestStr2));

  upb_test_ModelExtension1_set_model_ext(dst, ext2, arena);

  const upb_test_ModelExtension1* src_ext =
      upb_test_ModelExtension1_model_ext(msg);

  EXPECT_EQ(UPB_UPCAST(src_ext), UPB_UPCAST(ext1));

  const upb_test_ModelExtension1* dst_ext =
      upb_test_ModelExtension1_model_ext(dst);

  EXPECT_EQ(UPB_UPCAST(dst_ext), UPB_UPCAST(ext2));

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, ShallowCopyHandlesClearedExtensions) {
  upb_Arena* source_arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg =
      upb_test_ModelWithExtensions_new(source_arena);
  upb_test_ModelExtension1* ext1 = upb_test_ModelExtension1_new(source_arena);
  upb_test_ModelExtension1_set_str(ext1, upb_StringView_FromString(kTestStr1));
  upb_test_ModelExtension1_set_model_ext(msg, ext1, source_arena);

  // Clear the extension, creating a tombstone in aux_data.
  upb_Message_ClearExtension(UPB_UPCAST(msg),
                             upb_test_ModelExtension1_model_ext_ext);

  // Verify source has a tombstone (size is 1).
  upb_Message_Internal* msg_in =
      UPB_PRIVATE(_upb_Message_GetInternal)(UPB_UPCAST(msg));
  EXPECT_NE(msg_in, nullptr);
  EXPECT_EQ(msg_in->size, 1);

  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* dst = upb_test_ModelWithExtensions_new(arena);

  // This should not crash.
  EXPECT_TRUE(upb_Message_ShallowCopy(UPB_UPCAST(dst), UPB_UPCAST(msg),
                                      &upb_0test__ModelWithExtensions_msg_init,
                                      arena));

  // The extension should not be present in dst.
  EXPECT_EQ(upb_test_ModelExtension1_model_ext(dst), nullptr);

  // Verify internal array size is compacted in dst (size is 0).
  upb_Message_Internal* dst_in =
      UPB_PRIVATE(_upb_Message_GetInternal)(UPB_UPCAST(dst));
  EXPECT_NE(dst_in, nullptr);
  EXPECT_EQ(dst_in->size, 0);

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, DeepCloneHandlesClearedExtensions) {
  upb_Arena* source_arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg =
      upb_test_ModelWithExtensions_new(source_arena);
  upb_test_ModelExtension1* ext1 = upb_test_ModelExtension1_new(source_arena);
  upb_test_ModelExtension1_set_str(ext1, upb_StringView_FromString(kTestStr1));
  upb_test_ModelExtension1_set_model_ext(msg, ext1, source_arena);

  // Clear the extension, creating a tombstone in aux_data.
  upb_Message_ClearExtension(UPB_UPCAST(msg),
                             upb_test_ModelExtension1_model_ext_ext);

  // Verify source has a tombstone (size is 1).
  upb_Message_Internal* msg_in =
      UPB_PRIVATE(_upb_Message_GetInternal)(UPB_UPCAST(msg));
  EXPECT_NE(msg_in, nullptr);
  EXPECT_EQ(msg_in->size, 1);

  upb_Arena* arena = upb_Arena_New();

  // This should not crash.
  upb_Message* clone = upb_Message_DeepClone(
      UPB_UPCAST(msg), &upb_0test__ModelWithExtensions_msg_init, arena);
  EXPECT_NE(clone, nullptr);

  // The extension should not be present in clone.
  EXPECT_EQ(
      upb_test_ModelExtension1_model_ext((upb_test_ModelWithExtensions*)clone),
      nullptr);

  // Verify internal array is compacted (DeepClone doesn't create internal data
  // if empty).
  upb_Message_Internal* clone_in = UPB_PRIVATE(_upb_Message_GetInternal)(clone);
  EXPECT_EQ(clone_in, nullptr);

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

std::vector<std::string_view> GetUnknownFields(const upb_Message* msg) {
  std::vector<std::string_view> result;
  upb_MessageUnknown data;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  while (upb_Message_NextUnknown2(msg, &data, &iter)) {
    EXPECT_EQ(data.type, kUpb_MessageUnknownType_StringView);
    result.push_back(
        std::string_view(data.value.bytes.data, data.value.bytes.size));
  }
  return result;
}

TEST(GeneratedCode, ShallowCopyIncludesUnknowns) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);

  // Add some unknown data.
  std::string_view unknown_data1 =
      "\x08\x01";  // Field 1, wire type 0 (varint), value 1
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      UPB_UPCAST(msg), unknown_data1.data(), unknown_data1.size(), source_arena,
      kUpb_AddUnknown_Copy));
  std::vector<std::string_view> only_unknown_data1 = {unknown_data1};

  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* dst =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  EXPECT_TRUE(upb_Message_ShallowCopy(
      UPB_UPCAST(dst), UPB_UPCAST(msg),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, arena));

  // Verify dst has the unknown data.
  EXPECT_EQ(GetUnknownFields(UPB_UPCAST(dst)), only_unknown_data1);

  // Modify the unknown data view in dst to ensure it's a separate view.
  // Use upb_Message_DeleteUnknown to delete a trailing part.
  upb_MessageUnknown dst_unknown;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  EXPECT_TRUE(upb_Message_NextUnknown2(UPB_UPCAST(dst), &dst_unknown, &iter));
  ASSERT_EQ(dst_unknown.type, kUpb_MessageUnknownType_StringView);
  upb_StringView to_delete;
  to_delete.size = 1;
  to_delete.data = dst_unknown.value.bytes.data + dst_unknown.value.bytes.size -
                   to_delete.size;
  upb_Message_DeleteUnknownStatus status =
      upb_Message_DeleteUnknown(UPB_UPCAST(dst), &to_delete, &iter, arena);
  EXPECT_EQ(status, kUpb_DeleteUnknown_DeletedLast);

  // Verify src still has the original data and size.
  EXPECT_EQ(GetUnknownFields(UPB_UPCAST(msg)), only_unknown_data1);

  // Add MORE unknown data to dst.
  std::string_view unknown_data2 =
      "\x10\x02";  // Field 2, wire type 0 (varint), value 2
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      UPB_UPCAST(dst), unknown_data2.data(), unknown_data2.size(), arena,
      kUpb_AddUnknown_Copy));

  // Verify src still only has the first unknown data.
  EXPECT_EQ(GetUnknownFields(UPB_UPCAST(msg)), only_unknown_data1);

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, ShallowCloneMessage) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(
      msg, kTestInt32);

  protobuf_test_messages_proto2_TestAllTypesProto2* clone =
      (protobuf_test_messages_proto2_TestAllTypesProto2*)
          upb_Message_ShallowClone(
              UPB_UPCAST(msg),
              &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
              arena);

  EXPECT_EQ(
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(clone),
      kTestInt32);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, ShallowCloneFrozenMessage) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(
      msg, kTestInt32);

  const upb_MiniTableField* nested_message_field =
      find_proto2_field(kFieldOptionalNestedMessage);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* nested =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(
          source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(
      nested, kTestNestedInt32);
  upb_Message_SetMessage(UPB_UPCAST(msg), nested_message_field,
                         UPB_UPCAST(nested));

  upb_Message_Freeze(
      UPB_UPCAST(msg),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init);
  EXPECT_TRUE(upb_Message_IsFrozen(UPB_UPCAST(msg)));
  EXPECT_TRUE(upb_Message_IsFrozen(UPB_UPCAST(nested)));

  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* clone =
      (protobuf_test_messages_proto2_TestAllTypesProto2*)
          upb_Message_ShallowClone(
              UPB_UPCAST(msg),
              &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
              arena);
  ASSERT_NE(clone, nullptr);

  // The shallow clone of a frozen message must be unfrozen and mutable.
  EXPECT_FALSE(upb_Message_IsFrozen(UPB_UPCAST(clone)));

  // Scalar fields can be mutated without affecting the frozen source.
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(clone,
                                                                      999);
  EXPECT_EQ(
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(clone),
      999);
  EXPECT_EQ(
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(msg),
      kTestInt32);

  // Submessages are shallow-copied and remain frozen.
  const upb_Message* clone_nested =
      upb_Message_GetMessage(UPB_UPCAST(clone), nested_message_field);
  EXPECT_EQ(clone_nested, UPB_UPCAST(nested));
  EXPECT_TRUE(upb_Message_IsFrozen(clone_nested));

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, ShallowCopyFrozenMessageWithExtensions) {
  upb_Arena* source_arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg =
      upb_test_ModelWithExtensions_new(source_arena);
  upb_test_ModelExtension1* ext1 = upb_test_ModelExtension1_new(source_arena);
  upb_test_ModelExtension1_set_str(ext1, upb_StringView_FromString(kTestStr1));
  upb_test_ModelExtension1_set_model_ext(msg, ext1, source_arena);

  upb_Message_Freeze(UPB_UPCAST(msg), &upb_0test__ModelWithExtensions_msg_init);
  EXPECT_TRUE(upb_Message_IsFrozen(UPB_UPCAST(msg)));

  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* clone =
      (upb_test_ModelWithExtensions*)upb_Message_ShallowClone(
          UPB_UPCAST(msg), &upb_0test__ModelWithExtensions_msg_init, arena);
  ASSERT_NE(clone, nullptr);

  // Clone is unfrozen and has the cloned extension.
  EXPECT_FALSE(upb_Message_IsFrozen(UPB_UPCAST(clone)));
  const upb_test_ModelExtension1* clone_ext =
      upb_test_ModelExtension1_model_ext(clone);
  ASSERT_NE(clone_ext, nullptr);
  EXPECT_TRUE(upb_StringView_IsEqual(upb_test_ModelExtension1_str(clone_ext),
                                     upb_StringView_FromString(kTestStr1)));

  // Can mutate/overwrite extension on clone without affecting frozen source.
  upb_test_ModelExtension1* new_ext = upb_test_ModelExtension1_new(arena);
  upb_test_ModelExtension1_set_str(new_ext,
                                   upb_StringView_FromString(kTestStr2));
  upb_test_ModelExtension1_set_model_ext(clone, new_ext, arena);
  EXPECT_TRUE(upb_StringView_IsEqual(
      upb_test_ModelExtension1_str(upb_test_ModelExtension1_model_ext(clone)),
      upb_StringView_FromString(kTestStr2)));
  EXPECT_TRUE(upb_StringView_IsEqual(
      upb_test_ModelExtension1_str(upb_test_ModelExtension1_model_ext(msg)),
      upb_StringView_FromString(kTestStr1)));

  // ShallowCopy from a frozen message without extensions into a destination
  // with extensions clears the destination's extensions and leaves it unfrozen.
  upb_test_ModelWithExtensions* empty_msg =
      upb_test_ModelWithExtensions_new(source_arena);
  upb_Message_Freeze(UPB_UPCAST(empty_msg),
                     &upb_0test__ModelWithExtensions_msg_init);
  EXPECT_TRUE(upb_Message_IsFrozen(UPB_UPCAST(empty_msg)));

  EXPECT_TRUE(upb_Message_ShallowCopy(UPB_UPCAST(clone), UPB_UPCAST(empty_msg),
                                      &upb_0test__ModelWithExtensions_msg_init,
                                      arena));
  EXPECT_FALSE(upb_Message_IsFrozen(UPB_UPCAST(clone)));
  EXPECT_EQ(upb_test_ModelExtension1_model_ext(clone), nullptr);

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, DeepCloneMessageNonCanonicalExtensions) {
  upb_Arena* source_arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg =
      upb_test_ModelWithExtensions_new(source_arena);
  upb_test_ModelExtension1* ext1 = upb_test_ModelExtension1_new(source_arena);
  upb_test_ModelExtension1_set_str(ext1,
                                   upb_StringView_FromString("LifecycleValue"));

  // Attach as non-canonical extension
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg), upb_test_ModelExtension1_model_ext_ext, &ext1,
      source_arena));

  // Deep clone msg to clone
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* clone =
      (upb_test_ModelWithExtensions*)upb_Message_DeepClone(
          UPB_UPCAST(msg), &upb_0test__ModelWithExtensions_msg_init, arena);
  ASSERT_NE(clone, nullptr);

  // Mutate original
  upb_test_ModelExtension1_set_str(ext1, upb_StringView_FromString("Mutated"));
  upb_Arena_Free(source_arena);

  // A non-canonical extension's minitable may belong to a schema that does not
  // outlive the clone, so deep clone serializes it into unknown bytes instead
  // of keeping a typed non-canonical extension.
  upb_MessageUnknown data;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  ASSERT_TRUE(upb_Message_NextUnknown2(UPB_UPCAST(clone), &data, &iter));
  EXPECT_EQ(data.type, kUpb_MessageUnknownType_StringView);
  EXPECT_FALSE(upb_Message_NextUnknown2(UPB_UPCAST(clone), &data, &iter));

  // Round-trip with a registry to verify the cloned value is unmodified.
  char* buf;
  size_t size;
  ASSERT_EQ(
      upb_Encode(UPB_UPCAST(clone), &upb_0test__ModelWithExtensions_msg_init, 0,
                 arena, &buf, &size),
      kUpb_EncodeStatus_Ok);
  upb_ExtensionRegistry* extreg = upb_ExtensionRegistry_New(arena);
  ASSERT_NE(extreg, nullptr);
  ASSERT_EQ(
      upb_ExtensionRegistry_Add(extreg, upb_test_ModelExtension1_model_ext_ext),
      kUpb_ExtensionRegistryStatus_Ok);
  upb_test_ModelWithExtensions* parsed =
      upb_test_ModelWithExtensions_parse_ex(buf, size, extreg, 0, arena);
  ASSERT_NE(parsed, nullptr);
  ASSERT_TRUE(upb_test_ModelExtension1_has_model_ext(parsed));
  const upb_test_ModelExtension1* cloned_ext =
      upb_test_ModelExtension1_model_ext(parsed);
  EXPECT_TRUE(
      upb_StringView_IsEqual(upb_test_ModelExtension1_str(cloned_ext),
                             upb_StringView_FromString("LifecycleValue")));

  upb_Arena_Free(arena);
}

protobuf_test_messages_proto2_TestAllTypesProto2* MakeRecursiveChain(
    upb_Arena* arena, int depth, int32_t leaf_value) {
  auto* root = protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  auto* cur = root;
  for (int i = 0; i < depth; ++i) {
    auto* child = protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
    protobuf_test_messages_proto2_TestAllTypesProto2_set_recursive_message(
        cur, child);
    cur = child;
  }
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(
      cur, leaf_value);
  return root;
}

const protobuf_test_messages_proto2_TestAllTypesProto2* FollowRecursiveChain(
    const protobuf_test_messages_proto2_TestAllTypesProto2* msg, int depth) {
  const auto* cur = msg;
  for (int i = 0; i < depth; ++i) {
    if (cur == nullptr) return nullptr;
    cur =
        protobuf_test_messages_proto2_TestAllTypesProto2_recursive_message(cur);
  }
  return cur;
}

const upb_MiniTableExtension* BuildSubMessageExt(const upb_MiniTable* extendee,
                                                 const upb_MiniTable* sub,
                                                 int field_number,
                                                 upb_Arena* arena) {
  upb::MtDataEncoder ext_encoder;
  EXPECT_TRUE(
      ext_encoder.EncodeExtension(kUpb_FieldType_Message, field_number, 0));
  upb_Status status;
  upb_Status_Clear(&status);
  const upb_MiniTableExtension* ext = upb_MiniTableExtension_BuildMessage(
      ext_encoder.data().data(), ext_encoder.data().size(), extendee, sub,
      arena, &status);
  EXPECT_TRUE(status.ok) << status.msg;
  EXPECT_NE(ext, nullptr);
  return ext;
}

const upb_MiniTableExtension* BuildTestAllTypesProto2Ext(
    const upb_MiniTable* extendee, int field_number, upb_Arena* arena) {
  return BuildSubMessageExt(
      extendee, &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      field_number, arena);
}

TEST(GeneratedCode, DeepCloneWithMaxDepthZeroAllowsFlatMessage) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

  auto* flat = MakeRecursiveChain(src_arena, /*depth=*/0, /*leaf_value=*/42);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_string(
      flat, upb_StringView_FromString("flat"));
  protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_int32_set(
      flat, 7, 13, src_arena);

  auto* clone = (protobuf_test_messages_proto2_TestAllTypesProto2*)
      upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(flat), mt, dst_arena,
                                        /*max_depth=*/0);
  ASSERT_NE(clone, nullptr);
  EXPECT_EQ(
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(clone),
      42);
  EXPECT_TRUE(upb_StringView_IsEqual(
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_string(clone),
      upb_StringView_FromString("flat")));
  int32_t map_val = 0;
  EXPECT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_int32_get(
          clone, 7, &map_val));
  EXPECT_EQ(map_val, 13);

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCloneWithMaxDepthNegativeFails) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

  auto* flat = protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(flat), mt, dst_arena,
                                              /*max_depth=*/-1),
            nullptr);

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCloneWithMaxDepthScalarChainBoundary) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

  auto* chain = MakeRecursiveChain(src_arena, /*depth=*/3, /*leaf_value=*/123);
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(chain), mt, dst_arena,
                                              /*max_depth=*/2),
            nullptr);

  auto* clone = (protobuf_test_messages_proto2_TestAllTypesProto2*)
      upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(chain), mt, dst_arena,
                                        /*max_depth=*/3);
  ASSERT_NE(clone, nullptr);
  const auto* leaf = FollowRecursiveChain(clone, 3);
  ASSERT_NE(leaf, nullptr);
  EXPECT_EQ(
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(leaf),
      123);

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCloneWithMaxDepthRepeatedSubmessageBoundary) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

  // Depth 1: repeated_nested_message.
  auto* depth1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_nested_message(
      depth1, src_arena);
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(depth1), mt, dst_arena,
                                              /*max_depth=*/0),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(depth1), mt, dst_arena,
                                              /*max_depth=*/1),
            nullptr);

  // Depth 2: repeated_nested_message -> corecursive.
  auto* depth2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  auto* item =
      protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_nested_message(
          depth2, src_arena);
  auto* leaf = protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(leaf, 77);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_corecursive(
      item, leaf);
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(depth2), mt, dst_arena,
                                              /*max_depth=*/1),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(depth2), mt, dst_arena,
                                              /*max_depth=*/2),
            nullptr);

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCloneWithMaxDepthMapSubmessageBoundary) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

  // Depth 1: map_int32_nested_message.
  auto* depth1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  auto* val1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(
          src_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_nested_message_set(
      depth1, 1, val1, src_arena);
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(depth1), mt, dst_arena,
                                              /*max_depth=*/0),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(depth1), mt, dst_arena,
                                              /*max_depth=*/1),
            nullptr);

  // Depth 2: map_int32_nested_message -> corecursive.
  auto* depth2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  auto* val2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(
          src_arena);
  auto* leaf = protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(leaf, 88);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_corecursive(
      val2, leaf);
  protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_nested_message_set(
      depth2, 1, val2, src_arena);
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(depth2), mt, dst_arena,
                                              /*max_depth=*/1),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(depth2), mt, dst_arena,
                                              /*max_depth=*/2),
            nullptr);

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCloneWithMaxDepthCanonicalExtensionBoundary) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mt = &upb_0test__ModelWithExtensions_msg_init;

  auto* ext_root = upb_test_ModelWithExtensions_new(src_arena);
  auto* ext1 = upb_test_ModelExtension1_new(src_arena);
  upb_test_ModelExtension1_set_str(ext1, upb_StringView_FromString("ext"));
  upb_test_ModelExtension1_set_model_ext(ext_root, ext1, src_arena);

  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(ext_root), mt,
                                              dst_arena, /*max_depth=*/0),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(ext_root), mt,
                                              dst_arena, /*max_depth=*/1),
            nullptr);

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCloneAndCopyWithMaxDepthNonCanonicalExtensionBoundary) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;
  const upb_MiniTableExtension* ext =
      BuildTestAllTypesProto2Ext(mt, /*field_number=*/125, src_arena);
  ASSERT_NE(ext, nullptr);

  // Depth 1 non-canonical extension fails at max_depth=0, succeeds at 1.
  auto* root1 = protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  auto* ext_val1 = MakeRecursiveChain(src_arena, /*depth=*/0, /*leaf_value=*/5);
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(root1), ext, &ext_val1, src_arena));
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(root1), mt, dst_arena,
                                              /*max_depth=*/0),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(root1), mt, dst_arena,
                                              /*max_depth=*/1),
            nullptr);

  // Root -> non-canonical ext -> child (total depth 2) fails at max_depth=1
  // and succeeds at max_depth=2 (verifying encoder depth forwarding).
  auto* root2 = protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  auto* ext_val2 = MakeRecursiveChain(src_arena, /*depth=*/1, /*leaf_value=*/7);
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(root2), ext, &ext_val2, src_arena));

  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(root2), mt, dst_arena,
                                              /*max_depth=*/1),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(root2), mt, dst_arena,
                                              /*max_depth=*/2),
            nullptr);

  auto* dst = protobuf_test_messages_proto2_TestAllTypesProto2_new(dst_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(dst, 19);
  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(root2), mt, dst_arena, /*max_depth=*/1));
  EXPECT_FALSE(
      protobuf_test_messages_proto2_TestAllTypesProto2_has_optional_int32(dst));
  EXPECT_TRUE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(root2), mt, dst_arena, /*max_depth=*/2));

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCopyWithMaxDepthAtLimitSucceeds) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

  auto* flat = MakeRecursiveChain(src_arena, /*depth=*/0, /*leaf_value=*/55);
  auto* dst = protobuf_test_messages_proto2_TestAllTypesProto2_new(dst_arena);
  ASSERT_TRUE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(flat), mt, dst_arena, /*max_depth=*/0));
  EXPECT_EQ(
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(dst), 55);

  auto* chain = MakeRecursiveChain(src_arena, /*depth=*/3, /*leaf_value=*/234);
  ASSERT_TRUE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(chain), mt, dst_arena, /*max_depth=*/3));
  const auto* leaf = FollowRecursiveChain(dst, 3);
  ASSERT_NE(leaf, nullptr);
  EXPECT_EQ(
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(leaf),
      234);

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCopyWithMaxDepthOverLimitClearsDst) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;
  const upb_MiniTableExtension* ext =
      BuildTestAllTypesProto2Ext(mt, /*field_number=*/125, src_arena);
  ASSERT_NE(ext, nullptr);

  auto* dst = protobuf_test_messages_proto2_TestAllTypesProto2_new(dst_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(dst, 99);
  // Field 1001, wire type 0 (varint), value 2 (reserved/unknown field number).
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      UPB_UPCAST(dst), "\xc8\x3e\x02", 3, dst_arena, kUpb_AddUnknown_Copy));

  // Populate base fields and an unknown field before a submessage extension
  // that exceeds max_depth=1 so _upb_Message_Copy() copies partial state into
  // `dst` before hitting the rollback path (`err: upb_Message_Clear`).
  auto* src = protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(src, 42);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_string(
      src, upb_StringView_FromString("partial"));
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(
          src, 7, src_arena));
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_int32_set(
          src, 1, 2, src_arena));
  // Field 1000, wire type 0 (varint), value 1 (reserved/unknown field number).
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      UPB_UPCAST(src), "\xc0\x3e\x01", 3, src_arena, kUpb_AddUnknown_Copy));

  auto* ext_val = MakeRecursiveChain(src_arena, /*depth=*/1, /*leaf_value=*/7);
  upb_AuxField* ext_entry = UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
      UPB_UPCAST(src), ext, src_arena);
  ASSERT_NE(ext_entry, nullptr);
  upb_Extension_SetMessage(ext_entry, UPB_UPCAST(ext_val));

  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(src), mt, dst_arena, /*max_depth=*/1));
  EXPECT_FALSE(
      protobuf_test_messages_proto2_TestAllTypesProto2_has_optional_int32(dst));
  EXPECT_FALSE(
      protobuf_test_messages_proto2_TestAllTypesProto2_has_optional_string(
          dst));
  size_t repeated_size = 0;
  protobuf_test_messages_proto2_TestAllTypesProto2_repeated_int32(
      dst, &repeated_size);
  EXPECT_EQ(repeated_size, 0u);
  EXPECT_EQ(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_int32_size(
          dst),
      0u);
  EXPECT_FALSE(upb_Message_HasExtension(UPB_UPCAST(dst), ext));
  EXPECT_TRUE(GetUnknownFields(UPB_UPCAST(dst)).empty());

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCopyWithMaxDepthNegativeClearsDst) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

  auto* dst = protobuf_test_messages_proto2_TestAllTypesProto2_new(dst_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(dst, 99);
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      UPB_UPCAST(dst), "\xc8\x3e\x02", 3, dst_arena, kUpb_AddUnknown_Copy));

  auto* chain = MakeRecursiveChain(src_arena, /*depth=*/0, /*leaf_value=*/7);
  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(chain), mt, dst_arena, /*max_depth=*/-1));
  EXPECT_FALSE(
      protobuf_test_messages_proto2_TestAllTypesProto2_has_optional_int32(dst));
  EXPECT_TRUE(GetUnknownFields(UPB_UPCAST(dst)).empty());

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCloneAndCopyWithMaxDepthRejectCycles) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

  // 1. Scalar self-cycle.
  auto* self = protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_recursive_message(self,
                                                                         self);
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(self), mt, dst_arena,
                                              /*max_depth=*/100),
            nullptr);

  auto* dst = protobuf_test_messages_proto2_TestAllTypesProto2_new(dst_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(dst, 5);
  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(self), mt, dst_arena, /*max_depth=*/100));
  EXPECT_FALSE(
      protobuf_test_messages_proto2_TestAllTypesProto2_has_optional_int32(dst));

  // 2. Repeated submessage cycle.
  auto* rep_root =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  auto* rep_item =
      protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_nested_message(
          rep_root, src_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_corecursive(
      rep_item, rep_root);
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(rep_root), mt,
                                              dst_arena, /*max_depth=*/100),
            nullptr);
  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(rep_root), mt, dst_arena, /*max_depth=*/100));

  // 3. Map submessage value cycle.
  auto* map_root =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  auto* map_val =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(
          src_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_corecursive(
      map_val, map_root);
  protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_nested_message_set(
      map_root, 1, map_val, src_arena);
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(map_root), mt,
                                              dst_arena, /*max_depth=*/100),
            nullptr);
  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(map_root), mt, dst_arena, /*max_depth=*/100));

  // 4. Canonical extension cycle.
  const upb_MiniTableExtension* self_ext =
      BuildTestAllTypesProto2Ext(mt, /*field_number=*/126, src_arena);
  ASSERT_NE(self_ext, nullptr);
  auto* canon_cycle =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  upb_AuxField* ext_entry = UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
      UPB_UPCAST(canon_cycle), self_ext, src_arena);
  ASSERT_NE(ext_entry, nullptr);
  upb_Extension_SetMessage(ext_entry, UPB_UPCAST(canon_cycle));
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(canon_cycle), mt,
                                              dst_arena, /*max_depth=*/100),
            nullptr);
  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(UPB_UPCAST(dst),
                                                UPB_UPCAST(canon_cycle), mt,
                                                dst_arena, /*max_depth=*/100));

  // 5. Non-canonical extension cycle.
  auto* non_canon_cycle =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(non_canon_cycle), self_ext, &non_canon_cycle, src_arena));
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(non_canon_cycle), mt,
                                              dst_arena, /*max_depth=*/100),
            nullptr);
  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(UPB_UPCAST(dst),
                                                UPB_UPCAST(non_canon_cycle), mt,
                                                dst_arena, /*max_depth=*/100));

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCloneAndCopyDefaultUnlimitedBehaviorUnchanged) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

  // Deeper than kUpb_WireFormat_DefaultDepthLimit (100).
  constexpr int kDeepDepth = 105;
  auto* deep = MakeRecursiveChain(src_arena, kDeepDepth, 987);

  auto* clone =
      (protobuf_test_messages_proto2_TestAllTypesProto2*)upb_Message_DeepClone(
          UPB_UPCAST(deep), mt, dst_arena);
  ASSERT_NE(clone, nullptr);
  const auto* clone_leaf = FollowRecursiveChain(clone, kDeepDepth);
  ASSERT_NE(clone_leaf, nullptr);
  EXPECT_EQ(protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(
                clone_leaf),
            987);

  auto* dst = protobuf_test_messages_proto2_TestAllTypesProto2_new(dst_arena);
  ASSERT_TRUE(
      upb_Message_DeepCopy(UPB_UPCAST(dst), UPB_UPCAST(deep), mt, dst_arena));
  const auto* copy_leaf = FollowRecursiveChain(dst, kDeepDepth);
  ASSERT_NE(copy_leaf, nullptr);
  EXPECT_EQ(protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(
                copy_leaf),
            987);

  // Nested non-canonical extension deeper than encoder default (100):
  // default DeepClone/DeepCopy continue failing with encoder default 100,
  // while DeepCloneWithMaxDepth with an explicit higher bound succeeds.
  const upb_MiniTableExtension* ext =
      BuildTestAllTypesProto2Ext(mt, /*field_number=*/125, src_arena);
  ASSERT_NE(ext, nullptr);
  auto* outer = protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  auto* middle =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_recursive_message(
      outer, middle);
  auto* deep_ext_val = MakeRecursiveChain(src_arena, /*depth=*/105, 88);
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(middle), ext, &deep_ext_val, src_arena));

  EXPECT_EQ(upb_Message_DeepClone(UPB_UPCAST(outer), mt, dst_arena), nullptr);
  EXPECT_FALSE(
      upb_Message_DeepCopy(UPB_UPCAST(dst), UPB_UPCAST(outer), mt, dst_arena));
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(outer), mt, dst_arena,
                                              /*max_depth=*/107),
            nullptr);

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCloneAndCopyWithMaxDepthCanonicalMessageSet) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mset_mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2__MessageSetCorrect_msg_init;

  auto* mset =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          src_arena);
  auto* mset_ext =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_new(
          src_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_set_str(
      mset_ext, upb_StringView_FromString("mset"));
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_set_message_set_extension(
      mset, mset_ext, src_arena);

  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(mset), mset_mt,
                                              dst_arena, /*max_depth=*/0),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(mset), mset_mt,
                                              dst_arena, /*max_depth=*/1),
            nullptr);

  auto* dst =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          dst_arena);
  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(mset), mset_mt, dst_arena, /*max_depth=*/0));
  EXPECT_TRUE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(mset), mset_mt, dst_arena, /*max_depth=*/1));

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCloneAndCopyWithMaxDepthNonCanonicalMessageSet) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mset_mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2__MessageSetCorrect_msg_init;
  const upb_MiniTableExtension* non_canon_ext =
      BuildTestAllTypesProto2Ext(mset_mt, /*field_number=*/4000, src_arena);
  ASSERT_NE(non_canon_ext, nullptr);

  // Flat MessageSet item (total depth 1) fails at 0 and succeeds at 1.
  auto* flat_mset =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          src_arena);
  auto* flat_item =
      MakeRecursiveChain(src_arena, /*depth=*/0, /*leaf_value=*/3);
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(flat_mset), non_canon_ext, &flat_item, src_arena));
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(flat_mset), mset_mt,
                                              dst_arena, /*max_depth=*/0),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(flat_mset), mset_mt,
                                              dst_arena, /*max_depth=*/1),
            nullptr);

  // Depth-1 scalar item (total depth 2) fails at max_depth=1 and succeeds at 2.
  auto* mset_d1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          src_arena);
  auto* item_d1 = MakeRecursiveChain(src_arena, /*depth=*/1, /*leaf_value=*/5);
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(mset_d1), non_canon_ext, &item_d1, src_arena));
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(mset_d1), mset_mt,
                                              dst_arena, /*max_depth=*/1),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(mset_d1), mset_mt,
                                              dst_arena, /*max_depth=*/2),
            nullptr);

  auto* dst_mset =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          dst_arena);
  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(UPB_UPCAST(dst_mset),
                                                UPB_UPCAST(mset_d1), mset_mt,
                                                dst_arena, /*max_depth=*/1));
  EXPECT_TRUE(upb_Message_DeepCopyWithMaxDepth(UPB_UPCAST(dst_mset),
                                               UPB_UPCAST(mset_d1), mset_mt,
                                               dst_arena, /*max_depth=*/2));

  // Depth-1 repeated submessage item (total depth 2) fails at max_depth=1 and
  // succeeds at max_depth=2.
  auto* mset_rep =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          src_arena);
  auto* item_rep =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(src_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_nested_message(
      item_rep, src_arena);
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(mset_rep), non_canon_ext, &item_rep, src_arena));
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(mset_rep), mset_mt,
                                              dst_arena, /*max_depth=*/1),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(mset_rep), mset_mt,
                                              dst_arena, /*max_depth=*/2),
            nullptr);

  // Depth-2 item payload (total depth 3) fails at max_depth=2 and succeeds at
  // max_depth=3 (and above UINT16_MAX) for both Clone and Copy.
  auto* deep_mset =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          src_arena);
  auto* deep_item =
      MakeRecursiveChain(src_arena, /*depth=*/2, /*leaf_value=*/9);
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(deep_mset), non_canon_ext, &deep_item, src_arena));
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(deep_mset), mset_mt,
                                              dst_arena, /*max_depth=*/2),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(deep_mset), mset_mt,
                                              dst_arena, /*max_depth=*/3),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(deep_mset), mset_mt,
                                              dst_arena, /*max_depth=*/100000),
            nullptr);

  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(UPB_UPCAST(dst_mset),
                                                UPB_UPCAST(deep_mset), mset_mt,
                                                dst_arena, /*max_depth=*/2));
  EXPECT_TRUE(upb_Message_DeepCopyWithMaxDepth(UPB_UPCAST(dst_mset),
                                               UPB_UPCAST(deep_mset), mset_mt,
                                               dst_arena, /*max_depth=*/3));

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

TEST(GeneratedCode, DeepCloneAndCopyWithMaxDepthNestedMessageSetChainAndCycle) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Arena* dst_arena = upb_Arena_New();
  const upb_MiniTable* mset_mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2__MessageSetCorrect_msg_init;
  const upb_MiniTableExtension* mset_item_ext =
      BuildSubMessageExt(mset_mt, mset_mt, /*field_number=*/4001, src_arena);
  ASSERT_NE(mset_item_ext, nullptr);

  // Non-canonical MessageSet -> MessageSet chain (total depth 2): fails at
  // max_depth=1 and succeeds at max_depth=2.
  auto* mset0 =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          src_arena);
  auto* item1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          src_arena);
  auto* item2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          src_arena);
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(item1), mset_item_ext, &item2, src_arena));
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(mset0), mset_item_ext, &item1, src_arena));
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(mset0), mset_mt,
                                              dst_arena, /*max_depth=*/1),
            nullptr);
  EXPECT_NE(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(mset0), mset_mt,
                                              dst_arena, /*max_depth=*/2),
            nullptr);

  auto* dst =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          dst_arena);
  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(mset0), mset_mt, dst_arena, /*max_depth=*/1));
  EXPECT_TRUE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(mset0), mset_mt, dst_arena, /*max_depth=*/2));

  // Non-canonical MessageSet self-cycle fails cleanly without stack overflow.
  auto* nc_cycle =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          src_arena);
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(nc_cycle), mset_item_ext, &nc_cycle, src_arena));
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(nc_cycle), mset_mt,
                                              dst_arena, /*max_depth=*/100),
            nullptr);
  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(UPB_UPCAST(dst),
                                                UPB_UPCAST(nc_cycle), mset_mt,
                                                dst_arena, /*max_depth=*/100));

  // Canonical MessageSet self-cycle fails cleanly without stack overflow.
  auto* canon_cycle =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          src_arena);
  upb_AuxField* ext_entry = UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
      UPB_UPCAST(canon_cycle), mset_item_ext, src_arena);
  ASSERT_NE(ext_entry, nullptr);
  upb_Extension_SetMessage(ext_entry, UPB_UPCAST(canon_cycle));
  EXPECT_EQ(upb_Message_DeepCloneWithMaxDepth(UPB_UPCAST(canon_cycle), mset_mt,
                                              dst_arena, /*max_depth=*/100),
            nullptr);
  EXPECT_FALSE(upb_Message_DeepCopyWithMaxDepth(
      UPB_UPCAST(dst), UPB_UPCAST(canon_cycle), mset_mt, dst_arena,
      /*max_depth=*/100));

  upb_Arena_Free(src_arena);
  upb_Arena_Free(dst_arena);
}

}  // namespace

#include "upb/port/undef.inc"
