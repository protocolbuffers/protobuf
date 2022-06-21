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

#include "upb/decode.h"

#include <setjmp.h>
#include <string.h>

#include "upb/internal/decode.h"
#include "upb/internal/upb.h"
#include "upb/upb.h"

/* Must be last. */
#include "upb/port_def.inc"

/* Maps descriptor type -> elem_size_lg2.  */
static const uint8_t desctype_to_elem_size_lg2[] = {
    -1,             /* invalid descriptor type */
    3,              /* DOUBLE */
    2,              /* FLOAT */
    3,              /* INT64 */
    3,              /* UINT64 */
    2,              /* INT32 */
    3,              /* FIXED64 */
    2,              /* FIXED32 */
    0,              /* BOOL */
    UPB_SIZE(3, 4), /* STRING */
    UPB_SIZE(2, 3), /* GROUP */
    UPB_SIZE(2, 3), /* MESSAGE */
    UPB_SIZE(3, 4), /* BYTES */
    2,              /* UINT32 */
    2,              /* ENUM */
    2,              /* SFIXED32 */
    3,              /* SFIXED64 */
    2,              /* SINT32 */
    3,              /* SINT64 */
};

/* Maps descriptor type -> upb map size.  */
static const uint8_t desctype_to_mapsize[] = {
    -1,                 /* invalid descriptor type */
    8,                  /* DOUBLE */
    4,                  /* FLOAT */
    8,                  /* INT64 */
    8,                  /* UINT64 */
    4,                  /* INT32 */
    8,                  /* FIXED64 */
    4,                  /* FIXED32 */
    1,                  /* BOOL */
    UPB_MAPTYPE_STRING, /* STRING */
    sizeof(void*),      /* GROUP */
    sizeof(void*),      /* MESSAGE */
    UPB_MAPTYPE_STRING, /* BYTES */
    4,                  /* UINT32 */
    4,                  /* ENUM */
    4,                  /* SFIXED32 */
    8,                  /* SFIXED64 */
    4,                  /* SINT32 */
    8,                  /* SINT64 */
};

static const unsigned FIXED32_OK_MASK = (1 << kUpb_FieldType_Float) |
                                        (1 << kUpb_FieldType_Fixed32) |
                                        (1 << kUpb_FieldType_SFixed32);

static const unsigned FIXED64_OK_MASK = (1 << kUpb_FieldType_Double) |
                                        (1 << kUpb_FieldType_Fixed64) |
                                        (1 << kUpb_FieldType_SFixed64);

/* Three fake field types for MessageSet. */
#define TYPE_MSGSET_ITEM 19
#define TYPE_COUNT 19

/* Op: an action to be performed for a wire-type/field-type combination. */
#define OP_UNKNOWN -1 /* Unknown field. */
#define OP_MSGSET_ITEM -2
#define OP_SCALAR_LG2(n) (n) /* n in [0, 2, 3] => op in [0, 2, 3] */
#define OP_ENUM 1
#define OP_STRING 4
#define OP_BYTES 5
#define OP_SUBMSG 6
/* Scalar fields use only ops above. Repeated fields can use any op.  */
#define OP_FIXPCK_LG2(n) (n + 5) /* n in [2, 3] => op in [7, 8] */
#define OP_VARPCK_LG2(n) (n + 9) /* n in [0, 2, 3] => op in [9, 11, 12] */
#define OP_PACKED_ENUM 13

static const int8_t varint_ops[] = {
    OP_UNKNOWN,       /* field not found */
    OP_UNKNOWN,       /* DOUBLE */
    OP_UNKNOWN,       /* FLOAT */
    OP_SCALAR_LG2(3), /* INT64 */
    OP_SCALAR_LG2(3), /* UINT64 */
    OP_SCALAR_LG2(2), /* INT32 */
    OP_UNKNOWN,       /* FIXED64 */
    OP_UNKNOWN,       /* FIXED32 */
    OP_SCALAR_LG2(0), /* BOOL */
    OP_UNKNOWN,       /* STRING */
    OP_UNKNOWN,       /* GROUP */
    OP_UNKNOWN,       /* MESSAGE */
    OP_UNKNOWN,       /* BYTES */
    OP_SCALAR_LG2(2), /* UINT32 */
    OP_ENUM,          /* ENUM */
    OP_UNKNOWN,       /* SFIXED32 */
    OP_UNKNOWN,       /* SFIXED64 */
    OP_SCALAR_LG2(2), /* SINT32 */
    OP_SCALAR_LG2(3), /* SINT64 */
    OP_UNKNOWN,       /* MSGSET_ITEM */
};

