#include "upb/wire/encode.h"

#include <setjmp.h>
#include <stddef.h>

#include <gtest/gtest.h>
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/map_sorter.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode_test.upb.h"
#include "upb/wire/internal/encoder.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace {

static void DoEncodeFieldMaxDepthExceeded(jmp_buf err, upb_encstate& e,
                                          upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          char*& buf, size_t& size) {
  if (UPB_SETJMP(err) == 0) {
    UPB_PRIVATE(_upb_Encode_Field)(&e, msg, field, &buf, &size, e.options);
    FAIL() << "Should have jumped";
  } else {
    EXPECT_EQ(e.status, kUpb_EncodeStatus_MaxDepthExceeded);
  }
}

static void DoEncodeExtensionMaxDepthExceeded(jmp_buf err, upb_encstate& e,
                                              const upb_MiniTableExtension* ext,
                                              upb_MessageValue ext_val,
                                              char*& buf, size_t& size) {
  if (UPB_SETJMP(err) == 0) {
    UPB_PRIVATE(_upb_Encode_Extension)(&e, ext, ext_val, false, &buf, &size,
                                       e.options);
    FAIL() << "Should have jumped";
  } else {
    EXPECT_EQ(e.status, kUpb_EncodeStatus_MaxDepthExceeded);
  }
}

TEST(EncodeTest, EncodeFieldSuccess) {
  // Set up a message with a single int32 field.
  upb_Arena* arena = upb_Arena_New();
  upb_wire_test_TestInt32* msg = upb_wire_test_TestInt32_new(arena);
  upb_wire_test_TestInt32_set_i32(msg, 42);

  upb_encstate e;
  jmp_buf err;
  UPB_PRIVATE(_upb_encstate_init)(&e, &err, arena);

  // Encode field.
  const upb_MiniTable* mt = &upb_0wire_0test__TestInt32_msg_init;
  const upb_MiniTableField* field = upb_MiniTable_FindFieldByNumber(mt, 1);
  char* buf = e.alloc.limit;
  size_t size;
  upb_EncodeStatus status = UPB_PRIVATE(_upb_Encode_Field)(
      &e, (upb_Message*)msg, field, &buf, &size, e.options);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);
  EXPECT_GT(size, 0u);

  // Verify that the encoded field can be decoded back to the original message
  // with the same value.
  upb_wire_test_TestInt32* decoded_msg = upb_wire_test_TestInt32_new(arena);
  upb_DecodeStatus decode_status =
      upb_Decode(buf, size, (upb_Message*)decoded_msg, mt, nullptr, 0, arena);
  EXPECT_EQ(decode_status, kUpb_DecodeStatus_Ok);
  EXPECT_EQ(upb_wire_test_TestInt32_i32(decoded_msg), 42);

  _upb_mapsorter_destroy(&e.sorter);
  upb_Arena_Free(arena);
}

TEST(EncodeTest, EncodeFieldSuccessEmptyMessage) {
  // Set up a message with a single int32 field, but do not set the field value.
  upb_Arena* arena = upb_Arena_New();
  upb_wire_test_TestInt32* msg = upb_wire_test_TestInt32_new(arena);

  upb_encstate e;
  jmp_buf err;
  UPB_PRIVATE(_upb_encstate_init)(&e, &err, arena);

  // Encode empty message field.
  const upb_MiniTable* mt = &upb_0wire_0test__TestInt32_msg_init;
  const upb_MiniTableField* field = upb_MiniTable_FindFieldByNumber(mt, 1);
  char* buf = e.alloc.limit;
  size_t size;
  upb_EncodeStatus status = UPB_PRIVATE(_upb_Encode_Field)(
      &e, (upb_Message*)msg, field, &buf, &size, e.options);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);
  EXPECT_EQ(size, 0u);

  _upb_mapsorter_destroy(&e.sorter);
  upb_Arena_Free(arena);
}

TEST(EncodeTest, EncodeFieldMaxDepthExceeded) {
  upb_Arena* arena = upb_Arena_New();
  upb_wire_test_TestRecursive* msg = upb_wire_test_TestRecursive_new(arena);

  upb_encstate e;
  jmp_buf err;
  UPB_PRIVATE(_upb_encstate_init)(&e, &err, arena);

  upb_wire_test_TestRecursive* sub_msg = upb_wire_test_TestRecursive_new(arena);
  upb_wire_test_TestRecursive_set_recursive(msg, sub_msg);

  const upb_MiniTable* mt = &upb_0wire_0test__TestRecursive_msg_init;
  const upb_MiniTableField* field = upb_MiniTable_FindFieldByNumber(mt, 1);
  char* buf = e.alloc.limit;
  size_t size;
  e.options = upb_EncodeOptions_MaxDepth(1);
  DoEncodeFieldMaxDepthExceeded(err, e, (upb_Message*)msg, field, buf, size);

  _upb_mapsorter_destroy(&e.sorter);
  upb_Arena_Free(arena);
}

