// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/upb/jspb/encode.h"

#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "upb/upb/base/descriptor_constants.h"
#include "upb/upb/base/status.h"
#include "upb/upb/base/string_view.h"
#include "upb/upb/collections/array.h"
#include "upb/upb/collections/map.h"
#include "upb/upb/lex/round_trip.h"
#include "upb/upb/mem/arena.h"
#include "upb/upb/message/internal/accessors.h"
#include "upb/upb/message/types.h"
#include "upb/upb/mini_table/extension_registry.h"
#include "upb/upb/mini_table/field.h"
#include "upb/upb/mini_table/internal/field.h"
#include "upb/upb/mini_table/message.h"
#include "upb/upb/port/vsnprintf_compat.h"

// Must be last.
#include "upb/upb/port/def.inc"

typedef struct {
  char *buf, *ptr, *end;
  size_t overflow;
  int depth;
  const upb_ExtensionRegistry* extreg;
  jmp_buf err;
  upb_Status* status;
  upb_Arena* arena;
} jspbenc;

static void jspbenc_msg(jspbenc* e, const upb_Message* msg,
                        const upb_MiniTable* m);
static void jspbenc_scalar(jspbenc* e, upb_MessageValue val,
                           const upb_MiniTable* m, const upb_MiniTableField* f);
static void jspbenc_sparse_msgfields(jspbenc* e, const upb_Message* msg,
                                     const upb_MiniTable* m, bool first);

UPB_NORETURN static void jspbenc_err(jspbenc* e, const char* msg) {
  upb_Status_SetErrorMessage(e->status, msg);
  longjmp(e->err, 1);
}

UPB_PRINTF(2, 3)
UPB_NORETURN static void jspbenc_errf(jspbenc* e, const char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  upb_Status_VSetErrorFormat(e->status, fmt, argp);
  va_end(argp);
  longjmp(e->err, 1);
}

static void jspbenc_putbytes(jspbenc* e, const void* data, size_t len) {
  size_t have = e->end - e->ptr;
  if (UPB_LIKELY(have >= len)) {
    memcpy(e->ptr, data, len);
    e->ptr += len;
  } else {
    if (have) {
      memcpy(e->ptr, data, have);
      e->ptr += have;
    }
    e->overflow += (len - have);
  }
}

static void jspbenc_putstr(jspbenc* e, const char* str) {
  jspbenc_putbytes(e, str, strlen(str));
}

UPB_PRINTF(2, 3)
static void jspbenc_printf(jspbenc* e, const char* fmt, ...) {
  size_t n;
  size_t have = e->end - e->ptr;
  va_list args;

  va_start(args, fmt);
  n = _upb_vsnprintf(e->ptr, have, fmt, args);
  va_end(args);

  if (UPB_LIKELY(have > n)) {
    e->ptr += n;
  } else {
    e->ptr = UPB_PTRADD(e->ptr, have);
    e->overflow += (n - have);
  }
}

static void jspbenc_bytes(jspbenc* e, upb_StringView str) {
  /* This is the regular base64, not the "web-safe" version. */
  static const char base64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const unsigned char* ptr = (unsigned char*)str.data;
  const unsigned char* end = UPB_PTRADD(ptr, str.size);
  char buf[4];

  jspbenc_putstr(e, "\"");

  while (end - ptr >= 3) {
    buf[0] = base64[ptr[0] >> 2];
    buf[1] = base64[((ptr[0] & 0x3) << 4) | (ptr[1] >> 4)];
    buf[2] = base64[((ptr[1] & 0xf) << 2) | (ptr[2] >> 6)];
    buf[3] = base64[ptr[2] & 0x3f];
    jspbenc_putbytes(e, buf, 4);
    ptr += 3;
  }

  switch (end - ptr) {
    case 2:
      buf[0] = base64[ptr[0] >> 2];
      buf[1] = base64[((ptr[0] & 0x3) << 4) | (ptr[1] >> 4)];
      buf[2] = base64[(ptr[1] & 0xf) << 2];
      buf[3] = '=';
      jspbenc_putbytes(e, buf, 4);
      break;
    case 1:
      buf[0] = base64[ptr[0] >> 2];
      buf[1] = base64[((ptr[0] & 0x3) << 4)];
      buf[2] = '=';
      buf[3] = '=';
      jspbenc_putbytes(e, buf, 4);
      break;
  }

  jspbenc_putstr(e, "\"");
}

