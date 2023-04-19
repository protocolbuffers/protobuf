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

/* Test of mini table accessors.
 *
 * Messages are created and mutated using generated code, and then
 * accessed through reflective APIs exposed through mini table accessors.
 */

#include "upb/message/promote.h"

#include <string>

#include "gtest/gtest.h"
#include "google/protobuf/test_messages_proto2.upb.h"
#include "google/protobuf/test_messages_proto3.upb.h"
#include "upb/base/string_view.h"
#include "upb/collections/array.h"
#include "upb/message/accessors.h"
#include "upb/mini_table/common.h"
#include "upb/mini_table/decode.h"
#include "upb/mini_table/encode_internal.hpp"
#include "upb/mini_table/field_internal.h"
#include "upb/test/test.upb.h"
#include "upb/upb.h"
#include "upb/wire/common.h"
#include "upb/wire/decode.h"

// Must be last
#include "upb/port/def.inc"

namespace {

// Proto2 test messages field numbers used for reflective access.
const uint32_t kFieldOptionalInt32 = 1;
const uint32_t kFieldOptionalUInt32 = 3;
const uint32_t kFieldOptionalBool = 13;
const uint32_t kFieldOptionalString = 14;
const uint32_t kFieldOptionalNestedMessage = 18;
const uint32_t kFieldOptionalRepeatedInt32 = 31;
const uint32_t kFieldOptionalNestedMessageA = 1;
const uint32_t kFieldOptionalOneOfUInt32 = 111;
const uint32_t kFieldOptionalOneOfString = 113;

const uint32_t kFieldProto3OptionalInt64 = 2;
const uint32_t kFieldProto3OptionalUInt64 = 4;

const char kTestStr1[] = "Hello1";
const char kTestStr2[] = "Hello2";
const int32_t kTestInt32 = 567;
const int32_t kTestUInt32 = 0xF1234567;
const uint64_t kTestUInt64 = 0xFEDCBAFF87654321;

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

  upb_FindUnknownRet result = upb_MiniTable_FindUnknown(
      base_msg, upb_test_ModelExtension1_model_ext_ext.field.number,
      kUpb_WireFormat_DefaultDepthLimit);
  EXPECT_EQ(kUpb_FindUnknown_Ok, result.status);

  result = upb_MiniTable_FindUnknown(
      base_msg, upb_test_ModelExtension2_model_ext_ext.field.number,
      kUpb_WireFormat_DefaultDepthLimit);
  EXPECT_EQ(kUpb_FindUnknown_NotPresent, result.status);

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

  const upb_Message_Extension* upb_ext2;
  upb_test_ModelExtension1* ext1;
  upb_test_ModelExtension2* ext2;
  upb_GetExtension_Status promote_status;

  // Test known GetExtension 1
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      msg, &upb_test_ModelExtension1_model_ext_ext, 0, arena, &upb_ext2);
  ext1 = (upb_test_ModelExtension1*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_TRUE(upb_StringView_IsEqual(upb_StringView_FromString("World"),
                                     upb_test_ModelExtension1_str(ext1)));

  // Test known GetExtension 2
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      msg, &upb_test_ModelExtension2_model_ext_ext, 0, arena, &upb_ext2);
  ext2 = (upb_test_ModelExtension2*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(5, upb_test_ModelExtension2_i(ext2));

  // Test known GetExtension 3
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      msg, &upb_test_ModelExtension2_model_ext_2_ext, 0, arena, &upb_ext2);
  ext2 = (upb_test_ModelExtension2*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(6, upb_test_ModelExtension2_i(ext2));

  // Test known GetExtension 4
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      msg, &upb_test_ModelExtension2_model_ext_3_ext, 0, arena, &upb_ext2);
  ext2 = (upb_test_ModelExtension2*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(7, upb_test_ModelExtension2_i(ext2));

  // Test known GetExtension 5
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      msg, &upb_test_ModelExtension2_model_ext_4_ext, 0, arena, &upb_ext2);
  ext2 = (upb_test_ModelExtension2*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(8, upb_test_ModelExtension2_i(ext2));

  // Test known GetExtension 6
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      msg, &upb_test_ModelExtension2_model_ext_5_ext, 0, arena, &upb_ext2);
  ext2 = (upb_test_ModelExtension2*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(9, upb_test_ModelExtension2_i(ext2));

  upb_test_EmptyMessageWithExtensions* base_msg =
      upb_test_EmptyMessageWithExtensions_parse(serialized, serialized_size,
                                                arena);

  // Get unknown extension bytes before promotion.
  size_t start_len;
  upb_Message_GetUnknown(base_msg, &start_len);
  EXPECT_GT(start_len, 0);
  EXPECT_EQ(0, upb_Message_ExtensionCount(base_msg));

  // Test unknown GetExtension.
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      base_msg, &upb_test_ModelExtension1_model_ext_ext, 0, arena, &upb_ext2);
  ext1 = (upb_test_ModelExtension1*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_TRUE(upb_StringView_IsEqual(upb_StringView_FromString("World"),
                                     upb_test_ModelExtension1_str(ext1)));

  // Test unknown GetExtension.
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      base_msg, &upb_test_ModelExtension2_model_ext_ext, 0, arena, &upb_ext2);
  ext2 = (upb_test_ModelExtension2*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(5, upb_test_ModelExtension2_i(ext2));

  // Test unknown GetExtension.
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      base_msg, &upb_test_ModelExtension2_model_ext_2_ext, 0, arena, &upb_ext2);
  ext2 = (upb_test_ModelExtension2*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(6, upb_test_ModelExtension2_i(ext2));

  // Test unknown GetExtension.
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      base_msg, &upb_test_ModelExtension2_model_ext_3_ext, 0, arena, &upb_ext2);
  ext2 = (upb_test_ModelExtension2*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(7, upb_test_ModelExtension2_i(ext2));

  // Test unknown GetExtension.
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      base_msg, &upb_test_ModelExtension2_model_ext_4_ext, 0, arena, &upb_ext2);
  ext2 = (upb_test_ModelExtension2*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(8, upb_test_ModelExtension2_i(ext2));

  // Test unknown GetExtension.
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      base_msg, &upb_test_ModelExtension2_model_ext_5_ext, 0, arena, &upb_ext2);
  ext2 = (upb_test_ModelExtension2*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(9, upb_test_ModelExtension2_i(ext2));

  size_t end_len;
  upb_Message_GetUnknown(base_msg, &end_len);
  EXPECT_LT(end_len, start_len);
  EXPECT_EQ(6, upb_Message_ExtensionCount(base_msg));

  upb_Arena_Free(arena);
}

