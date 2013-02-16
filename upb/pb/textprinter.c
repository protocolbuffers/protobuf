/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/pb/textprinter.h"

#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct _upb_textprinter {
  upb_bytesink *sink;
  int indent_depth;
  bool single_line;
  upb_status status;
};

#define CHECK(x) if ((x) < 0) goto err;

static int indent(upb_textprinter *p) {
  if (!p->single_line)
    CHECK(upb_bytesink_putrepeated(p->sink, ' ', p->indent_depth*2));
  return 0;
err:
  return -1;
}

static int endfield(upb_textprinter *p) {
  CHECK(upb_bytesink_putc(p->sink, p->single_line ? ' ' : '\n'));
  return 0;
err:
  return -1;
}

static int putescaped(upb_textprinter *p, const char *buf, size_t len,
                      bool preserve_utf8) {
  // Based on CEscapeInternal() from Google's protobuf release.
  char dstbuf[4096], *dst = dstbuf, *dstend = dstbuf + sizeof(dstbuf);
  const char *end = buf + len;

  // I think hex is prettier and more useful, but proto2 uses octal; should
  // investigate whether it can parse hex also.
  const bool use_hex = false;
  bool last_hex_escape = false; // true if last output char was \xNN

  for (; buf < end; buf++) {
    if (dstend - dst < 4) {
      CHECK(upb_bytesink_write(p->sink, dstbuf, dst - dstbuf));
      dst = dstbuf;
    }

    bool is_hex_escape = false;
    switch (*buf) {
      case '\n': *(dst++) = '\\'; *(dst++) = 'n';  break;
      case '\r': *(dst++) = '\\'; *(dst++) = 'r';  break;
      case '\t': *(dst++) = '\\'; *(dst++) = 't';  break;
      case '\"': *(dst++) = '\\'; *(dst++) = '\"'; break;
      case '\'': *(dst++) = '\\'; *(dst++) = '\''; break;
      case '\\': *(dst++) = '\\'; *(dst++) = '\\'; break;
      default:
        // Note that if we emit \xNN and the buf character after that is a hex
        // digit then that digit must be escaped too to prevent it being
        // interpreted as part of the character code by C.
        if ((!preserve_utf8 || (uint8_t)*buf < 0x80) &&
            (!isprint(*buf) || (last_hex_escape && isxdigit(*buf)))) {
          sprintf(dst, (use_hex ? "\\x%02x" : "\\%03o"), (uint8_t)*buf);
          is_hex_escape = use_hex;
          dst += 4;
        } else {
          *(dst++) = *buf; break;
        }
    }
    last_hex_escape = is_hex_escape;
  }
  // Flush remaining data.
  CHECK(upb_bytesink_write(p->sink, dst, dst - dstbuf));
  return 0;
err:
  return -1;
}

#define TYPE(name, ctype, fmt) \
  static bool put ## name(void *_p, void *fval, ctype val) {                 \
    upb_textprinter *p = _p;                                                 \
    const upb_fielddef *f = fval;                                            \
    CHECK(indent(p));                                                        \
    CHECK(upb_bytesink_writestr(p->sink, upb_fielddef_name(f)));             \
    CHECK(upb_bytesink_writestr(p->sink, ": "));                             \
    CHECK(upb_bytesink_printf(p->sink, fmt, val));                           \
    CHECK(endfield(p));                                                      \
    return true;                                                             \
  err:                                                                       \
    return false;                                                            \
}

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY_MACROVAL(x) STRINGIFY_HELPER(x)

TYPE(int32,  int32_t,  "%" PRId32)
TYPE(int64,  int64_t,  "%" PRId64)
TYPE(uint32, uint32_t, "%" PRIu32);
TYPE(uint64, uint64_t, "%" PRIu64)
TYPE(float,  float,    "%." STRINGIFY_MACROVAL(FLT_DIG) "g")
TYPE(double, double,   "%." STRINGIFY_MACROVAL(DBL_DIG) "g")
TYPE(bool,   bool,     "%hhu");

// Output a symbolic value from the enum if found, else just print as int32.
static bool putenum(void *_p, void *fval, int32_t val) {

  upb_textprinter *p = _p;
  const upb_fielddef *f = fval;
  const upb_enumdef *enum_def = upb_downcast_enumdef(upb_fielddef_subdef(f));
  const char *label = upb_enumdef_iton(enum_def, val);
  if (label) {
    CHECK(upb_bytesink_writestr(p->sink, label));
  } else {
    CHECK(putint32(_p, fval, val));
  }
  return true;
err:
  return false;
}