TEST(EncodeTest, EncodeExtensionSuccess) {
  upb_Arena* arena = upb_Arena_New();

  upb_encstate e;
  jmp_buf err;
  UPB_PRIVATE(_upb_encstate_init)(&e, &err, arena);

  const upb_MiniTableExtension* ext = upb_wire_test_ext_i32_ext;
  upb_MessageValue ext_val;
  ext_val.int32_val = 42;

  // Encode extension.
  char* buf = e.alloc.limit;
  size_t size;
  upb_EncodeStatus status = UPB_PRIVATE(_upb_Encode_Extension)(
      &e, ext, ext_val, false, &buf, &size, e.options);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);
  EXPECT_GT(size, 0u);

  // Verify that the encoded extension can be decoded back to the original
  // extension value.
  upb_ExtensionRegistry* ext_reg = upb_ExtensionRegistry_New(arena);
  const upb_MiniTableExtension* ext_array[1] = {upb_wire_test_ext_i32_ext};
  upb_ExtensionRegistry_AddArray(ext_reg, ext_array, 1);

  upb_wire_test_TestExtensions* decoded_msg =
      upb_wire_test_TestExtensions_parse_ex(buf, size, ext_reg, 0, arena);
  EXPECT_NE(decoded_msg, nullptr);
  EXPECT_TRUE(upb_wire_test_has_ext_i32(decoded_msg));
  EXPECT_EQ(upb_wire_test_ext_i32(decoded_msg), 42);

  _upb_mapsorter_destroy(&e.sorter);
  upb_Arena_Free(arena);
}

TEST(EncodeTest, EncodeExtensionSuccessEmptyMessage) {
  upb_Arena* arena = upb_Arena_New();

  upb_encstate e;
  jmp_buf err;
  UPB_PRIVATE(_upb_encstate_init)(&e, &err, arena);

  const upb_MiniTableExtension* ext = upb_wire_test_ext_i32_ext;
  // Zero int32 won't be serialized.
  upb_MessageValue ext_val;
  ext_val.int32_val = 0;

  // Encode empty extension.
  char* buf = e.alloc.limit;
  size_t size;
  upb_EncodeStatus status = UPB_PRIVATE(_upb_Encode_Extension)(
      &e, ext, ext_val, false, &buf, &size, e.options);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);
  EXPECT_GT(size, 0u);

  _upb_mapsorter_destroy(&e.sorter);
  upb_Arena_Free(arena);
}

TEST(EncodeTest, EncodeExtensionMaxDepthExceeded) {
  upb_Arena* arena = upb_Arena_New();

  upb_encstate e;
  jmp_buf err;
  UPB_PRIVATE(_upb_encstate_init)(&e, &err, arena);

  const upb_MiniTableExtension* ext = upb_wire_test_ext_recursive_ext;
  upb_MessageValue ext_val;
  ext_val.msg_val = (upb_Message*)upb_wire_test_TestRecursive_new(arena);

  char* buf = e.alloc.limit;
  size_t size;
  e.options = upb_EncodeOptions_MaxDepth(1);
  DoEncodeExtensionMaxDepthExceeded(err, e, ext, ext_val, buf, size);

  _upb_mapsorter_destroy(&e.sorter);
  upb_Arena_Free(arena);
}

TEST(EncodeTest, EncodeNonCanonicalExtensionSuccess) {
  upb_Arena* arena = upb_Arena_New();

  upb_wire_test_TestExtensions* msg = upb_wire_test_TestExtensions_new(arena);

  // Attach scalar extension as non-canonical
  int32_t val = 42;
  UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      (upb_Message*)msg, upb_wire_test_ext_i32_ext, &val, arena);

  // Encode the message.
  char* buf;
  size_t size;
  upb_EncodeStatus status =
      upb_Encode((upb_Message*)msg, &upb_0wire_0test__TestExtensions_msg_init,
                 0, arena, &buf, &size);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);
  EXPECT_GT(size, 0u);

  // Verify that the encoded bytes can be decoded back using the registry!
  upb_ExtensionRegistry* ext_reg = upb_ExtensionRegistry_New(arena);
  const upb_MiniTableExtension* ext_array[1] = {upb_wire_test_ext_i32_ext};
  upb_ExtensionRegistry_AddArray(ext_reg, ext_array, 1);

  upb_wire_test_TestExtensions* decoded_msg =
      upb_wire_test_TestExtensions_parse_ex(buf, size, ext_reg, 0, arena);
  EXPECT_NE(decoded_msg, nullptr);
  EXPECT_TRUE(upb_wire_test_has_ext_i32(decoded_msg));
  EXPECT_EQ(upb_wire_test_ext_i32(decoded_msg), 42);

  upb_Arena_Free(arena);
}