static const int8_t delim_ops[] = {
    /* For non-repeated field type. */
    OP_UNKNOWN, /* field not found */
    OP_UNKNOWN, /* DOUBLE */
    OP_UNKNOWN, /* FLOAT */
    OP_UNKNOWN, /* INT64 */
    OP_UNKNOWN, /* UINT64 */
    OP_UNKNOWN, /* INT32 */
    OP_UNKNOWN, /* FIXED64 */
    OP_UNKNOWN, /* FIXED32 */
    OP_UNKNOWN, /* BOOL */
    OP_STRING,  /* STRING */
    OP_UNKNOWN, /* GROUP */
    OP_SUBMSG,  /* MESSAGE */
    OP_BYTES,   /* BYTES */
    OP_UNKNOWN, /* UINT32 */
    OP_UNKNOWN, /* ENUM */
    OP_UNKNOWN, /* SFIXED32 */
    OP_UNKNOWN, /* SFIXED64 */
    OP_UNKNOWN, /* SINT32 */
    OP_UNKNOWN, /* SINT64 */
    OP_UNKNOWN, /* MSGSET_ITEM */
    /* For repeated field type. */
    OP_FIXPCK_LG2(3), /* REPEATED DOUBLE */
    OP_FIXPCK_LG2(2), /* REPEATED FLOAT */
    OP_VARPCK_LG2(3), /* REPEATED INT64 */
    OP_VARPCK_LG2(3), /* REPEATED UINT64 */
    OP_VARPCK_LG2(2), /* REPEATED INT32 */
    OP_FIXPCK_LG2(3), /* REPEATED FIXED64 */
    OP_FIXPCK_LG2(2), /* REPEATED FIXED32 */
    OP_VARPCK_LG2(0), /* REPEATED BOOL */
    OP_STRING,        /* REPEATED STRING */
    OP_SUBMSG,        /* REPEATED GROUP */
    OP_SUBMSG,        /* REPEATED MESSAGE */
    OP_BYTES,         /* REPEATED BYTES */
    OP_VARPCK_LG2(2), /* REPEATED UINT32 */
    OP_PACKED_ENUM,   /* REPEATED ENUM */
    OP_FIXPCK_LG2(2), /* REPEATED SFIXED32 */
    OP_FIXPCK_LG2(3), /* REPEATED SFIXED64 */
    OP_VARPCK_LG2(2), /* REPEATED SINT32 */
    OP_VARPCK_LG2(3), /* REPEATED SINT64 */
    /* Omitting MSGSET_*, because we never emit a repeated msgset type */
};

typedef union {
  bool bool_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  uint32_t size;
} wireval;

static const char* decode_msg(upb_Decoder* d, const char* ptr, upb_Message* msg,
                              const upb_MiniTable* layout);

UPB_NORETURN static void* decode_err(upb_Decoder* d, upb_DecodeStatus status) {
  assert(status != kUpb_DecodeStatus_Ok);
  UPB_LONGJMP(d->err, status);
}

const char* fastdecode_err(upb_Decoder* d, int status) {
  assert(status != kUpb_DecodeStatus_Ok);
  UPB_LONGJMP(d->err, status);
  return NULL;
}
static void decode_verifyutf8(upb_Decoder* d, const char* buf, int len) {
  if (!decode_verifyutf8_inl(buf, len))
    decode_err(d, kUpb_DecodeStatus_BadUtf8);
}

static bool decode_reserve(upb_Decoder* d, upb_Array* arr, size_t elem) {
  bool need_realloc = arr->size - arr->len < elem;
  if (need_realloc && !_upb_array_realloc(arr, arr->len + elem, &d->arena)) {
    decode_err(d, kUpb_DecodeStatus_OutOfMemory);
  }
  return need_realloc;
}

typedef struct {
  const char* ptr;
  uint64_t val;
} decode_vret;

UPB_NOINLINE
static decode_vret decode_longvarint64(const char* ptr, uint64_t val) {
  decode_vret ret = {NULL, 0};
  uint64_t byte;
  int i;
  for (i = 1; i < 10; i++) {
    byte = (uint8_t)ptr[i];
    val += (byte - 1) << (i * 7);
    if (!(byte & 0x80)) {
      ret.ptr = ptr + i + 1;
      ret.val = val;
      return ret;
    }
  }
  return ret;
}

UPB_FORCEINLINE
static const char* decode_varint64(upb_Decoder* d, const char* ptr,
                                   uint64_t* val) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  } else {
    decode_vret res = decode_longvarint64(ptr, byte);
    if (!res.ptr) return decode_err(d, kUpb_DecodeStatus_Malformed);
    *val = res.val;
    return res.ptr;
  }
}

UPB_FORCEINLINE
static const char* decode_tag(upb_Decoder* d, const char* ptr, uint32_t* val) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  } else {
    const char* start = ptr;
    decode_vret res = decode_longvarint64(ptr, byte);
    if (!res.ptr || res.ptr - start > 5 || res.val > UINT32_MAX) {
      return decode_err(d, kUpb_DecodeStatus_Malformed);
    }
    *val = res.val;
    return res.ptr;
  }
}

UPB_FORCEINLINE
static const char* upb_Decoder_DecodeSize(upb_Decoder* d, const char* ptr,
                                          uint32_t* size) {
  uint64_t size64;
  ptr = decode_varint64(d, ptr, &size64);
  if (size64 >= INT32_MAX || ptr - d->end + (int)size64 > d->limit) {
    decode_err(d, kUpb_DecodeStatus_Malformed);
  }
  *size = size64;
  return ptr;
}

static void decode_munge_int32(wireval* val) {
  if (!_upb_IsLittleEndian()) {
    /* The next stage will memcpy(dst, &val, 4) */
    val->uint32_val = val->uint64_val;
  }
}

static void decode_munge(int type, wireval* val) {
  switch (type) {
    case kUpb_FieldType_Bool:
      val->bool_val = val->uint64_val != 0;
      break;
    case kUpb_FieldType_SInt32: {
      uint32_t n = val->uint64_val;
      val->uint32_val = (n >> 1) ^ -(int32_t)(n & 1);
      break;
    }
    case kUpb_FieldType_SInt64: {
      uint64_t n = val->uint64_val;
      val->uint64_val = (n >> 1) ^ -(int64_t)(n & 1);
      break;
    }
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_UInt32:
    case kUpb_FieldType_Enum:
      decode_munge_int32(val);
      break;
  }
}

static upb_Message* decode_newsubmsg(upb_Decoder* d,
                                     const upb_MiniTable_Sub* subs,
                                     const upb_MiniTable_Field* field) {
  const upb_MiniTable* subl = subs[field->submsg_index].submsg;
  upb_Message* msg = _upb_Message_New_inl(subl, &d->arena);
  if (!msg) decode_err(d, kUpb_DecodeStatus_OutOfMemory);
  return msg;
}

