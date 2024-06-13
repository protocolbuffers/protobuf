// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/text/encode.h"

#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/lex/round_trip.h"
#include "upb/message/array.h"
#include "upb/message/internal/map_entry.h"
#include "upb/message/internal/map_sorter.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/value.h"
#include "upb/port/vsnprintf_compat.h"
#include "upb/reflection/def.h"
#include "upb/reflection/message.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/reader.h"
#include "upb/wire/types.h"
#include "utf8_range.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  char *buf, *ptr, *end;
  size_t overflow;
  int indent_depth;
  int options;
  const upb_DefPool* ext_pool;
  _upb_mapsorter sorter;
} txtenc;

static void txtenc_msg(txtenc* e, const upb_Message* msg,
                       const upb_MessageDef* m);

static void txtenc_putbytes(txtenc* e, const void* data, size_t len) {
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

static void txtenc_putstr(txtenc* e, const char* str) {
  txtenc_putbytes(e, str, strlen(str));
}

static void txtenc_printf(txtenc* e, const char* fmt, ...) {
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

static void txtenc_indent(txtenc* e) {
  if ((e->options & UPB_TXTENC_SINGLELINE) == 0) {
    int i = e->indent_depth;
    while (i-- > 0) {
      txtenc_putstr(e, "  ");
    }
  }
}

static void txtenc_endfield(txtenc* e) {
  if (e->options & UPB_TXTENC_SINGLELINE) {
    txtenc_putstr(e, " ");
  } else {
    txtenc_putstr(e, "\n");
  }
}

static void txtenc_enum(int32_t val, const upb_FieldDef* f, txtenc* e) {
  const upb_EnumDef* e_def = upb_FieldDef_EnumSubDef(f);
  const upb_EnumValueDef* ev = upb_EnumDef_FindValueByNumber(e_def, val);

  if (ev) {
    txtenc_printf(e, "%s", upb_EnumValueDef_Name(ev));
  } else {
    txtenc_printf(e, "%" PRId32, val);
  }
}

static void txtenc_escaped(txtenc* e, unsigned char ch) {
  switch (ch) {
    case '\n':
      txtenc_putstr(e, "\\n");
      break;
    case '\r':
      txtenc_putstr(e, "\\r");
      break;
    case '\t':
      txtenc_putstr(e, "\\t");
      break;
    case '\"':
      txtenc_putstr(e, "\\\"");
      break;
    case '\'':
      txtenc_putstr(e, "\\'");
      break;
    case '\\':
      txtenc_putstr(e, "\\\\");
      break;
    default:
      txtenc_printf(e, "\\%03o", ch);
      break;
  }
}

// Returns true if `ch` needs to be escaped in TextFormat, independent of any
// UTF-8 validity issues.
static bool upb_DefinitelyNeedsEscape(unsigned char ch) {
  if (ch < 32) return true;
  switch (ch) {
    case '\"':
    case '\'':
    case '\\':
    case 127:
      return true;
  }
  return false;
}

static bool upb_AsciiIsPrint(unsigned char ch) { return ch >= 32 && ch < 127; }

// Returns true if this is a high byte that requires UTF-8 validation.  If the
// UTF-8 validation fails, we must escape the byte.
static bool upb_NeedsUtf8Validation(unsigned char ch) { return ch > 127; }

// Returns the number of bytes in the prefix of `val` that do not need escaping.
// This is like utf8_range::SpanStructurallyValid(), except that it also
// terminates at any ASCII char that needs to be escaped in TextFormat (any char
// that has `DefinitelyNeedsEscape(ch) == true`).
//
// If we could get a variant of utf8_range::SpanStructurallyValid() that could
// terminate on any of these chars, that might be more efficient, but it would
// be much more complicated to modify that heavily SIMD code.
static size_t SkipPassthroughBytes(const char* ptr, size_t size) {
  for (size_t i = 0; i < size; i++) {
    unsigned char uc = ptr[i];
    if (upb_DefinitelyNeedsEscape(uc)) return i;
    if (upb_NeedsUtf8Validation(uc)) {
      // Find the end of this region of consecutive high bytes, so that we only
      // give high bytes to the UTF-8 checker.  This avoids needing to perform
      // a second scan of the ASCII characters looking for characters that
      // need escaping.
      //
      // We assume that high bytes are less frequent than plain, printable ASCII
      // bytes, so we accept the double-scan of high bytes.
      size_t end = i + 1;
      for (; end < size; end++) {
        if (!upb_NeedsUtf8Validation(ptr[end])) break;
      }
      size_t n = end - i;
      size_t ok = utf8_range_ValidPrefix(ptr + i, n);
      if (ok != n) return i + ok;
      i += ok - 1;
    }
  }
  return size;
}

static void upb_HardenedPrintString(txtenc* e, const char* ptr, size_t len) {
  // Print as UTF-8, while guarding against any invalid UTF-8 in the string
  // field.
  //
  // If in the future we have a guaranteed invariant that invalid UTF-8 will
  // never be present, we could avoid the UTF-8 check here.
  txtenc_putstr(e, "\"");
  const char* end = ptr + len;
  while (ptr < end) {
    size_t n = SkipPassthroughBytes(ptr, end - ptr);
    if (n != 0) {
      txtenc_putbytes(e, ptr, n);
      ptr += n;
      if (ptr == end) break;
    }

    // If repeated calls to CEscape() and PrintString() are expensive, we could
    // consider batching them, at the cost of some complexity.
    txtenc_escaped(e, *ptr);
    ptr++;
  }
  txtenc_putstr(e, "\"");
}

static void txtenc_bytes(txtenc* e, upb_StringView data) {
  const char* ptr = data.data;
  const char* end = ptr + data.size;
  txtenc_putstr(e, "\"");
  for (; ptr < end; ptr++) {
    unsigned char uc = *ptr;
    if (upb_AsciiIsPrint(uc)) {
      txtenc_putbytes(e, ptr, 1);
    } else {
      txtenc_escaped(e, uc);
    }
  }
  txtenc_putstr(e, "\"");
}

static void txtenc_field(txtenc* e, upb_MessageValue val,
                         const upb_FieldDef* f) {
  txtenc_indent(e);
  const upb_CType ctype = upb_FieldDef_CType(f);
  const bool is_ext = upb_FieldDef_IsExtension(f);
  const char* full = upb_FieldDef_FullName(f);
  const char* name = upb_FieldDef_Name(f);

  if (ctype == kUpb_CType_Message) {
// begin:google_only
//     // TODO: Turn this into a feature check and opensource it.
//     if (_upb_FieldDef_IsGroupLike(f)) {
//       const upb_MessageDef* m = upb_FieldDef_MessageSubDef(f);
//       name = upb_MessageDef_Name(m);
//     }
// end:google_only
    if (is_ext) {
      txtenc_printf(e, "[%s] {", full);
    } else {
      txtenc_printf(e, "%s {", name);
    }
    txtenc_endfield(e);
    e->indent_depth++;
    txtenc_msg(e, val.msg_val, upb_FieldDef_MessageSubDef(f));
    e->indent_depth--;
    txtenc_indent(e);
    txtenc_putstr(e, "}");
    txtenc_endfield(e);
    return;
  }

  if (is_ext) {
    txtenc_printf(e, "[%s]: ", full);
  } else {
    txtenc_printf(e, "%s: ", name);
  }

  switch (ctype) {
    case kUpb_CType_Bool:
      txtenc_putstr(e, val.bool_val ? "true" : "false");
      break;
    case kUpb_CType_Float: {
      char buf[32];
      _upb_EncodeRoundTripFloat(val.float_val, buf, sizeof(buf));
      txtenc_putstr(e, buf);
      break;
    }
    case kUpb_CType_Double: {
      char buf[32];
      _upb_EncodeRoundTripDouble(val.double_val, buf, sizeof(buf));
      txtenc_putstr(e, buf);
      break;
    }
    case kUpb_CType_Int32:
      txtenc_printf(e, "%" PRId32, val.int32_val);
      break;
    case kUpb_CType_UInt32:
      txtenc_printf(e, "%" PRIu32, val.uint32_val);
      break;
    case kUpb_CType_Int64:
      txtenc_printf(e, "%" PRId64, val.int64_val);
      break;
    case kUpb_CType_UInt64:
      txtenc_printf(e, "%" PRIu64, val.uint64_val);
      break;
    case kUpb_CType_String:
      upb_HardenedPrintString(e, val.str_val.data, val.str_val.size);
      break;
    case kUpb_CType_Bytes:
      txtenc_bytes(e, val.str_val);
      break;
    case kUpb_CType_Enum:
      txtenc_enum(val.int32_val, f, e);
      break;
    default:
      UPB_UNREACHABLE();
  }

  txtenc_endfield(e);
}

/*
 * Arrays print as simple repeated elements, eg.
 *
 *    foo_field: 1
 *    foo_field: 2
 *    foo_field: 3
 */
static void txtenc_array(txtenc* e, const upb_Array* arr,
                         const upb_FieldDef* f) {
  size_t i;
  size_t size = upb_Array_Size(arr);

  for (i = 0; i < size; i++) {
    txtenc_field(e, upb_Array_Get(arr, i), f);
  }
}

static void txtenc_mapentry(txtenc* e, upb_MessageValue key,
                            upb_MessageValue val, const upb_FieldDef* f) {
  const upb_MessageDef* entry = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_Field(entry, 0);
  const upb_FieldDef* val_f = upb_MessageDef_Field(entry, 1);
  txtenc_indent(e);
  txtenc_printf(e, "%s {", upb_FieldDef_Name(f));
  txtenc_endfield(e);
  e->indent_depth++;

  txtenc_field(e, key, key_f);
  txtenc_field(e, val, val_f);

  e->indent_depth--;
  txtenc_indent(e);
  txtenc_putstr(e, "}");
  txtenc_endfield(e);
}

/*
 * Maps print as messages of key/value, etc.
 *
 *    foo_map: {
 *      key: "abc"
 *      value: 123
 *    }
 *    foo_map: {
 *      key: "def"
 *      value: 456
 *    }
 */
static void txtenc_map(txtenc* e, const upb_Map* map, const upb_FieldDef* f) {
  if (e->options & UPB_TXTENC_NOSORT) {
    size_t iter = kUpb_Map_Begin;
    upb_MessageValue key, val;
    while (upb_Map_Next(map, &key, &val, &iter)) {
      txtenc_mapentry(e, key, val, f);
    }
  } else {
    if (upb_Map_Size(map) == 0) return;

    const upb_MessageDef* entry = upb_FieldDef_MessageSubDef(f);
    const upb_FieldDef* key_f = upb_MessageDef_Field(entry, 0);
    _upb_sortedmap sorted;
    upb_MapEntry ent;

    _upb_mapsorter_pushmap(&e->sorter, upb_FieldDef_Type(key_f), map, &sorted);
    while (_upb_sortedmap_next(&e->sorter, map, &sorted, &ent)) {
      upb_MessageValue key, val;
      memcpy(&key, &ent.k, sizeof(key));
      memcpy(&val, &ent.v, sizeof(val));
      txtenc_mapentry(e, key, val, f);
    }
    _upb_mapsorter_popmap(&e->sorter, &sorted);
  }
}

#define CHK(x)      \
  do {              \
    if (!(x)) {     \
      return false; \
    }               \
  } while (0)

/*
 * Unknown fields are printed by number.
 *
 * 1001: 123
 * 1002: "hello"
 * 1006: 0xdeadbeef
 * 1003: {
 *   1: 111
 * }
 */
static const char* txtenc_unknown(txtenc* e, const char* ptr,
                                  upb_EpsCopyInputStream* stream,
                                  int groupnum) {
  // We are guaranteed that the unknown data is valid wire format, and will not
  // contain tag zero.
  uint32_t end_group = groupnum > 0
                           ? ((groupnum << kUpb_WireReader_WireTypeBits) |
                              kUpb_WireType_EndGroup)
                           : 0;

  while (!upb_EpsCopyInputStream_IsDone(stream, &ptr)) {
    uint32_t tag;
    CHK(ptr = upb_WireReader_ReadTag(ptr, &tag));
    if (tag == end_group) return ptr;

    txtenc_indent(e);
    txtenc_printf(e, "%d: ", (int)upb_WireReader_GetFieldNumber(tag));

    switch (upb_WireReader_GetWireType(tag)) {
      case kUpb_WireType_Varint: {
        uint64_t val;
        CHK(ptr = upb_WireReader_ReadVarint(ptr, &val));
        txtenc_printf(e, "%" PRIu64, val);
        break;
      }
      case kUpb_WireType_32Bit: {
        uint32_t val;
        ptr = upb_WireReader_ReadFixed32(ptr, &val);
        txtenc_printf(e, "0x%08" PRIu32, val);
        break;
      }
      case kUpb_WireType_64Bit: {
        uint64_t val;
        ptr = upb_WireReader_ReadFixed64(ptr, &val);
        txtenc_printf(e, "0x%016" PRIu64, val);
        break;
      }
      case kUpb_WireType_Delimited: {
        int size;
        char* start = e->ptr;
        size_t start_overflow = e->overflow;
        CHK(ptr = upb_WireReader_ReadSize(ptr, &size));
        CHK(upb_EpsCopyInputStream_CheckDataSizeAvailable(stream, ptr, size));

        // Speculatively try to parse as message.
        txtenc_putstr(e, "{");
        txtenc_endfield(e);

        // EpsCopyInputStream can't back up, so create a sub-stream for the
        // speculative parse.
        upb_EpsCopyInputStream sub_stream;
        const char* sub_ptr = upb_EpsCopyInputStream_GetAliasedPtr(stream, ptr);
        upb_EpsCopyInputStream_Init(&sub_stream, &sub_ptr, size, true);

        e->indent_depth++;
        if (txtenc_unknown(e, sub_ptr, &sub_stream, -1)) {
          ptr = upb_EpsCopyInputStream_Skip(stream, ptr, size);
          e->indent_depth--;
          txtenc_indent(e);
          txtenc_putstr(e, "}");
        } else {
          // Didn't work out, print as raw bytes.
          e->indent_depth--;
          e->ptr = start;
          e->overflow = start_overflow;
          const char* str = ptr;
          ptr = upb_EpsCopyInputStream_ReadString(stream, &str, size, NULL);
          UPB_ASSERT(ptr);
          txtenc_bytes(e, (upb_StringView){.data = str, .size = size});
        }
        break;
      }
      case kUpb_WireType_StartGroup:
        txtenc_putstr(e, "{");
        txtenc_endfield(e);
        e->indent_depth++;
        CHK(ptr = txtenc_unknown(e, ptr, stream,
                                 upb_WireReader_GetFieldNumber(tag)));
        e->indent_depth--;
        txtenc_indent(e);
        txtenc_putstr(e, "}");
        break;
      default:
        return NULL;
    }
    txtenc_endfield(e);
  }

  return end_group == 0 && !upb_EpsCopyInputStream_IsError(stream) ? ptr : NULL;
}

#undef CHK

static void txtenc_msg(txtenc* e, const upb_Message* msg,
                       const upb_MessageDef* m) {
  size_t iter = kUpb_Message_Begin;
  const upb_FieldDef* f;
  upb_MessageValue val;

  while (upb_Message_Next(msg, m, e->ext_pool, &f, &val, &iter)) {
    if (upb_FieldDef_IsMap(f)) {
      txtenc_map(e, val.map_val, f);
    } else if (upb_FieldDef_IsRepeated(f)) {
      txtenc_array(e, val.array_val, f);
    } else {
      txtenc_field(e, val, f);
    }
  }

  if ((e->options & UPB_TXTENC_SKIPUNKNOWN) == 0) {
    size_t size;
    const char* ptr = upb_Message_GetUnknown(msg, &size);
    if (size != 0) {
      char* start = e->ptr;
      upb_EpsCopyInputStream stream;
      upb_EpsCopyInputStream_Init(&stream, &ptr, size, true);
      if (!txtenc_unknown(e, ptr, &stream, -1)) {
        /* Unknown failed to parse, back up and don't print it at all. */
        e->ptr = start;
      }
    }
  }
}

size_t txtenc_nullz(txtenc* e, size_t size) {
  size_t ret = e->ptr - e->buf + e->overflow;

  if (size > 0) {
    if (e->ptr == e->end) e->ptr--;
    *e->ptr = '\0';
  }

  return ret;
}

size_t upb_TextEncode(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, int options, char* buf,
                      size_t size) {
  txtenc e;

  e.buf = buf;
  e.ptr = buf;
  e.end = UPB_PTRADD(buf, size);
  e.overflow = 0;
  e.indent_depth = 0;
  e.options = options;
  e.ext_pool = ext_pool;
  _upb_mapsorter_init(&e.sorter);

  txtenc_msg(&e, msg, m);
  _upb_mapsorter_destroy(&e.sorter);
  return txtenc_nullz(&e, size);
}
