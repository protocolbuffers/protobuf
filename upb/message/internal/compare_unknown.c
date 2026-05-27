// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/compare_unknown.h"

#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "upb/base/string_view.h"
#include "upb/mem/alloc.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/message/unknown_fields.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/message.h"
#include "upb/wire/encode.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/back_alloc.h"
#include "upb/wire/internal/encoder.h"
#include "upb/wire/reader.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct upb_UnknownFields upb_UnknownFields;

typedef struct {
  uint32_t tag;
  union {
    uint64_t varint;
    uint64_t uint64;
    uint32_t uint32;
    upb_StringView delimited;
    upb_UnknownFields* group;
  } data;
} upb_UnknownField;

struct upb_UnknownFields {
  size_t size;
  size_t capacity;
  upb_UnknownField* fields;
};

typedef struct {
  upb_EpsCopyInputStream stream;
  upb_encstate encoder;  // For encoding non-canonical extensions.
  upb_Arena* arena;
  upb_UnknownField* tmp;
  size_t tmp_size;
  int depth;
  upb_UnknownCompareResult status;
  jmp_buf err;
} upb_UnknownField_Context;

typedef struct {
  upb_UnknownField* arr_base;
  upb_UnknownField* arr_ptr;
  upb_UnknownField* arr_end;
  uint32_t last_tag;
  bool sorted;
} upb_UnknownFields_Builder;

UPB_NORETURN static void upb_UnknownFields_OutOfMemory(
    upb_UnknownField_Context* ctx) {
  ctx->status = kUpb_UnknownCompareResult_OutOfMemory;
  UPB_LONGJMP(ctx->err, 1);
}

static void upb_UnknownFields_Grow(upb_UnknownField_Context* ctx,
                                   upb_UnknownField** base,
                                   upb_UnknownField** ptr,
                                   upb_UnknownField** end) {
  size_t old = (*ptr - *base);
  size_t new = UPB_MAX(4, old * 2);

  *base = upb_Arena_Realloc(ctx->arena, *base, old * sizeof(**base),
                            new * sizeof(**base));
  if (!*base) upb_UnknownFields_OutOfMemory(ctx);

  *ptr = *base + old;
  *end = *base + new;
}

// We have to implement our own sort here, since qsort() is not an in-order
// sort. Here we use merge sort, the simplest in-order sort.
static void upb_UnknownFields_Merge(upb_UnknownField* arr, size_t start,
                                    size_t mid, size_t end,
                                    upb_UnknownField* tmp) {
  memcpy(tmp, &arr[start], (end - start) * sizeof(*tmp));

  upb_UnknownField* ptr1 = tmp;
  upb_UnknownField* end1 = &tmp[mid - start];
  upb_UnknownField* ptr2 = &tmp[mid - start];
  upb_UnknownField* end2 = &tmp[end - start];
  upb_UnknownField* out = &arr[start];

  while (ptr1 < end1 && ptr2 < end2) {
    if (ptr1->tag <= ptr2->tag) {
      *out++ = *ptr1++;
    } else {
      *out++ = *ptr2++;
    }
  }

  if (ptr1 < end1) {
    memcpy(out, ptr1, (end1 - ptr1) * sizeof(*out));
  } else if (ptr2 < end2) {
    memcpy(out, ptr2, (end2 - ptr2) * sizeof(*out));
  }
}

static void upb_UnknownFields_SortRecursive(upb_UnknownField* arr, size_t start,
                                            size_t end, upb_UnknownField* tmp) {
  if (end - start > 1) {
    size_t mid = start + ((end - start) / 2);
    upb_UnknownFields_SortRecursive(arr, start, mid, tmp);
    upb_UnknownFields_SortRecursive(arr, mid, end, tmp);
    upb_UnknownFields_Merge(arr, start, mid, end, tmp);
  }
}

