// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/* Test of unknown fields APIs.
 */

#include "upb/message/unknown_fields.h"

#include <string.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/message/test.upb.h"
#include "upb/message/test.upb_minitable.h"
#include "upb/mini_table/extension.h"
#include "upb/test/test.upb.h"
#include "upb/test/test.upb_minitable.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode_extension.h"

// Must be last.
#include "upb/port/def.inc"
#include "upb/wire/encode.h"

namespace {

TEST(GeneratedCode, FindUnknown2) {
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

  // Case 1: Find raw bytes unknown
  upb_FindUnknownRet2 result = upb_Message_FindUnknown2(
      UPB_UPCAST(base_msg),
      upb_MiniTableExtension_Number(upb_test_ModelExtension1_model_ext_ext), 0);
  EXPECT_EQ(kUpb_FindUnknown_Ok, result.status);
  EXPECT_EQ(kUpb_MessageUnknownType_StringView, result.unknown.type);

  // Case 2: Find non-canonical extension
  upb_test_ModelExtension2* extension2 = upb_test_ModelExtension2_new(arena);
  upb_test_ModelExtension2_set_i(extension2, 42);

  bool set_ext_ok = UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(base_msg), upb_test_ModelExtension2_model_ext_ext, &extension2,
      arena);
  EXPECT_TRUE(set_ext_ok);

  result = upb_Message_FindUnknown2(
      UPB_UPCAST(base_msg),
      upb_MiniTableExtension_Number(upb_test_ModelExtension2_model_ext_ext), 0);
  EXPECT_EQ(kUpb_FindUnknown_Ok, result.status);
  EXPECT_EQ(kUpb_MessageUnknownType_NonCanonicalExtension, result.unknown.type);

