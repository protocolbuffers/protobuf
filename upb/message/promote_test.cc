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
#include "upb/message/copy.h"
#include "upb/message/internal/message.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/tagged_ptr.h"
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
#include "upb/wire/encode.h"

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
      upb_MiniTableExtension_Number(&upb_test_ModelExtension1_model_ext_ext),
      0);
  EXPECT_EQ(kUpb_FindUnknown_Ok, result.status);

  result = upb_Message_FindUnknown(
      UPB_UPCAST(base_msg),
      upb_MiniTableExtension_Number(&upb_test_ModelExtension2_model_ext_ext),
      0);
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
      UPB_UPCAST(parsed), &upb_test_ModelExtension1_model_ext_ext, options,
      arena, &value);
  ASSERT_EQ(result, kUpb_GetExtension_Ok);
  upb_test_ModelExtension1* parsed_ex =
      (upb_test_ModelExtension1*)value.msg_val;
  upb_StringView field = upb_test_ModelExtension1_str(parsed_ex);
  EXPECT_EQ(absl::string_view(field.data, field.size), "Everyone");

  upb_FindUnknownRet found = upb_Message_FindUnknown(
      UPB_UPCAST(parsed),
      upb_MiniTableExtension_Number(&upb_test_ModelExtension1_model_ext_ext),
      0);
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
      UPB_UPCAST(msg), &upb_test_ModelExtension1_model_ext_ext, 0, arena,
      &value);
  ext1 = (upb_test_ModelExtension1*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_TRUE(upb_StringView_IsEqual(upb_StringView_FromString("World"),
                                     upb_test_ModelExtension1_str(ext1)));

  // Test known GetExtension 2
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), &upb_test_ModelExtension2_model_ext_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(5, upb_test_ModelExtension2_i(ext2));

  // Test known GetExtension 3
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), &upb_test_ModelExtension2_model_ext_2_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(6, upb_test_ModelExtension2_i(ext2));

  // Test known GetExtension 4
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), &upb_test_ModelExtension2_model_ext_3_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(7, upb_test_ModelExtension2_i(ext2));

  // Test known GetExtension 5
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), &upb_test_ModelExtension2_model_ext_4_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(8, upb_test_ModelExtension2_i(ext2));

  // Test known GetExtension 6
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(msg), &upb_test_ModelExtension2_model_ext_5_ext, 0, arena,
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
      UPB_UPCAST(base_msg), &upb_test_ModelExtension1_model_ext_ext, 0, arena,
      &value);
  ext1 = (upb_test_ModelExtension1*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_TRUE(upb_StringView_IsEqual(upb_StringView_FromString("World"),
                                     upb_test_ModelExtension1_str(ext1)));
  EXPECT_EQ(1, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  // Test unknown GetExtension.
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(base_msg), &upb_test_ModelExtension2_model_ext_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(5, upb_test_ModelExtension2_i(ext2));
  EXPECT_EQ(2, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  // Test unknown GetExtension.
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(base_msg), &upb_test_ModelExtension2_model_ext_2_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(6, upb_test_ModelExtension2_i(ext2));
  EXPECT_EQ(3, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  // Test unknown GetExtension.
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(base_msg), &upb_test_ModelExtension2_model_ext_3_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(7, upb_test_ModelExtension2_i(ext2));
  EXPECT_EQ(4, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  // Test unknown GetExtension.
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(base_msg), &upb_test_ModelExtension2_model_ext_4_ext, 0, arena,
      &value);
  ext2 = (upb_test_ModelExtension2*)value.msg_val;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(8, upb_test_ModelExtension2_i(ext2));
  EXPECT_EQ(5, upb_Message_ExtensionCount(UPB_UPCAST(base_msg)));

  // Test unknown GetExtension.
  promote_status = upb_Message_GetOrPromoteExtension(
      UPB_UPCAST(base_msg), &upb_test_ModelExtension2_model_ext_5_ext, 0, arena,
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
  return table;
}

upb_MiniTable* CreateMapEntryMiniTable(upb_Arena* arena) {
  upb::MtDataEncoder e;
  e.EncodeMap(kUpb_FieldType_Int32, kUpb_FieldType_Message, 0, 0);
  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTable* table =
      upb_MiniTable_Build(e.data().data(), e.data().size(), arena, &status);
  EXPECT_EQ(status.ok, true);
  return table;
}

// Create a minitable to mimic ModelWithMaps with unlinked subs
// to lazily promote unknowns after parsing.
upb_MiniTable* CreateMiniTableWithEmptySubTablesForMaps(upb_Arena* arena) {
  upb::MtDataEncoder e;
  e.StartMessage(0);
  e.PutField(kUpb_FieldType_Int32, 1, 0);
  e.PutField(kUpb_FieldType_Message, 3, kUpb_FieldModifier_IsRepeated);
  e.PutField(kUpb_FieldType_Message, 5, kUpb_FieldModifier_IsRepeated);

  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTable* table =
      upb_MiniTable_Build(e.data().data(), e.data().size(), arena, &status);

  // Field 5 corresponds to ModelWithMaps.map_sm.
  upb_MiniTableField* map_field = const_cast<upb_MiniTableField*>(
      upb_MiniTable_FindFieldByNumber(table, 5));
  EXPECT_NE(map_field, nullptr);
  upb_MiniTable* sub_table = CreateMapEntryMiniTable(arena);
  upb_MiniTable_SetSubMessage(table, map_field, sub_table);
  EXPECT_EQ(status.ok, true);
  return table;
}

void CheckReserialize(const upb_Message* msg, const upb_MiniTable* mini_table,
                      upb_Arena* arena, char* serialized,
                      size_t serialized_size) {
  // We can safely encode the "empty" message.  We expect to get the same bytes
  // out as were parsed.
  size_t reserialized_size;
  char* reserialized;
  upb_EncodeStatus encode_status =
      upb_Encode(msg, mini_table, kUpb_EncodeOption_Deterministic, arena,
                 &reserialized, &reserialized_size);
  EXPECT_EQ(encode_status, kUpb_EncodeStatus_Ok);
  EXPECT_EQ(reserialized_size, serialized_size);
  EXPECT_EQ(0, memcmp(reserialized, serialized, serialized_size));

  // We should get the same result if we copy+reserialize.
  upb_Message* clone = upb_Message_DeepClone(msg, mini_table, arena);
  encode_status = upb_Encode(clone, mini_table, kUpb_EncodeOption_Deterministic,
                             arena, &reserialized, &reserialized_size);
  EXPECT_EQ(encode_status, kUpb_EncodeStatus_Ok);
  EXPECT_EQ(reserialized_size, serialized_size);
  EXPECT_EQ(0, memcmp(reserialized, serialized, serialized_size));
}

TEST(GeneratedCode, PromoteUnknownMessage) {
  upb::Arena arena;
  upb_test_ModelWithSubMessages* input_msg =
      upb_test_ModelWithSubMessages_new(arena.ptr());
  upb_test_ModelWithExtensions* sub_message =
      upb_test_ModelWithExtensions_new(arena.ptr());
  upb_test_ModelWithSubMessages_set_id(input_msg, 11);
  upb_test_ModelWithExtensions_set_random_int32(sub_message, 12);
  upb_test_ModelWithSubMessages_set_optional_child(input_msg, sub_message);
  size_t serialized_size;
  char* serialized = upb_test_ModelWithSubMessages_serialize(
      input_msg, arena.ptr(), &serialized_size);

  upb_MiniTable* mini_table = CreateMiniTableWithEmptySubTables(arena.ptr());
  upb_DecodeStatus decode_status;

  // If we parse without allowing unlinked objects, the parse will fail.
  // TODO: re-enable this test once the old method of tree shaking is
  // removed
  // upb_Message* fail_msg = _upb_Message_New(mini_table, arena.ptr());
  // decode_status =
  //     upb_Decode(serialized, serialized_size, fail_msg, mini_table, nullptr,
  //     0,
  //                arena.ptr());
  // EXPECT_EQ(decode_status, kUpb_DecodeStatus_UnlinkedSubMessage);

  // if we parse while allowing unlinked objects, the parse will succeed.
  upb_Message* msg = _upb_Message_New(mini_table, arena.ptr());
  decode_status =
      upb_Decode(serialized, serialized_size, msg, mini_table, nullptr,
                 kUpb_DecodeOption_ExperimentalAllowUnlinked, arena.ptr());
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);

  CheckReserialize(msg, mini_table, arena.ptr(), serialized, serialized_size);

  // We can encode the "empty" message and get the same output bytes.
  size_t reserialized_size;
  char* reserialized;
  upb_EncodeStatus encode_status = upb_Encode(
      msg, mini_table, 0, arena.ptr(), &reserialized, &reserialized_size);
  EXPECT_EQ(encode_status, kUpb_EncodeStatus_Ok);
  EXPECT_EQ(reserialized_size, serialized_size);
  EXPECT_EQ(0, memcmp(reserialized, serialized, serialized_size));

  // Int32 field is present, as normal.
  int32_t val = upb_Message_GetInt32(
      msg, upb_MiniTable_FindFieldByNumber(mini_table, 4), 0);
  EXPECT_EQ(val, 11);

  // Unlinked sub-message is present, but getting the value returns NULL.
  const upb_MiniTableField* submsg_field =
      upb_MiniTable_FindFieldByNumber(mini_table, 5);
  ASSERT_TRUE(submsg_field != nullptr);
  EXPECT_TRUE(upb_Message_HasBaseField(msg, submsg_field));
  upb_TaggedMessagePtr tagged =
      upb_Message_GetTaggedMessagePtr(msg, submsg_field, nullptr);
  EXPECT_TRUE(upb_TaggedMessagePtr_IsEmpty(tagged));

  // Update mini table and promote unknown to a message.
  EXPECT_TRUE(
      upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)submsg_field,
                                  &upb_0test__ModelWithExtensions_msg_init));

  const int decode_options =
      upb_DecodeOptions_MaxDepth(0);  // UPB_DECODE_ALIAS disabled.
  upb_test_ModelWithExtensions* promoted;
  upb_DecodeStatus promote_result =
      upb_Message_PromoteMessage(msg, mini_table, submsg_field, decode_options,
                                 arena.ptr(), (upb_Message**)&promoted);
  EXPECT_EQ(promote_result, kUpb_DecodeStatus_Ok);
  EXPECT_NE(nullptr, promoted);
  EXPECT_EQ(UPB_UPCAST(promoted), upb_Message_GetMessage(msg, submsg_field));
  EXPECT_EQ(upb_test_ModelWithExtensions_random_int32(promoted), 12);
}

// Tests a second parse that reuses an empty/unlinked message while the message
// is still unlinked.
TEST(GeneratedCode, ReparseUnlinked) {
  upb::Arena arena;
  upb_test_ModelWithSubMessages* input_msg =
      upb_test_ModelWithSubMessages_new(arena.ptr());
  upb_test_ModelWithExtensions* sub_message =
      upb_test_ModelWithExtensions_new(arena.ptr());
  upb_test_ModelWithSubMessages_set_id(input_msg, 11);
  upb_test_ModelWithExtensions_add_repeated_int32(sub_message, 12, arena.ptr());
  upb_test_ModelWithSubMessages_set_optional_child(input_msg, sub_message);
  size_t serialized_size;
  char* serialized = upb_test_ModelWithSubMessages_serialize(
      input_msg, arena.ptr(), &serialized_size);

  upb_MiniTable* mini_table = CreateMiniTableWithEmptySubTables(arena.ptr());

  // Parse twice without linking the MiniTable.
  upb_Message* msg = _upb_Message_New(mini_table, arena.ptr());
  upb_DecodeStatus decode_status =
      upb_Decode(serialized, serialized_size, msg, mini_table, nullptr,
                 kUpb_DecodeOption_ExperimentalAllowUnlinked, arena.ptr());
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);

  decode_status =
      upb_Decode(serialized, serialized_size, msg, mini_table, nullptr,
                 kUpb_DecodeOption_ExperimentalAllowUnlinked, arena.ptr());
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);

  // Update mini table and promote unknown to a message.
  const upb_MiniTableField* submsg_field =
      upb_MiniTable_FindFieldByNumber(mini_table, 5);
  EXPECT_TRUE(
      upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)submsg_field,
                                  &upb_0test__ModelWithExtensions_msg_init));

  const int decode_options =
      upb_DecodeOptions_MaxDepth(0);  // UPB_DECODE_ALIAS disabled.
  upb_test_ModelWithExtensions* promoted;
  upb_DecodeStatus promote_result =
      upb_Message_PromoteMessage(msg, mini_table, submsg_field, decode_options,
                                 arena.ptr(), (upb_Message**)&promoted);
  EXPECT_EQ(promote_result, kUpb_DecodeStatus_Ok);
  EXPECT_NE(nullptr, promoted);
  EXPECT_EQ(UPB_UPCAST(promoted), upb_Message_GetMessage(msg, submsg_field));

  // The repeated field should have two entries for the two parses.
  size_t repeated_size;
  const int32_t* entries =
      upb_test_ModelWithExtensions_repeated_int32(promoted, &repeated_size);
  EXPECT_EQ(repeated_size, 2);
  EXPECT_EQ(entries[0], 12);
  EXPECT_EQ(entries[1], 12);
}

