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

#include "upb/message/accessors.h"

#include <string.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto2.upb.h"
#include "google/protobuf/test_messages_proto2.upb_minitable.h"
#include "google/protobuf/test_messages_proto3.upb.h"
#include "google/protobuf/test_messages_proto3.upb_minitable.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/status.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/message.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.hpp"
#include "upb/mini_descriptor/internal/modifiers.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"

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
const uint32_t kFieldOptionalRepeatedNestedMessage = 48;
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

const upb_MiniTableField* find_proto3_field(int field_number) {
  return upb_MiniTable_FindFieldByNumber(
      &protobuf_0test_0messages__proto3__TestAllTypesProto3_msg_init,
      field_number);
}

const upb_MiniTableField* find_proto2_field(int field_number) {
  return upb_MiniTable_FindFieldByNumber(
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      field_number);
}

TEST(GeneratedCode, HazzersProto2) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  // Scalar/Boolean.
  const upb_MiniTableField* optional_bool_field =
      find_proto2_field(kFieldOptionalBool);
  EXPECT_EQ(false,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_bool_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_bool(msg, true);
  EXPECT_EQ(true,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_bool_field));
  upb_Message_ClearBaseField(UPB_UPCAST(msg), optional_bool_field);
  EXPECT_EQ(false,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_bool_field));
  EXPECT_EQ(
      false,
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_bool(msg));

  // String.
  const upb_MiniTableField* optional_string_field =
      find_proto2_field(kFieldOptionalString);
  EXPECT_EQ(false,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_string_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_string(
      msg, upb_StringView_FromString(kTestStr1));
  EXPECT_EQ(true,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_string_field));
  EXPECT_EQ(
      strlen(kTestStr1),
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_string(msg)
          .size);
  upb_Message_ClearBaseField(UPB_UPCAST(msg), optional_string_field);
  EXPECT_EQ(false,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_string_field));
  EXPECT_EQ(
      0, protobuf_test_messages_proto2_TestAllTypesProto2_optional_string(msg)
             .size);

  // Message.
  const upb_MiniTableField* optional_message_field =
      find_proto2_field(kFieldOptionalNestedMessage);
  EXPECT_EQ(false,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_message_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_mutable_optional_nested_message(
      msg, arena);
  EXPECT_EQ(true,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_message_field));
  upb_Message_ClearBaseField(UPB_UPCAST(msg), optional_message_field);
  EXPECT_EQ(false,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_message_field));
  EXPECT_EQ(
      true,
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_nested_message(
          msg) == nullptr);

  // One of.
  const upb_MiniTableField* optional_oneof_uint32_field =
      find_proto2_field(kFieldOptionalOneOfUInt32);
  const upb_MiniTableField* optional_oneof_string_field =
      find_proto2_field(kFieldOptionalOneOfString);

  EXPECT_EQ(false, upb_Message_HasBaseField(UPB_UPCAST(msg),
                                            optional_oneof_uint32_field));
  EXPECT_EQ(false, upb_Message_HasBaseField(UPB_UPCAST(msg),
                                            optional_oneof_string_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_set_oneof_uint32(msg, 123);
  EXPECT_EQ(true, upb_Message_HasBaseField(UPB_UPCAST(msg),
                                           optional_oneof_uint32_field));
  EXPECT_EQ(false, upb_Message_HasBaseField(UPB_UPCAST(msg),
                                            optional_oneof_string_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_set_oneof_string(
      msg, upb_StringView_FromString(kTestStr1));
  EXPECT_EQ(false, upb_Message_HasBaseField(UPB_UPCAST(msg),
                                            optional_oneof_uint32_field));
  EXPECT_EQ(true, upb_Message_HasBaseField(UPB_UPCAST(msg),
                                           optional_oneof_string_field));
  upb_Message_ClearBaseField(UPB_UPCAST(msg), optional_oneof_uint32_field);
  EXPECT_EQ(false, upb_Message_HasBaseField(UPB_UPCAST(msg),
                                            optional_oneof_uint32_field));
  EXPECT_EQ(true, upb_Message_HasBaseField(UPB_UPCAST(msg),
                                           optional_oneof_string_field));
  upb_Message_ClearBaseField(UPB_UPCAST(msg), optional_oneof_string_field);
  EXPECT_EQ(false, upb_Message_HasBaseField(UPB_UPCAST(msg),
                                            optional_oneof_uint32_field));
  EXPECT_EQ(false, upb_Message_HasBaseField(UPB_UPCAST(msg),
                                            optional_oneof_string_field));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, ScalarsProto2) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  const upb_MiniTableField* optional_int32_field =
      find_proto2_field(kFieldOptionalInt32);

  EXPECT_EQ(
      0, protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(msg));

  EXPECT_EQ(0, upb_Message_GetInt32(UPB_UPCAST(msg), optional_int32_field, 0));
  upb_Message_SetBaseFieldInt32(UPB_UPCAST(msg), optional_int32_field,
                                kTestInt32);
  EXPECT_EQ(true,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_int32_field));
  EXPECT_EQ(kTestInt32,
            upb_Message_GetInt32(UPB_UPCAST(msg), optional_int32_field, 0));
  EXPECT_EQ(
      kTestInt32,
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(msg));

  const upb_MiniTableField* optional_uint32_field =
      find_proto2_field(kFieldOptionalUInt32);

  EXPECT_EQ(
      0, protobuf_test_messages_proto2_TestAllTypesProto2_optional_uint32(msg));
  EXPECT_EQ(0,
            upb_Message_GetUInt32(UPB_UPCAST(msg), optional_uint32_field, 0));
  upb_Message_SetBaseFieldUInt32(UPB_UPCAST(msg), optional_uint32_field,
                                 kTestUInt32);
  EXPECT_EQ(kTestUInt32,
            upb_Message_GetUInt32(UPB_UPCAST(msg), optional_uint32_field, 0));
  EXPECT_EQ(
      kTestUInt32,
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_uint32(msg));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, ScalarProto3) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena);

  const upb_MiniTableField* optional_int64_field =
      find_proto3_field(kFieldProto3OptionalInt64);
  const upb_MiniTableField* optional_uint64_field =
      find_proto3_field(kFieldProto3OptionalUInt64);

  EXPECT_EQ(
      0, protobuf_test_messages_proto3_TestAllTypesProto3_optional_int64(msg));
  upb_Message_SetBaseFieldInt64(UPB_UPCAST(msg), optional_int64_field, -1);
  EXPECT_EQ(
      -1, protobuf_test_messages_proto3_TestAllTypesProto3_optional_int64(msg));
  EXPECT_EQ(-1, upb_Message_GetInt64(UPB_UPCAST(msg), optional_int64_field, 0));

  EXPECT_EQ(
      0, protobuf_test_messages_proto3_TestAllTypesProto3_optional_uint64(msg));
  upb_Message_SetBaseFieldUInt64(UPB_UPCAST(msg), optional_uint64_field,
                                 kTestUInt64);
  EXPECT_EQ(
      kTestUInt64,
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_uint64(msg));
  EXPECT_EQ(kTestUInt64,
            upb_Message_GetUInt64(UPB_UPCAST(msg), optional_uint64_field, 0));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, Strings) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  const upb_MiniTableField* optional_string_field =
      find_proto2_field(kFieldOptionalString);

  // Test default.
  EXPECT_EQ(false,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_string_field));
  // Test read after write using C.
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_string(
      msg, upb_StringView_FromString(kTestStr1));
  EXPECT_EQ(true,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_string_field));
  upb_StringView value = upb_Message_GetString(
      UPB_UPCAST(msg), optional_string_field, upb_StringView{nullptr, 0});
  std::string read_value = std::string(value.data, value.size);
  EXPECT_EQ(kTestStr1, read_value);
  // Clear.
  upb_Message_ClearBaseField(UPB_UPCAST(msg), optional_string_field);
  EXPECT_EQ(false,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_string_field));
  EXPECT_EQ(
      false,
      protobuf_test_messages_proto2_TestAllTypesProto2_has_optional_string(
          msg));
  upb_Message_SetBaseFieldString(UPB_UPCAST(msg), optional_string_field,
                                 upb_StringView_FromString(kTestStr2));
  EXPECT_EQ(true,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_string_field));
  EXPECT_EQ(
      true,
      protobuf_test_messages_proto2_TestAllTypesProto2_has_optional_string(
          msg));
  value = protobuf_test_messages_proto2_TestAllTypesProto2_optional_string(msg);
  read_value = std::string(value.data, value.size);
  EXPECT_EQ(kTestStr2, read_value);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, SubMessage) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  const upb_MiniTableField* optional_message_field =
      find_proto2_field(kFieldOptionalNestedMessage);

  const upb_Message* test_message =
      upb_Message_GetMessage(UPB_UPCAST(msg), optional_message_field);
  EXPECT_EQ(nullptr, test_message);

  EXPECT_EQ(false,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_message_field));

  // Get mutable using C API.
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* nested_message =
      protobuf_test_messages_proto2_TestAllTypesProto2_mutable_optional_nested_message(
          msg, arena);
  EXPECT_EQ(true, nested_message != nullptr);
  EXPECT_EQ(true,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_message_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(
      nested_message, 5);

  // Read back using mini table API.
  const upb_Message* sub_message =
      upb_Message_GetMessage(UPB_UPCAST(msg), optional_message_field);
  EXPECT_EQ(true, sub_message != nullptr);

  const upb_MiniTableField* nested_message_a_field =
      upb_MiniTable_FindFieldByNumber(
          &protobuf_0test_0messages__proto2__TestAllTypesProto2__NestedMessage_msg_init,
          kFieldOptionalNestedMessageA);
  EXPECT_EQ(5, upb_Message_GetInt32(sub_message, nested_message_a_field, 0));

  upb_Message_ClearBaseField(UPB_UPCAST(msg), optional_message_field);
  EXPECT_EQ(
      nullptr,
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_nested_message(
          msg));
  EXPECT_EQ(false,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_message_field));

  upb_Message* new_nested_message = UPB_UPCAST(
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(
          arena));
  upb_Message_SetBaseFieldInt32(new_nested_message, nested_message_a_field,
                                123);
  upb_Message_SetMessage(UPB_UPCAST(msg), optional_message_field,
                         new_nested_message);

  upb_Message* mutable_message = upb_Message_GetOrCreateMutableMessage(
      UPB_UPCAST(msg),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      optional_message_field, arena);
  EXPECT_EQ(
      true,
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_nested_message(
          msg) != nullptr);
  EXPECT_EQ(true,
            upb_Message_HasBaseField(UPB_UPCAST(msg), optional_message_field));
  EXPECT_EQ(123,
            upb_Message_GetInt32(mutable_message, nested_message_a_field, 0));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, RepeatedScalar) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  const upb_MiniTableField* repeated_int32_field =
      find_proto2_field(kFieldOptionalRepeatedInt32);

  size_t len;
  const int32_t* arr =
      protobuf_test_messages_proto2_TestAllTypesProto2_repeated_int32(msg,
                                                                      &len);
  // Test Get/Set Array values, validate with C API.
  EXPECT_EQ(0, len);
  EXPECT_EQ(nullptr, arr);
  EXPECT_EQ(nullptr,
            upb_Message_GetArray(UPB_UPCAST(msg), repeated_int32_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_resize_repeated_int32(
      msg, 10, arena);
  int32_t* mutable_values =
      protobuf_test_messages_proto2_TestAllTypesProto2_mutable_repeated_int32(
          msg, &len);
  mutable_values[5] = 123;
  const upb_Array* readonly_arr =
      upb_Message_GetArray(UPB_UPCAST(msg), repeated_int32_field);
  EXPECT_EQ(123, upb_Array_Get(readonly_arr, 5).int32_val);

  upb_MessageValue new_value;
  new_value.int32_val = 567;
  upb_Array* mutable_array =
      upb_Message_GetMutableArray(UPB_UPCAST(msg), repeated_int32_field);
  upb_Array_Set(mutable_array, 5, new_value);
  EXPECT_EQ(new_value.int32_val,
            protobuf_test_messages_proto2_TestAllTypesProto2_repeated_int32(
                msg, &len)[5]);

  // Test resize.
  bool result = upb_Array_Resize(mutable_array, 20, arena);
  EXPECT_EQ(true, result);
  upb_Array_Set(mutable_array, 19, new_value);
  EXPECT_EQ(new_value.int32_val,
            protobuf_test_messages_proto2_TestAllTypesProto2_repeated_int32(
                msg, &len)[19]);
  upb_Array_Resize(mutable_array, 0, arena);
  const int32_t* zero_length_array =
      protobuf_test_messages_proto2_TestAllTypesProto2_repeated_int32(msg,
                                                                      &len);
  EXPECT_EQ(0, len);
  EXPECT_EQ(true, zero_length_array != nullptr);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, RepeatedMessage) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  const upb_MiniTableField* repeated_nested_message_field =
      find_proto2_field(kFieldOptionalRepeatedNestedMessage);
  upb_Message* msg1 = (upb_Message*)
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(arena);
  upb_Message* msg2 = (upb_Message*)
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(arena);

  upb_Array* array = upb_Message_GetOrCreateMutableArray(
      UPB_UPCAST(msg), repeated_nested_message_field, arena);
  upb_MessageValue val1;
  val1.msg_val = msg1;
  upb_MessageValue val2;
  val2.msg_val = msg2;
  EXPECT_TRUE(upb_Array_Append(array, val1, arena));
  EXPECT_TRUE(upb_Array_Append(array, val2, arena));

  EXPECT_EQ(msg1, upb_Array_GetMutable(array, 0));
  EXPECT_EQ(msg1, upb_Array_Get(array, 0).msg_val);
  EXPECT_EQ(msg2, upb_Array_GetMutable(array, 1));
  EXPECT_EQ(msg2, upb_Array_Get(array, 1).msg_val);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, GetMutableMessage) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  // Message.
  const upb_MiniTableField* optional_message_field =
      find_proto2_field(kFieldOptionalNestedMessage);
  upb_Message* msg1 = upb_Message_GetOrCreateMutableMessage(
      UPB_UPCAST(msg),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      optional_message_field, arena);
  upb_Message* msg2 = upb_Message_GetOrCreateMutableMessage(
      UPB_UPCAST(msg),
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
      optional_message_field, arena);
  // Verify that newly constructed sub message is stored in msg.
  EXPECT_EQ(msg1, msg2);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, EnumClosedCheck) {
  upb_Arena* arena = upb_Arena_New();

  upb::MtDataEncoder e;
  e.StartMessage(0);
  e.PutField(kUpb_FieldType_Int32, 4, 0);
  e.PutField(kUpb_FieldType_Enum, 5, 0);

  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTable* table =
      upb_MiniTable_Build(e.data().data(), e.data().size(), arena, &status);

  const upb_MiniTableField* enumField = &table->UPB_PRIVATE(fields)[1];
  EXPECT_EQ(upb_MiniTableField_Type(enumField), kUpb_FieldType_Enum);
  EXPECT_FALSE(upb_MiniTableField_IsClosedEnum(enumField));

  upb::MtDataEncoder e2;
  e2.StartMessage(0);
  e2.PutField(kUpb_FieldType_Int32, 4, 0);
  e2.PutField(kUpb_FieldType_Enum, 6, kUpb_FieldModifier_IsClosedEnum);

  upb_Status_Clear(&status);
  table =
      upb_MiniTable_Build(e2.data().data(), e2.data().size(), arena, &status);

  const upb_MiniTableField* closedEnumField = &table->UPB_PRIVATE(fields)[1];
  EXPECT_EQ(upb_MiniTableField_Type(closedEnumField), kUpb_FieldType_Enum);
  EXPECT_TRUE(upb_MiniTableField_IsClosedEnum(closedEnumField));
  upb_Arena_Free(arena);
}

