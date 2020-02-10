
#include "upb/text_encode.h"

#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "upb/reflection.h"
#include "upb/port_def.inc"

typedef struct {
  char *buf, *ptr, *end;
  size_t overflow;
  int indent_depth;
  int options;
  const upb_symtab *ext_pool;
} txtenc;

static void txtenc_msg(txtenc *e, const upb_msg *msg, const upb_msgdef *m);

static void txtenc_putbytes(txtenc *e, const void *data, size_t len) {
  size_t have = e->end - e->ptr;
  if (UPB_LIKELY(have >= len)) {
    memcpy(e->ptr, data, len);
    e->ptr += len;
  } else {
    memcpy(e->ptr, data, have);
    e->ptr += have;
    e->overflow += (len - have);
  }
}

static void txtenc_putstr(txtenc *e, const char *str) {
  txtenc_putbytes(e, str, strlen(str));
}

static void txtenc_printf(txtenc *e, const char *fmt, ...) {
  size_t n;
  size_t have = e->end - e->ptr;
  va_list args;

  va_start(args, fmt);
  n = _upb_vsnprintf(e->ptr, have, fmt, args);
  va_end(args);

  if (UPB_LIKELY(have > n)) {
    e->ptr += n;
  } else {
    e->ptr += have;
    e->overflow += (n - have);
  }
}

static void txtenc_indent(txtenc *e) {
  if ((e->options & UPB_TXTENC_SINGLELINE) == 0) {
    int i = e->indent_depth;
    while (i-- > 0) {
      txtenc_putstr(e, "  ");
    }
  }
}

static void txtenc_endfield(txtenc *e) {
  if (e->options & UPB_TXTENC_SINGLELINE) {
    txtenc_putstr(e, " ");
  } else {
    txtenc_putstr(e, "\n");
  }
}

static void txtenc_enum(int32_t val, const upb_fielddef *f, txtenc *e) {
  const upb_enumdef *e_def = upb_fielddef_enumsubdef(f);
  const char *name = upb_enumdef_iton(e_def, val);

  if (name) {
    txtenc_printf(e, "%s", name);
  } else {
    txtenc_printf(e, "%" PRId32, val);
  }
}

static void txtenc_string(txtenc *e, upb_strview str, bool bytes) {
  const char *ptr = str.data;
  const char *end = ptr + str.size;
  txtenc_putstr(e, "\"");

  while (ptr < end) {
    switch (*ptr) {
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
        if ((bytes || (uint8_t)*ptr < 0x80) && !isprint(*ptr)) {
          txtenc_printf(e, "\\%03o", (int)(uint8_t)*ptr);
        } else {
          txtenc_putbytes(e, ptr, 1);
        }
    }
    ptr++;
  }

  txtenc_putstr(e, "\"");
}

