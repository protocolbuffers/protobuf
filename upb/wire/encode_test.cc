#include "upb/wire/encode.h"

#include <setjmp.h>
#include <stddef.h>

#include <cstdint>

#include <gtest/gtest.h>
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/map_sorter.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/message/unknown_fields.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode_test.upb.h"
#include "upb/wire/encode_test.upb_minitable.h"
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
  EXPECT_EQ(upb_ExtensionRegistry_AddArray(ext_reg, ext_array, 1),
            kUpb_ExtensionRegistryStatus_Ok);

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
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      (upb_Message*)msg, upb_wire_test_ext_i32_ext, &val, arena));

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
  EXPECT_EQ(upb_ExtensionRegistry_AddArray(ext_reg, ext_array, 1),
            kUpb_ExtensionRegistryStatus_Ok);

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
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      (upb_Message*)msg, upb_wire_test_ext_recursive_ext, &sub_msg, arena));

  // 3. Also add some standard raw unknown bytes (tag 150)
  char raw_unknown[] = "\x08\x96\x01";  // tag 1 = 150
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      (upb_Message*)msg, raw_unknown, sizeof(raw_unknown) - 1, arena,
      kUpb_AddUnknown_Copy));

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
  EXPECT_EQ(upb_ExtensionRegistry_AddArray(ext_reg, ext_array, 2),
            kUpb_ExtensionRegistryStatus_Ok);

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
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      (upb_Message*)msg, upb_wire_test_ext_i32_ext, &val, arena));

  // 2. Attach recursive extension as non-canonical (tag 101)
  upb_wire_test_TestRecursive* sub_msg = upb_wire_test_TestRecursive_new(arena);
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      (upb_Message*)msg, upb_wire_test_ext_recursive_ext, &sub_msg, arena));

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
  EXPECT_EQ(upb_ExtensionRegistry_AddArray(ext_reg, ext_array, 2),
            kUpb_ExtensionRegistryStatus_Ok);

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
    if (udata.type == kUpb_MessageUnknownType_StringView) {
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
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      (upb_Message*)msg, upb_wire_test_ext_recursive_ext, &sub_msg, arena));

  // 3. Also add some standard raw unknown bytes (tag 150)
  char raw_unknown[] = "\x08\x96\x01";  // tag 1 = 150
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      (upb_Message*)msg, raw_unknown, sizeof(raw_unknown) - 1, arena,
      kUpb_AddUnknown_Copy));

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
  EXPECT_EQ(upb_ExtensionRegistry_AddArray(ext_reg, ext_array, 2),
            kUpb_ExtensionRegistryStatus_Ok);

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

TEST(EncodeTest, MixedExtensionAndUnknownOrderSuccess) {
  upb_Arena* arena = upb_Arena_New();
  upb_wire_test_TestExtensions* msg = upb_wire_test_TestExtensions_new(arena);

  // 1. Add Unknown 1 (Tag 10, Varint)
  char unknown1[] = "\x50\x64";
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)((upb_Message*)msg, unknown1,
                                                   sizeof(unknown1) - 1, arena,
                                                   kUpb_AddUnknown_Copy));

  // 2. Add Non-Canonical Extension 1 (Tag 100, ext_i32)
  int32_t val1 = 42;
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      (upb_Message*)msg, upb_wire_test_ext_i32_ext, &val1, arena));

  // 3. Add Unknown 2 (Tag 12, Varint)
  char unknown2[] = "\x60\x64";
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)((upb_Message*)msg, unknown2,
                                                   sizeof(unknown2) - 1, arena,
                                                   kUpb_AddUnknown_Copy));

  // 4. Add Canonical Extension 1 (Tag 101, ext_recursive)
  upb_wire_test_TestRecursive* sub_msg = upb_wire_test_TestRecursive_new(arena);
  upb_Extension* canonical_ext = UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
      (upb_Message*)msg, upb_wire_test_ext_recursive_ext, arena);
  canonical_ext->data.msg_val = (upb_Message*)sub_msg;

  // Encode the message
  char* buf;
  size_t size;
  upb_EncodeStatus status =
      upb_Encode((upb_Message*)msg, &upb_0wire_0test__TestExtensions_msg_init,
                 kUpb_EncodeOption_Deterministic, arena, &buf, &size);
  ASSERT_EQ(status, kUpb_EncodeStatus_Ok);

  // Verify encoded buffer directly
  ASSERT_EQ(size, 10u);

  // Expected order in buffer (reading from index 0):
  // 1. Canonical Extension 1 (Tag 101) -> Grouped and placed first
  // 2. Unknown 1 (Tag 10)
  // 3. Non-Canonical Extension 1 (Tag 100)
  // 4. Unknown 2 (Tag 12)

  // 1. Canonical Extension 1 (Tag 101, LenDelim) -> 0xAA 0x06,
  // Length 0 -> 0x00
  EXPECT_EQ((uint8_t)buf[0], 0xAA);
  EXPECT_EQ((uint8_t)buf[1], 0x06);
  EXPECT_EQ((uint8_t)buf[2], 0x00);

  // 2. Unknown 1 (Tag 10, Varint) -> 0x50, Value 100 -> 0x64
  EXPECT_EQ((uint8_t)buf[3], 0x50);
  EXPECT_EQ((uint8_t)buf[4], 0x64);

  // 3. Non-Canonical Extension 1 (Tag 100, Varint) -> 0xA0 0x06,
  // Value 42 -> 0x2A
  EXPECT_EQ((uint8_t)buf[5], 0xA0);
  EXPECT_EQ((uint8_t)buf[6], 0x06);
  EXPECT_EQ((uint8_t)buf[7], 0x2A);

  // 4. Unknown 2 (Tag 12, Varint) -> 0x60, Value 100 -> 0x64
  EXPECT_EQ((uint8_t)buf[8], 0x60);
  EXPECT_EQ((uint8_t)buf[9], 0x64);

  upb_Arena_Free(arena);
}

