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
#include "upb/mini_table/extension.h"
#include "upb/test/test.upb.h"
#include "upb/test/test.upb_minitable.h"
#include "upb/wire/decode.h"

// Must be last.
#include "upb/port/def.inc"

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

}  // namespace