// Tests a second parse that promotes a message within the parser because we are
// merging into an empty/unlinked message after the message has been linked.
TEST(GeneratedCode, PromoteInParser) {
  upb::Arena arena;
  upb_test_ModelWithSubMessages* input_msg =
      upb_test_ModelWithSubMessages_new(arena.ptr());
  upb_test_ModelWithExtensions* sub_message =
      upb_test_ModelWithExtensions_new(arena.ptr());
  upb_test_ModelWithSubMessages_set_id(input_msg, 11);
  upb_test_ModelWithExtensions_add_repeated_int32(sub_message, 12, arena.ptr());
  upb_test_ModelWithSubMessages_set_optional_child(input_msg, sub_message);
  size_t serialized_size;
  char* serialized = upb_test_ModelWithSubMessages_serialize(
      input_msg, arena.ptr(), &serialized_size);

  upb_MiniTable* mini_table = CreateMiniTableWithEmptySubTables(arena.ptr());

  // Parse once without linking the MiniTable.
  upb_Message* msg = _upb_Message_New(mini_table, arena.ptr());
  upb_DecodeStatus decode_status =
      upb_Decode(serialized, serialized_size, msg, mini_table, nullptr,
                 kUpb_DecodeOption_ExperimentalAllowUnlinked, arena.ptr());
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);

  // Link the MiniTable.
  const upb_MiniTableField* submsg_field =
      upb_MiniTable_FindFieldByNumber(mini_table, 5);
  EXPECT_TRUE(
      upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)submsg_field,
                                  &upb_0test__ModelWithExtensions_msg_init));

  // Parse again.  This will promote the message.  An explicit promote will not
  // be required.
  decode_status =
      upb_Decode(serialized, serialized_size, msg, mini_table, nullptr,
                 kUpb_DecodeOption_ExperimentalAllowUnlinked, arena.ptr());
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);
  upb_test_ModelWithExtensions* promoted =
      (upb_test_ModelWithExtensions*)upb_Message_GetMessage(msg, submsg_field);

  EXPECT_NE(nullptr, promoted);
  EXPECT_EQ(UPB_UPCAST(promoted), upb_Message_GetMessage(msg, submsg_field));

  // The repeated field should have two entries for the two parses.
  size_t repeated_size;
  const int32_t* entries =
      upb_test_ModelWithExtensions_repeated_int32(promoted, &repeated_size);
  EXPECT_EQ(repeated_size, 2);
  EXPECT_EQ(entries[0], 12);
  EXPECT_EQ(entries[1], 12);
}

