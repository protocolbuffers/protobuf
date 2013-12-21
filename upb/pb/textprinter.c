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

#include "upb/sink.h"

struct _upb_textprinter {
  int indent_depth;
  bool single_line;
  upb_status status;
};

#define CHECK(x) if ((x) < 0) goto err;

static int indent(upb_textprinter *p) {
  int i;
  if (!p->single_line)
    for (i = 0; i < p->indent_depth * 2; i++)
      putchar(' ');
  return 0;
  return -1;
}

static int endfield(upb_textprinter *p) {
  putchar(p->single_line ? ' ' : '\n');
  return 0;
}

static int putescaped(const char* buf, size_t len, bool preserve_utf8) {
  // Based on CEscapeInternal() from Google's protobuf release.
  char dstbuf[4096], *dst = dstbuf, *dstend = dstbuf + sizeof(dstbuf);
  const char *end = buf + len;

  // I think hex is prettier and more useful, but proto2 uses octal; should
  // investigate whether it can parse hex also.
  const bool use_hex = false;
  bool last_hex_escape = false; // true if last output char was \xNN

  for (; buf < end; buf++) {
    if (dstend - dst < 4) {
      fwrite(dstbuf, dst - dstbuf, 1, stdout);
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
  fwrite(dst, dst - dstbuf, 1, stdout);
  return 0;
}

#define TYPE(name, ctype, fmt) \
  static bool put ## name(void *closure, const void *handler_data, ctype val) {\
    upb_textprinter *p = closure;                                              \
    const upb_fielddef *f = handler_data;                                      \
    CHECK(indent(p));                                                          \
    puts(upb_fielddef_name(f));                                                \
    puts(": ");                                                                \
    printf(fmt, val);                                                          \
    CHECK(endfield(p));                                                        \
    return true;                                                               \
  err:                                                                         \
    return false;                                                              \
}

static bool putbool(void *closure, const void *handler_data, bool val) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = handler_data;
  CHECK(indent(p));
  puts(upb_fielddef_name(f));
  puts(": ");
  puts(val ? "true" : "false");
  CHECK(endfield(p));
  return true;
err:
  return false;
}

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY_MACROVAL(x) STRINGIFY_HELPER(x)

TYPE(int32,  int32_t,  "%" PRId32)
TYPE(int64,  int64_t,  "%" PRId64)
TYPE(uint32, uint32_t, "%" PRIu32);
TYPE(uint64, uint64_t, "%" PRIu64)
TYPE(float,  float,    "%." STRINGIFY_MACROVAL(FLT_DIG) "g")
TYPE(double, double,   "%." STRINGIFY_MACROVAL(DBL_DIG) "g")

// Output a symbolic value from the enum if found, else just print as int32.
static bool putenum(void *closure, const void *handler_data, int32_t val) {
  const upb_fielddef *f = handler_data;
  const upb_enumdef *enum_def = upb_downcast_enumdef(upb_fielddef_subdef(f));
  const char *label = upb_enumdef_iton(enum_def, val);
  if (label) {
    puts(label);
  } else {
    CHECK(putint32(closure, handler_data, val));
  }
  return true;
err:
  return false;
}

static void *startstr(void *closure, const void *handler_data,
                      size_t size_hint) {
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  upb_textprinter *p = closure;
  putchar('"');
  return p;
}

static bool endstr(void *closure, const void *handler_data) {
  UPB_UNUSED(closure);
  UPB_UNUSED(handler_data);
  putchar('"');
  return true;
}

static size_t putstr(void *closure, const void *hd, const char *buf,
                     size_t len) {
  UPB_UNUSED(closure);
  const upb_fielddef *f = hd;
  CHECK(putescaped(buf, len, upb_fielddef_type(f) == UPB_TYPE_STRING));
  return len;
err:
  return 0;
}

static void *startsubmsg(void *closure, const void *handler_data) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = handler_data;
  CHECK(indent(p));
  printf("%s {", upb_fielddef_name(f));
  if (!p->single_line)
    putchar('\n');
  p->indent_depth++;
  return p;
err:
  return UPB_BREAK;
}

static bool endsubmsg(void *closure, const void *handler_data) {
  UPB_UNUSED(handler_data);
  upb_textprinter *p = closure;
  p->indent_depth--;
  CHECK(indent(p));
  putchar('}');
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

void upb_textprinter_reset(upb_textprinter *p, bool single_line) {
  p->single_line = single_line;
  p->indent_depth = 0;
}

static void onmreg(void *c, upb_handlers *h) {
  (void)c;
  const upb_msgdef *m = upb_handlers_msgdef(h);
  upb_msg_iter i;
  for(upb_msg_begin(&i, m); !upb_msg_done(&i); upb_msg_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    upb_handlerattr attr = UPB_HANDLERATTR_INITIALIZER;
    upb_handlerattr_sethandlerdata(&attr, f, NULL);
    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_INT32:
        upb_handlers_setint32(h, f, putint32, &attr);
        break;
      case UPB_TYPE_INT64:
        upb_handlers_setint64(h, f, putint64, &attr);
        break;
      case UPB_TYPE_UINT32:
        upb_handlers_setuint32(h, f, putuint32, &attr);
        break;
      case UPB_TYPE_UINT64:
        upb_handlers_setuint64(h, f, putuint64, &attr);
        break;
      case UPB_TYPE_FLOAT:
        upb_handlers_setfloat(h, f, putfloat, &attr);
        break;
      case UPB_TYPE_DOUBLE:
        upb_handlers_setdouble(h, f, putdouble, &attr);
        break;
      case UPB_TYPE_BOOL:
        upb_handlers_setbool(h, f, putbool, &attr);
        break;
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
        upb_handlers_setstartstr(h, f, startstr, &attr);
        upb_handlers_setstring(h, f, putstr, &attr);
        upb_handlers_setendstr(h, f, endstr, &attr);
        break;
      case UPB_TYPE_MESSAGE:
        upb_handlers_setstartsubmsg(h, f, startsubmsg, &attr);
        upb_handlers_setendsubmsg(h, f, endsubmsg, &attr);
        break;
      case UPB_TYPE_ENUM:
        upb_handlers_setint32(h, f, putenum, &attr);
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
