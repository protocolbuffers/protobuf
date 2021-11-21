
#include "upb/util/compare.h"

#include <stdbool.h>
#include <setjmp.h>

#include "upb/port_def.inc"

struct upb_UnknownFields;
typedef struct upb_UnknownFields upb_UnknownFields;

typedef struct {
  uint32_t tag;
  union {
    uint64_t varint;
    uint64_t uint64;
    uint32_t uint32;
    upb_strview delimited;
    upb_UnknownFields* group;
  } data;
} upb_UnknownField;

struct upb_UnknownFields {
  size_t size;
  size_t capacity;
  upb_UnknownField* fields;
};

typedef struct {
  const char *end;
  upb_arena *arena;
  int depth;
  jmp_buf err;
} upb_UnknownField_Context;

static void upb_UnknownFields_Grow(upb_UnknownField_Context *ctx,
                                   upb_UnknownField **base,
                                   upb_UnknownField **ptr,
                                   upb_UnknownField **end) {
  size_t old = (*ptr - *base);
  size_t new = UPB_MAX(4, old * 2);

  *base = upb_arena_realloc(ctx->arena, *base, old * sizeof(*base),
                            new * sizeof(*base));
  if (!*base) UPB_LONGJMP(ctx->err, kUpb_UnknownCompareResult_OutOfMemory);

  *ptr = *base + old;
  *end = *base + new;
}

static const char *upb_UnknownFields_ParseVarint(const char *ptr,
                                                 const char *limit,
                                                 uint64_t *val) {
  uint8_t byte;
  int bitpos = 0;
  *val = 0;

  do {
    // Unknown field data must be valid.
    UPB_ASSERT(bitpos < 70 && ptr < limit);
    byte = *ptr;
    *val |= (uint64_t)(byte & 0x7F) << bitpos;
    ptr++;
    bitpos += 7;
  } while (byte & 0x80);

  return ptr;
}

static upb_UnknownFields *upb_UnknownFields_DoBuild(
    upb_UnknownField_Context *ctx, const char **buf) {
  upb_UnknownField *arr_base = NULL;
  upb_UnknownField *arr_ptr = NULL;
  upb_UnknownField *arr_end = NULL;
  const char *ptr = *buf;
  while (ptr < ctx->end) {
    if (arr_ptr == arr_end) {
      upb_UnknownFields_Grow(ctx, &arr_base, &arr_ptr, &arr_end);
    }
    upb_UnknownField *field = arr_ptr;
    arr_ptr++;
    uint64_t val;
    ptr = upb_UnknownFields_ParseVarint(ptr, ctx->end, &val);
    UPB_ASSERT(val <= UINT32_MAX);
    field->tag = val;
    switch (field->tag & 7) {
      case UPB_WIRE_TYPE_VARINT:
        ptr = upb_UnknownFields_ParseVarint(ptr, ctx->end, &field->data.varint);
        break;
      case UPB_WIRE_TYPE_64BIT:
        UPB_ASSERT(ctx->end - ptr >= 8);
        memcpy(&field->data.uint64, ptr, 8);
        ptr += 8;
        break;
      case UPB_WIRE_TYPE_32BIT:
        UPB_ASSERT(ctx->end - ptr >= 4);
        memcpy(&field->data.uint32, ptr, 4);
        ptr += 8;
        break;
      case UPB_WIRE_TYPE_DELIMITED: {
        uint64_t size;
        ptr = upb_UnknownFields_ParseVarint(ptr, ctx->end, &size);
        UPB_ASSERT(ctx->end - ptr >= size);
        field->data.delimited.data = ptr;
        field->data.delimited.size = size;
        break;
      }
      case UPB_WIRE_TYPE_START_GROUP:
        if (--ctx->depth == 0) {
          UPB_LONGJMP(ctx->err, kUpb_UnknownCompareResult_MaxDepthExceeded);
        }
        field->data.group = upb_UnknownFields_DoBuild(ctx, &ptr);
        ctx->depth++;
        break;
      case UPB_WIRE_TYPE_END_GROUP:
        goto done;
      default:
        UPB_UNREACHABLE();
    }
  }

done:
  *buf = ptr;
  upb_UnknownFields *ret = upb_arena_malloc(ctx->arena, sizeof(*ret));
  if (!ret) UPB_LONGJMP(ctx->err, kUpb_UnknownCompareResult_OutOfMemory);
  ret->fields = arr_base;
  ret->size = arr_ptr - arr_base;
  ret->capacity = arr_end - arr_base;
  return ret;
}

static upb_UnknownFields *upb_UnknownFields_Build(upb_UnknownField_Context *ctx,
                                                  const char *buf,
                                                  size_t size) {
  ctx->end = buf + size;
  upb_UnknownFields *fields = upb_UnknownFields_DoBuild(ctx, &buf);
  UPB_ASSERT(buf == ctx->end);
  return fields;
}

static bool upb_UnknownFields_IsEqual(const upb_UnknownFields *uf1,
                                      const upb_UnknownFields *uf2) {
  if (uf1->size != uf2->size) return false;
  for (size_t i = 0, n = uf1->size; i < n; i++) {
    upb_UnknownField *f1 = &uf1->fields[i];
    upb_UnknownField *f2 = &uf2->fields[i];
    if (f1->tag != f2->tag) return false;
    switch (f1->tag & 7) {
      case UPB_WIRE_TYPE_VARINT:
        if (f1->data.varint != f2->data.varint) return false;
        break;
      case UPB_WIRE_TYPE_64BIT:
        if (f1->data.uint64 != f2->data.uint64) return false;
        break;
      case UPB_WIRE_TYPE_32BIT:
        if (f1->data.uint32 != f2->data.uint32) return false;
        break;
      case UPB_WIRE_TYPE_DELIMITED:
        if (!upb_strview_eql(f1->data.delimited, f2->data.delimited)) {
          return false;
        }
        break;
      case UPB_WIRE_TYPE_START_GROUP:
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

upb_UnknownCompareResult upb_Message_UnknownFieldsAreEqual(const upb_msg *msg1,
                                                           const upb_msg *msg2,
                                                           int max_depth) {
  size_t size1, size2;
  const char *buf1 = upb_msg_getunknown(msg1, &size1);
  const char *buf2 = upb_msg_getunknown(msg2, &size2);
  if (size1 == 0 && size2 == 0) return kUpb_UnknownCompareResult_Equal;
  if (size1 == 0 || size2 == 0) return kUpb_UnknownCompareResult_NotEqual;
  if (memcmp(buf1, buf2, size1) == 0) return kUpb_UnknownCompareResult_Equal;

  upb_UnknownField_Context ctx = {
    .arena = upb_arena_new(),
    .depth = max_depth,
  };
  if (!ctx.arena) return kUpb_UnknownCompareResult_OutOfMemory;

  upb_UnknownFields *uf1 = upb_UnknownFields_Build(&ctx, buf1, size1);
  upb_UnknownFields *uf2 = upb_UnknownFields_Build(&ctx, buf2, size2);
  bool ret = upb_UnknownFields_IsEqual(uf1, uf2);
  upb_arena_free(ctx.arena);
  return ret ? kUpb_UnknownCompareResult_Equal
             : kUpb_UnknownCompareResult_NotEqual;
}