static void jspbenc_stringbody(jspbenc* e, upb_StringView str) {
  const char* ptr = str.data;
  const char* end = UPB_PTRADD(ptr, str.size);

  while (ptr < end) {
    switch (*ptr) {
      case '\n':
        jspbenc_putstr(e, "\\n");
        break;
      case '\r':
        jspbenc_putstr(e, "\\r");
        break;
      case '\t':
        jspbenc_putstr(e, "\\t");
        break;
      case '\"':
        jspbenc_putstr(e, "\\\"");
        break;
      case '\f':
        jspbenc_putstr(e, "\\f");
        break;
      case '\b':
        jspbenc_putstr(e, "\\b");
        break;
      case '\\':
        jspbenc_putstr(e, "\\\\");
        break;
      default:
        if ((uint8_t)*ptr < 0x20) {
          jspbenc_printf(e, "\\u%04x", (int)(uint8_t)*ptr);
        } else {
          /* This could be a non-ASCII byte.  We rely on the string being valid
           * UTF-8. */
          jspbenc_putbytes(e, ptr, 1);
        }
        break;
    }
    ptr++;
  }
}

static void jspbenc_string(jspbenc* e, upb_StringView str) {
  jspbenc_putstr(e, "\"");
  jspbenc_stringbody(e, str);
  jspbenc_putstr(e, "\"");
}

static bool upb_JspbEncode_HandleSpecialDoubles(jspbenc* e, double val) {
  if (val == INFINITY) {
    jspbenc_putstr(e, "\"Infinity\"");
  } else if (val == -INFINITY) {
    jspbenc_putstr(e, "\"-Infinity\"");
  } else if (val != val) {
    jspbenc_putstr(e, "\"NaN\"");
  } else {
    return false;
  }
  return true;
}

static void upb_JspbEncode_Double(jspbenc* e, double val) {
  if (upb_JspbEncode_HandleSpecialDoubles(e, val)) return;
  char buf[32];
  _upb_EncodeRoundTripDouble(val, buf, sizeof(buf));
  jspbenc_putstr(e, buf);
}

static void upb_JspbEncode_Float(jspbenc* e, float val) {
  if (upb_JspbEncode_HandleSpecialDoubles(e, val)) return;
  char buf[32];
  _upb_EncodeRoundTripFloat(val, buf, sizeof(buf));
  jspbenc_putstr(e, buf);
}

static void jspbenc_putsep(jspbenc* e, const char* str, bool* first) {
  if (*first) {
    *first = false;
  } else {
    jspbenc_putstr(e, str);
  }
}

static void jspbenc_put_objstart(jspbenc* e, const char* s) {
  if (--e->depth < 0) {
    jspbenc_err(e, "Recursion limit exceeded");
  }
  jspbenc_putstr(e, s);
}

static void jspbenc_put_objend(jspbenc* e, const char* s) {
  ++e->depth;
  jspbenc_putstr(e, s);
}

static void jspbenc_scalar(jspbenc* e, upb_MessageValue val,
                           const upb_MiniTable* m,
                           const upb_MiniTableField* f) {
  switch (upb_MiniTableField_CType(f)) {
    case kUpb_CType_Bool:
      jspbenc_putstr(e, val.bool_val ? "1" : "0");
      break;
    case kUpb_CType_Float:
      upb_JspbEncode_Float(e, val.float_val);
      break;
    case kUpb_CType_Double:
      upb_JspbEncode_Double(e, val.double_val);
      break;
    case kUpb_CType_Int32:
    case kUpb_CType_Enum:
      jspbenc_printf(e, "%" PRId32, val.int32_val);
      break;
    case kUpb_CType_UInt32:
      jspbenc_printf(e, "%" PRIu32, val.uint32_val);
      break;
    case kUpb_CType_Int64:
      jspbenc_printf(e, "\"%" PRId64 "\"", val.int64_val);
      break;
    case kUpb_CType_UInt64:
      jspbenc_printf(e, "\"%" PRIu64 "\"", val.uint64_val);
      break;
    case kUpb_CType_String:
      jspbenc_string(e, val.str_val);
      break;
    case kUpb_CType_Bytes:
      jspbenc_bytes(e, val.str_val);
      break;
    case kUpb_CType_Message:
      jspbenc_msg(e, val.msg_val, upb_MiniTable_GetSubMessageTable(m, f));
      break;
  }
}

static void jspbenc_array(jspbenc* e, const upb_Array* arr,
                          const upb_MiniTable* m,
                          const upb_MiniTableField* f) {
  _upb_MiniTableField_CheckIsArray(f);
  size_t i;
  size_t size = arr ? upb_Array_Size(arr) : 0;
  bool first = true;

  jspbenc_put_objstart(e, "[");

  for (i = 0; i < size; i++) {
    jspbenc_putsep(e, ",", &first);
    jspbenc_scalar(e, upb_Array_Get(arr, i), m, f);
  }

  jspbenc_put_objend(e, "]");
}