TEST(EncodeTest, BackAllocPoolReuseAndReturn) {
  upb_Arena* arena = upb_Arena_Init(nullptr, 4096, &upb_alloc_global);

  // First FreePool call initializes the pool table itself; subsequent calls
  // populate the 256-byte and 512-byte bins.
  void* pool_table = upb_Arena_AllocPool(arena, 512);
  void* p512 = upb_Arena_AllocPool(arena, 512);
  void* p256 = upb_Arena_AllocPool(arena, 256);
  upb_Arena_FreePool(arena, pool_table, 512);
  upb_Arena_FreePool(arena, p512, 512);
  upb_Arena_FreePool(arena, p256, 256);

  // Exhaust the arena's active block so _upb_Arena_Steal won't satisfy a 200B
  // reservation, leaving ~100 bytes in the active block.
  size_t remaining = UPB_PRIVATE(_upb_ArenaHas)(arena);
  if (remaining > 96) {
    (void)upb_Arena_Malloc(arena, remaining - 96);
  }
  uintptr_t space_before = upb_Arena_SpaceAllocated(arena, nullptr);

  // 1. Reserve 200 bytes: Steal fails (only <=96B left), so BackAlloc should
  // grab the 256-byte block from the pool without allocating a new block.
  upb_BackAlloc ba;
  char* ptr = upb_BackAlloc_Init(&ba, arena);
  ptr = upb_BackAlloc_Reserve(&ba, ptr, 200);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(ba.type, kUpb_BackAlloc_Pooled);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(ba.buf, p256));
  EXPECT_EQ(upb_Arena_SpaceAllocated(arena, nullptr), space_before);

  // 2. Grow by another 200 bytes (400B total): BackAlloc should grab the
  // 512-byte block from the pool and return the 256-byte block to the pool.
  ptr = upb_BackAlloc_Reserve(&ba, ptr, 200);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(ba.type, kUpb_BackAlloc_Pooled);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(ba.buf, p512));
  EXPECT_EQ(upb_Arena_SpaceAllocated(arena, nullptr), space_before);

  // Verify the 256-byte block was returned to the pool.
  void* p256_again = upb_Arena_TryAllocPool(arena, 256);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(p256_again, p256));

  // 3. Abort BackAlloc: the 512-byte pooled block should be returned to the
  // pool.
  upb_BackAlloc_Abort(&ba);
  void* p512_again = upb_Arena_TryAllocPool(arena, 512);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(p512_again, p512));

  upb_Arena_Free(arena);
}

TEST(EncodeTest, BackAllocFinishHarvestsDeclinedRemainder) {
  upb_Arena* arena = upb_Arena_Init(nullptr, 4096, &upb_alloc_global);

  // Prime the pool with a 256-byte block.
  void* p512 = upb_Arena_AllocPool(arena, 512);
  void* p256 = upb_Arena_AllocPool(arena, 256);
  upb_Arena_FreePool(arena, p512, 512);
  upb_Arena_FreePool(arena, p256, 256);

  // Leave ~128 bytes in the arena's active block so Steal(192) fails,
  // while _upb_ArenaHas(arena) (~128) > 64.
  size_t remaining = UPB_PRIVATE(_upb_ArenaHas)(arena);
  if (remaining > 128) {
    (void)upb_Arena_Malloc(arena, remaining - 128);
  }

  // Reserve 192 bytes from the 256-byte pooled block, leaving 64 bytes unused
  // at the front of ba.buf.
  upb_BackAlloc ba;
  char* ptr = upb_BackAlloc_Init(&ba, arena);
  ptr = upb_BackAlloc_Reserve(&ba, ptr, 192);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(ba.type, kUpb_BackAlloc_Pooled);

  // Finish: ptr - ba.buf == 64 <= _upb_ArenaHas(arena) (~128), so UseBlock
  // declines to replace the active block and harvests the 64-byte prefix into
  // the pool.
  EXPECT_EQ(upb_BackAlloc_Finish(&ba, ptr), 192u);
  void* harvested_64 = upb_Arena_TryAllocPool(arena, 64);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(harvested_64, p256));

  upb_Arena_Free(arena);
}
}  // namespace
}  // namespace upb

#include "upb/port/undef.inc"
