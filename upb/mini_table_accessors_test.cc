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

#include "upb/mini_table_accessors.h"

#include "gtest/gtest.h"
#include "google/protobuf/test_messages_proto2.upb.h"
#include "google/protobuf/test_messages_proto3.upb.h"
#include "upb/array.h"
#include "upb/mini_table.h"
#include "upb/test.upb.h"

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

const upb_MiniTable_Field* find_proto3_field(int field_number) {
  return upb_MiniTable_FindFieldByNumber(
      &protobuf_test_messages_proto3_TestAllTypesProto3_msginit, field_number);
}

const upb_MiniTable_Field* find_proto2_field(int field_number) {
  return upb_MiniTable_FindFieldByNumber(
      &protobuf_test_messages_proto2_TestAllTypesProto2_msginit, field_number);
}

TEST(GeneratedCode, HazzersProto2) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  // Scalar/Boolean.
  const upb_MiniTable_Field* optional_bool_field =
      find_proto2_field(kFieldOptionalBool);
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_bool_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_bool(msg, true);
  EXPECT_EQ(true, upb_MiniTable_HasField(msg, optional_bool_field));
  upb_MiniTable_ClearField(msg, optional_bool_field);
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_bool_field));
  EXPECT_EQ(
      false,
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_bool(msg));

  // String.
  const upb_MiniTable_Field* optional_string_field =
      find_proto2_field(kFieldOptionalString);
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_string_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_string(
      msg, upb_StringView_FromString(kTestStr1));
  EXPECT_EQ(true, upb_MiniTable_HasField(msg, optional_string_field));
  EXPECT_EQ(
      strlen(kTestStr1),
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_string(msg)
          .size);
  upb_MiniTable_ClearField(msg, optional_string_field);
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_string_field));
  EXPECT_EQ(
      0, protobuf_test_messages_proto2_TestAllTypesProto2_optional_string(msg)
             .size);

  // Message.
  const upb_MiniTable_Field* optional_message_field =
      find_proto2_field(kFieldOptionalNestedMessage);
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_message_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_mutable_optional_nested_message(
      msg, arena);
  EXPECT_EQ(true, upb_MiniTable_HasField(msg, optional_message_field));
  upb_MiniTable_ClearField(msg, optional_message_field);
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_message_field));
  EXPECT_EQ(
      true,
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_nested_message(
          msg) == nullptr);

  // One of.
  const upb_MiniTable_Field* optional_oneof_uint32_field =
      find_proto2_field(kFieldOptionalOneOfUInt32);
  const upb_MiniTable_Field* optional_oneof_string_field =
      find_proto2_field(kFieldOptionalOneOfString);

  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_oneof_uint32_field));
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_oneof_string_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_set_oneof_uint32(msg, 123);
  EXPECT_EQ(true, upb_MiniTable_HasField(msg, optional_oneof_uint32_field));
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_oneof_string_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_set_oneof_string(
      msg, upb_StringView_FromString(kTestStr1));
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_oneof_uint32_field));
  EXPECT_EQ(true, upb_MiniTable_HasField(msg, optional_oneof_string_field));
  upb_MiniTable_ClearField(msg, optional_oneof_uint32_field);
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_oneof_uint32_field));
  EXPECT_EQ(true, upb_MiniTable_HasField(msg, optional_oneof_string_field));
  upb_MiniTable_ClearField(msg, optional_oneof_string_field);
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_oneof_uint32_field));
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_oneof_string_field));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, ScalarsProto2) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  const upb_MiniTable_Field* optional_int32_field =
      find_proto2_field(kFieldOptionalInt32);

  EXPECT_EQ(
      0, protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(msg));

  EXPECT_EQ(0, upb_MiniTable_GetInt32(msg, optional_int32_field));
  upb_MiniTable_SetInt32(msg, optional_int32_field, kTestInt32);
  EXPECT_EQ(true, upb_MiniTable_HasField(msg, optional_int32_field));
  EXPECT_EQ(kTestInt32, upb_MiniTable_GetInt32(msg, optional_int32_field));
  EXPECT_EQ(
      kTestInt32,
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_int32(msg));

  const upb_MiniTable_Field* optional_uint32_field =
      find_proto2_field(kFieldOptionalUInt32);

  EXPECT_EQ(
      0, protobuf_test_messages_proto2_TestAllTypesProto2_optional_uint32(msg));
  EXPECT_EQ(0, upb_MiniTable_GetUInt32(msg, optional_uint32_field));
  upb_MiniTable_SetUInt32(msg, optional_uint32_field, kTestUInt32);
  EXPECT_EQ(kTestUInt32, upb_MiniTable_GetUInt32(msg, optional_uint32_field));
  EXPECT_EQ(
      kTestUInt32,
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_uint32(msg));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, ScalarProto3) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto3_TestAllTypesProto3* msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena);

  const upb_MiniTable_Field* optional_int64_field =
      find_proto3_field(kFieldProto3OptionalInt64);
  const upb_MiniTable_Field* optional_uint64_field =
      find_proto3_field(kFieldProto3OptionalUInt64);

  EXPECT_EQ(
      0, protobuf_test_messages_proto3_TestAllTypesProto3_optional_int64(msg));
  upb_MiniTable_SetInt64(msg, optional_int64_field, -1);
  EXPECT_EQ(
      -1, protobuf_test_messages_proto3_TestAllTypesProto3_optional_int64(msg));
  EXPECT_EQ(-1, upb_MiniTable_GetInt64(msg, optional_int64_field));

  EXPECT_EQ(
      0, protobuf_test_messages_proto3_TestAllTypesProto3_optional_uint64(msg));
  upb_MiniTable_SetUInt64(msg, optional_uint64_field, kTestUInt64);
  EXPECT_EQ(
      kTestUInt64,
      protobuf_test_messages_proto3_TestAllTypesProto3_optional_uint64(msg));
  EXPECT_EQ(kTestUInt64, upb_MiniTable_GetUInt64(msg, optional_uint64_field));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, Strings) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  const upb_MiniTable_Field* optional_string_field =
      find_proto2_field(kFieldOptionalString);

  // Test default.
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_string_field));
  // Test read after write using C.
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_string(
      msg, upb_StringView_FromString(kTestStr1));
  EXPECT_EQ(true, upb_MiniTable_HasField(msg, optional_string_field));
  upb_StringView value = upb_MiniTable_GetString(msg, optional_string_field);
  std::string read_value = std::string(value.data, value.size);
  EXPECT_EQ(kTestStr1, read_value);
  // Clear.
  upb_MiniTable_ClearField(msg, optional_string_field);
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_string_field));
  EXPECT_EQ(
      false,
      protobuf_test_messages_proto2_TestAllTypesProto2_has_optional_string(
          msg));
  upb_MiniTable_SetString(msg, optional_string_field,
                          upb_StringView_FromString(kTestStr2));
  EXPECT_EQ(true, upb_MiniTable_HasField(msg, optional_string_field));
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

  const upb_MiniTable_Field* optional_message_field =
      find_proto2_field(kFieldOptionalNestedMessage);

  const upb_Message* test_message =
      upb_MiniTable_GetMessage(msg, optional_message_field);
  EXPECT_EQ(NULL, test_message);

  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_message_field));

  // Get mutable using C API.
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage* nested_message =
      protobuf_test_messages_proto2_TestAllTypesProto2_mutable_optional_nested_message(
          msg, arena);
  EXPECT_EQ(true, nested_message != nullptr);
  EXPECT_EQ(true, upb_MiniTable_HasField(msg, optional_message_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_set_a(
      nested_message, 5);

  // Read back using mini table API.
  const upb_Message* sub_message =
      upb_MiniTable_GetMessage(msg, optional_message_field);
  EXPECT_EQ(true, sub_message != NULL);

  const upb_MiniTable_Field* nested_message_a_field =
      upb_MiniTable_FindFieldByNumber(
          &protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_msginit,
          kFieldOptionalNestedMessageA);
  EXPECT_EQ(5, upb_MiniTable_GetInt32(sub_message, nested_message_a_field));

  upb_MiniTable_ClearField(msg, optional_message_field);
  EXPECT_EQ(
      NULL,
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_nested_message(
          msg));
  EXPECT_EQ(false, upb_MiniTable_HasField(msg, optional_message_field));

  upb_Message* new_nested_message =
      protobuf_test_messages_proto2_TestAllTypesProto2_NestedMessage_new(arena);
  upb_MiniTable_SetInt32(new_nested_message, nested_message_a_field, 123);
  upb_MiniTable_SetMessage(msg, optional_message_field, new_nested_message);

  upb_Message* mutable_message = upb_MiniTable_GetMutableMessage(
      msg, &protobuf_test_messages_proto2_TestAllTypesProto2_msginit,
      optional_message_field, arena);
  EXPECT_EQ(
      true,
      protobuf_test_messages_proto2_TestAllTypesProto2_optional_nested_message(
          msg) != NULL);
  EXPECT_EQ(true, upb_MiniTable_HasField(msg, optional_message_field));
  EXPECT_EQ(123,
            upb_MiniTable_GetInt32(mutable_message, nested_message_a_field));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, RepeatedScalar) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  const upb_MiniTable_Field* repeated_int32_field =
      find_proto2_field(kFieldOptionalRepeatedInt32);

  size_t len;
  const int32_t* arr =
      protobuf_test_messages_proto2_TestAllTypesProto2_repeated_int32(msg,
                                                                      &len);
  // Test Get/Set Array values, validate with C API.
  EXPECT_EQ(0, len);
  EXPECT_EQ(NULL, arr);
  EXPECT_EQ(NULL, upb_MiniTable_GetArray(msg, repeated_int32_field));
  protobuf_test_messages_proto2_TestAllTypesProto2_resize_repeated_int32(
      msg, 10, arena);
  int32_t* mutable_values =
      protobuf_test_messages_proto2_TestAllTypesProto2_mutable_repeated_int32(
          msg, &len);
  mutable_values[5] = 123;
  const upb_Array* readonly_arr =
      upb_MiniTable_GetArray(msg, repeated_int32_field);
  EXPECT_EQ(123, upb_Array_Get(readonly_arr, 5).int32_val);

  upb_MessageValue new_value;
  new_value.int32_val = 567;
  upb_Array* mutable_array =
      upb_MiniTable_GetMutableArray(msg, repeated_int32_field);
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
  EXPECT_EQ(true, zero_length_array != NULL);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, GetMutableMessage) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  // Message.
  const upb_MiniTable_Field* optional_message_field =
      find_proto2_field(kFieldOptionalNestedMessage);
  upb_Message* msg1 = upb_MiniTable_GetMutableMessage(
      msg, &protobuf_test_messages_proto2_TestAllTypesProto2_msginit,
      optional_message_field, arena);
  upb_Message* msg2 = upb_MiniTable_GetMutableMessage(
      msg, &protobuf_test_messages_proto2_TestAllTypesProto2_msginit,
      optional_message_field, arena);
  // Verify that newly constructed sub message is stored in msg.
  EXPECT_EQ(msg1, msg2);

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

  upb_test_ModelExtension1_set_model_ext(msg, extension1, arena);
  upb_test_ModelExtension2_set_model_ext(msg, extension2, arena);

  size_t serialized_size;
  char* serialized =
      upb_test_ModelWithExtensions_serialize(msg, arena, &serialized_size);

  // Test known GetExtension
  const upb_Message_Extension* upb_ext2;
  upb_GetExtension_Status promote_status = upb_MiniTable_GetOrPromoteExtension(
      msg, &upb_test_ModelExtension2_model_ext_ext, 0, arena, &upb_ext2);

  upb_test_ModelExtension2* ext2 =
      (upb_test_ModelExtension2*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(5, upb_test_ModelExtension2_i(ext2));

  upb_test_EmptyMessageWithExtensions* base_msg =
      upb_test_EmptyMessageWithExtensions_parse(serialized, serialized_size,
                                                arena);

  // Get unknown extension bytes before promotion.
  const char* extension_data;
  size_t len;
  upb_GetExtensionAsBytes_Status status = status =
      upb_MiniTable_GetExtensionAsBytes(base_msg,
                                        &upb_test_ModelExtension2_model_ext_ext,
                                        0, arena, &extension_data, &len);
  EXPECT_EQ(kUpb_GetExtensionAsBytes_Ok, status);
  EXPECT_EQ(0x48, extension_data[0]);
  EXPECT_EQ(5, extension_data[1]);

  // Test unknown GetExtension.
  promote_status = upb_MiniTable_GetOrPromoteExtension(
      base_msg, &upb_test_ModelExtension2_model_ext_ext, 0, arena, &upb_ext2);

  ext2 = (upb_test_ModelExtension2*)upb_ext2->data.ptr;
  EXPECT_EQ(kUpb_GetExtension_Ok, promote_status);
  EXPECT_EQ(5, upb_test_ModelExtension2_i(ext2));

  // Get unknown extension bytes after promotion.
  status = upb_MiniTable_GetExtensionAsBytes(
      base_msg, &upb_test_ModelExtension2_model_ext_ext, 0, arena,
      &extension_data, &len);
  EXPECT_EQ(kUpb_GetExtensionAsBytes_Ok, status);
  EXPECT_EQ(0x48, extension_data[0]);
  EXPECT_EQ(5, extension_data[1]);

  upb_Arena_Free(arena);
}

}  // namespace