  // Case 3: Not Present
  result = upb_Message_FindUnknown2(UPB_UPCAST(base_msg), 12345, 0);
  EXPECT_EQ(kUpb_FindUnknown_NotPresent, result.status);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, FindUnknown2_NoUnknowns) {
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg = upb_test_ModelWithExtensions_new(arena);

  upb_FindUnknownRet2 result =
      upb_Message_FindUnknown2(UPB_UPCAST(msg), 123, 0);
  EXPECT_EQ(kUpb_FindUnknown_NotPresent, result.status);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, FindUnknown2_MultipleMatchingValues) {
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg = upb_test_ModelWithExtensions_new(arena);
  upb_test_ModelWithExtensions_set_random_int32(msg, 10);

  upb_test_ModelExtension1* extension1 = upb_test_ModelExtension1_new(arena);
  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("World"));

  upb_test_ModelExtension1_set_model_ext(msg, extension1, arena);

  size_t serialized_size;
  char* serialized =
      upb_test_ModelWithExtensions_serialize(msg, arena, &serialized_size);

  uint32_t field_number =
      upb_MiniTableExtension_Number(upb_test_ModelExtension1_model_ext_ext);

  // Case A: StringView first, then NonCanonicalExtension
  upb_test_EmptyMessageWithExtensions* base_msg =
      upb_test_EmptyMessageWithExtensions_parse(serialized, serialized_size,
                                                arena);

  upb_test_ModelExtension1* extension1_non_canonical =
      upb_test_ModelExtension1_new(arena);
  upb_test_ModelExtension1_set_str(extension1_non_canonical,
                                   upb_StringView_FromString("NonCanonical"));

  bool set_ext_ok = UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(base_msg), upb_test_ModelExtension1_model_ext_ext,
      &extension1_non_canonical, arena);
  EXPECT_TRUE(set_ext_ok);

  upb_FindUnknownRet2 result =
      upb_Message_FindUnknown2(UPB_UPCAST(base_msg), field_number, 0);
  EXPECT_EQ(kUpb_FindUnknown_Ok, result.status);
  EXPECT_EQ(kUpb_MessageUnknownType_StringView, result.unknown.type);

  // Case B: NonCanonicalExtension first, then StringView
  upb_test_EmptyMessageWithExtensions* base_msg2 =
      upb_test_EmptyMessageWithExtensions_new(arena);

  bool set_ext_ok2 = UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(base_msg2), upb_test_ModelExtension1_model_ext_ext,
      &extension1_non_canonical, arena);
  EXPECT_TRUE(set_ext_ok2);

  upb_DecodeStatus decode_status = upb_Decode(
      serialized, serialized_size, UPB_UPCAST(base_msg2),
      &upb_0test__EmptyMessageWithExtensions_msg_init, nullptr, 0, arena);
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);

  result = upb_Message_FindUnknown2(UPB_UPCAST(base_msg2), field_number, 0);
  EXPECT_EQ(kUpb_FindUnknown_Ok, result.status);
  EXPECT_EQ(kUpb_MessageUnknownType_NonCanonicalExtension, result.unknown.type);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, HasUnknownIgnoresTombstones) {
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg = upb_test_ModelWithExtensions_new(arena);

  // Add a non-canonical extension
  upb_test_ModelExtension1* extension1 = upb_test_ModelExtension1_new(arena);
  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("World"));
  bool set_ext_ok = UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg), upb_test_ModelExtension1_model_ext_ext, &extension1,
      arena);
  ASSERT_TRUE(set_ext_ok);

  // Verify HasUnknown returns true (non-canonical are treated as unknowns)
  EXPECT_TRUE(upb_Message_HasUnknown(UPB_UPCAST(msg)));

  upb_Message_Internal* in =
      UPB_PRIVATE(_upb_Message_GetInternal)(UPB_UPCAST(msg));
  ASSERT_NE(in, nullptr);
  uint32_t original_size = in->size;
  EXPECT_GT(original_size, 0);

  // Delete it using DeleteUnknown2 to create a tombstone
  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_MessageUnknown unknown;
  ASSERT_TRUE(upb_Message_NextUnknown2(UPB_UPCAST(msg), &unknown, &iter));
  upb_Message_DeleteUnknownStatus status =
      upb_Message_DeleteUnknown2(UPB_UPCAST(msg), &unknown, &iter, arena);
  EXPECT_EQ(status, kUpb_DeleteUnknown_DeletedLast);

  // Verify HasUnknown now returns false (ignores tombstone)
  EXPECT_FALSE(upb_Message_HasUnknown(UPB_UPCAST(msg)));

  // Size should STILL be original_size (contains tombstone)
  EXPECT_EQ(in->size, original_size);

  // Call _upb_Message_DiscardUnknown_shallow
  _upb_Message_DiscardUnknown_shallow(UPB_UPCAST(msg));

  // Size should now be 0
  EXPECT_EQ(in->size, 0);

  // Verify HasUnknown still returns false
  EXPECT_FALSE(upb_Message_HasUnknown(UPB_UPCAST(msg)));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, HasUnknownMultipleUnknownsDeleteOne) {
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg = upb_test_ModelWithExtensions_new(arena);

  // Add non-canonical extension 1
  upb_test_ModelExtension1* extension1 = upb_test_ModelExtension1_new(arena);
  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("Ext1"));
  bool set_ext1_ok = UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg), upb_test_ModelExtension1_model_ext_ext, &extension1,
      arena);
  ASSERT_TRUE(set_ext1_ok);

  // Add non-canonical extension 2
  upb_test_ModelExtension2* extension2 = upb_test_ModelExtension2_new(arena);
  upb_test_ModelExtension2_set_i(extension2, 42);
  bool set_ext2_ok = UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg), upb_test_ModelExtension2_model_ext_ext, &extension2,
      arena);
  ASSERT_TRUE(set_ext2_ok);

  // Verify HasUnknown returns true
  EXPECT_TRUE(upb_Message_HasUnknown(UPB_UPCAST(msg)));

  // Delete the first unknown
  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_MessageUnknown unknown;
  ASSERT_TRUE(upb_Message_NextUnknown2(UPB_UPCAST(msg), &unknown, &iter));
  upb_Message_DeleteUnknownStatus status =
      upb_Message_DeleteUnknown2(UPB_UPCAST(msg), &unknown, &iter, arena);

  // Deleting the first unknown should return IterUpdated
  EXPECT_EQ(status, kUpb_DeleteUnknown_IterUpdated);

  // Verify HasUnknown STILL returns true because one unknown remains
  EXPECT_TRUE(upb_Message_HasUnknown(UPB_UPCAST(msg)));

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MessageUnknown_Encode_NonCanonicalExtension) {
  upb_Arena* arena = upb_Arena_New();

  upb_test_ModelWithExtensions* msg = upb_test_ModelWithExtensions_new(arena);
  upb_test_ModelExtension2* extension2 = upb_test_ModelExtension2_new(arena);
  upb_test_ModelExtension2_set_i(extension2, 42);

  bool set_ext_ok = UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg), upb_test_ModelExtension2_model_ext_ext, &extension2,
      arena);
  EXPECT_TRUE(set_ext_ok);

  upb_FindUnknownRet2 result = upb_Message_FindUnknown2(
      UPB_UPCAST(msg),
      upb_MiniTableExtension_Number(upb_test_ModelExtension2_model_ext_ext), 0);
  EXPECT_EQ(kUpb_FindUnknown_Ok, result.status);
  EXPECT_EQ(kUpb_MessageUnknownType_NonCanonicalExtension, result.unknown.type);

  upb_StringView view;
  upb_EncodeStatus status =
      upb_EncodeExtension(result.unknown.value.extension, arena, &view,
                          /*encode_options=*/0);
  EXPECT_EQ(kUpb_EncodeStatus_Ok, status);
  EXPECT_GT(view.size, 0);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, MessageUnknown_Encode_NonCanonicalMessageSetExtension) {
  upb_Arena* arena = upb_Arena_New();

  upb_test_TestMessageSet* mset = upb_test_TestMessageSet_new(arena);
  upb_test_MessageSetMember* member = upb_test_MessageSetMember_new(arena);
  upb_test_MessageSetMember_set_optional_int32(member, 42);

  bool set_ext_ok = UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(mset), upb_test_MessageSetMember_message_set_extension_ext,
      &member, arena);
  EXPECT_TRUE(set_ext_ok);

  upb_FindUnknownRet2 result = upb_Message_FindUnknown2(
      UPB_UPCAST(mset),
      upb_MiniTableExtension_Number(
          upb_test_MessageSetMember_message_set_extension_ext),
      0);
  EXPECT_EQ(kUpb_FindUnknown_Ok, result.status);
  EXPECT_EQ(kUpb_MessageUnknownType_NonCanonicalExtension, result.unknown.type);

  upb_StringView view;
  upb_EncodeStatus status =
      upb_EncodeExtension(result.unknown.value.extension, arena, &view,
                          /*encode_options=*/0);
  EXPECT_EQ(kUpb_EncodeStatus_Ok, status);
  EXPECT_GT(view.size, 0);

  upb_Arena_Free(arena);
}