static void upb_UnknownFields_Sort(upb_UnknownField_Context* ctx,
                                   upb_UnknownFields* fields) {
  if (ctx->tmp_size < fields->size) {
    const int oldsize = ctx->tmp_size * sizeof(*ctx->tmp);
    ctx->tmp_size = UPB_MAX(8, ctx->tmp_size);
    while (ctx->tmp_size < fields->size) ctx->tmp_size *= 2;
    const int newsize = ctx->tmp_size * sizeof(*ctx->tmp);
    ctx->tmp = upb_grealloc(ctx->tmp, oldsize, newsize);
  }
  upb_UnknownFields_SortRecursive(fields->fields, 0, fields->size, ctx->tmp);
}

static upb_UnknownFields* upb_UnknownFields_BuildFromBuffer(
    upb_UnknownField_Context* ctx, const char** buf);

// Combines two unknown fields into one.
static void upb_CombineUnknownFields(upb_UnknownField_Context* ctx,
                                     upb_UnknownFields_Builder* builder,
                                     const char** buf) {
  upb_UnknownField* arr_base = builder->arr_base;
  upb_UnknownField* arr_ptr = builder->arr_ptr;
  upb_UnknownField* arr_end = builder->arr_end;
  const char* ptr = *buf;
  uint32_t last_tag = builder->last_tag;
  bool sorted = builder->sorted;

  // Parse the unknown field data. It is an invariant of the data structure that
  // unknown field data is valid, so parse errors here should be impossible.
  while (!upb_EpsCopyInputStream_IsDone(&ctx->stream, &ptr)) {
    uint32_t tag;
    ptr = upb_WireReader_ReadTag(ptr, &tag, &ctx->stream);
    UPB_ASSERT(tag <= UINT32_MAX);
    int wire_type = upb_WireReader_GetWireType(tag);
    if (wire_type == kUpb_WireType_EndGroup) break;
    if (tag < last_tag) sorted = false;
    last_tag = tag;

    if (arr_ptr == arr_end) {
      upb_UnknownFields_Grow(ctx, &arr_base, &arr_ptr, &arr_end);
    }
    upb_UnknownField* field = arr_ptr;
    field->tag = tag;
    arr_ptr++;

    switch (wire_type) {
      case kUpb_WireType_Varint:
        ptr = upb_WireReader_ReadVarint(ptr, &field->data.varint, &ctx->stream);
        UPB_ASSERT(ptr);
        break;
      case kUpb_WireType_64Bit:
        ptr =
            upb_WireReader_ReadFixed64(ptr, &field->data.uint64, &ctx->stream);
        UPB_ASSERT(ptr);
        break;
      case kUpb_WireType_32Bit:
        ptr =
            upb_WireReader_ReadFixed32(ptr, &field->data.uint32, &ctx->stream);
        UPB_ASSERT(ptr);
        break;
      case kUpb_WireType_Delimited: {
        int size;
        upb_StringView sv;
        ptr = upb_WireReader_ReadSize(ptr, &size, &ctx->stream);
        UPB_ASSERT(ptr);
        ptr = upb_EpsCopyInputStream_ReadStringAlwaysAlias(&ctx->stream, ptr,
                                                           size, &sv);
        UPB_ASSERT(ptr);
        field->data.delimited.data = sv.data;
        field->data.delimited.size = sv.size;
        break;
      }
      case kUpb_WireType_StartGroup:
        if (--ctx->depth < 0) {
          ctx->status = kUpb_UnknownCompareResult_MaxDepthExceeded;
          UPB_LONGJMP(ctx->err, 1);
        }
        field->data.group = upb_UnknownFields_BuildFromBuffer(ctx, &ptr);
        ctx->depth++;
        break;
      default:
        UPB_UNREACHABLE();
    }
  }
  *buf = ptr;
  builder->arr_base = arr_base;
  builder->arr_ptr = arr_ptr;
  builder->arr_end = arr_end;
  builder->sorted = sorted;
  builder->last_tag = last_tag;
}