UPB_NOINLINE
const char* decode_isdonefallback(upb_Decoder* d, const char* ptr,
                                  int overrun) {
  int status;
  ptr = decode_isdonefallback_inl(d, ptr, overrun, &status);
  if (ptr == NULL) {
    return decode_err(d, status);
  }
  return ptr;
}

static const char* decode_readstr(upb_Decoder* d, const char* ptr, int size,
                                  upb_StringView* str) {
  if (d->options & kUpb_DecodeOption_AliasString) {
    str->data = ptr;
  } else {
    char* data = upb_Arena_Malloc(&d->arena, size);
    if (!data) return decode_err(d, kUpb_DecodeStatus_OutOfMemory);
    memcpy(data, ptr, size);
    str->data = data;
  }
  str->size = size;
  return ptr + size;
}

UPB_FORCEINLINE
static const char* decode_tosubmsg2(upb_Decoder* d, const char* ptr,
                                    upb_Message* submsg,
                                    const upb_MiniTable* subl, int size) {
  int saved_delta = decode_pushlimit(d, ptr, size);
  if (--d->depth < 0) return decode_err(d, kUpb_DecodeStatus_MaxDepthExceeded);
  ptr = decode_msg(d, ptr, submsg, subl);
  if (d->end_group != DECODE_NOGROUP)
    return decode_err(d, kUpb_DecodeStatus_Malformed);
  decode_poplimit(d, ptr, saved_delta);
  d->depth++;
  return ptr;
}

UPB_FORCEINLINE
static const char* decode_tosubmsg(upb_Decoder* d, const char* ptr,
                                   upb_Message* submsg,
                                   const upb_MiniTable_Sub* subs,
                                   const upb_MiniTable_Field* field, int size) {
  return decode_tosubmsg2(d, ptr, submsg, subs[field->submsg_index].submsg,
                          size);
}

UPB_FORCEINLINE
static const char* decode_group(upb_Decoder* d, const char* ptr,
                                upb_Message* submsg, const upb_MiniTable* subl,
                                uint32_t number) {
  if (--d->depth < 0) return decode_err(d, kUpb_DecodeStatus_MaxDepthExceeded);
  if (decode_isdone(d, &ptr)) {
    return decode_err(d, kUpb_DecodeStatus_Malformed);
  }
  ptr = decode_msg(d, ptr, submsg, subl);
  if (d->end_group != number) return decode_err(d, kUpb_DecodeStatus_Malformed);
  d->end_group = DECODE_NOGROUP;
  d->depth++;
  return ptr;
}

UPB_FORCEINLINE
static const char* decode_togroup(upb_Decoder* d, const char* ptr,
                                  upb_Message* submsg,
                                  const upb_MiniTable_Sub* subs,
                                  const upb_MiniTable_Field* field) {
  const upb_MiniTable* subl = subs[field->submsg_index].submsg;
  return decode_group(d, ptr, submsg, subl, field->number);
}

static char* upb_Decoder_EncodeVarint32(uint32_t val, char* ptr) {
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    *(ptr++) = byte;
  } while (val);
  return ptr;
}

static void upb_Decode_AddUnknownVarints(upb_Decoder* d, upb_Message* msg,
                                         uint32_t val1, uint32_t val2) {
  char buf[20];
  char* end = buf;
  end = upb_Decoder_EncodeVarint32(val1, end);
  end = upb_Decoder_EncodeVarint32(val2, end);

  if (!_upb_Message_AddUnknown(msg, buf, end - buf, &d->arena)) {
    decode_err(d, kUpb_DecodeStatus_OutOfMemory);
  }
}

UPB_NOINLINE
static bool decode_checkenum_slow(upb_Decoder* d, const char* ptr,
                                  upb_Message* msg, const upb_MiniTable_Enum* e,
                                  const upb_MiniTable_Field* field,
                                  uint32_t v) {
  // OPT: binary search long lists?
  int n = e->value_count;
  for (int i = 0; i < n; i++) {
    if ((uint32_t)e->values[i] == v) return true;
  }

  // Unrecognized enum goes into unknown fields.
  // For packed fields the tag could be arbitrarily far in the past, so we
  // just re-encode the tag and value here.
  uint32_t tag = ((uint32_t)field->number << 3) | kUpb_WireType_Varint;
  upb_Decode_AddUnknownVarints(d, msg, tag, v);
  return false;
}

UPB_FORCEINLINE
static bool decode_checkenum(upb_Decoder* d, const char* ptr, upb_Message* msg,
                             const upb_MiniTable_Enum* e,
                             const upb_MiniTable_Field* field, wireval* val) {
  uint32_t v = val->uint32_val;

  if (UPB_LIKELY(v < 64) && UPB_LIKELY(((1ULL << v) & e->mask))) return true;

  return decode_checkenum_slow(d, ptr, msg, e, field, v);
}

UPB_NOINLINE
static const char* decode_enum_toarray(upb_Decoder* d, const char* ptr,
                                       upb_Message* msg, upb_Array* arr,
                                       const upb_MiniTable_Sub* subs,
                                       const upb_MiniTable_Field* field,
                                       wireval* val) {
  const upb_MiniTable_Enum* e = subs[field->submsg_index].subenum;
  if (!decode_checkenum(d, ptr, msg, e, field, val)) return ptr;
  void* mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->len * 4, void);
  arr->len++;
  memcpy(mem, val, 4);
  return ptr;
}