// Create a minitable to mimic ModelWithSubMessages with unlinked subs
// to lazily promote unknowns after parsing.
upb_MiniTable* CreateMiniTableWithEmptySubTables(upb_Arena* arena) {
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
  // Initialize sub table to null. Not using upb_MiniTable_SetSubMessage
  // since it checks ->ext on parameter.
  upb_MiniTableSub* sub = const_cast<upb_MiniTableSub*>(
      &table->subs[table->fields[1].UPB_PRIVATE(submsg_index)]);
  sub->submsg = nullptr;
  sub = const_cast<upb_MiniTableSub*>(
      &table->subs[table->fields[2].UPB_PRIVATE(submsg_index)]);
  sub->submsg = nullptr;
  return table;
}

// Create a minitable to mimic ModelWithMaps with unlinked subs
// to lazily promote unknowns after parsing.
upb_MiniTable* CreateMiniTableWithEmptySubTablesForMaps(upb_Arena* arena) {
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
  // Initialize sub table to null. Not using upb_MiniTable_SetSubMessage
  // since it checks ->ext on parameter.
  upb_MiniTableSub* sub = const_cast<upb_MiniTableSub*>(
      &table->subs[table->fields[1].UPB_PRIVATE(submsg_index)]);
  sub->submsg = nullptr;
  sub = const_cast<upb_MiniTableSub*>(
      &table->subs[table->fields[2].UPB_PRIVATE(submsg_index)]);
  sub->submsg = nullptr;
  return table;
}

upb_MiniTable* CreateMapEntryMiniTable(upb_Arena* arena) {
  upb::MtDataEncoder e;
  e.EncodeMap(kUpb_FieldType_String, kUpb_FieldType_String, 0, 0);
  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTable* table =
      upb_MiniTable_Build(e.data().data(), e.data().size(), arena, &status);
  EXPECT_EQ(status.ok, true);
  return table;
}

