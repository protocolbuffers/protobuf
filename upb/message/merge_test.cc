#include "upb/message/merge.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto2.upb.h"
#include "google/protobuf/test_messages_proto2.upb_minitable.h"
#include "google/protobuf/test_messages_proto3.upb.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/extension.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/test.upb.h"
#include "upb/message/test.upb_minitable.h"
#include "upb/message/unknown_fields.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/test/test.upb.h"
#include "upb/test/test.upb_minitable.h"

// Must be last.
#include "upb/port/def.inc"

namespace {

// Proto2 test messages field numbers used for reflective access.
const uint32_t kFieldOptionalInt32 = 1;
const uint32_t kFieldOptionalString = 14;
const uint32_t kFieldOptionalNestedMessage = 18;
const uint32_t kFieldOptionalOneOfUInt32 = 111;
const uint32_t kFieldOptionalOneOfString = 113;

const char kTestStr1[] = "Hello1";
const int32_t kTestInt32 = 567;

const upb_MiniTableField* find_proto2_field(int field_number) {
  return upb_MiniTable_FindFieldByNumber(
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      field_number);
}

TEST(GeneratedCode, MergeMessageScalarAndString) {
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
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(clone), UPB_UPCAST(msg),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));
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

TEST(GeneratedCode, MergeMessageSubMessage) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  const upb_MiniTableField* nested_message_field =
      find_proto2_field(kFieldOptionalNestedMessage);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* nested1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(
          source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(nested1,
                                                                       12);
  upb_Message_SetMessage(UPB_UPCAST(msg1), nested_message_field,
                         UPB_UPCAST(nested1));

  protobuf_test_messages_proto2_TestAllTypesProto2* msg2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* nested2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(
          source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(nested2,
                                                                       24);
  upb_Message_SetMessage(UPB_UPCAST(msg2), nested_message_field,
                         UPB_UPCAST(nested2));

  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* clone =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  // Merge msg1 (expect nested message to be created/deep cloned)
  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(clone), UPB_UPCAST(msg1),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  EXPECT_TRUE(
      upb_Message_HasBaseField(UPB_UPCAST(clone), nested_message_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage*
      cloned_nested =
          (protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage*)
              upb_Message_GetMessage(UPB_UPCAST(clone), nested_message_field);
  EXPECT_EQ(protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_a(
                cloned_nested),
            12);

  // Merge msg2 (expect nested message to be recursively merged)
  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(clone), UPB_UPCAST(msg2),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  cloned_nested =
      (protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage*)
          upb_Message_GetMessage(UPB_UPCAST(clone), nested_message_field);
  EXPECT_EQ(protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_a(
                cloned_nested),
            24);

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageArrayField) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(
          msg1, 3, source_arena));
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(
          msg1, 4, source_arena));

  protobuf_test_messages_proto2_TestAllTypesProto2* msg2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(
          msg2, 5, source_arena));
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(
          msg2, 6, source_arena));

  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* clone =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(clone), UPB_UPCAST(msg1),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  size_t size = 0;
  const int32_t* values =
      protobuf_test_messages_proto2_TestAllTypesProto2_repeated_int32(clone,
                                                                      &size);
  EXPECT_EQ(size, 2);
  EXPECT_EQ(values[0], 3);
  EXPECT_EQ(values[1], 4);

  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(clone), UPB_UPCAST(msg2),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  values = protobuf_test_messages_proto2_TestAllTypesProto2_repeated_int32(
      clone, &size);
  EXPECT_EQ(size, 4);
  EXPECT_EQ(values[0], 3);
  EXPECT_EQ(values[1], 4);
  EXPECT_EQ(values[2], 5);
  EXPECT_EQ(values[3], 6);

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageMapField) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_set(
          msg1, 1, 1.5, source_arena));
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_set(
          msg1, 2, 2.5, source_arena));

  protobuf_test_messages_proto2_TestAllTypesProto2* msg2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_set(
          msg2, 2, 22.5, source_arena));
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_set(
          msg2, 3, 3.5, source_arena));

  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* clone =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(clone), UPB_UPCAST(msg1),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  double val;
  EXPECT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_get(
          clone, 1, &val));
  EXPECT_EQ(val, 1.5);
  EXPECT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_get(
          clone, 2, &val));
  EXPECT_EQ(val, 2.5);

  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(clone), UPB_UPCAST(msg2),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  EXPECT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_get(
          clone, 1, &val));
  EXPECT_EQ(val, 1.5);
  EXPECT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_get(
          clone, 2, &val));
  EXPECT_EQ(val, 22.5);
  EXPECT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_get(
          clone, 3, &val));
  EXPECT_EQ(val, 3.5);

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageMapWithNullValue) {
  upb_Arena* arena = upb_Arena_New();

  // Create dst message
  protobuf_test_messages_proto2_TestAllTypesProto2* dst =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  // Create src message
  protobuf_test_messages_proto2_TestAllTypesProto2* src =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  // Get map field
  const upb_MiniTableField* map_field =
      find_proto2_field(103);  // map_int32_nested_message
  ASSERT_NE(map_field, nullptr);

  // Initialize the map on dst
  const upb_MiniTable* map_entry_mt =
      upb_MiniTable_MapEntrySubMessage(map_field);
  upb_Map* dst_map = upb_Message_GetOrCreateMutableMap(
      UPB_UPCAST(dst), map_entry_mt, map_field, arena);
  ASSERT_NE(dst_map, nullptr);

  // Insert key 1 with value = nullptr in dst_map:
  upb_MessageValue key;
  key.int32_val = 1;
  upb_MessageValue val;
  val.msg_val = nullptr;
  ASSERT_TRUE(upb_Map_Set(dst_map, key, val, arena));

  // Now, insert key 1 with a valid NestedMessage in src_map:
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* nested =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(nested,
                                                                       42);

  auto set_map_entry =
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_nested_message_set;
  ASSERT_TRUE(set_map_entry(src, 1, nested, arena));

  // Merge src into dst.
  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(dst), UPB_UPCAST(src),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  // Check that the merged value in dst contains key 1 mapping to a message with
  // a = 42
  upb_MessageValue dst_val;
  EXPECT_TRUE(upb_Map_Get(dst_map, key, &dst_val));
  ASSERT_NE(dst_val.msg_val, nullptr);

  typedef protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage
      NestedMsg;
  const NestedMsg* merged_nested = (const NestedMsg*)dst_val.msg_val;
  auto get_a = protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_a;
  EXPECT_EQ(get_a(merged_nested), 42);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageWithHasbitSetButNullMessage) {
  upb_Arena* arena = upb_Arena_New();

  // Create src message
  protobuf_test_messages_proto2_TestAllTypesProto2* src =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  // Set the hasbit on 'optional_nested_message' field on src, but leave the
  // pointer NULL.
  const upb_MiniTableField* nested_message_field =
      find_proto2_field(kFieldOptionalNestedMessage);
  ASSERT_NE(nested_message_field, nullptr);

  // Manually set the hasbit.
  UPB_PRIVATE(_upb_Message_SetHasbit)(UPB_UPCAST(src), nested_message_field);

  // Ensure that hasbit is set, but the actual message pointer is NULL.
  EXPECT_TRUE(upb_Message_HasBaseField(UPB_UPCAST(src), nested_message_field));
  EXPECT_EQ(upb_Message_GetMessage(UPB_UPCAST(src), nested_message_field),
            nullptr);

  // Create dst message
  protobuf_test_messages_proto2_TestAllTypesProto2* dst =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  // Merge src into dst. This should not crash or trigger assertions.
  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(dst), UPB_UPCAST(src),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  // Verify that dst has the field unset.
  EXPECT_FALSE(upb_Message_HasBaseField(UPB_UPCAST(dst), nested_message_field));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageNonCanonicalExtensions) {
  upb_Arena* source_arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg =
      upb_test_ModelWithExtensions_new(source_arena);
  upb_test_ModelExtension1* ext1 = upb_test_ModelExtension1_new(source_arena);
  upb_test_ModelExtension1_set_str(ext1,
                                   upb_StringView_FromString("LifecycleValue"));

  // Attach as non-canonical extension
  UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg), upb_test_ModelExtension1_model_ext_ext, &ext1,
      source_arena);

  // Merge msg to clone
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* clone = upb_test_ModelWithExtensions_new(arena);

  EXPECT_TRUE(upb_Message_MergeFrom(UPB_UPCAST(clone), UPB_UPCAST(msg),
                                    &upb_0test__ModelWithExtensions_msg_init, 0,
                                    arena));

  // Mutate original
  upb_test_ModelExtension1_set_str(ext1, upb_StringView_FromString("Mutated"));
  upb_Arena_Free(source_arena);

  // Check if clone has the non-canonical extension and it's unmodified
  upb_MessageUnknown data;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  bool has_non_canonical = false;
  const upb_Extension* ext_found = nullptr;
  while (upb_Message_NextUnknown2(UPB_UPCAST(clone), &data, &iter)) {
    if (data.type == kUpb_MessageUnknownType_NonCanonicalExtension) {
      has_non_canonical = true;
      ext_found = (const upb_Extension*)data.value.extension;
    }
  }
  EXPECT_TRUE(has_non_canonical);
  ASSERT_NE(ext_found, nullptr);

  const upb_test_ModelExtension1* cloned_ext =
      (const upb_test_ModelExtension1*)ext_found->data.msg_val;
  EXPECT_TRUE(
      upb_StringView_IsEqual(upb_test_ModelExtension1_str(cloned_ext),
                             upb_StringView_FromString("LifecycleValue")));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageOneofField) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_oneof_uint32(msg1, 123);

  protobuf_test_messages_proto2_TestAllTypesProto2* msg2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_oneof_string(
      msg2, upb_StringView_FromString(kTestStr1));

  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* clone =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  const upb_MiniTableField* oneof_uint32_field =
      find_proto2_field(kFieldOptionalOneOfUInt32);
  const upb_MiniTableField* oneof_string_field =
      find_proto2_field(kFieldOptionalOneOfString);

  // Merge msg1
  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(clone), UPB_UPCAST(msg1),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  EXPECT_TRUE(upb_Message_HasBaseField(UPB_UPCAST(clone), oneof_uint32_field));
  EXPECT_FALSE(upb_Message_HasBaseField(UPB_UPCAST(clone), oneof_string_field));
  EXPECT_EQ(
      protobuf_test_messages_proto2_TestAllTypesProto2_oneof_uint32(clone),
      123);

  // Merge msg2 (starts with msg1's oneof, should clear/overwrite it)
  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(clone), UPB_UPCAST(msg2),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  EXPECT_FALSE(upb_Message_HasBaseField(UPB_UPCAST(clone), oneof_uint32_field));
  EXPECT_TRUE(upb_Message_HasBaseField(UPB_UPCAST(clone), oneof_string_field));
  EXPECT_TRUE(upb_StringView_IsEqual(
      protobuf_test_messages_proto2_TestAllTypesProto2_oneof_string(clone),
      upb_StringView_FromString(kTestStr1)));

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageCanonicalExtensions) {
  typedef protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect
      MsgSetCorrect;
  typedef protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1
      MsgSetExt1;  // NOLINT

  upb_Arena* source_arena = upb_Arena_New();
  MsgSetCorrect* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          source_arena);
  MsgSetExt1* ext1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_new(
          source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_set_str(
      ext1, upb_StringView_FromString(kTestStr1));
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_set_message_set_extension(
      msg, ext1, source_arena);

  // Merge msg to clone
  upb_Arena* arena = upb_Arena_New();
  MsgSetCorrect* clone =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          arena);

  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(clone), UPB_UPCAST(msg),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2__MessageSetCorrect_msg_init,  // NOLINT
      0, arena));

  // Mutate original
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_set_str(
      ext1, upb_StringView_FromString("Mutated"));
  upb_Arena_Free(source_arena);

  const MsgSetExt1* cloned_ext =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_message_set_extension(
          clone);
  ASSERT_NE(cloned_ext, nullptr);
  EXPECT_TRUE(upb_StringView_IsEqual(
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_str(
          cloned_ext),
      upb_StringView_FromString(kTestStr1)));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageSelfFail) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  // Set scalar field
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(msg, 123);

  // Set repeated field
  protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(msg, 5,
                                                                      arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(msg, 6,
                                                                      arena);

  // Merging the message into itself should fail.
  EXPECT_FALSE(upb_Message_MergeFrom(
      UPB_UPCAST(msg), UPB_UPCAST(msg),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageRepeatedSelfMergeFail) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  protobuf_test_messages_proto2_TestAllTypesProto2* msg2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(msg1, 5,
                                                                      arena);

  // Alias the repeated field of msg1 into msg2.
  const upb_MiniTableField* repeated_field = find_proto2_field(31);
  ASSERT_NE(repeated_field, nullptr);
  upb_Array* arr =
      upb_Message_GetMutableArray(UPB_UPCAST(msg1), repeated_field);
  ASSERT_NE(arr, nullptr);
  upb_Message_SetBaseFieldArray(UPB_UPCAST(msg2), repeated_field, arr, nullptr);

  // Merging msg1 into msg2 should fail because their repeated fields alias.
  EXPECT_FALSE(upb_Message_MergeFrom(
      UPB_UPCAST(msg2), UPB_UPCAST(msg1),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageMapSelfMergeFail) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  protobuf_test_messages_proto2_TestAllTypesProto2* msg2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_set(
      msg1, 1, 1.5, arena);

  // Alias the map field of msg1 into msg2.
  const upb_MiniTableField* map_field = find_proto2_field(67);
  ASSERT_NE(map_field, nullptr);
  upb_Map* map = upb_Message_GetMutableMap(UPB_UPCAST(msg1), map_field);
  ASSERT_NE(map, nullptr);
  const upb_MiniTable* map_entry_mt =
      upb_MiniTable_MapEntrySubMessage(map_field);
  upb_Message_SetBaseFieldMap(UPB_UPCAST(msg2), map_field, map, map_entry_mt);

  // Merging msg1 into msg2 should fail because their map fields alias.
  EXPECT_FALSE(upb_Message_MergeFrom(
      UPB_UPCAST(msg2), UPB_UPCAST(msg1),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, NextSerializableField) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  const upb_MiniTable* mt =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

  // 1. Initial State: No fields are set.
  const upb_MiniTableField* f = nullptr;
  uintptr_t iter = kUpb_Message_SerializableFieldBegin;
  EXPECT_FALSE(
      upb_Message_NextSerializableField(UPB_UPCAST(msg), mt, &f, &iter));

  // 2. Test optional field (presence > 0).
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(msg, 123);
  f = nullptr;
  iter = kUpb_Message_SerializableFieldBegin;
  ASSERT_TRUE(
      upb_Message_NextSerializableField(UPB_UPCAST(msg), mt, &f, &iter));
  EXPECT_EQ(upb_MiniTableField_Number(f), 1);
  EXPECT_FALSE(
      upb_Message_NextSerializableField(UPB_UPCAST(msg), mt, &f, &iter));

  // 3. Test oneof fields (presence < 0).
  protobuf_test_messages_proto2_TestAllTypesProto2_set_oneof_uint32(msg, 456);
  f = nullptr;
  iter = kUpb_Message_SerializableFieldBegin;
  int found_fields = 0;
  bool found_opt = false;
  bool found_oneof = false;
  while (upb_Message_NextSerializableField(UPB_UPCAST(msg), mt, &f, &iter)) {
    found_fields++;
    if (upb_MiniTableField_Number(f) == 1) found_opt = true;
    if (upb_MiniTableField_Number(f) == 111) found_oneof = true;
  }
  EXPECT_EQ(found_fields, 2);
  EXPECT_TRUE(found_opt);
  EXPECT_TRUE(found_oneof);

  // Switch oneof field.
  protobuf_test_messages_proto2_TestAllTypesProto2_set_oneof_string(
      msg, upb_StringView_FromString("Hello"));
  f = nullptr;
  iter = kUpb_Message_SerializableFieldBegin;
  found_fields = 0;
  found_opt = false;
  found_oneof = false;
  bool found_new_oneof = false;
  while (upb_Message_NextSerializableField(UPB_UPCAST(msg), mt, &f, &iter)) {
    found_fields++;
    if (upb_MiniTableField_Number(f) == 1) found_opt = true;
    if (upb_MiniTableField_Number(f) == 111) found_oneof = true;
    if (upb_MiniTableField_Number(f) == 113) found_new_oneof = true;
  }
  EXPECT_EQ(found_fields, 2);
  EXPECT_TRUE(found_opt);
  EXPECT_FALSE(found_oneof);
  EXPECT_TRUE(found_new_oneof);

  // 4. Test repeated fields (presence == 0).
  protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(msg, 789,
                                                                      arena);
  f = nullptr;
  iter = kUpb_Message_SerializableFieldBegin;
  found_fields = 0;
  bool found_repeated = false;
  while (upb_Message_NextSerializableField(UPB_UPCAST(msg), mt, &f, &iter)) {
    found_fields++;
    if (upb_MiniTableField_Number(f) == 31) found_repeated = true;
  }
  EXPECT_EQ(found_fields, 3);
  EXPECT_TRUE(found_repeated);

  // 5. Test map fields (presence == 0).
  protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_set(
      msg, 10, 20.0, arena);
  f = nullptr;
  iter = kUpb_Message_SerializableFieldBegin;
  found_fields = 0;
  bool found_map = false;
  while (upb_Message_NextSerializableField(UPB_UPCAST(msg), mt, &f, &iter)) {
    found_fields++;
    if (upb_MiniTableField_Number(f) == 67) found_map = true;
  }
  EXPECT_EQ(found_fields, 4);
  EXPECT_TRUE(found_map);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeEmptyArraysAndMaps) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);

  const upb_MiniTableField* repeated_field = find_proto2_field(31);
  const upb_MiniTableField* map_field = find_proto2_field(67);

  // Allocate empty array and empty map in the source message.
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      UPB_UPCAST(msg), repeated_field, source_arena);
  ASSERT_NE(arr, nullptr);
  EXPECT_EQ(upb_Array_Size(arr), 0);

  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_set(
          msg, 1, 1.5, source_arena));
  upb_Map* map = upb_Message_GetMutableMap(UPB_UPCAST(msg), map_field);
  ASSERT_NE(map, nullptr);
  EXPECT_EQ(upb_Map_Size(map), 1);

  // Delete key to make the map empty.
  upb_MessageValue key;
  key.int32_val = 1;
  EXPECT_TRUE(upb_Map_Delete(map, key, nullptr));
  EXPECT_EQ(upb_Map_Size(map), 0);

  // Create destination message
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* clone =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  // Merge msg into clone. Since the map and array are empty, they should be
  // skipped and no array or map should be allocated/copied on the clone.
  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(clone), UPB_UPCAST(msg),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init, 0,
      arena));

  // Verify that clone does not have the array/map allocated.
  EXPECT_EQ(upb_Message_GetArray(UPB_UPCAST(clone), repeated_field), nullptr);
  EXPECT_EQ(upb_Message_GetMap(UPB_UPCAST(clone), map_field), nullptr);

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageCanonicalSubmessageExtensionRecursive) {
  upb_Arena* arena = upb_Arena_New();

  // Create dst message and set the submessage extension with field
  // 'optional_int32'
  upb_test_TestExtensions* dst = upb_test_TestExtensions_new(arena);
  protobuf_test_messages_proto3_TestAllTypesProto3* dst_sub =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena);
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_int32(dst_sub,
                                                                      123);
  upb_test_set_optional_msg_ext(dst, dst_sub, arena);

  // Create src message and set the submessage extension with field
  // 'optional_int64'
  upb_test_TestExtensions* src = upb_test_TestExtensions_new(arena);
  protobuf_test_messages_proto3_TestAllTypesProto3* src_sub =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena);
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_int64(src_sub,
                                                                      456);
  upb_test_set_optional_msg_ext(src, src_sub, arena);

  // Merge src into dst
  EXPECT_TRUE(upb_Message_MergeFrom(UPB_UPCAST(dst), UPB_UPCAST(src),
                                    &upb_0test__TestExtensions_msg_init, 0,
                                    arena));

  // Verify the extension has been recursively merged
  EXPECT_TRUE(upb_test_has_optional_msg_ext(dst));
  const protobuf_test_messages_proto3_TestAllTypesProto3* merged_sub =
      upb_test_optional_msg_ext(dst);
  ASSERT_NE(merged_sub, nullptr);

  // Both fields set on dst_sub and src_sub should be present in the merged
  // message extension
  EXPECT_EQ(protobuf_test_messages_proto3_TestAllTypesProto3_optional_int32(
                merged_sub),
            123);
  EXPECT_EQ(protobuf_test_messages_proto3_TestAllTypesProto3_optional_int64(
                merged_sub),
            456);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageAliasOption) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* src =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);

  // 1. String field
  const upb_MiniTableField* optional_string_field =
      find_proto2_field(kFieldOptionalString);
  char string_buffer[] = "AliasedStringData";
  ASSERT_TRUE(upb_Message_SetString(
      UPB_UPCAST(src), optional_string_field,
      upb_StringView_FromDataAndSize(string_buffer, sizeof(string_buffer) - 1),
      source_arena));

  // 2. Nested Submessage field
  const upb_MiniTableField* nested_message_field =
      find_proto2_field(kFieldOptionalNestedMessage);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* nested =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(
          source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(nested,
                                                                       42);
  upb_Message_SetMessage(UPB_UPCAST(src), nested_message_field,
                         UPB_UPCAST(nested));

  // 3. Repeated field
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(
          src, 100, source_arena));
  const upb_MiniTableField* repeated_field = find_proto2_field(31);
  const upb_Array* src_arr =
      upb_Message_GetArray(UPB_UPCAST(src), repeated_field);
  ASSERT_NE(src_arr, nullptr);

  // 4. Map field
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_set(
          src, 7, 7.7, source_arena));
  const upb_MiniTableField* map_field = find_proto2_field(67);
  const upb_Map* src_map = upb_Message_GetMap(UPB_UPCAST(src), map_field);
  ASSERT_NE(src_map, nullptr);

  // Merge src to dst with kUpb_MergeOption_Alias
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* dst =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(dst), UPB_UPCAST(src),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      kUpb_MergeOption_Alias, arena));

  // Verify string aliasing (pointer equality)
  upb_StringView dst_str =
      upb_Message_GetString(UPB_UPCAST(dst), optional_string_field,
                            upb_StringView_FromDataAndSize(nullptr, 0));
  EXPECT_EQ(dst_str.data, string_buffer);

  // Verify submessage aliasing (pointer equality)
  const upb_Message* dst_nested =
      upb_Message_GetMessage(UPB_UPCAST(dst), nested_message_field);
  EXPECT_EQ(dst_nested, UPB_UPCAST(nested));

  // Verify array aliasing (pointer equality)
  const upb_Array* dst_arr =
      upb_Message_GetArray(UPB_UPCAST(dst), repeated_field);
  EXPECT_EQ(dst_arr, src_arr);

  // Verify map aliasing (pointer equality)
  const upb_Map* dst_map = upb_Message_GetMap(UPB_UPCAST(dst), map_field);
  EXPECT_EQ(dst_map, src_map);

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageAliasOptionPrimitivesAndMessages) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* src =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);

  // Set primitive scalar values
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(src, 999);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_double(src,
                                                                       3.14);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_bool(src, true);

  // Submessage in src
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* src_nested =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(
          source_arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(
      src_nested, 77);
  const upb_MiniTableField* nested_field =
      find_proto2_field(kFieldOptionalNestedMessage);
  upb_Message_SetMessage(UPB_UPCAST(src), nested_field, UPB_UPCAST(src_nested));

  upb_Arena* arena = upb_Arena_New();

  // Test 1: Merge into empty dst (submessage should be aliased directly)
  protobuf_test_messages_proto2_TestAllTypesProto2* dst1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(dst1), UPB_UPCAST(src),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      kUpb_MergeOption_Alias, arena));

  EXPECT_EQ(
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(dst1),
      999);
  EXPECT_EQ(
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_double(dst1),
      3.14);
  EXPECT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_bool(dst1));
  EXPECT_EQ(upb_Message_GetMessage(UPB_UPCAST(dst1), nested_field),
            UPB_UPCAST(src_nested));

  // Test 2: Merge into dst with existing submessage (recursively merged)
  protobuf_test_messages_proto2_TestAllTypesProto2* dst2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* dst2_nested =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(
      dst2_nested, 11);
  upb_Message_SetMessage(UPB_UPCAST(dst2), nested_field,
                         UPB_UPCAST(dst2_nested));

  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(dst2), UPB_UPCAST(src),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      kUpb_MergeOption_Alias, arena));

  // The submessage pointer on dst2 should still be dst2_nested
  EXPECT_EQ(upb_Message_GetMessage(UPB_UPCAST(dst2), nested_field),
            UPB_UPCAST(dst2_nested));
  EXPECT_EQ(protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_a(
                dst2_nested),
            77);

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageAliasOptionRepeatedFields) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* src =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);

  // Repeated int32 in src
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(
          src, 10, source_arena));
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(
          src, 20, source_arena));

  // Repeated string in src
  char str_buf[] = "RepeatedAliasedString";
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_string(
          src, upb_StringView_FromDataAndSize(str_buf, sizeof(str_buf) - 1),
          source_arena));

  upb_Arena* arena = upb_Arena_New();

  // Test 1: Absent array in dst -> array pointer aliased directly
  protobuf_test_messages_proto2_TestAllTypesProto2* dst1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(dst1), UPB_UPCAST(src),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      kUpb_MergeOption_Alias, arena));

  const upb_MiniTableField* repeated_int32_field = find_proto2_field(31);
  const upb_MiniTableField* repeated_string_field = find_proto2_field(44);

  EXPECT_EQ(upb_Message_GetArray(UPB_UPCAST(dst1), repeated_int32_field),
            upb_Message_GetArray(UPB_UPCAST(src), repeated_int32_field));
  EXPECT_EQ(upb_Message_GetArray(UPB_UPCAST(dst1), repeated_string_field),
            upb_Message_GetArray(UPB_UPCAST(src), repeated_string_field));

  // Test 2: Existing array in dst -> elements appended
  protobuf_test_messages_proto2_TestAllTypesProto2* dst2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_add_repeated_int32(
          dst2, 5, arena));

  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(dst2), UPB_UPCAST(src),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      kUpb_MergeOption_Alias, arena));

  size_t size = 0;
  const int32_t* values =
      protobuf_test_messages_proto2_TestAllTypesProto2_repeated_int32(dst2,
                                                                      &size);
  EXPECT_EQ(size, 3);
  EXPECT_EQ(values[0], 5);
  EXPECT_EQ(values[1], 10);
  EXPECT_EQ(values[2], 20);

  // Check string element data pointer aliasing in appended repeated string
  const upb_Array* dst2_str_arr =
      upb_Message_GetArray(UPB_UPCAST(dst2), repeated_string_field);
  ASSERT_NE(dst2_str_arr, nullptr);
  EXPECT_EQ(upb_Array_Size(dst2_str_arr), 1);
  upb_MessageValue str_val = upb_Array_Get(dst2_str_arr, 0);
  EXPECT_EQ(str_val.str_val.data, str_buf);

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MergeMessageAliasOptionMapFields) {
  upb_Arena* source_arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* src =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(source_arena);

  // Map int32 -> double in src
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_set(
          src, 1, 10.5, source_arena));

  upb_Arena* arena = upb_Arena_New();

  // Test 1: Absent map in dst -> map pointer aliased directly
  protobuf_test_messages_proto2_TestAllTypesProto2* dst1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(dst1), UPB_UPCAST(src),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      kUpb_MergeOption_Alias, arena));

  const upb_MiniTableField* map_field = find_proto2_field(67);
  EXPECT_EQ(upb_Message_GetMap(UPB_UPCAST(dst1), map_field),
            upb_Message_GetMap(UPB_UPCAST(src), map_field));

  // Test 2: Existing map in dst -> entries inserted/merged
  protobuf_test_messages_proto2_TestAllTypesProto2* dst2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  ASSERT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_set(
          dst2, 2, 20.5, arena));

  EXPECT_TRUE(upb_Message_MergeFrom(
      UPB_UPCAST(dst2), UPB_UPCAST(src),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      kUpb_MergeOption_Alias, arena));

  double val;
  EXPECT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_get(
          dst2, 1, &val));
  EXPECT_EQ(val, 10.5);
  EXPECT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_map_int32_double_get(
          dst2, 2, &val));
  EXPECT_EQ(val, 20.5);

  upb_Arena_Free(source_arena);
  upb_Arena_Free(arena);
}

}  // namespace