UPB_FORCEINLINE
static const char* decode_fixed_packed(upb_Decoder* d, const char* ptr,
                                       upb_Array* arr, wireval* val,
                                       const upb_MiniTable_Field* field,
                                       int lg2) {
  int mask = (1 << lg2) - 1;
  size_t count = val->size >> lg2;
  if ((val->size & mask) != 0) {
    // Length isn't a round multiple of elem size.
    return decode_err(d, kUpb_DecodeStatus_Malformed);
  }
  decode_reserve(d, arr, count);
  void* mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << lg2, void);
  arr->len += count;
  // Note: if/when the decoder supports multi-buffer input, we will need to
  // handle buffer seams here.
  if (_upb_IsLittleEndian()) {
    memcpy(mem, ptr, val->size);
    ptr += val->size;
  } else {
    const char* end = ptr + val->size;
    char* dst = mem;
    while (ptr < end) {
      if (lg2 == 2) {
        uint32_t val;
        memcpy(&val, ptr, sizeof(val));
        val = _upb_BigEndian_Swap32(val);
        memcpy(dst, &val, sizeof(val));
      } else {
        UPB_ASSERT(lg2 == 3);
        uint64_t val;
        memcpy(&val, ptr, sizeof(val));
        val = _upb_BigEndian_Swap64(val);
        memcpy(dst, &val, sizeof(val));
      }
      ptr += 1 << lg2;
      dst += 1 << lg2;
    }
  }

  return ptr;
}

UPB_FORCEINLINE
static const char* decode_varint_packed(upb_Decoder* d, const char* ptr,
                                        upb_Array* arr, wireval* val,
                                        const upb_MiniTable_Field* field,
                                        int lg2) {
  int scale = 1 << lg2;
  int saved_limit = decode_pushlimit(d, ptr, val->size);
  char* out = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << lg2, void);
  while (!decode_isdone(d, &ptr)) {
    wireval elem;
    ptr = decode_varint64(d, ptr, &elem.uint64_val);
    decode_munge(field->descriptortype, &elem);
    if (decode_reserve(d, arr, 1)) {
      out = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << lg2, void);
    }
    arr->len++;
    memcpy(out, &elem, scale);
    out += scale;
  }
  decode_poplimit(d, ptr, saved_limit);
  return ptr;
}

UPB_NOINLINE
static const char* decode_enum_packed(upb_Decoder* d, const char* ptr,
                                      upb_Message* msg, upb_Array* arr,
                                      const upb_MiniTable_Sub* subs,
                                      const upb_MiniTable_Field* field,
                                      wireval* val) {
  const upb_MiniTable_Enum* e = subs[field->submsg_index].subenum;
  int saved_limit = decode_pushlimit(d, ptr, val->size);
  char* out = UPB_PTR_AT(_upb_array_ptr(arr), arr->len * 4, void);
  while (!decode_isdone(d, &ptr)) {
    wireval elem;
    ptr = decode_varint64(d, ptr, &elem.uint64_val);
    decode_munge_int32(&elem);
    if (!decode_checkenum(d, ptr, msg, e, field, &elem)) {
      continue;
    }
    if (decode_reserve(d, arr, 1)) {
      out = UPB_PTR_AT(_upb_array_ptr(arr), arr->len * 4, void);
    }
    arr->len++;
    memcpy(out, &elem, 4);
    out += 4;
  }
  decode_poplimit(d, ptr, saved_limit);
  return ptr;
}

static const char* decode_toarray(upb_Decoder* d, const char* ptr,
                                  upb_Message* msg,
                                  const upb_MiniTable_Sub* subs,
                                  const upb_MiniTable_Field* field,
                                  wireval* val, int op) {
  upb_Array** arrp = UPB_PTR_AT(msg, field->offset, void);
  upb_Array* arr = *arrp;
  void* mem;

  if (arr) {
    decode_reserve(d, arr, 1);
  } else {
    size_t lg2 = desctype_to_elem_size_lg2[field->descriptortype];
    arr = _upb_Array_New(&d->arena, 4, lg2);
    if (!arr) return decode_err(d, kUpb_DecodeStatus_OutOfMemory);
    *arrp = arr;
  }

  switch (op) {
    case OP_SCALAR_LG2(0):
    case OP_SCALAR_LG2(2):
    case OP_SCALAR_LG2(3):
      /* Append scalar value. */
      mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << op, void);
      arr->len++;
      memcpy(mem, val, 1 << op);
      return ptr;
    case OP_STRING:
      decode_verifyutf8(d, ptr, val->size);
      /* Fallthrough. */
    case OP_BYTES: {
      /* Append bytes. */
      upb_StringView* str = (upb_StringView*)_upb_array_ptr(arr) + arr->len;
      arr->len++;
      return decode_readstr(d, ptr, val->size, str);
    }
    case OP_SUBMSG: {
      /* Append submessage / group. */
      upb_Message* submsg = decode_newsubmsg(d, subs, field);
      *UPB_PTR_AT(_upb_array_ptr(arr), arr->len * sizeof(void*), upb_Message*) =
          submsg;
      arr->len++;
      if (UPB_UNLIKELY(field->descriptortype == kUpb_FieldType_Group)) {
        return decode_togroup(d, ptr, submsg, subs, field);
      } else {
        return decode_tosubmsg(d, ptr, submsg, subs, field, val->size);
      }
    }
    case OP_FIXPCK_LG2(2):
    case OP_FIXPCK_LG2(3):
      return decode_fixed_packed(d, ptr, arr, val, field,
                                 op - OP_FIXPCK_LG2(0));
    case OP_VARPCK_LG2(0):
    case OP_VARPCK_LG2(2):
    case OP_VARPCK_LG2(3):
      return decode_varint_packed(d, ptr, arr, val, field,
                                  op - OP_VARPCK_LG2(0));
    case OP_ENUM:
      return decode_enum_toarray(d, ptr, msg, arr, subs, field, val);
    case OP_PACKED_ENUM:
      return decode_enum_packed(d, ptr, msg, arr, subs, field, val);
    default:
      UPB_UNREACHABLE();
  }
}