static void *startstr(void *_p, void *fval, size_t size_hint) {
  UPB_UNUSED(size_hint);
  UPB_UNUSED(fval);
  upb_textprinter *p = _p;
  CHECK(upb_bytesink_putc(p->sink, '"'));
  return p;
err:
  return UPB_BREAK;
}

static bool endstr(void *_p, void *fval) {
  UPB_UNUSED(fval);
  upb_textprinter *p = _p;
  CHECK(upb_bytesink_putc(p->sink, '"'));
  return true;
err:
  return false;
}

static size_t putstr(void *_p, void *fval, const char *buf, size_t len) {
  upb_textprinter *p = _p;
  const upb_fielddef *f = fval;
  CHECK(putescaped(p, buf, len, upb_fielddef_type(f) == UPB_TYPE(STRING)));
  return len;
err:
  return 0;
}

static void *startsubmsg(void *_p, void *fval) {
  upb_textprinter *p = _p;
  const upb_fielddef *f = fval;
  CHECK(indent(p));
  CHECK(upb_bytesink_printf(p->sink, "%s {", upb_fielddef_name(f)));
  if (!p->single_line)
    CHECK(upb_bytesink_putc(p->sink, '\n'));
  p->indent_depth++;
  return _p;
err:
  return UPB_BREAK;
}

static bool endsubmsg(void *_p, void *fval) {
  UPB_UNUSED(fval);
  upb_textprinter *p = _p;
  p->indent_depth--;
  CHECK(indent(p));
  CHECK(upb_bytesink_putc(p->sink, '}'));
  CHECK(endfield(p));
  return true;
err:
  return false;
}

upb_textprinter *upb_textprinter_new() {
  upb_textprinter *p = malloc(sizeof(*p));
  return p;
}

void upb_textprinter_free(upb_textprinter *p) { free(p); }

void upb_textprinter_reset(upb_textprinter *p, upb_bytesink *sink,
                           bool single_line) {
  p->sink = sink;
  p->single_line = single_line;
  p->indent_depth = 0;
}

static void onmreg(void *c, upb_handlers *h) {
  (void)c;
  const upb_msgdef *m = upb_handlers_msgdef(h);
  upb_msg_iter i;
  for(upb_msg_begin(&i, m); !upb_msg_done(&i); upb_msg_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_INT32:
      case UPB_TYPE_SINT32:
      case UPB_TYPE_SFIXED32:
        upb_handlers_setint32(h, f, putint32, f, NULL);
        break;
      case UPB_TYPE_SINT64:
      case UPB_TYPE_SFIXED64:
      case UPB_TYPE_INT64:
        upb_handlers_setint64(h, f, putint64, f, NULL);
        break;
      case UPB_TYPE_UINT32:
      case UPB_TYPE_FIXED32:
        upb_handlers_setuint32(h, f, putuint32, f, NULL);
        break;
      case UPB_TYPE_UINT64:
      case UPB_TYPE_FIXED64:
        upb_handlers_setuint64(h, f, putuint64, f, NULL);
        break;
      case UPB_TYPE_FLOAT:
        upb_handlers_setfloat(h, f, putfloat, f, NULL);
        break;
      case UPB_TYPE_DOUBLE:
        upb_handlers_setdouble(h, f, putdouble, f, NULL);
        break;
      case UPB_TYPE_BOOL:
        upb_handlers_setbool(h, f, putbool, f, NULL);
        break;
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
        upb_handlers_setstartstr(h, f, startstr, f, NULL);
        upb_handlers_setstring(h, f, putstr, f, NULL);
        upb_handlers_setendstr(h, f, endstr, f, NULL);
        break;
      case UPB_TYPE_GROUP:
      case UPB_TYPE_MESSAGE:
        upb_handlers_setstartsubmsg(h, f, &startsubmsg, f, NULL);
        upb_handlers_setendsubmsg(h, f, &endsubmsg, f, NULL);
        break;
      case UPB_TYPE_ENUM:
        upb_handlers_setint32(h, f, putenum, f, NULL);
      default:
        assert(false);
        break;
    }
  }
}

const upb_handlers *upb_textprinter_newhandlers(const void *owner,
                                                const upb_msgdef *m) {
  return upb_handlers_newfrozen(m, owner, &onmreg, NULL);
}
