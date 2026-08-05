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

#include "upb/message/promote.h"

#include <string.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/status.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/message.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/unknown_fields.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.hpp"
#include "upb/mini_descriptor/internal/modifiers.h"
#include "upb/mini_descriptor/link.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/test/test.upb.h"
#include "upb/test/test.upb_minitable.h"
#include "upb/wire/decode.h"

// Must be last.
#include "upb/port/def.inc"

namespace {

size_t GetUnknownLength(const upb_Message* msg) {
  size_t len = 0;
  upb_StringView data;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  while (upb_Message_NextUnknown(msg, &data, &iter)) {
    len += data.size;
  }
  return len;
}

TEST(GeneratedCode, FindUnknown) {
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg = upb_test_ModelWithExtensions_new(arena);
  upb_test_ModelWithExtensions_set_random_int32(msg, 10);
  upb_test_ModelWithExtensions_set_random_name(
      msg, upb_StringView_FromString("Hello"));

  upb_test_ModelExtension1* extension1 = upb_test_ModelExtension1_new(arena);
  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("World"));

  upb_test_ModelExtension1_set_model_ext(msg, extension1, arena);

  size_t serialized_size;
  char* serialized =
      upb_test_ModelWithExtensions_serialize(msg, arena, &serialized_size);

  upb_test_EmptyMessageWithExtensions* base_msg =
      upb_test_EmptyMessageWithExtensions_parse(serialized, serialized_size,
                                                arena);

  upb_FindUnknownRet result = upb_Message_FindUnknown(
      UPB_UPCAST(base_msg),
      upb_MiniTableExtension_Number(upb_test_ModelExtension1_model_ext_ext), 0);
  EXPECT_EQ(kUpb_FindUnknown_Ok, result.status);