static const char* decode_tomap(upb_Decoder* d, const char* ptr,
                                upb_Message* msg, const upb_MiniTable_Sub* subs,
                                const upb_MiniTable_Field* field,
                                wireval* val) {
  upb_Map** map_p = UPB_PTR_AT(msg, field->offset, upb_Map*);
  upb_Map* map = *map_p;
  upb_MapEntry ent;
  const upb_MiniTable* entry = subs[field->submsg_index].submsg;

  if (!map) {
    /* Lazily create map. */
    const upb_MiniTable_Field* key_field = &entry->fields[0];
    const upb_MiniTable_Field* val_field = &entry->fields[1];
    char key_size = desctype_to_mapsize[key_field->descriptortype];
    char val_size = desctype_to_mapsize[val_field->descriptortype];
    UPB_ASSERT(key_field->offset == 0);
    UPB_ASSERT(val_field->offset == sizeof(upb_StringView));
    map = _upb_Map_New(&d->arena, key_size, val_size);
    *map_p = map;
  }

  /* Parse map entry. */
  memset(&ent, 0, sizeof(ent));

  if (entry->fields[1].descriptortype == kUpb_FieldType_Message ||
      entry->fields[1].descriptortype == kUpb_FieldType_Group) {
    /* Create proactively to handle the case where it doesn't appear. */
    ent.v.val =
        upb_value_ptr(_upb_Message_New(entry->subs[0].submsg, &d->arena));
  }

  const char* start = ptr;
  ptr = decode_tosubmsg(d, ptr, &ent.k, subs, field, val->size);
  // check if ent had any unknown fields
  size_t size;
  upb_Message_GetUnknown(&ent.k, &size);
  if (size != 0) {
    uint32_t tag = ((uint32_t)field->number << 3) | kUpb_WireType_Delimited;
    upb_Decode_AddUnknownVarints(d, msg, tag, (uint32_t)(ptr - start));
    if (!_upb_Message_AddUnknown(msg, start, ptr - start, &d->arena)) {
      decode_err(d, kUpb_DecodeStatus_OutOfMemory);
    }
  } else {
    if (_upb_Map_Insert(map, &ent.k, map->key_size, &ent.v, map->val_size,
                        &d->arena) == _kUpb_MapInsertStatus_OutOfMemory) {
      decode_err(d, kUpb_DecodeStatus_OutOfMemory);
    }
  }
  return ptr;
}

static const char* decode_tomsg(upb_Decoder* d, const char* ptr,
                                upb_Message* msg, const upb_MiniTable_Sub* subs,
                                const upb_MiniTable_Field* field, wireval* val,
                                int op) {
  void* mem = UPB_PTR_AT(msg, field->offset, void);
  int type = field->descriptortype;

  if (UPB_UNLIKELY(op == OP_ENUM) &&
      !decode_checkenum(d, ptr, msg, subs[field->submsg_index].subenum, field,
                        val)) {
    return ptr;
  }

  /* Set presence if necessary. */
  if (field->presence > 0) {
    _upb_sethas_field(msg, field);
  } else if (field->presence < 0) {
    /* Oneof case */
    uint32_t* oneof_case = _upb_oneofcase_field(msg, field);
    if (op == OP_SUBMSG && *oneof_case != field->number) {
      memset(mem, 0, sizeof(void*));
    }
    *oneof_case = field->number;
  }

  /* Store into message. */
  switch (op) {
    case OP_SUBMSG: {
      upb_Message** submsgp = mem;
      upb_Message* submsg = *submsgp;
      if (!submsg) {
        submsg = decode_newsubmsg(d, subs, field);
        *submsgp = submsg;
      }
      if (UPB_UNLIKELY(type == kUpb_FieldType_Group)) {
        ptr = decode_togroup(d, ptr, submsg, subs, field);
      } else {
        ptr = decode_tosubmsg(d, ptr, submsg, subs, field, val->size);
      }
      break;
    }
    case OP_STRING:
      decode_verifyutf8(d, ptr, val->size);
      /* Fallthrough. */
    case OP_BYTES:
      return decode_readstr(d, ptr, val->size, mem);
    case OP_SCALAR_LG2(3):
      memcpy(mem, val, 8);
      break;
    case OP_ENUM:
    case OP_SCALAR_LG2(2):
      memcpy(mem, val, 4);
      break;
    case OP_SCALAR_LG2(0):
      memcpy(mem, val, 1);
      break;
    default:
      UPB_UNREACHABLE();
  }

  return ptr;
}

UPB_NOINLINE
const char* decode_checkrequired(upb_Decoder* d, const char* ptr,
                                 const upb_Message* msg,
                                 const upb_MiniTable* l) {
  assert(l->required_count);
  if (UPB_LIKELY((d->options & kUpb_DecodeOption_CheckRequired) == 0)) {
    return ptr;
  }
  uint64_t msg_head;
  memcpy(&msg_head, msg, 8);
  msg_head = _upb_BigEndian_Swap64(msg_head);
  if (upb_MiniTable_requiredmask(l) & ~msg_head) {
    d->missing_required = true;
  }
  return ptr;
}

UPB_FORCEINLINE
static bool decode_tryfastdispatch(upb_Decoder* d, const char** ptr,
                                   upb_Message* msg,
                                   const upb_MiniTable* layout) {
#if UPB_FASTTABLE
  if (layout && layout->table_mask != (unsigned char)-1) {
    uint16_t tag = fastdecode_loadtag(*ptr);
    intptr_t table = decode_totable(layout);
    *ptr = fastdecode_tagdispatch(d, *ptr, msg, table, 0, tag);
    return true;
  }
#endif
  return false;
}

