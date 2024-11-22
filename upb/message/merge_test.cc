#include "upb/message/merge.h"

#include <cstdint>
#include <cstring>

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto2.upb.h"
#include "google/protobuf/test_messages_proto2.upb_minitable.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

namespace {

// Proto2 test messages field numbers used for reflective access.
const uint32_t kFieldOptionalInt32 = 1;
const uint32_t kFieldOptionalString = 14;

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
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  EXPECT_TRUE(
      (protobuf_test_messages_proto2_TestAllTypesProto2*)upb_Message_MergeFrom(
          UPB_UPCAST(clone), UPB_UPCAST(msg),
          &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init,
          nullptr, arena));
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

}  // namespace