  result = upb_Message_FindUnknown(
      UPB_UPCAST(base_msg),
      upb_MiniTableExtension_Number(upb_test_ModelExtension2_model_ext_ext), 0);
  EXPECT_EQ(kUpb_FindUnknown_NotPresent, result.status);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, PromoteFromMultiple) {
  int options = kUpb_DecodeOption_AliasString;
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg = upb_test_ModelWithExtensions_new(arena);

  upb_test_ModelExtension1* extension1 = upb_test_ModelExtension1_new(arena);
  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("World"));

  upb_test_ModelExtension1_set_model_ext(msg, extension1, arena);

  size_t serialized_size;
  char* serialized1 =
      upb_test_ModelWithExtensions_serialize(msg, arena, &serialized_size);

  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("Everyone"));
  size_t serialized_size2;
  char* serialized2 =
      upb_test_ModelWithExtensions_serialize(msg, arena, &serialized_size2);
  char* concat =
      (char*)upb_Arena_Malloc(arena, serialized_size + serialized_size2);
  memcpy(concat, serialized1, serialized_size);
  memcpy(concat + serialized_size, serialized2, serialized_size2);

  upb_test_ModelWithExtensions* parsed = upb_test_ModelWithExtensions_parse_ex(
      concat, serialized_size + serialized_size2,
      upb_ExtensionRegistry_New(arena), options, arena);

  upb_MessageValue value;
  upb_GetExtension_Status result = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(parsed), upb_test_ModelExtension1_model_ext_ext, options,
      arena, &value);
  ASSERT_EQ(result, kUpb_GetExtension_Ok);
  upb_test_ModelExtension1* parsed_ex =
      (upb_test_ModelExtension1*)value.msg_val;
  upb_StringView field = upb_test_ModelExtension1_str(parsed_ex);
  EXPECT_EQ(absl::string_view(field.data, field.size), "Everyone");

  upb_FindUnknownRet found = upb_Message_FindUnknown(
      UPB_UPCAST(parsed),
      upb_MiniTableExtension_Number(upb_test_ModelExtension1_model_ext_ext), 0);
  EXPECT_EQ(kUpb_FindUnknown_NotPresent, found.status);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, Extensions) {
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg = upb_test_ModelWithExtensions_new(arena);
  upb_test_ModelWithExtensions_set_random_int32(msg, 10);
  upb_test_ModelWithExtensions_set_random_name(
      msg, upb_StringView_FromString("Hello"));

  upb_test_ModelExtension1* extension1 = upb_test_ModelExtension1_new(arena);
  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("World"));

  upb_test_ModelExtension2* extension2 = upb_test_ModelExtension2_new(arena);
  upb_test_ModelExtension2_set_i(extension2, 5);

  upb_test_ModelExtension2* extension3 = upb_test_ModelExtension2_new(arena);
  upb_test_ModelExtension2_set_i(extension3, 6);

  upb_test_ModelExtension2* extension4 = upb_test_ModelExtension2_new(arena);
  upb_test_ModelExtension2_set_i(extension4, 7);

  upb_test_ModelExtension2* extension5 = upb_test_ModelExtension2_new(arena);
  upb_test_ModelExtension2_set_i(extension5, 8);

  upb_test_ModelExtension2* extension6 = upb_test_ModelExtension2_new(arena);
  upb_test_ModelExtension2_set_i(extension6, 9);

  // Set many extensions, to exercise code paths that involve reallocating the
  // extensions and unknown fields array.
  upb_test_ModelExtension1_set_model_ext(msg, extension1, arena);
  upb_test_ModelExtension2_set_model_ext(msg, extension2, arena);
  upb_test_ModelExtension2_set_model_ext_2(msg, extension3, arena);
  upb_test_ModelExtension2_set_model_ext_3(msg, extension4, arena);
  upb_test_ModelExtension2_set_model_ext_4(msg, extension5, arena);
  upb_test_ModelExtension2_set_model_ext_5(msg, extension6, arena);

  size_t serialized_size;
  char* serialized =
      upb_test_ModelWithExtensions_serialize(msg, arena, &serialized_size);

  upb_test_ModelExtension1* ext1;
  upb_test_ModelExtension2* ext2;
  upb_GetExtension_Status promote_status;
  upb_MessageValue value;

  // Test known GetExtension 1
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), upb_test_ModelExtension1_model_ext_ext, 0, arena,
      &value);
  ext1 = (upb_test_ModelExtension1*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_TRUE(upb_StringView_IsEqual(upb_StringView_FromString("World"),
                                     upb_test_ModelExtension1_str(ext1)));

  // Test known GetExtension 2
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), upb_test_ModelExtension2_model_ext_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(5, upb_test_ModelExtension2_i(ext2));

  // Test known GetExtension 3
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), upb_test_ModelExtension2_model_ext_2_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(6, upb_test_ModelExtension2_i(ext2));

  // Test known GetExtension 4
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), upb_test_ModelExtension2_model_ext_3_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(7, upb_test_ModelExtension2_i(ext2));

  // Test known GetExtension 5
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), upb_test_ModelExtension2_model_ext_4_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(8, upb_test_ModelExtension2_i(ext2));

  // Test known GetExtension 6
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), upb_test_ModelExtension2_model_ext_5_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(9, upb_test_ModelExtension2_i(ext2));

  upb_test_EmptyMessageWithExtensions* base_msg =
      upb_test_EmptyMessageWithExtensions_parse(serialized, serialized_size,
                                                arena);

  // Get unknown extension bytes before promotion.
  size_t start_len = GetUnknownLength(UPB_UPCAST(base_msg));
  EXPECT_GT(start_len, 0);
  EXPECT_EQ(0, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  // Test unknown GetExtension.
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(base_msg), upb_test_ModelExtension1_model_ext_ext, 0, arena,
      &value);
  ext1 = (upb_test_ModelExtension1*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_TRUE(upb_StringView_IsEqual(upb_StringView_FromString("World"),
                                     upb_test_ModelExtension1_str(ext1)));
  EXPECT_EQ(1, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  // Test unknown GetExtension.
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(base_msg), upb_test_ModelExtension2_model_ext_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(5, upb_test_ModelExtension2_i(ext2));
  EXPECT_EQ(2, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  // Test unknown GetExtension.
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(base_msg), upb_test_ModelExtension2_model_ext_2_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(6, upb_test_ModelExtension2_i(ext2));
  EXPECT_EQ(3, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  // Test unknown GetExtension.
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(base_msg), upb_test_ModelExtension2_model_ext_3_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(7, upb_test_ModelExtension2_i(ext2));
  EXPECT_EQ(4, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  // Test unknown GetExtension.
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(base_msg), upb_test_ModelExtension2_model_ext_4_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(8, upb_test_ModelExtension2_i(ext2));
  EXPECT_EQ(5, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  // Test unknown GetExtension.
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(base_msg), upb_test_ModelExtension2_model_ext_5_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(9, upb_test_ModelExtension2_i(ext2));
  EXPECT_EQ(6, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  size_t end_len = GetUnknownLength(UPB_UPCAST(base_msg));
  EXPECT_LT(end_len, start_len);
  EXPECT_EQ(6, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  upb_Arena_Free(arena);
}

// Create a minitable to mimic ModelWithSubMessages with unlinked subs
// to lazily promote unknowns after parsing.
upb_MiniTable* CreateMiniTableWithEmptySubTablesOld(upb_Arena* arena) {
  upb::MtDataEncoder e;
  e.StartMessage(0);
  e.PutField(kUpb_FieldType_Int32, 4, 0);
  e.PutField(kUpb_FieldType_Message, 5, 0);
  e.PutField(kUpb_FieldType_Message, 6, kUpb_FieldModifier_IsRepeated);

  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTable* table =
      upb_MiniTable_Build(e.data().data(), e.data().size(), arena, &status);
  EXPECT_EQ(status.ok, true);
  return table;
}

// Create a minitable to mimic ModelWithMaps with unlinked subs
// to lazily promote unknowns after parsing.
upb_MiniTable* CreateMiniTableWithEmptySubTablesForMapsOld(upb_Arena* arena) {
  upb::MtDataEncoder e;
  e.StartMessage(0);
  e.PutField(kUpb_FieldType_Int32, 1, 0);
  e.PutField(kUpb_FieldType_Message, 3, kUpb_FieldModifier_IsRepeated);
  e.PutField(kUpb_FieldType_Message, 4, kUpb_FieldModifier_IsRepeated);

  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTable* table =
      upb_MiniTable_Build(e.data().data(), e.data().size(), arena, &status);
  EXPECT_EQ(status.ok, true);
  return table;
}

upb_MiniTable* CreateMapEntryMiniTableOld(upb_Arena* arena) {
  upb::MtDataEncoder e;
  e.EncodeMap(kUpb_FieldType_String, kUpb_FieldType_String, 0, 0);
  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTable* table =
      upb_MiniTable_Build(e.data().data(), e.data().size(), arena, &status);
  EXPECT_EQ(status.ok, true);
  return table;
}

TEST(GeneratedCode, PromoteUnknownMessageOld) {
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithSubMessages* input_msg =
      upb_test_ModelWithSubMessages_new(arena);
  upb_test_ModelWithExtensions* sub_message =
      upb_test_ModelWithExtensions_new(arena);
  upb_test_ModelWithSubMessages_set_id(input_msg, 11);
  upb_test_ModelWithExtensions_set_random_int32(sub_message, 12);
  upb_test_ModelWithSubMessages_set_optional_child(input_msg, sub_message);
  size_t serialized_size;
  char* serialized = upb_test_ModelWithSubMessages_serialize(input_msg, arena,
                                                             &serialized_size);

  upb_MiniTable* mini_table = CreateMiniTableWithEmptySubTablesOld(arena);
  upb_Message* msg = _upb_Message_New(mini_table, arena);
  upb_DecodeStatus decode_status = upb_Decode(serialized, serialized_size, msg,
                                              mini_table, nullptr, 0, arena);
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);
  int32_t val = upb_Message_GetInt32(
      msg, upb_MiniTable_FindFieldByNumber(mini_table, 4), 0);
  EXPECT_EQ(val, 11);
  upb_FindUnknownRet unknown = upb_Message_FindUnknown(msg, 5, 0);
  EXPECT_EQ(unknown.status, kUpb_FindUnknown_Ok);
  // Update mini table and promote unknown to a message.
  EXPECT_TRUE(upb_MiniTable_SetSubMessage(
      mini_table,
      (upb_MiniTableField*)upb_MiniTable_GetFieldByIndex(mini_table, 1),
      &upb_0test__ModelWithExtensions_msg_init));
  const int decode_options =
      upb_DecodeOptions_MaxDepth(0);  // UPB_DECODE_ALIAS disabled.
  upb_UnknownToMessageRet promote_result =
      upb_MiniTable_PromoteUnknownToMessage(
          msg, mini_table, upb_MiniTable_GetFieldByIndex(mini_table, 1),
          &upb_0test__ModelWithExtensions_msg_init, decode_options, arena);
  EXPECT_EQ(promote_result.status, kUpb_UnknownToMessage_Ok);
  const upb_Message* promoted_message =
      upb_Message_GetMessage(msg, upb_MiniTable_GetFieldByIndex(mini_table, 1));
  EXPECT_EQ(upb_test_ModelWithExtensions_random_int32(
                (upb_test_ModelWithExtensions*)promoted_message),
            12);
  upb_Arena_Free(arena);
}

// A message-typed field carried as an unknown field with a non-delimited wire
// type must be rejected during promotion. Otherwise its varint value is treated
// as a message length and handed to upb_Decode, reading past the captured field.
TEST(GeneratedCode, PromoteUnknownMessageWrongWireType) {
  upb_Arena* arena = upb_Arena_New();
  upb::MtDataEncoder e;
  e.StartMessage(0);
  e.PutField(kUpb_FieldType_Message, 5, 0);
  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTable* mini_table =
      upb_MiniTable_Build(e.data().data(), e.data().size(), arena, &status);
  ASSERT_EQ(status.ok, true);

  // Field 5 as a varint (wire type 0) holding ~4 GiB. The wire type does not
  // match the message field, so it lands in the unknown fields.
  const char unknown[] = {static_cast<char>((5 << 3) | 0),
                          '\xff', '\xff', '\xff', '\xff', '\x0f'};
  upb_Message* msg = _upb_Message_New(mini_table, arena);
  upb_DecodeStatus decode_status =
      upb_Decode(unknown, sizeof(unknown), msg, mini_table, nullptr, 0, arena);
  ASSERT_EQ(decode_status, kUpb_DecodeStatus_Ok);
  ASSERT_EQ(upb_Message_FindUnknown(msg, 5, 0).status, kUpb_FindUnknown_Ok);

  upb_MiniTable_SetSubMessage(
      mini_table,
      (upb_MiniTableField*)upb_MiniTable_GetFieldByIndex(mini_table, 0),
      &upb_0test__ModelWithExtensions_msg_init);
  upb_UnknownToMessageRet promote_result =
      upb_MiniTable_PromoteUnknownToMessage(
          msg, mini_table, upb_MiniTable_GetFieldByIndex(mini_table, 0),
          &upb_0test__ModelWithExtensions_msg_init,
          upb_DecodeOptions_MaxDepth(0), arena);
  EXPECT_EQ(promote_result.status, kUpb_UnknownToMessage_ParseError);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, PromoteUnknownRepeatedMessageOld) {
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithSubMessages* input_msg =
      upb_test_ModelWithSubMessages_new(arena);
  upb_test_ModelWithSubMessages_set_id(input_msg, 123);

  // Add 2 repeated messages to input_msg.
  upb_test_ModelWithExtensions* item =
      upb_test_ModelWithSubMessages_add_items(input_msg, arena);
  upb_test_ModelWithExtensions_set_random_int32(item, 5);
  item = upb_test_ModelWithSubMessages_add_items(input_msg, arena);
  upb_test_ModelWithExtensions_set_random_int32(item, 6);

  size_t serialized_size;
  char* serialized = upb_test_ModelWithSubMessages_serialize(input_msg, arena,
                                                             &serialized_size);

  upb_MiniTable* mini_table = CreateMiniTableWithEmptySubTablesOld(arena);
  upb_Message* msg = _upb_Message_New(mini_table, arena);
  upb_DecodeStatus decode_status = upb_Decode(serialized, serialized_size, msg,
                                              mini_table, nullptr, 0, arena);
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);
  int32_t val = upb_Message_GetInt32(
      msg, upb_MiniTable_FindFieldByNumber(mini_table, 4), 0);
  EXPECT_EQ(val, 123);

  // Check that we have repeated field data in an unknown.
  upb_FindUnknownRet unknown = upb_Message_FindUnknown(msg, 6, 0);
  EXPECT_EQ(unknown.status, kUpb_FindUnknown_Ok);

  // Update mini table and promote unknown to a message.
  EXPECT_TRUE(upb_MiniTable_SetSubMessage(
      mini_table,
      (upb_MiniTableField*)upb_MiniTable_GetFieldByIndex(mini_table, 2),
      &upb_0test__ModelWithExtensions_msg_init));
  const int decode_options =
      upb_DecodeOptions_MaxDepth(0);  // UPB_DECODE_ALIAS disabled.
  upb_UnknownToMessage_Status promote_result =
      upb_MiniTable_PromoteUnknownToMessageArray(
          msg, upb_MiniTable_GetFieldByIndex(mini_table, 2),
          &upb_0test__ModelWithExtensions_msg_init, decode_options, arena);
  EXPECT_EQ(promote_result, kUpb_UnknownToMessage_Ok);

  upb_Array* array = upb_Message_GetMutableArray(
      msg, upb_MiniTable_GetFieldByIndex(mini_table, 2));
  const upb_Message* promoted_message = upb_Array_Get(array, 0).msg_val;
  EXPECT_EQ(upb_test_ModelWithExtensions_random_int32(
                (upb_test_ModelWithExtensions*)promoted_message),
            5);
  promoted_message = upb_Array_Get(array, 1).msg_val;
  EXPECT_EQ(upb_test_ModelWithExtensions_random_int32(
                (upb_test_ModelWithExtensions*)promoted_message),
            6);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, PromoteUnknownToMapOld) {
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithMaps* input_msg = upb_test_ModelWithMaps_new(arena);
  upb_test_ModelWithMaps_set_id(input_msg, 123);

  // Add 2 map entries.
  upb_test_ModelWithMaps_map_ss_set(input_msg,
                                    upb_StringView_FromString("key1"),
                                    upb_StringView_FromString("value1"), arena);
  upb_test_ModelWithMaps_map_ss_set(input_msg,
                                    upb_StringView_FromString("key2"),
                                    upb_StringView_FromString("value2"), arena);

  size_t serialized_size;
  char* serialized =
      upb_test_ModelWithMaps_serialize(input_msg, arena, &serialized_size);

  upb_MiniTable* mini_table =
      CreateMiniTableWithEmptySubTablesForMapsOld(arena);
  upb_MiniTable* map_entry_mini_table = CreateMapEntryMiniTableOld(arena);
  upb_Message* msg = _upb_Message_New(mini_table, arena);
  const int decode_options = upb_DecodeOptions_MaxDepth(0);
  upb_DecodeStatus decode_status =
      upb_Decode(serialized, serialized_size, msg, mini_table, nullptr,
                 decode_options, arena);
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);
  int32_t val = upb_Message_GetInt32(
      msg, upb_MiniTable_FindFieldByNumber(mini_table, 1), 0);
  EXPECT_EQ(val, 123);

  // Check that we have map data in an unknown.
  upb_FindUnknownRet unknown = upb_Message_FindUnknown(msg, 3, 0);
  EXPECT_EQ(unknown.status, kUpb_FindUnknown_Ok);

  // Update mini table and promote unknown to a message.
  EXPECT_TRUE(upb_MiniTable_SetSubMessage(
      mini_table,
      (upb_MiniTableField*)upb_MiniTable_GetFieldByIndex(mini_table, 1),
      map_entry_mini_table));
  upb_UnknownToMessage_Status promote_result =
      upb_MiniTable_PromoteUnknownToMap(
          msg, mini_table, upb_MiniTable_GetFieldByIndex(mini_table, 1),
          decode_options, arena);
  EXPECT_EQ(promote_result, kUpb_UnknownToMessage_Ok);

  upb_Map* map = upb_Message_GetOrCreateMutableMap(
      msg, map_entry_mini_table, upb_MiniTable_GetFieldByIndex(mini_table, 1),
      arena);
  EXPECT_NE(map, nullptr);
  // Lookup in map.
  upb_MessageValue key;
  key.str_val = upb_StringView_FromString("key2");
  upb_MessageValue value;
  EXPECT_TRUE(upb_Map_Get(map, key, &value));
  EXPECT_EQ(0, strncmp(value.str_val.data, "value2", 5));
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, PromoteNonCanonicalExtension) {
  upb::Arena arena;

  // 1. Build custom different mini-table
  upb::MtDataEncoder e;
  e.StartMessage(0);
  e.PutField(kUpb_FieldType_String, 25, 0);

  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTable* custom_sub_table = upb_MiniTable_Build(
      e.data().data(), e.data().size(), arena.ptr(), &status);
  ASSERT_TRUE(status.ok);

  upb_MiniTableExtension custom_ext = *upb_test_ModelExtension1_model_ext_ext;
  upb_MiniTableExtension_SetSubMessage(&custom_ext, custom_sub_table);

  // 2. Create base message msg to hold our non-canonical extension
  upb_test_ModelWithExtensions* msg =
      upb_test_ModelWithExtensions_new(arena.ptr());

  // 3. Create submsg parsed under custom_sub_table ("World")
  upb_Message* extension1 = _upb_Message_New(custom_sub_table, arena.ptr());
  upb_MessageValue val_str;
  val_str.str_val = upb_StringView_FromString("World");
  const upb_MiniTableField* custom_f =
      upb_MiniTable_GetFieldByIndex(custom_sub_table, 0);
  upb_Message_SetString(extension1, custom_f, val_str.str_val, arena.ptr());

  // 4. Attach custom parsed submessage "World" to msg as a non-canonical
  // extension under the different custom mini-table layout.
  UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg), &custom_ext, &extension1, arena.ptr());

  // 5. Promote the extension using standard compiled mini-table ModelExtension1
  upb_MessageValue val;
  upb_GetExtension_Status promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), upb_test_ModelExtension1_model_ext_ext,
      kUpb_DecodeOption_AliasString, arena.ptr(), &val);

  ASSERT_EQ(kUpb_GetExtension_Ok, promote_status);

  // 6. Verify that the engine correctly converted the shape and promoted the
  // value!
  upb_test_ModelExtension1* ext_msg = (upb_test_ModelExtension1*)val.msg_val;
  upb_StringView field = upb_test_ModelExtension1_str(ext_msg);
  EXPECT_EQ(absl::string_view(field.data, field.size), "World");

  EXPECT_EQ(1, upb_Message_ExtensionCount(UPB_UPCAST(msg)));

  // 7. Verify that upb_Message_NextExtension works and iterates over the
  // promoted extension!
  uintptr_t ext_iter = kUpb_Message_ExtensionBegin;
  const upb_MiniTableExtension* ext_out = nullptr;
  upb_MessageValue val_out;
  EXPECT_TRUE(upb_Message_NextExtension(UPB_UPCAST(msg), &ext_out, &val_out,
                                        &ext_iter));
  EXPECT_EQ(
      upb_MiniTableExtension_Number(ext_out),
      upb_MiniTableExtension_Number(upb_test_ModelExtension1_model_ext_ext));

  upb_test_ModelExtension1* ext_msg_iter =
      (upb_test_ModelExtension1*)val_out.msg_val;
  upb_StringView field_iter = upb_test_ModelExtension1_str(ext_msg_iter);
  EXPECT_EQ(absl::string_view(field_iter.data, field_iter.size), "World");

  EXPECT_FALSE(upb_Message_NextExtension(UPB_UPCAST(msg), &ext_out, &val_out,
                                         &ext_iter));

  // 8. Verify that the promoted non-canonical extension is indeed no longer
  // present in unknowns
  upb_FindUnknownRet found = upb_Message_FindUnknown(UPB_UPCAST(msg), 1547, 0);
  EXPECT_EQ(kUpb_FindUnknown_NotPresent, found.status);
}