TEST(GeneratedCode, PromoteUnknownMessage) {
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

  upb_MiniTable* mini_table = CreateMiniTableWithEmptySubTables(arena);
  upb_Message* msg = _upb_Message_New(mini_table, arena);
  upb_DecodeStatus decode_status = upb_Decode(serialized, serialized_size, msg,
                                              mini_table, nullptr, 0, arena);
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);
  int32_t val = upb_Message_GetInt32(
      msg, upb_MiniTable_FindFieldByNumber(mini_table, 4), 0);
  EXPECT_EQ(val, 11);
  upb_FindUnknownRet unknown =
      upb_MiniTable_FindUnknown(msg, 5, kUpb_WireFormat_DefaultDepthLimit);
  EXPECT_EQ(unknown.status, kUpb_FindUnknown_Ok);
  // Update mini table and promote unknown to a message.
  EXPECT_TRUE(upb_MiniTable_SetSubMessage(
      mini_table, (upb_MiniTableField*)&mini_table->fields[1],
      &upb_test_ModelWithExtensions_msg_init));
  const int decode_options = upb_DecodeOptions_MaxDepth(
      kUpb_WireFormat_DefaultDepthLimit);  // UPB_DECODE_ALIAS disabled.
  upb_UnknownToMessageRet promote_result =
      upb_MiniTable_PromoteUnknownToMessage(
          msg, mini_table, &mini_table->fields[1],
          &upb_test_ModelWithExtensions_msg_init, decode_options, arena);
  EXPECT_EQ(promote_result.status, kUpb_UnknownToMessage_Ok);
  const upb_Message* promoted_message =
      upb_Message_GetMessage(msg, &mini_table->fields[1], nullptr);
  EXPECT_EQ(upb_test_ModelWithExtensions_random_int32(
                (upb_test_ModelWithExtensions*)promoted_message),
            12);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, PromoteUnknownRepeatedMessage) {
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

  upb_MiniTable* mini_table = CreateMiniTableWithEmptySubTables(arena);
  upb_Message* msg = _upb_Message_New(mini_table, arena);
  upb_DecodeStatus decode_status = upb_Decode(serialized, serialized_size, msg,
                                              mini_table, nullptr, 0, arena);
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);
  int32_t val = upb_Message_GetInt32(
      msg, upb_MiniTable_FindFieldByNumber(mini_table, 4), 0);
  EXPECT_EQ(val, 123);

  // Check that we have repeated field data in an unknown.
  upb_FindUnknownRet unknown =
      upb_MiniTable_FindUnknown(msg, 6, kUpb_WireFormat_DefaultDepthLimit);
  EXPECT_EQ(unknown.status, kUpb_FindUnknown_Ok);

  // Update mini table and promote unknown to a message.
  EXPECT_TRUE(upb_MiniTable_SetSubMessage(
      mini_table, (upb_MiniTableField*)&mini_table->fields[2],
      &upb_test_ModelWithExtensions_msg_init));
  const int decode_options = upb_DecodeOptions_MaxDepth(
      kUpb_WireFormat_DefaultDepthLimit);  // UPB_DECODE_ALIAS disabled.
  upb_UnknownToMessage_Status promote_result =
      upb_MiniTable_PromoteUnknownToMessageArray(
          msg, &mini_table->fields[2], &upb_test_ModelWithExtensions_msg_init,
          decode_options, arena);
  EXPECT_EQ(promote_result, kUpb_UnknownToMessage_Ok);

  upb_Array* array = upb_Message_GetMutableArray(msg, &mini_table->fields[2]);
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

TEST(GeneratedCode, PromoteUnknownToMap) {
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

  upb_MiniTable* mini_table = CreateMiniTableWithEmptySubTablesForMaps(arena);
  upb_MiniTable* map_entry_mini_table = CreateMapEntryMiniTable(arena);
  upb_Message* msg = _upb_Message_New(mini_table, arena);
  const int decode_options =
      upb_DecodeOptions_MaxDepth(kUpb_WireFormat_DefaultDepthLimit);
  upb_DecodeStatus decode_status =
      upb_Decode(serialized, serialized_size, msg, mini_table, nullptr,
                 decode_options, arena);
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);
  int32_t val = upb_Message_GetInt32(
      msg, upb_MiniTable_FindFieldByNumber(mini_table, 1), 0);
  EXPECT_EQ(val, 123);

  // Check that we have map data in an unknown.
  upb_FindUnknownRet unknown =
      upb_MiniTable_FindUnknown(msg, 3, kUpb_WireFormat_DefaultDepthLimit);
  EXPECT_EQ(unknown.status, kUpb_FindUnknown_Ok);

  // Update mini table and promote unknown to a message.
  EXPECT_TRUE(upb_MiniTable_SetSubMessage(
      mini_table, (upb_MiniTableField*)&mini_table->fields[1],
      map_entry_mini_table));
  upb_UnknownToMessage_Status promote_result =
      upb_MiniTable_PromoteUnknownToMap(msg, mini_table, &mini_table->fields[1],
                                        decode_options, arena);
  EXPECT_EQ(promote_result, kUpb_UnknownToMessage_Ok);

  upb_Map* map = upb_Message_GetOrCreateMutableMap(
      msg, map_entry_mini_table, &mini_table->fields[1], arena);
  EXPECT_NE(map, nullptr);
  // Lookup in map.
  upb_MessageValue key;
  key.str_val = upb_StringView_FromString("key2");
  upb_MessageValue value;
  EXPECT_TRUE(upb_Map_Get(map, key, &value));
  EXPECT_EQ(0, strncmp(value.str_val.data, "value2", 5));
  upb_Arena_Free(arena);
}

}  // namespace