TEST(EncodeTest, SkipUnknownNonCanonicalExtensionSuccess) {
  upb_Arena* arena = upb_Arena_New();

  upb_wire_test_TestExtensions* msg = upb_wire_test_TestExtensions_new(arena);

  // 1. Add a canonical extension (ext_i32, tag 100) to msg
  upb_Extension* canonical_ext = UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
      (upb_Message*)msg, upb_wire_test_ext_i32_ext, arena);
  canonical_ext->data.int32_val = 1000;

  // 2. Attach a non-canonical extension (ext_recursive, tag 101) to msg
  upb_wire_test_TestRecursive* sub_msg = upb_wire_test_TestRecursive_new(arena);
  UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      (upb_Message*)msg, upb_wire_test_ext_recursive_ext, &sub_msg, arena);

  // 3. Also add some standard raw unknown bytes (tag 150)
  char raw_unknown[] = "\x08\x96\x01";  // tag 1 = 150
  UPB_PRIVATE(_upb_Message_AddUnknown)((upb_Message*)msg, raw_unknown,
                                       sizeof(raw_unknown) - 1, arena,
                                       kUpb_AddUnknown_Copy);

  // Encode the message WITH kUpb_EncodeOption_SkipUnknown option!
  char* buf;
  size_t size;
  upb_EncodeStatus status =
      upb_Encode((upb_Message*)msg, &upb_0wire_0test__TestExtensions_msg_init,
                 kUpb_EncodeOption_SkipUnknown, arena, &buf, &size);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);

  // Parse back the serialized bytes.
  // It MUST contain the canonical extension,
  // but the non-canonical extension and standard raw unknown bytes MUST be
  // successfully skipped.
  upb_ExtensionRegistry* ext_reg = upb_ExtensionRegistry_New(arena);
  const upb_MiniTableExtension* ext_array[2] = {
      upb_wire_test_ext_i32_ext, upb_wire_test_ext_recursive_ext};
  upb_ExtensionRegistry_AddArray(ext_reg, ext_array, 2);

  upb_wire_test_TestExtensions* decoded_msg =
      upb_wire_test_TestExtensions_parse_ex(buf, size, ext_reg, 0, arena);
  EXPECT_NE(decoded_msg, nullptr);

  // Verify canonical extension was NOT skipped and is present
  EXPECT_TRUE(upb_wire_test_has_ext_i32(decoded_msg));
  EXPECT_EQ(upb_wire_test_ext_i32(decoded_msg), 1000);

  // Verify non-canonical extension WAS skipped
  EXPECT_FALSE(upb_Message_HasExtension((const upb_Message*)decoded_msg,
                                        upb_wire_test_ext_recursive_ext));

  // Verify raw unknown bytes WERE skipped and are discarded
  upb_MessageUnknown data;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  EXPECT_FALSE(
      upb_Message_NextUnknown2((const upb_Message*)decoded_msg, &data, &iter));

  upb_Arena_Free(arena);
}

TEST(EncodeTest, EncodeNonCanonicalExtensionDeterministicSuccess) {
  upb_Arena* arena = upb_Arena_New();

  upb_wire_test_TestExtensions* msg = upb_wire_test_TestExtensions_new(arena);

  // 1. Attach scalar extension as non-canonical (tag 100)
  int32_t val = 42;
  UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      (upb_Message*)msg, upb_wire_test_ext_i32_ext, &val, arena);

  // 2. Attach recursive extension as non-canonical (tag 101)
  upb_wire_test_TestRecursive* sub_msg = upb_wire_test_TestRecursive_new(arena);
  UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      (upb_Message*)msg, upb_wire_test_ext_recursive_ext, &sub_msg, arena);

  // Encode the message with deterministic option!
  char* buf;
  size_t size;
  upb_EncodeStatus status =
      upb_Encode((upb_Message*)msg, &upb_0wire_0test__TestExtensions_msg_init,
                 kUpb_EncodeOption_Deterministic, arena, &buf, &size);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);
  EXPECT_GT(size, 0u);

  // Verify that the encoded bytes can be decoded back using the registry!
  upb_ExtensionRegistry* ext_reg = upb_ExtensionRegistry_New(arena);
  const upb_MiniTableExtension* ext_array[2] = {
      upb_wire_test_ext_i32_ext, upb_wire_test_ext_recursive_ext};
  upb_ExtensionRegistry_AddArray(ext_reg, ext_array, 2);

  upb_wire_test_TestExtensions* decoded_msg =
      upb_wire_test_TestExtensions_parse_ex(buf, size, ext_reg, 0, arena);
  EXPECT_NE(decoded_msg, nullptr);

  EXPECT_TRUE(upb_wire_test_has_ext_i32(decoded_msg));
  EXPECT_EQ(upb_wire_test_ext_i32(decoded_msg), 42);
  EXPECT_TRUE(upb_wire_test_has_ext_recursive(decoded_msg));

  // Explicitly verify that the extensions are successfully serialized and
  // resolved from the encoded message payload.
  EXPECT_EQ((int)upb_Message_ExtensionCount((const upb_Message*)decoded_msg),
            2);

  // Verify that if we decode without a registry, the non-canonical extensions
  // remain as raw unknown bytes inside the decoded message.
  upb_wire_test_TestExtensions* decoded_as_unknown =
      upb_wire_test_TestExtensions_parse_ex(buf, size, nullptr, 0, arena);
  EXPECT_NE(decoded_as_unknown, nullptr);
  EXPECT_EQ(
      (int)upb_Message_ExtensionCount((const upb_Message*)decoded_as_unknown),
      0);
  EXPECT_TRUE(upb_Message_HasUnknown((const upb_Message*)decoded_as_unknown));

  int unknown_bytes_count = 0;
  upb_MessageUnknown udata;
  uintptr_t uiter = kUpb_Message_UnknownBegin;
  while (upb_Message_NextUnknown2((const upb_Message*)decoded_as_unknown,
                                  &udata, &uiter)) {
    if (udata.type == kUpb_MessageUnknownType_Bytes) {
      unknown_bytes_count++;
    }
  }
  EXPECT_GT(unknown_bytes_count, 0);

  upb_Arena_Free(arena);
}