static const char* upb_Decoder_SkipField(upb_Decoder* d, const char* ptr,
                                         uint32_t tag) {
  int field_number = tag >> 3;
  int wire_type = tag & 7;
  switch (wire_type) {
    case kUpb_WireType_Varint: {
      uint64_t val;
      return decode_varint64(d, ptr, &val);
    }
    case kUpb_WireType_64Bit:
      return ptr + 8;
    case kUpb_WireType_32Bit:
      return ptr + 4;
    case kUpb_WireType_Delimited: {
      uint32_t size;
      ptr = upb_Decoder_DecodeSize(d, ptr, &size);
      return ptr + size;
    }
    case kUpb_WireType_StartGroup:
      return decode_group(d, ptr, NULL, NULL, field_number);
    default:
      decode_err(d, kUpb_DecodeStatus_Malformed);
  }
}

enum {
  kStartItemTag = ((1 << 3) | kUpb_WireType_StartGroup),
  kEndItemTag = ((1 << 3) | kUpb_WireType_EndGroup),
  kTypeIdTag = ((2 << 3) | kUpb_WireType_Varint),
  kMessageTag = ((3 << 3) | kUpb_WireType_Delimited),
};

static void upb_Decoder_AddKnownMessageSetItem(
    upb_Decoder* d, upb_Message* msg, const upb_MiniTable_Extension* item_mt,
    const char* data, uint32_t size) {
  upb_Message_Extension* ext =
      _upb_Message_GetOrCreateExtension(msg, item_mt, &d->arena);
  if (UPB_UNLIKELY(!ext)) decode_err(d, kUpb_DecodeStatus_OutOfMemory);
  upb_Message* submsg = decode_newsubmsg(d, &ext->ext->sub, &ext->ext->field);
  upb_DecodeStatus status = upb_Decode(data, size, submsg, item_mt->sub.submsg,
                                       d->extreg, d->options, &d->arena);
  memcpy(&ext->data, &submsg, sizeof(submsg));
  if (status != kUpb_DecodeStatus_Ok) decode_err(d, status);
}

static void upb_Decoder_AddUnknownMessageSetItem(upb_Decoder* d,
                                                 upb_Message* msg,
                                                 uint32_t type_id,
                                                 const char* message_data,
                                                 uint32_t message_size) {
  char buf[60];
  char* ptr = buf;
  ptr = upb_Decoder_EncodeVarint32(kStartItemTag, ptr);
  ptr = upb_Decoder_EncodeVarint32(kTypeIdTag, ptr);
  ptr = upb_Decoder_EncodeVarint32(type_id, ptr);
  ptr = upb_Decoder_EncodeVarint32(kMessageTag, ptr);
  ptr = upb_Decoder_EncodeVarint32(message_size, ptr);
  char* split = ptr;

  ptr = upb_Decoder_EncodeVarint32(kEndItemTag, ptr);
  char* end = ptr;

  if (!_upb_Message_AddUnknown(msg, buf, split - buf, &d->arena) ||
      !_upb_Message_AddUnknown(msg, message_data, message_size, &d->arena) ||
      !_upb_Message_AddUnknown(msg, split, end - split, &d->arena)) {
    decode_err(d, kUpb_DecodeStatus_OutOfMemory);
  }
}

static void upb_Decoder_AddMessageSetItem(upb_Decoder* d, upb_Message* msg,
                                          const upb_MiniTable* layout,
                                          uint32_t type_id, const char* data,
                                          uint32_t size) {
  const upb_MiniTable_Extension* item_mt =
      _upb_extreg_get(d->extreg, layout, type_id);
  if (item_mt) {
    upb_Decoder_AddKnownMessageSetItem(d, msg, item_mt, data, size);
  } else {
    upb_Decoder_AddUnknownMessageSetItem(d, msg, type_id, data, size);
  }
}

static const char* upb_Decoder_DecodeMessageSetItem(
    upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* layout) {
  uint32_t type_id = 0;
  upb_StringView preserved = {NULL, 0};
  typedef enum {
    kUpb_HaveId = 1 << 0,
    kUpb_HavePayload = 1 << 1,
  } StateMask;
  StateMask state_mask = 0;
  while (!decode_isdone(d, &ptr)) {
    uint32_t tag;
    ptr = decode_tag(d, ptr, &tag);
    switch (tag) {
      case kEndItemTag:
        return ptr;
      case kTypeIdTag: {
        uint64_t tmp;
        ptr = decode_varint64(d, ptr, &tmp);
        if (state_mask & kUpb_HaveId) break;  // Ignore dup.
        state_mask |= kUpb_HaveId;
        type_id = tmp;
        if (state_mask & kUpb_HavePayload) {
          upb_Decoder_AddMessageSetItem(d, msg, layout, type_id, preserved.data,
                                        preserved.size);
        }
        break;
      }
      case kMessageTag: {
        uint32_t size;
        ptr = upb_Decoder_DecodeSize(d, ptr, &size);
        const char* data = ptr;
        ptr += size;
        if (state_mask & kUpb_HavePayload) break;  // Ignore dup.
        state_mask |= kUpb_HavePayload;
        if (state_mask & kUpb_HaveId) {
          upb_Decoder_AddMessageSetItem(d, msg, layout, type_id, data, size);
        } else {
          // Out of order, we must preserve the payload.
          preserved.data = data;
          preserved.size = size;
        }
        break;
      }
      default:
        // We do not preserve unexpected fields inside a message set item.
        ptr = upb_Decoder_SkipField(d, ptr, tag);
        break;
    }
  }
  decode_err(d, kUpb_DecodeStatus_Malformed);
}