static upb_UnknownFields* upb_UnknownFields_DoBuild(
    upb_UnknownField_Context* ctx, upb_UnknownFields_Builder* builder) {
  upb_UnknownFields* ret = upb_Arena_Malloc(ctx->arena, sizeof(*ret));
  if (!ret) upb_UnknownFields_OutOfMemory(ctx);
  ret->fields = builder->arr_base;
  ret->size = builder->arr_ptr - builder->arr_base;
  ret->capacity = builder->arr_end - builder->arr_base;
  if (!builder->sorted) {
    upb_UnknownFields_Sort(ctx, ret);
  }
  return ret;
}

// Builds a upb_UnknownFields data structure from the binary data in buf.
static upb_UnknownFields* upb_UnknownFields_BuildFromBuffer(
    upb_UnknownField_Context* ctx, const char** buf) {
  upb_UnknownFields_Builder builder = {
      .arr_base = NULL,
      .arr_ptr = NULL,
      .arr_end = NULL,
      .sorted = true,
      .last_tag = 0,
  };
  const char* ptr = *buf;
  upb_CombineUnknownFields(ctx, &builder, &ptr);
  upb_UnknownFields* fields = upb_UnknownFields_DoBuild(ctx, &builder);
  *buf = ptr;
  return fields;
}

// Builds a upb_UnknownFields data structure from the unknown fields of a
// upb_Message.
static upb_UnknownFields* upb_UnknownFields_Build(upb_UnknownField_Context* ctx,
                                                  const upb_Message* msg) {
  upb_UnknownFields_Builder builder = {
      .arr_base = NULL,
      .arr_ptr = NULL,
      .arr_end = NULL,
      .sorted = true,
      .last_tag = 0,
  };
  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_MessageUnknown data;
  while (upb_Message_NextUnknown2(msg, &data, &iter)) {
    const char* ptr;
    size_t size;
    if (data.type == kUpb_MessageUnknownType_StringView) {
      upb_StringView view = data.value.bytes;
      ptr = view.data;
      size = view.size;
    } else {
      UPB_ASSERT(data.type == kUpb_MessageUnknownType_NonCanonicalExtension);
      char* enc_buf = upb_BackAlloc_Init(&ctx->encoder.alloc, ctx->arena);
      ctx->encoder.status = kUpb_EncodeStatus_Ok;
      // Encode non-canonical extension to buffer.
      const upb_Extension* ext = (const upb_Extension*)data.value.extension;
      bool is_message_set = false;
      const upb_MiniTable* extendee = upb_MiniTableExtension_Extendee(ext->ext);
      if (extendee) {
        is_message_set = upb_MiniTable_IsMessageSet(extendee);
      }
      UPB_PRIVATE(_upb_Encode_Extension)(&ctx->encoder, ext->ext, ext->data,
                                         is_message_set, &enc_buf, &size,
                                         /*options=*/0);
      ptr = enc_buf;
    }

    upb_EpsCopyInputStream_Init(&ctx->stream, &ptr, size);
    upb_CombineUnknownFields(ctx, &builder, &ptr);
    UPB_ASSERT(upb_EpsCopyInputStream_IsDone(&ctx->stream, &ptr) &&
               !upb_EpsCopyInputStream_IsError(&ctx->stream));
  }
  upb_UnknownFields* fields = upb_UnknownFields_DoBuild(ctx, &builder);
  return fields;
}

// Compares two sorted upb_UnknownFields structures for equality.
static bool upb_UnknownFields_IsEqual(const upb_UnknownFields* uf1,
                                      const upb_UnknownFields* uf2) {
  if (uf1->size != uf2->size) return false;
  for (size_t i = 0, n = uf1->size; i < n; i++) {
    upb_UnknownField* f1 = &uf1->fields[i];
    upb_UnknownField* f2 = &uf2->fields[i];
    if (f1->tag != f2->tag) return false;
    int wire_type = f1->tag & 7;
    switch (wire_type) {
      case kUpb_WireType_Varint:
        if (f1->data.varint != f2->data.varint) return false;
        break;
      case kUpb_WireType_64Bit:
        if (f1->data.uint64 != f2->data.uint64) return false;
        break;
      case kUpb_WireType_32Bit:
        if (f1->data.uint32 != f2->data.uint32) return false;
        break;
      case kUpb_WireType_Delimited:
        if (!upb_StringView_IsEqual(f1->data.delimited, f2->data.delimited)) {
          return false;
        }
        break;
      case kUpb_WireType_StartGroup:
        if (!upb_UnknownFields_IsEqual(f1->data.group, f2->data.group)) {
          return false;
        }
        break;
      default:
        UPB_UNREACHABLE();
    }
  }
  return true;
}