static void txtenc_field(txtenc *e, upb_msgval val, const upb_fielddef *f) {
  txtenc_indent(e);
  txtenc_printf(e, "%s: ", upb_fielddef_name(f));

  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_BOOL:
      txtenc_putstr(e, val.bool_val ? "true" : "false");
      break;
    case UPB_TYPE_FLOAT:
      txtenc_printf(e, "%f", val.float_val);
      break;
    case UPB_TYPE_DOUBLE:
      txtenc_printf(e, "%f", val.double_val);
      break;
    case UPB_TYPE_INT32:
      txtenc_printf(e, "%" PRId32, val.int32_val);
      break;
    case UPB_TYPE_UINT32:
      txtenc_printf(e, "%" PRIu32, val.uint32_val);
      break;
    case UPB_TYPE_INT64:
      txtenc_printf(e, "%" PRId64, val.int64_val);
      break;
    case UPB_TYPE_UINT64:
      txtenc_printf(e, "%" PRIu64, val.uint64_val);
      break;
    case UPB_TYPE_STRING:
      txtenc_string(e, val.str_val, false);
      break;
    case UPB_TYPE_BYTES:
      txtenc_string(e, val.str_val, true);
      break;
    case UPB_TYPE_ENUM:
      txtenc_enum(val.int32_val, f, e);
      break;
    case UPB_TYPE_MESSAGE:
      txtenc_putstr(e, "{");
      txtenc_endfield(e);
      e->indent_depth++;
      txtenc_msg(e, val.msg_val, upb_fielddef_msgsubdef(f));
      e->indent_depth--;
      txtenc_indent(e);
      txtenc_putstr(e, "}");
      break;
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
static void txtenc_array(txtenc *e, const upb_array *arr,
                         const upb_fielddef *f) {
  size_t i;
  size_t size = upb_array_size(arr);

  for (i = 0; i < size; i++) {
    txtenc_field(e, upb_array_get(arr, i), f);
  }
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
static void txtenc_map(txtenc *e, const upb_map *map, const upb_fielddef *f) {
  const upb_msgdef *entry = upb_fielddef_msgsubdef(f);
  const upb_fielddef *key_f = upb_msgdef_itof(entry, 1);
  const upb_fielddef *val_f = upb_msgdef_itof(entry, 2);
  size_t iter = UPB_MAP_BEGIN;

  while (upb_mapiter_next(map, &iter)) {
    upb_msgval key = upb_mapiter_key(map, iter);
    upb_msgval val = upb_mapiter_value(map, iter);

    txtenc_indent(e);
    txtenc_printf(e, "%s: {", upb_fielddef_name(f));
    txtenc_endfield(e);
    e->indent_depth++;

    txtenc_field(e, key, key_f);
    txtenc_field(e, val, val_f);

    e->indent_depth--;
    txtenc_indent(e);
    txtenc_putstr(e, "}");
    txtenc_endfield(e);
  }
}

#define CHK(x) do { if (!(x)) { return false; } } while(0)

static const char *txtenc_parsevarint(const char *ptr, const char *limit,
                                      uint64_t *val) {
  uint8_t byte;
  int bitpos = 0;
  *val = 0;

  do {
    CHK(bitpos < 70 && ptr < limit);
    byte = *ptr;
    *val |= (uint64_t)(byte & 0x7F) << bitpos;
    ptr++;
    bitpos += 7;
  } while (byte & 0x80);

  return ptr;
}

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
static const char *txtenc_unknown(txtenc *e, const char *ptr, const char *end,
                                  int groupnum) {
  while (ptr < end) {
    uint64_t tag_64;
    uint32_t tag;
    CHK(ptr = txtenc_parsevarint(ptr, end, &tag_64));
    CHK(tag_64 < UINT32_MAX);
    tag = tag_64;

    if ((tag & 7) == UPB_WIRE_TYPE_END_GROUP) {
      CHK((tag >> 3) == groupnum);
      return ptr;
    }

    txtenc_indent(e);
    txtenc_printf(e, "%d: ", (int)(tag >> 3));

    switch (tag & 7) {
      case UPB_WIRE_TYPE_VARINT: {
        uint64_t val;
        CHK(ptr = txtenc_parsevarint(ptr, end, &val));
        txtenc_printf(e, "%" PRIu64, val);
        break;
      }
      case UPB_WIRE_TYPE_32BIT: {
        uint32_t val;
        CHK(end - ptr >= 4);
        memcpy(&val, ptr, 4);
        ptr += 4;
        txtenc_printf(e, "0x%08" PRIu32, val);
        break;
      }
      case UPB_WIRE_TYPE_64BIT: {
        uint64_t val;
        CHK(end - ptr >= 8);
        memcpy(&val, ptr, 8);
        ptr += 8;
        txtenc_printf(e, "0x%016" PRIu64, val);
        break;
      }
      case UPB_WIRE_TYPE_DELIMITED: {
        uint64_t len;
        char *start = e->ptr;
        size_t start_overflow = e->overflow;
        CHK(ptr = txtenc_parsevarint(ptr, end, &len));
        CHK(end - ptr >= len);

        /* Speculatively try to parse as message. */
        txtenc_putstr(e, "{");
        txtenc_endfield(e);
        e->indent_depth++;
        if (txtenc_unknown(e, ptr, end, -1)) {
          e->indent_depth--;
          txtenc_indent(e);
          txtenc_putstr(e, "}");
        } else {
          /* Didn't work out, print as raw bytes. */
          e->indent_depth--;
          e->ptr = start;
          e->overflow = start_overflow;
          upb_strview str = {ptr, len};
          txtenc_string(e, str, true);
        }
        ptr += len;
        break;
      }
      case UPB_WIRE_TYPE_START_GROUP:
        txtenc_putstr(e, "{");
        txtenc_endfield(e);
        e->indent_depth++;
        CHK(ptr = txtenc_unknown(e, ptr, end, tag >> 3));
        e->indent_depth--;
        txtenc_indent(e);
        txtenc_putstr(e, "}");
        break;
    }
    txtenc_endfield(e);
  }

  return groupnum == -1 ? ptr : NULL;
}

#undef CHK

static void txtenc_msg(txtenc *e, const upb_msg *msg,
                       const upb_msgdef *m) {
  size_t iter = UPB_MSG_BEGIN;
  const upb_fielddef *f;
  upb_msgval val;

  while (upb_msg_next(msg, m, e->ext_pool, &f, &val, &iter)) {
    if (upb_fielddef_ismap(f)) {
      txtenc_map(e, val.map_val, f);
    } else if (upb_fielddef_isseq(f)) {
      txtenc_array(e, val.array_val, f);
    } else {
      txtenc_field(e, val, f);
    }
  }

  if ((e->options & UPB_TXTENC_SKIPUNKNOWN) == 0) {
    size_t len;
    const char *ptr = upb_msg_getunknown(msg, &len);
    char *start = e->ptr;
    if (ptr) {
      if (!txtenc_unknown(e, ptr, ptr + len, -1)) {
        /* Unknown failed to parse, back up and don't print it at all. */
        e->ptr = start;
      }
    }
  }
}

size_t txtenc_nullz(txtenc *e, size_t size) {
  size_t ret = e->ptr - e->buf + e->overflow;

  if (size > 0) {
    if (e->ptr == e->end) e->ptr--;
    *e->ptr = '\0';
  }

  return ret;
}

size_t upb_text_encode(const upb_msg *msg, const upb_msgdef *m,
                       const upb_symtab *ext_pool, int options, char *buf,
                       size_t size) {
  txtenc e;

  e.buf = buf;
  e.ptr = buf;
  e.end = buf + size;
  e.overflow = 0;
  e.indent_depth = 0;
  e.options = options;
  e.ext_pool = ext_pool;

  txtenc_msg(&e, msg, m);
  return txtenc_nullz(&e, size);
}