static const upb_MiniTable_Field* decode_findfield(upb_Decoder* d,
                                                   const upb_MiniTable* l,
                                                   uint32_t field_number,
                                                   int* last_field_index) {
  static upb_MiniTable_Field none = {0, 0, 0, 0, 0, 0};
  if (l == NULL) return &none;

  size_t idx = ((size_t)field_number) - 1;  // 0 wraps to SIZE_MAX
  if (idx < l->dense_below) {
    /* Fastest case: index into dense fields. */
    goto found;
  }

  if (l->dense_below < l->field_count) {
    /* Linear search non-dense fields. Resume scanning from last_field_index
     * since fields are usually in order. */
    int last = *last_field_index;
    for (idx = last; idx < l->field_count; idx++) {
      if (l->fields[idx].number == field_number) {
        goto found;
      }
    }

    for (idx = l->dense_below; idx < last; idx++) {
      if (l->fields[idx].number == field_number) {
        goto found;
      }
    }
  }

  if (d->extreg) {
    switch (l->ext) {
      case kUpb_ExtMode_Extendable: {
        const upb_MiniTable_Extension* ext =
            _upb_extreg_get(d->extreg, l, field_number);
        if (ext) return &ext->field;
        break;
      }
      case kUpb_ExtMode_IsMessageSet:
        if (field_number == _UPB_MSGSET_ITEM) {
          static upb_MiniTable_Field item = {0, 0, 0, 0, TYPE_MSGSET_ITEM, 0};
          return &item;
        }
        break;
    }
  }

  return &none; /* Unknown field. */

found:
  UPB_ASSERT(l->fields[idx].number == field_number);
  *last_field_index = idx;
  return &l->fields[idx];
}

UPB_FORCEINLINE
static const char* decode_wireval(upb_Decoder* d, const char* ptr,
                                  const upb_MiniTable_Field* field,
                                  int wire_type, wireval* val, int* op) {
  switch (wire_type) {
    case kUpb_WireType_Varint:
      ptr = decode_varint64(d, ptr, &val->uint64_val);
      *op = varint_ops[field->descriptortype];
      decode_munge(field->descriptortype, val);
      return ptr;
    case kUpb_WireType_32Bit:
      memcpy(&val->uint32_val, ptr, 4);
      val->uint32_val = _upb_BigEndian_Swap32(val->uint32_val);
      *op = OP_SCALAR_LG2(2);
      if (((1 << field->descriptortype) & FIXED32_OK_MASK) == 0) {
        *op = OP_UNKNOWN;
      }
      return ptr + 4;
    case kUpb_WireType_64Bit:
      memcpy(&val->uint64_val, ptr, 8);
      val->uint64_val = _upb_BigEndian_Swap64(val->uint64_val);
      *op = OP_SCALAR_LG2(3);
      if (((1 << field->descriptortype) & FIXED64_OK_MASK) == 0) {
        *op = OP_UNKNOWN;
      }
      return ptr + 8;
    case kUpb_WireType_Delimited: {
      int ndx = field->descriptortype;
      if (upb_FieldMode_Get(field) == kUpb_FieldMode_Array) ndx += TYPE_COUNT;
      ptr = upb_Decoder_DecodeSize(d, ptr, &val->size);
      *op = delim_ops[ndx];
      return ptr;
    }
    case kUpb_WireType_StartGroup:
      val->uint32_val = field->number;
      if (field->descriptortype == kUpb_FieldType_Group) {
        *op = OP_SUBMSG;
      } else if (field->descriptortype == TYPE_MSGSET_ITEM) {
        *op = OP_MSGSET_ITEM;
      } else {
        *op = OP_UNKNOWN;
      }
      return ptr;
    default:
      break;
  }
  return decode_err(d, kUpb_DecodeStatus_Malformed);
}

UPB_FORCEINLINE
static const char* decode_known(upb_Decoder* d, const char* ptr,
                                upb_Message* msg, const upb_MiniTable* layout,
                                const upb_MiniTable_Field* field, int op,
                                wireval* val) {
  const upb_MiniTable_Sub* subs = layout->subs;
  uint8_t mode = field->mode;

  if (UPB_UNLIKELY(mode & kUpb_LabelFlags_IsExtension)) {
    const upb_MiniTable_Extension* ext_layout =
        (const upb_MiniTable_Extension*)field;
    upb_Message_Extension* ext =
        _upb_Message_GetOrCreateExtension(msg, ext_layout, &d->arena);
    if (UPB_UNLIKELY(!ext)) return decode_err(d, kUpb_DecodeStatus_OutOfMemory);
    msg = &ext->data;
    subs = &ext->ext->sub;
  }

  switch (mode & kUpb_FieldMode_Mask) {
    case kUpb_FieldMode_Array:
      return decode_toarray(d, ptr, msg, subs, field, val, op);
    case kUpb_FieldMode_Map:
      return decode_tomap(d, ptr, msg, subs, field, val);
    case kUpb_FieldMode_Scalar:
      return decode_tomsg(d, ptr, msg, subs, field, val, op);
    default:
      UPB_UNREACHABLE();
  }
}

static const char* decode_reverse_skip_varint(const char* ptr, uint32_t val) {
  uint32_t seen = 0;
  do {
    ptr--;
    seen <<= 7;
    seen |= *ptr & 0x7f;
  } while (seen != val);
  return ptr;
}

