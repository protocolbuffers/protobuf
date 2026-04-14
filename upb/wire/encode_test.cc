#include "upb/wire/internal/encode.h"

#include <setjmp.h>
#include <stddef.h>

#include <cstdint>

#include <gtest/gtest.h>
#include "upb/base/descriptor_constants.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/map_sorter.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/sub.h"
#include "upb/mini_table/message.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace {

static void DoEncodeFieldMaxDepthExceeded(jmp_buf err, upb_encstate& e,
                                          upb_Message* msg,
                                          const upb_MiniTableField& field,
                                          char*& buf, size_t& size) {
  if (UPB_SETJMP(err) == 0) {
    UPB_PRIVATE(_upb_Encode_Field)(&e, msg, &field, &buf, &size, e.options);
    FAIL() << "Should have jumped";
  } else {
    EXPECT_EQ(e.status, kUpb_EncodeStatus_MaxDepthExceeded);
  }
}

static void DoEncodeExtensionMaxDepthExceeded(jmp_buf err, upb_encstate& e,
                                              const upb_MiniTableExtension& ext,
                                              upb_MessageValue ext_val,
                                              char*& buf, size_t& size) {
  if (UPB_SETJMP(err) == 0) {
    UPB_PRIVATE(_upb_Encode_Extension)(&e, &ext, ext_val, false, &buf, &size,
                                       e.options);
    FAIL() << "Should have jumped";
  } else {
    EXPECT_EQ(e.status, kUpb_EncodeStatus_MaxDepthExceeded);
  }
}

static void InitEncState(upb_encstate* e, jmp_buf* err, upb_Arena* arena) {
  e->status = kUpb_EncodeStatus_Ok;
  e->err = err;
  e->arena = arena;
  e->buf = nullptr;
  e->limit = nullptr;
  e->depth = 0;
  e->options = 0;
  _upb_mapsorter_init(&e->sorter);
}

TEST(EncodeTest, EncodeFieldSuccess) {
  upb_Arena* arena = upb_Arena_New();
  upb_MiniTable mt = {nullptr};
  mt.UPB_PRIVATE(size) = 8;
  upb_Message* msg = upb_Message_New(&mt, arena);

  upb_encstate e;
  jmp_buf err;
  InitEncState(&e, &err, arena);

  struct {
    upb_MiniTableField field;
    upb_MiniTableSubInternal sub;
  } field_with_sub;
  upb_MiniTableField& field = field_with_sub.field;

  field.UPB_PRIVATE(number) = 1;
  field.UPB_PRIVATE(offset) = 0;
  field.presence = 0;
  field.UPB_PRIVATE(mode) =
      kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift);
  field.UPB_PRIVATE(descriptortype) = kUpb_FieldType_Int32;
  field.UPB_PRIVATE(submsg_ofs) = offsetof(decltype(field_with_sub), sub) / 4;
  field_with_sub.sub.UPB_PRIVATE(submsg) = &mt;

  *UPB_PTR_AT(msg, field.UPB_PRIVATE(offset), int32_t) = 42;

  char* buf;
  size_t size;
  upb_EncodeStatus status =
      UPB_PRIVATE(_upb_Encode_Field)(&e, msg, &field, &buf, &size, e.options);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);
  EXPECT_GT(size, 0);

  _upb_mapsorter_destroy(&e.sorter);
  upb_Arena_Free(arena);
}

TEST(EncodeTest, EncodeFieldSuccessEmptyMessage) {
  upb_Arena* arena = upb_Arena_New();
  upb_MiniTable mt = {nullptr};
  mt.UPB_PRIVATE(size) = 8;
  upb_Message* msg = upb_Message_New(&mt, arena);

  upb_encstate e;
  jmp_buf err;
  InitEncState(&e, &err, arena);

  struct {
    upb_MiniTableField field;
    upb_MiniTableSubInternal sub;
  } field_with_sub;
  upb_MiniTableField& field = field_with_sub.field;

  field.UPB_PRIVATE(number) = 1;
  field.UPB_PRIVATE(offset) = 0;
  field.presence = 0;
  field.UPB_PRIVATE(mode) =
      kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift);
  field.UPB_PRIVATE(descriptortype) = kUpb_FieldType_Int32;
  field.UPB_PRIVATE(submsg_ofs) = offsetof(decltype(field_with_sub), sub) / 4;
  field_with_sub.sub.UPB_PRIVATE(submsg) = &mt;

  // Zero int32 won't be serialized.
  *UPB_PTR_AT(msg, field.UPB_PRIVATE(offset), int32_t) = 0;

  char* buf;
  size_t size;
  upb_EncodeStatus status =
      UPB_PRIVATE(_upb_Encode_Field)(&e, msg, &field, &buf, &size, e.options);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);
  EXPECT_EQ(size, 0);

  _upb_mapsorter_destroy(&e.sorter);
  upb_Arena_Free(arena);
}