TEST(EncodeTest, SkipUnknownNonCanonicalExtensionDeterministicSuccess) {
  upb_Arena* arena = upb_Arena_New();

  upb_wire_test_TestExtensions* msg = upb_wire_test_TestExtensions_new(arena);

  // 1. Add a canonical extension (ext_i32, tag 100) to msg
  upb_Extension* canonical_ext = UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
      (upb_Message*)msg, upb_wire_test_ext_i32_ext, arena);
  canonical_ext->data.int32_val = 1000;

  // 2. Attach a non-canonical extension (ext_recursive, tag 101) to msg
  upb_wire_test_TestRecursive* sub_msg = upb_wire_test_TestRecursive_new(arena);
  UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      (upb_Message*)msg, upb_wire_test_ext_recursive_ext, &sub_msg, arena);

  // 3. Also add some standard raw unknown bytes (tag 150)
  char raw_unknown[] = "\x08\x96\x01";  // tag 1 = 150
  UPB_PRIVATE(_upb_Message_AddUnknown)((upb_Message*)msg, raw_unknown,
                                       sizeof(raw_unknown) - 1, arena,
                                       kUpb_AddUnknown_Copy);

  // Encode the message WITH kUpb_EncodeOption_SkipUnknown |
  // kUpb_EncodeOption_Deterministic!
  char* buf;
  size_t size;
  upb_EncodeStatus status = upb_Encode(
      (upb_Message*)msg, &upb_0wire_0test__TestExtensions_msg_init,
      kUpb_EncodeOption_SkipUnknown | kUpb_EncodeOption_Deterministic, arena,
      &buf, &size);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);

  // Parse back the serialized bytes.
  // It MUST contain the canonical extension,
  // but the non-canonical extension and standard raw unknown bytes MUST be
  // successfully skipped.
  upb_ExtensionRegistry* ext_reg = upb_ExtensionRegistry_New(arena);
  const upb_MiniTableExtension* ext_array[2] = {
      upb_wire_test_ext_i32_ext, upb_wire_test_ext_recursive_ext};
  upb_ExtensionRegistry_AddArray(ext_reg, ext_array, 2);

  upb_wire_test_TestExtensions* decoded_msg =
      upb_wire_test_TestExtensions_parse_ex(buf, size, ext_reg, 0, arena);
  EXPECT_NE(decoded_msg, nullptr);

  // Verify canonical extension was NOT skipped and is present
  EXPECT_TRUE(upb_wire_test_has_ext_i32(decoded_msg));
  EXPECT_EQ(upb_wire_test_ext_i32(decoded_msg), 1000);

  // Verify non-canonical extension WAS skipped
  EXPECT_FALSE(upb_Message_HasExtension((const upb_Message*)decoded_msg,
                                        upb_wire_test_ext_recursive_ext));

  // Verify raw unknown bytes WERE skipped and are discarded
  upb_MessageUnknown data;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  EXPECT_FALSE(
      upb_Message_NextUnknown2((const upb_Message*)decoded_msg, &data, &iter));

  upb_Arena_Free(arena);
}

}  // namespace
}  // namespace upb

#include "upb/port/undef.inc"