static upb_UnknownCompareResult upb_UnknownField_DoCompare(
    upb_UnknownField_Context* ctx, const upb_Message* msg1,
    const upb_Message* msg2) {
  upb_UnknownCompareResult ret;
  // First build both unknown fields into a sorted data structure (similar
  // to the UnknownFieldSet in C++).
  upb_UnknownFields* uf1 = upb_UnknownFields_Build(ctx, msg1);
  upb_UnknownFields* uf2 = upb_UnknownFields_Build(ctx, msg2);

  // Now perform the equality check on the sorted structures.
  if (upb_UnknownFields_IsEqual(uf1, uf2)) {
    ret = kUpb_UnknownCompareResult_Equal;
  } else {
    ret = kUpb_UnknownCompareResult_NotEqual;
  }
  return ret;
}

static upb_UnknownCompareResult upb_UnknownField_Compare(
    upb_UnknownField_Context* const ctx, const upb_Message* msg1,
    const upb_Message* msg2) {
  upb_UnknownCompareResult ret;
  if (UPB_SETJMP(ctx->err) == 0) {
    ret = upb_UnknownField_DoCompare(ctx, msg1, msg2);
  } else {
    // If status is still Equal, the jump must have originated from the Encoder
    // (which only updates ctx->encoder.status). We must map it to a context
    // error.
    if (ctx->status == kUpb_UnknownCompareResult_Equal) {
      if (ctx->encoder.status == kUpb_EncodeStatus_OutOfMemory) {
        ctx->status = kUpb_UnknownCompareResult_OutOfMemory;
      } else if (ctx->encoder.status == kUpb_EncodeStatus_MaxDepthExceeded) {
        ctx->status = kUpb_UnknownCompareResult_MaxDepthExceeded;
      } else {
        ctx->status = kUpb_UnknownCompareResult_OutOfMemory;
      }
      upb_BackAlloc_Abort(&ctx->encoder.alloc);
    }
    ret = ctx->status;
    UPB_ASSERT(ret != kUpb_UnknownCompareResult_Equal);
  }

  UPB_PRIVATE(_upb_encstate_destroy)(&ctx->encoder);
  upb_Arena_Free(ctx->arena);
  upb_gfree(ctx->tmp);
  return ret;
}

upb_UnknownCompareResult UPB_PRIVATE(_upb_Message_UnknownFieldsAreEqual)(
    const upb_Message* msg1, const upb_Message* msg2, int max_depth) {
  bool msg1_empty = !upb_Message_HasUnknown(msg1);
  bool msg2_empty = !upb_Message_HasUnknown(msg2);
  if (msg1_empty && msg2_empty) return kUpb_UnknownCompareResult_Equal;
  if (msg1_empty || msg2_empty) return kUpb_UnknownCompareResult_NotEqual;

  upb_UnknownField_Context ctx = {
      .arena = upb_Arena_New(),
      .depth = max_depth,
      .tmp = NULL,
      .tmp_size = 0,
      .status = kUpb_UnknownCompareResult_Equal,
  };

  if (!ctx.arena) return kUpb_UnknownCompareResult_OutOfMemory;
  UPB_PRIVATE(_upb_encstate_init)(&ctx.encoder, &ctx.err, ctx.arena);

  return upb_UnknownField_Compare(&ctx, msg1, msg2);
}
