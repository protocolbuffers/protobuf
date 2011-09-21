/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <stdlib.h>
#include "upb/pb/textprinter.h"

struct _upb_textprinter {
  upb_bytesink *sink;
  int indent_depth;
  bool single_line;
  upb_status status;
};

#define CHECK(x) if ((x) < 0) goto err;

static int upb_textprinter_indent(upb_textprinter *p) {
  if (!p->single_line)
    CHECK(upb_bytesink_putrepeated(p->sink, ' ', p->indent_depth*2));
  return 0;
err:
  return -1;
}

static int upb_textprinter_endfield(upb_textprinter *p) {
  CHECK(upb_bytesink_putc(p->sink, p->single_line ? ' ' : '\n'));
  return 0;
err:
  return -1;
}

static int upb_textprinter_putescaped(upb_textprinter *p, const upb_strref *strref,
                                      bool preserve_utf8) {
  // Based on CEscapeInternal() from Google's protobuf release.
  // TODO; we could read directly from a bytesrc's buffer instead.
  // TODO; we could write strrefs to the sink when possible.
  char dstbuf[4096], *dst = dstbuf, *dstend = dstbuf + sizeof(dstbuf);
  char *buf = malloc(strref->len), *src = buf;
  char *end = src + strref->len;
  upb_bytesrc_read(strref->bytesrc, strref->stream_offset, strref->len, buf);

  // I think hex is prettier and more useful, but proto2 uses octal; should
  // investigate whether it can parse hex also.
  const bool use_hex = false;
  bool last_hex_escape = false; // true if last output char was \xNN

  for (; src < end; src++) {
    if (dstend - dst < 4) {
      CHECK(upb_bytesink_write(p->sink, dstbuf, dst - dstbuf));
      dst = dstbuf;
    }

    bool is_hex_escape = false;
    switch (*src) {
      case '\n': *(dst++) = '\\'; *(dst++) = 'n';  break;
      case '\r': *(dst++) = '\\'; *(dst++) = 'r';  break;
      case '\t': *(dst++) = '\\'; *(dst++) = 't';  break;
      case '\"': *(dst++) = '\\'; *(dst++) = '\"'; break;
      case '\'': *(dst++) = '\\'; *(dst++) = '\''; break;
      case '\\': *(dst++) = '\\'; *(dst++) = '\\'; break;
      default:
        // Note that if we emit \xNN and the src character after that is a hex
        // digit then that digit must be escaped too to prevent it being
        // interpreted as part of the character code by C.
        if ((!preserve_utf8 || (uint8_t)*src < 0x80) &&
            (!isprint(*src) || (last_hex_escape && isxdigit(*src)))) {
          sprintf(dst, (use_hex ? "\\x%02x" : "\\%03o"), (uint8_t)*src);
          is_hex_escape = use_hex;
          dst += 4;
        } else {
          *(dst++) = *src; break;
        }
    }
    last_hex_escape = is_hex_escape;
  }
  // Flush remaining data.
  CHECK(upb_bytesink_write(p->sink, dst, dst - dstbuf));
  free(buf);
  return 0;
err:
  free(buf);
  return -1;
}