TEST(GeneratedCode, NextWireFormatUnknown) {
  upb_Arena* arena = upb_Arena_New();

  upb_test_ModelWithExtensions* msg = upb_test_ModelWithExtensions_new(arena);

  // Add a raw unknown field string view
  const char raw_bytes[] =
      "\x08\x96\x01";  // tag 1 (field 1, varint), value 150
  ASSERT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      UPB_UPCAST(msg), raw_bytes, 3, arena, kUpb_AddUnknown_Copy));

  // Add non-canonical extension
  upb_test_ModelExtension2* extension2 = upb_test_ModelExtension2_new(arena);
  upb_test_ModelExtension2_set_i(extension2, 42);
  bool set_ext_ok = UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg), upb_test_ModelExtension2_model_ext_ext, &extension2,
      arena);
  EXPECT_TRUE(set_ext_ok);

  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_StringView view;
  upb_Arena* enc_arena = nullptr;

  // First unknown should be raw string view (enc_arena remains nullptr)
  EXPECT_TRUE(upb_Message_NextWireFormatUnknown(UPB_UPCAST(msg), &enc_arena,
                                                &view, &iter));
  EXPECT_EQ(view.size, 3);
  EXPECT_EQ(memcmp(view.data, raw_bytes, 3), 0);
  EXPECT_EQ(enc_arena, nullptr);

  // Second unknown should be auto-encoded non-canonical extension (enc_arena
  // lazily created)
  EXPECT_TRUE(upb_Message_NextWireFormatUnknown(UPB_UPCAST(msg), &enc_arena,
                                                &view, &iter));
  EXPECT_GT(view.size, 0);
  EXPECT_NE(enc_arena, nullptr);

  // No more unknowns
  EXPECT_FALSE(upb_Message_NextWireFormatUnknown(UPB_UPCAST(msg), &enc_arena,
                                                 &view, &iter));

  if (enc_arena) upb_Arena_Free(enc_arena);
  upb_Arena_Free(arena);
}

}  // namespace