TEST(GeneratedCode, PromoteUnknownRepeatedMessage) {
  upb::Arena arena;
  upb_test_ModelWithSubMessages* input_msg =
      upb_test_ModelWithSubMessages_new(arena.ptr());
  upb_test_ModelWithSubMessages_set_id(input_msg, 123);

  // Add 2 repeated messages to input_msg.
  upb_test_ModelWithExtensions* item =
      upb_test_ModelWithSubMessages_add_items(input_msg, arena.ptr());
  upb_test_ModelWithExtensions_set_random_int32(item, 5);
  item = upb_test_ModelWithSubMessages_add_items(input_msg, arena.ptr());
  upb_test_ModelWithExtensions_set_random_int32(item, 6);

  size_t serialized_size;
  char* serialized = upb_test_ModelWithSubMessages_serialize(
      input_msg, arena.ptr(), &serialized_size);

  upb_MiniTable* mini_table = CreateMiniTableWithEmptySubTables(arena.ptr());
  upb_DecodeStatus decode_status;

  // If we parse without allowing unlinked objects, the parse will fail.
  // TODO: re-enable this test once the old method of tree shaking is
  // removed
  // upb_Message* fail_msg = _upb_Message_New(mini_table, arena.ptr());
  // decode_status =
  //     upb_Decode(serialized, serialized_size, fail_msg, mini_table, nullptr,
  //     0,
  //                arena.ptr());
  // EXPECT_EQ(decode_status, kUpb_DecodeStatus_UnlinkedSubMessage);

  // if we parse while allowing unlinked objects, the parse will succeed.
  upb_Message* msg = _upb_Message_New(mini_table, arena.ptr());
  decode_status =
      upb_Decode(serialized, serialized_size, msg, mini_table, nullptr,
                 kUpb_DecodeOption_ExperimentalAllowUnlinked, arena.ptr());

  CheckReserialize(msg, mini_table, arena.ptr(), serialized, serialized_size);

  // Int32 field is present, as normal.
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);
  int32_t val = upb_Message_GetInt32(
      msg, upb_MiniTable_FindFieldByNumber(mini_table, 4), 0);
  EXPECT_EQ(val, 123);

  const upb_MiniTableField* repeated_field =
      upb_MiniTable_FindFieldByNumber(mini_table, 6);

  upb_Array* array = upb_Message_GetMutableArray(msg, repeated_field);

  // Array length is 2 even though the messages are empty.
  EXPECT_EQ(2, upb_Array_Size(array));

  // Update mini table and promote unknown to a message.
  EXPECT_TRUE(upb_MiniTable_SetSubMessage(
      mini_table, (upb_MiniTableField*)repeated_field,
      &upb_0test__ModelWithExtensions_msg_init));
  const int decode_options =
      upb_DecodeOptions_MaxDepth(0);  // UPB_DECODE_ALIAS disabled.
  upb_DecodeStatus promote_result =
      upb_Array_PromoteMessages(array, &upb_0test__ModelWithExtensions_msg_init,
                                decode_options, arena.ptr());
  EXPECT_EQ(promote_result, kUpb_DecodeStatus_Ok);
  const upb_Message* promoted_message = upb_Array_Get(array, 0).msg_val;
  EXPECT_EQ(upb_test_ModelWithExtensions_random_int32(
                (upb_test_ModelWithExtensions*)promoted_message),
            5);
  promoted_message = upb_Array_Get(array, 1).msg_val;
  EXPECT_EQ(upb_test_ModelWithExtensions_random_int32(
                (upb_test_ModelWithExtensions*)promoted_message),
            6);
}