TEST(EncodeTest, EncodeFieldMaxDepthExceeded) {
  upb_Arena* arena = upb_Arena_New();
  upb_MiniTable mt = {nullptr};
  mt.UPB_PRIVATE(size) = 8;
  upb_Message* msg = upb_Message_New(&mt, arena);

  upb_encstate e;
  jmp_buf err;
  InitEncState(&e, &err, arena);

  struct {
    upb_MiniTableField field;
    upb_MiniTableSubInternal sub;
  } field_with_sub;
  upb_MiniTableField& field = field_with_sub.field;

  field.UPB_PRIVATE(number) = 1;
  field.UPB_PRIVATE(offset) = 0;
  field.presence = 0;
  field.UPB_PRIVATE(submsg_ofs) = offsetof(decltype(field_with_sub), sub) / 4;
  field_with_sub.sub.UPB_PRIVATE(submsg) = &mt;

  char* buf;
  size_t size;

  field.UPB_PRIVATE(descriptortype) = kUpb_FieldType_Message;
  field.UPB_PRIVATE(mode) =
      kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift);
  upb_Message* sub_msg = upb_Message_New(&mt, arena);
  *UPB_PTR_AT(msg, field.UPB_PRIVATE(offset), upb_Message*) = sub_msg;

  e.options = upb_EncodeOptions_MaxDepth(1);
  DoEncodeFieldMaxDepthExceeded(err, e, msg, field, buf, size);

  _upb_mapsorter_destroy(&e.sorter);
  upb_Arena_Free(arena);
}

TEST(EncodeTest, EncodeExtensionSuccess) {
  upb_Arena* arena = upb_Arena_New();

  upb_encstate e;
  jmp_buf err;
  InitEncState(&e, &err, arena);

  upb_MiniTableExtension ext;
  ext.UPB_PRIVATE(field).UPB_PRIVATE(number) = 1;
  ext.UPB_PRIVATE(field).UPB_PRIVATE(offset) = 0;
  ext.UPB_PRIVATE(field).presence = 0;
  ext.UPB_PRIVATE(field).UPB_PRIVATE(mode) =
      kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift);
  ext.UPB_PRIVATE(field).UPB_PRIVATE(descriptortype) = kUpb_FieldType_Int32;

  upb_MessageValue ext_val;
  ext_val.int32_val = 42;

  char* buf;
  size_t size;
  upb_EncodeStatus status = UPB_PRIVATE(_upb_Encode_Extension)(
      &e, &ext, ext_val, false, &buf, &size, e.options);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);
  EXPECT_GT(size, 0);

  _upb_mapsorter_destroy(&e.sorter);
  upb_Arena_Free(arena);
}

TEST(EncodeTest, EncodeExtensionSuccessEmptyMessage) {
  upb_Arena* arena = upb_Arena_New();

  upb_encstate e;
  jmp_buf err;
  InitEncState(&e, &err, arena);

  upb_MiniTableExtension ext;
  ext.UPB_PRIVATE(field).UPB_PRIVATE(number) = 1;
  ext.UPB_PRIVATE(field).UPB_PRIVATE(offset) = 0;
  ext.UPB_PRIVATE(field).presence = 0;
  ext.UPB_PRIVATE(field).UPB_PRIVATE(mode) =
      kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift);
  ext.UPB_PRIVATE(field).UPB_PRIVATE(descriptortype) = kUpb_FieldType_Int32;

  // Zero int32 won't be serialized.
  upb_MessageValue ext_val;
  ext_val.int32_val = 0;

  char* buf;
  size_t size;

  upb_EncodeStatus status = UPB_PRIVATE(_upb_Encode_Extension)(
      &e, &ext, ext_val, false, &buf, &size, e.options);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);
  EXPECT_GT(size, 0);

  _upb_mapsorter_destroy(&e.sorter);
  upb_Arena_Free(arena);
}

TEST(EncodeTest, EncodeExtensionMaxDepthExceeded) {
  upb_Arena* arena = upb_Arena_New();

  upb_encstate e;
  jmp_buf err;
  InitEncState(&e, &err, arena);

  upb_MiniTableExtension ext;
  ext.UPB_PRIVATE(field).UPB_PRIVATE(number) = 1;
  ext.UPB_PRIVATE(field).UPB_PRIVATE(offset) = 0;
  ext.UPB_PRIVATE(field).presence = 0;

  upb_MessageValue ext_val;

  char* buf;
  size_t size;

  ext.UPB_PRIVATE(field).UPB_PRIVATE(descriptortype) = kUpb_FieldType_Message;
  ext.UPB_PRIVATE(field).UPB_PRIVATE(mode) =
      kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift);
  ext.UPB_PRIVATE(field).UPB_PRIVATE(submsg_ofs) =
      offsetof(upb_MiniTableExtension,
               sub_dont_copy_me__upb_internal_use_only) /
      4;
  upb_MiniTable mt = {nullptr};
  mt.UPB_PRIVATE(size) = 8;
  ext.UPB_PRIVATE(sub).UPB_PRIVATE(submsg) = &mt;
  ext_val.msg_val = upb_Message_New(&mt, arena);

  e.options = upb_EncodeOptions_MaxDepth(1);
  DoEncodeExtensionMaxDepthExceeded(err, e, ext, ext_val, buf, size);

  _upb_mapsorter_destroy(&e.sorter);
  upb_Arena_Free(arena);
}

}  // namespace
}  // namespace upb

#include "upb/port/undef.inc"