static void jspbenc_map(jspbenc* e, const upb_Map* map, const upb_MiniTable* m,
                        const upb_MiniTableField* f) {
  jspbenc_put_objstart(e, "[");

  const upb_MiniTable* entry = upb_MiniTable_GetSubMessageTable(m, f);
  const upb_MiniTableField* key_f = upb_MiniTable_FindFieldByNumber(entry, 1);
  const upb_MiniTableField* val_f = upb_MiniTable_FindFieldByNumber(entry, 2);

  if (map) {
    size_t iter = kUpb_Map_Begin;
    bool first = true;

    upb_MessageValue key, val;
    while (upb_Map_Next(map, &key, &val, &iter)) {
      jspbenc_putsep(e, ",", &first);
      jspbenc_put_objstart(e, "[");
      jspbenc_scalar(e, key, m, key_f);
      jspbenc_putstr(e, ",");
      jspbenc_scalar(e, val, m, val_f);
      jspbenc_put_objend(e, "]");
    }
  }
  jspbenc_put_objend(e, "]");
}

static bool jspbenc_shouldencode(jspbenc* e, const upb_Message* msg,
                                 const upb_MiniTableField* f) {
  if (f->presence == 0) {
    /* Proto3 presence or map/array. */
    const void* mem = UPB_PTR_AT(msg, f->offset, void);
    switch (_upb_MiniTableField_GetRep(f)) {
      case kUpb_FieldRep_1Byte: {
        char ch;
        memcpy(&ch, mem, 1);
        return ch != 0;
      }
      case kUpb_FieldRep_4Byte: {
        uint32_t u32;
        memcpy(&u32, mem, 4);
        return u32 != 0;
      }
      case kUpb_FieldRep_8Byte: {
        uint64_t u64;
        memcpy(&u64, mem, 8);
        return u64 != 0;
      }
      case kUpb_FieldRep_StringView: {
        const upb_StringView* str = (const upb_StringView*)mem;
        return str->size != 0;
      }
      default:
        UPB_UNREACHABLE();
    }
  } else if (f->presence > 0) {
    /* Proto2 presence: hasbit. */
    return _upb_hasbit_field(msg, f);
  } else {
    /* Field is in a oneof. */
    return _upb_getoneofcase_field(msg, f) == f->number;
  }
}

static void jspbenc_sparse_fieldval(jspbenc* e, const upb_Message* msg,
                                    const upb_MiniTable* m,
                                    const upb_MiniTableField* f, bool* first) {
  upb_MessageValue val;
  upb_MessageValue def;

  if (!jspbenc_shouldencode(e, msg, f)) {
    return;
  }

  // TODO: Use the type-specific getters instead of generic _GetField.
  memset(&def, 0, sizeof(def));
  _upb_Message_GetField(msg, f, &def, &val);

  jspbenc_putsep(e, ",", first);

  jspbenc_printf(e, "\"%" PRIu32 "\":", f->number);

  if (upb_FieldMode_Get(f) == kUpb_FieldMode_Map) {
    jspbenc_map(e, val.map_val, m, f);
  } else if (upb_FieldMode_Get(f) == kUpb_FieldMode_Array) {
    jspbenc_array(e, val.array_val, m, f);
  } else {
    jspbenc_scalar(e, val, m, f);
  }
}

static void jspbenc_sparse_msgfields(jspbenc* e, const upb_Message* msg,
                                     const upb_MiniTable* m, bool first) {
  if (m->field_count) {
    bool first = true;
    const upb_MiniTableField* f = &m->fields[m->field_count];
    const upb_MiniTableField* first_field = &m->fields[0];
    while (f != first_field) {
      f--;
      jspbenc_sparse_fieldval(e, msg, m, f, &first);
    }
  }
}

static void jspbenc_msg(jspbenc* e, const upb_Message* msg,
                        const upb_MiniTable* m) {

  // This currently encodes all fields into the sparse object and does not use
  // the dense representation at all, which is valid but unnecessarily
  // inefficient jspb representation.
  jspbenc_put_objstart(e, "[{");
  jspbenc_sparse_msgfields(e, msg, m, true);
  jspbenc_put_objend(e, "}]");
}

static size_t jspbenc_nullz(jspbenc* e, size_t size) {
  size_t ret = e->ptr - e->buf + e->overflow;

  if (size > 0) {
    if (e->ptr == e->end) e->ptr--;
    *e->ptr = '\0';
  }

  return ret;
}

static size_t upb_JspbEncoder_Encode(jspbenc* const e,
                                     const upb_Message* const msg,
                                     const upb_MiniTable* const m,
                                     const size_t size) {
  if (UPB_SETJMP(e->err) != 0) {
    return -1;
  }

  jspbenc_msg(e, msg, m);
  if (e->arena) upb_Arena_Free(e->arena);
  return jspbenc_nullz(e, size);
}

size_t upb_JspbEncode(const upb_Message* msg, const upb_MiniTable* m,
                      const upb_ExtensionRegistry* extreg, int options_unused,
                      char* buf, size_t size, upb_Status* status) {
  jspbenc e;

  e.buf = buf;
  e.ptr = buf;
  e.end = UPB_PTRADD(buf, size);
  e.overflow = 0;
  e.depth = 64;
  e.extreg = extreg;
  e.status = status;
  e.arena = NULL;

  return upb_JspbEncoder_Encode(&e, msg, m, size);
}