TEST(GeneratedCode, PromoteUnknownToMap) {
  upb::Arena arena;
  upb_test_ModelWithMaps* input_msg = upb_test_ModelWithMaps_new(arena.ptr());
  upb_test_ModelWithMaps_set_id(input_msg, 123);

  upb_test_ModelWithExtensions* submsg0 =
      upb_test_ModelWithExtensions_new(arena.ptr());
  upb_test_ModelWithExtensions_set_random_int32(submsg0, 100);
  upb_test_ModelWithExtensions* submsg1 =
      upb_test_ModelWithExtensions_new(arena.ptr());
  upb_test_ModelWithExtensions_set_random_int32(submsg1, 123);
  upb_test_ModelWithExtensions* submsg2 =
      upb_test_ModelWithExtensions_new(arena.ptr());
  upb_test_ModelWithExtensions_set_random_int32(submsg2, 456);

  // Add 3 map entries.
  upb_test_ModelWithMaps_map_im_set(input_msg, 0, submsg0, arena.ptr());
  upb_test_ModelWithMaps_map_im_set(input_msg, 111, submsg1, arena.ptr());
  upb_test_ModelWithMaps_map_im_set(input_msg, 222, submsg2, arena.ptr());

  size_t serialized_size;
  char* serialized = upb_test_ModelWithMaps_serialize_ex(
      input_msg, kUpb_EncodeOption_Deterministic, arena.ptr(),
      &serialized_size);

  upb_MiniTable* mini_table =
      CreateMiniTableWithEmptySubTablesForMaps(arena.ptr());

  // If we parse without allowing unlinked objects, the parse will fail.
  upb_Message* fail_msg1 = _upb_Message_New(mini_table, arena.ptr());
  upb_DecodeStatus decode_status =
      upb_Decode(serialized, serialized_size, fail_msg1, mini_table, nullptr, 0,
                 arena.ptr());
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_UnlinkedSubMessage);

  // if we parse while allowing unlinked objects, the parse will succeed.
  upb_Message* msg = _upb_Message_New(mini_table, arena.ptr());
  decode_status =
      upb_Decode(serialized, serialized_size, msg, mini_table, nullptr,
                 kUpb_DecodeOption_ExperimentalAllowUnlinked, arena.ptr());
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);

  CheckReserialize(msg, mini_table, arena.ptr(), serialized, serialized_size);

  upb_MiniTableField* map_field = const_cast<upb_MiniTableField*>(
      upb_MiniTable_FindFieldByNumber(mini_table, 5));

  upb_Map* map = upb_Message_GetMutableMap(msg, map_field);

  // Map size is 3 even though messages are unlinked.
  EXPECT_EQ(3, upb_Map_Size(map));

  // Update mini table and promote unknown to a message.
  upb_MiniTable* entry = const_cast<upb_MiniTable*>(
      upb_MiniTable_GetSubMessageTable(mini_table, map_field));
  upb_MiniTableField* entry_value = const_cast<upb_MiniTableField*>(
      upb_MiniTable_FindFieldByNumber(entry, 2));
  upb_MiniTable_SetSubMessage(entry, entry_value,
                              &upb_0test__ModelWithExtensions_msg_init);
  upb_DecodeStatus promote_result = upb_Map_PromoteMessages(
      map, &upb_0test__ModelWithExtensions_msg_init, 0, arena.ptr());
  EXPECT_EQ(promote_result, kUpb_DecodeStatus_Ok);

  upb_MessageValue key;
  upb_MessageValue val;
  key.int32_val = 0;
  EXPECT_TRUE(upb_Map_Get(map, key, &val));
  EXPECT_EQ(100, upb_test_ModelWithExtensions_random_int32(
                     static_cast<const upb_test_ModelWithExtensions*>(
                         (void*)(val.msg_val))));

  key.int32_val = 111;
  EXPECT_TRUE(upb_Map_Get(map, key, &val));
  EXPECT_EQ(123, upb_test_ModelWithExtensions_random_int32(
                     static_cast<const upb_test_ModelWithExtensions*>(
                         (void*)(val.msg_val))));

  key.int32_val = 222;
  EXPECT_TRUE(upb_Map_Get(map, key, &val));
  EXPECT_EQ(456, upb_test_ModelWithExtensions_random_int32(
                     static_cast<const upb_test_ModelWithExtensions*>(
                         (void*)(val.msg_val))));
}

}  // namespace

// OLD tests, to be removed!

namespace {

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

}  // namespace