#define TYPE(member, fmt) \
  static upb_flow_t upb_textprinter_put ## member(void *_p, upb_value fval,  \
                                                  upb_value val) {           \
    upb_textprinter *p = _p;                                                 \
    const upb_fielddef *f = upb_value_getfielddef(fval);                     \
    uint64_t start_ofs = upb_bytesink_getoffset(p->sink);                    \
    CHECK(upb_textprinter_indent(p));                                        \
    CHECK(upb_bytesink_writestr(p->sink, f->name));                          \
    CHECK(upb_bytesink_writestr(p->sink, ": "));                             \
    CHECK(upb_bytesink_printf(p->sink, fmt, upb_value_get ## member(val)));  \
    CHECK(upb_textprinter_endfield(p));                                      \
    return UPB_CONTINUE;                                                     \
  err:                                                                       \
    upb_bytesink_rewind(p->sink, start_ofs);                                 \
    return UPB_BREAK;                                                        \
}

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY_MACROVAL(x) STRINGIFY_HELPER(x)

TYPE(double, "%." STRINGIFY_MACROVAL(DBL_DIG) "g")
TYPE(float,  "%." STRINGIFY_MACROVAL(FLT_DIG) "g")
TYPE(int64,  "%" PRId64)
TYPE(uint64, "%" PRIu64)
TYPE(int32,  "%" PRId32)
TYPE(uint32, "%" PRIu32);
TYPE(bool,   "%hhu");

// Output a symbolic value from the enum if found, else just print as int32.
static upb_flow_t upb_textprinter_putenum(void *_p, upb_value fval,
                                          upb_value val) {

  upb_textprinter *p = _p;
  uint64_t start_ofs = upb_bytesink_getoffset(p->sink);
  const upb_fielddef *f = upb_value_getfielddef(fval);
  upb_enumdef *enum_def = upb_downcast_enumdef(f->def);
  const char *label = upb_enumdef_iton(enum_def, upb_value_getint32(val));
  if (label) {
    CHECK(upb_bytesink_writestr(p->sink, label));
  } else {
    CHECK(upb_textprinter_putint32(_p, fval, val));
  }
  return UPB_CONTINUE;
err:
  upb_bytesink_rewind(p->sink, start_ofs);
  return UPB_BREAK;
}

static upb_flow_t upb_textprinter_putstr(void *_p, upb_value fval,
                                         upb_value val) {
  upb_textprinter *p = _p;
  uint64_t start_ofs = upb_bytesink_getoffset(p->sink);
  const upb_fielddef *f = upb_value_getfielddef(fval);
  CHECK(upb_bytesink_putc(p->sink, '"'));
  CHECK(upb_textprinter_putescaped(p, upb_value_getstrref(val),
                                   f->type == UPB_TYPE(STRING)));
  CHECK(upb_bytesink_putc(p->sink, '"'));
  return UPB_CONTINUE;
err:
  upb_bytesink_rewind(p->sink, start_ofs);
  return UPB_BREAK;
}

static upb_sflow_t upb_textprinter_startsubmsg(void *_p, upb_value fval) {
  upb_textprinter *p = _p;
  uint64_t start_ofs = upb_bytesink_getoffset(p->sink);
  const upb_fielddef *f = upb_value_getfielddef(fval);
  CHECK(upb_textprinter_indent(p));
  CHECK(upb_bytesink_printf(p->sink, "%s {", f->name));
  if (!p->single_line)
    CHECK(upb_bytesink_putc(p->sink, '\n'));
  p->indent_depth++;
  return UPB_CONTINUE_WITH(_p);
err:
  upb_bytesink_rewind(p->sink, start_ofs);
  return UPB_SBREAK;
}

static upb_flow_t upb_textprinter_endsubmsg(void *_p, upb_value fval) {
  (void)fval;
  upb_textprinter *p = _p;
  uint64_t start_ofs = upb_bytesink_getoffset(p->sink);
  p->indent_depth--;
  CHECK(upb_textprinter_indent(p));
  CHECK(upb_bytesink_putc(p->sink, '}'));
  CHECK(upb_textprinter_endfield(p));
  return UPB_CONTINUE;
err:
  upb_bytesink_rewind(p->sink, start_ofs);
  return UPB_BREAK;
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

static void upb_textprinter_onfreg(void *c, upb_fhandlers *fh, const upb_fielddef *f) {
  (void)c;
  upb_fhandlers_setstartsubmsg(fh, &upb_textprinter_startsubmsg);
  upb_fhandlers_setendsubmsg(fh, &upb_textprinter_endsubmsg);
#define F(type) &upb_textprinter_put ## type
  static upb_value_handler *fptrs[] = {NULL, F(double), F(float), F(int64),
      F(uint64), F(int32), F(uint64), F(uint32), F(bool), F(str),
      NULL, NULL, F(str), F(uint32), F(enum), F(int32),
      F(int64), F(int32), F(int64)};
  upb_fhandlers_setvalue(fh, fptrs[f->type]);
  upb_value fval;
  upb_value_setfielddef(&fval, f);
  upb_fhandlers_setfval(fh, fval);
}

upb_mhandlers *upb_textprinter_reghandlers(upb_handlers *h, const upb_msgdef *m) {
  return upb_handlers_regmsgdef(
      h, m, NULL, &upb_textprinter_onfreg, NULL);
}