TEST(GeneratedCode, OneofClear) {
  upb_Arena* arena = upb_Arena_New();

  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  const upb_MiniTable* table =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

  // oneof_uint32
  const upb_MiniTableField* oneofField =
      upb_MiniTable_FindFieldByNumber(table, 111);
  EXPECT_TRUE(upb_MiniTableField_IsInOneof(oneofField));
  protobuf_test_messages_proto2_TestAllTypesProto2_set_oneof_uint32(msg, 522);
  EXPECT_TRUE(
      protobuf_test_messages_proto2_TestAllTypesProto2_has_oneof_uint32(msg));

  upb_Message_ClearOneof((upb_Message*)msg, table, oneofField);
  EXPECT_FALSE(
      protobuf_test_messages_proto2_TestAllTypesProto2_has_oneof_uint32(msg));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, OneofAccess) {
  upb_Arena* arena = upb_Arena_New();

  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  const upb_MiniTable* table =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

  // oneof_uint32
  const upb_MiniTableField* oneofField =
      upb_MiniTable_FindFieldByNumber(table, 111);
  EXPECT_TRUE(upb_MiniTableField_IsInOneof(oneofField));

  const upb_MiniTableField* oneOfFirstFetch =
      upb_Message_WhichOneof((upb_Message*)msg, table, oneofField);
  // one of not set, so should initially yield nullptr
  EXPECT_EQ(oneOfFirstFetch, nullptr);

  protobuf_test_messages_proto2_TestAllTypesProto2_set_oneof_uint32(msg, 522);
  const upb_MiniTableField* oneOfSecondFetch =
      upb_Message_WhichOneof((upb_Message*)msg, table, oneofField);
  // this oneof has now been set, so should yield the MiniTableField
  EXPECT_EQ(oneOfSecondFetch, oneofField);

  upb_Arena_Free(arena);
}

}  // namespace