TEST(GeneratedCode, PromoteNonCanonicalExtensionWithSameMinitable) {
  upb::Arena arena;
  upb_test_ModelWithExtensions* msg =
      upb_test_ModelWithExtensions_new(arena.ptr());
  upb_test_ModelExtension1* extension1 =
      upb_test_ModelExtension1_new(arena.ptr());
  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("World"));

  UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg), upb_test_ModelExtension1_model_ext_ext,
      (upb_Message**)&extension1, arena.ptr());

  upb_MessageValue val;
  upb_GetExtension_Status promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), upb_test_ModelExtension1_model_ext_ext,
      kUpb_DecodeOption_AliasString, arena.ptr(), &val);

  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  upb_test_ModelExtension1* ext_msg = (upb_test_ModelExtension1*)val.msg_val;
  upb_StringView field = upb_test_ModelExtension1_str(ext_msg);
  EXPECT_EQ(absl::string_view(field.data, field.size), "World");
  EXPECT_EQ(1, upb_Message_ExtensionCount(UPB_UPCAST(msg)));
  uintptr_t ext_iter = kUpb_Message_ExtensionBegin;
  const upb_MiniTableExtension* ext_out = nullptr;
  upb_MessageValue val_out;
  EXPECT_TRUE(upb_Message_NextExtension(UPB_UPCAST(msg), &ext_out, &val_out,
                                        &ext_iter));
  EXPECT_FALSE(upb_Message_NextExtension(UPB_UPCAST(msg), &ext_out, &val_out,
                                         &ext_iter));
  upb_FindUnknownRet found = upb_Message_FindUnknown(UPB_UPCAST(msg), 1547, 0);
  EXPECT_EQ(kUpb_FindUnknown_NotPresent, found.status);
}