static const char* decode_unknown(upb_Decoder* d, const char* ptr,
                                  upb_Message* msg, int field_number,
                                  int wire_type, wireval val) {
  if (field_number == 0) return decode_err(d, kUpb_DecodeStatus_Malformed);

  // Since unknown fields are the uncommon case, we do a little extra work here
  // to walk backwards through the buffer to find the field start.  This frees
  // up a register in the fast paths (when the field is known), which leads to
  // significant speedups in benchmarks.
  const char* start = ptr;

  if (wire_type == kUpb_WireType_Delimited) ptr += val.size;
  if (msg) {
    switch (wire_type) {
      case kUpb_WireType_Varint:
      case kUpb_WireType_Delimited:
        start--;
        while (start[-1] & 0x80) start--;
        break;
      case kUpb_WireType_32Bit:
        start -= 4;
        break;
      case kUpb_WireType_64Bit:
        start -= 8;
        break;
      default:
        break;
    }

    assert(start == d->debug_valstart);
    uint32_t tag = ((uint32_t)field_number << 3) | wire_type;
    start = decode_reverse_skip_varint(start, tag);
    assert(start == d->debug_tagstart);

    if (wire_type == kUpb_WireType_StartGroup) {
      d->unknown = start;
      d->unknown_msg = msg;
      ptr = decode_group(d, ptr, NULL, NULL, field_number);
      start = d->unknown;
      d->unknown_msg = NULL;
      d->unknown = NULL;
    }
    if (!_upb_Message_AddUnknown(msg, start, ptr - start, &d->arena)) {
      return decode_err(d, kUpb_DecodeStatus_OutOfMemory);
    }
  } else if (wire_type == kUpb_WireType_StartGroup) {
    ptr = decode_group(d, ptr, NULL, NULL, field_number);
  }
  return ptr;
}

UPB_NOINLINE
static const char* decode_msg(upb_Decoder* d, const char* ptr, upb_Message* msg,
                              const upb_MiniTable* layout) {
  int last_field_index = 0;

#if UPB_FASTTABLE
  // The first time we want to skip fast dispatch, because we may have just been
  // invoked by the fast parser to handle a case that it bailed on.
  if (!decode_isdone(d, &ptr)) goto nofast;
#endif

  while (!decode_isdone(d, &ptr)) {
    uint32_t tag;
    const upb_MiniTable_Field* field;
    int field_number;
    int wire_type;
    wireval val;
    int op;

    if (decode_tryfastdispatch(d, &ptr, msg, layout)) break;

#if UPB_FASTTABLE
  nofast:
#endif

#ifndef NDEBUG
    d->debug_tagstart = ptr;
#endif

    UPB_ASSERT(ptr < d->limit_ptr);
    ptr = decode_tag(d, ptr, &tag);
    field_number = tag >> 3;
    wire_type = tag & 7;

#ifndef NDEBUG
    d->debug_valstart = ptr;
#endif

    if (wire_type == kUpb_WireType_EndGroup) {
      d->end_group = field_number;
      return ptr;
    }

    field = decode_findfield(d, layout, field_number, &last_field_index);
    ptr = decode_wireval(d, ptr, field, wire_type, &val, &op);

    if (op >= 0) {
      ptr = decode_known(d, ptr, msg, layout, field, op, &val);
    } else {
      switch (op) {
        case OP_UNKNOWN:
          ptr = decode_unknown(d, ptr, msg, field_number, wire_type, val);
          break;
        case OP_MSGSET_ITEM:
          ptr = upb_Decoder_DecodeMessageSetItem(d, ptr, msg, layout);
          break;
      }
    }
  }

  return UPB_UNLIKELY(layout && layout->required_count)
             ? decode_checkrequired(d, ptr, msg, layout)
             : ptr;
}

const char* fastdecode_generic(struct upb_Decoder* d, const char* ptr,
                               upb_Message* msg, intptr_t table,
                               uint64_t hasbits, uint64_t data) {
  (void)data;
  *(uint32_t*)msg |= hasbits;
  return decode_msg(d, ptr, msg, decode_totablep(table));
}

static upb_DecodeStatus decode_top(struct upb_Decoder* d, const char* buf,
                                   void* msg, const upb_MiniTable* l) {
  if (!decode_tryfastdispatch(d, &buf, msg, l)) {
    decode_msg(d, buf, msg, l);
  }
  if (d->end_group != DECODE_NOGROUP) return kUpb_DecodeStatus_Malformed;
  if (d->missing_required) return kUpb_DecodeStatus_MissingRequired;
  return kUpb_DecodeStatus_Ok;
}

upb_DecodeStatus upb_Decode(const char* buf, size_t size, void* msg,
                            const upb_MiniTable* l,
                            const upb_ExtensionRegistry* extreg, int options,
                            upb_Arena* arena) {
  upb_Decoder state;
  unsigned depth = (unsigned)options >> 16;

  if (size <= 16) {
    memset(&state.patch, 0, 32);
    if (size) memcpy(&state.patch, buf, size);
    buf = state.patch;
    state.end = buf + size;
    state.limit = 0;
    options &= ~kUpb_DecodeOption_AliasString;  // Can't alias patch buf.
  } else {
    state.end = buf + size - 16;
    state.limit = 16;
  }

  state.extreg = extreg;
  state.limit_ptr = state.end;
  state.unknown_msg = NULL;
  state.depth = depth ? depth : 64;
  state.end_group = DECODE_NOGROUP;
  state.options = (uint16_t)options;
  state.missing_required = false;
  state.arena.head = arena->head;
  state.arena.last_size = arena->last_size;
  state.arena.cleanup_metadata = arena->cleanup_metadata;
  state.arena.parent = arena;

  upb_DecodeStatus status = UPB_SETJMP(state.err);
  if (UPB_LIKELY(status == kUpb_DecodeStatus_Ok)) {
    status = decode_top(&state, buf, msg, l);
  }

  arena->head.ptr = state.arena.head.ptr;
  arena->head.end = state.arena.head.end;
  arena->cleanup_metadata = state.arena.cleanup_metadata;
  return status;
}

#undef OP_UNKNOWN
#undef OP_SKIP
#undef OP_SCALAR_LG2
#undef OP_FIXPCK_LG2
#undef OP_VARPCK_LG2
#undef OP_STRING
#undef OP_BYTES
#undef OP_SUBMSG