TEST(GeneratedCode, PromoteNonCanonicalExtensionWithDifferentMinitable) {
  upb::Arena arena;

  // 1. Build custom different mini-table for the non-canonical extension ("ext"
  // layout) It has an int32 field at tag 1.
  upb_Status status;
  upb_Status_Clear(&status);
  upb::MtDataEncoder e_ext;
  e_ext.StartMessage(0);
  e_ext.PutField(kUpb_FieldType_Int32, 1, 0);

  upb_MiniTable* custom_sub_table_ext = upb_MiniTable_Build(
      e_ext.data().data(), e_ext.data().size(), arena.ptr(), &status);
  ASSERT_TRUE(status.ok);

  // 2. Build target mini-table for the base field ("base" layout, matching
  // field tag 1 int32)
  upb::MtDataEncoder e_base;
  e_base.StartMessage(0);
  e_base.PutField(kUpb_FieldType_Int32, 1, 0);

  upb_MiniTable* custom_sub_table_base = upb_MiniTable_Build(
      e_base.data().data(), e_base.data().size(), arena.ptr(), &status);
  ASSERT_TRUE(status.ok);

  // 3. Create target extension descriptor pointing to custom_sub_table_base
  upb_MiniTableExtension target_ext = *upb_test_ModelExtension1_model_ext_ext;
  upb_MiniTableExtension_SetSubMessage(&target_ext, custom_sub_table_base);

  // 4. Create a custom extension descriptor matching field number 1547 and
  // pointing to custom_sub_table_ext
  upb_MiniTableExtension custom_ext = *upb_test_ModelExtension1_model_ext_ext;
  upb_MiniTableExtension_SetSubMessage(&custom_ext, custom_sub_table_ext);

  // 5. Create base msg
  upb_test_ModelWithExtensions* msg =
      upb_test_ModelWithExtensions_new(arena.ptr());

  // 6. Populate the submsg parsed under custom_sub_table_ext with value 42 at
  // tag 1
  upb_Message* extension1 = _upb_Message_New(custom_sub_table_ext, arena.ptr());
  const upb_MiniTableField* custom_f =
      upb_MiniTable_GetFieldByIndex(custom_sub_table_ext, 0);
  upb_Message_SetInt32(extension1, custom_f, 42, arena.ptr());

  // 7. Attach it as a non-canonical extension to msg using field 1547
  UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg), &custom_ext, &extension1, arena.ptr());

  // 8. Run extension promotion using targeting target_ext layout
  upb_MessageValue val;
  upb_GetExtension_Status promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), &target_ext, kUpb_DecodeOption_AliasString, arena.ptr(),
      &val);

  EXPECT_EQ(promote_status, kUpb_GetExtension_Ok);

  // 9. Retrieve and verify that it successfully converted and promoted the
  // actual value
  upb_Message* promoted_message = (upb_Message*)val.msg_val;
  ASSERT_NE(promoted_message, nullptr);

  const upb_MiniTableField* base_f =
      upb_MiniTable_GetFieldByIndex(custom_sub_table_base, 0);
  int32_t promoted_value = upb_Message_GetInt32(promoted_message, base_f, 0);
  EXPECT_EQ(promoted_value, 42);

  EXPECT_EQ(1, upb_Message_ExtensionCount(UPB_UPCAST(msg)));

  // 10. Verify that upb_Message_NextExtension successfully works and returns
  // true!
  uintptr_t ext_iter = kUpb_Message_ExtensionBegin;
  const upb_MiniTableExtension* ext_out = nullptr;
  upb_MessageValue val_out;
  EXPECT_TRUE(upb_Message_NextExtension(UPB_UPCAST(msg), &ext_out, &val_out,
                                        &ext_iter));
  EXPECT_EQ(upb_MiniTableExtension_Number(ext_out), 1547);

  upb_Message* ext_msg_iter = (upb_Message*)val_out.msg_val;
  int32_t promoted_value_iter = upb_Message_GetInt32(ext_msg_iter, base_f, 0);
  EXPECT_EQ(promoted_value_iter, 42);

  EXPECT_FALSE(upb_Message_NextExtension(UPB_UPCAST(msg), &ext_out, &val_out,
                                         &ext_iter));

  // 11. Verify that the promoted non-canonical extension is indeed no longer
  // present in unknowns
  upb_FindUnknownRet found = upb_Message_FindUnknown(UPB_UPCAST(msg), 1547, 0);
  EXPECT_EQ(kUpb_FindUnknown_NotPresent, found.status);
}
}  // namespace
