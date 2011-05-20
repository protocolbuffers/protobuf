/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb_textprinter.h"

#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <stdlib.h>

struct _upb_textprinter {
  upb_bytesink *bytesink;
  int indent_depth;
  bool single_line;
  upb_status status;
};

#define CHECK(x) if ((x) < 0) goto err;

static int upb_textprinter_putescaped(upb_textprinter *p, upb_string *str,
                                      bool preserve_utf8) {
  // Based on CEscapeInternal() from Google's protobuf release.
  // TODO; we could write directly into a bytesink's buffer instead.
  char dstbuf[4096], *dst = dstbuf, *dstend = dstbuf + sizeof(dstbuf);
  const char *src = upb_string_getrobuf(str), *end = src + upb_string_len(str);

  // I think hex is prettier and more useful, but proto2 uses octal; should
  // investigate whether it can parse hex also.
  bool use_hex = false;
  bool last_hex_escape = false; // true if last output char was \xNN

  for (; src < end; src++) {
    if (dstend - dst < 4) {
      upb_string str = UPB_STACK_STRING_LEN(dstbuf, dst - dstbuf);
      CHECK(upb_bytesink_putstr(p->bytesink, &str, &p->status));
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
  upb_string outstr = UPB_STACK_STRING_LEN(dstbuf, dst - dstbuf);
  CHECK(upb_bytesink_putstr(p->bytesink, &outstr, &p->status));
  return 0;
err:
  return -1;
}

static int upb_textprinter_indent(upb_textprinter *p) {
  if(!p->single_line)
    for(int i = 0; i < p->indent_depth; i++)
      CHECK(upb_bytesink_putstr(p->bytesink, UPB_STRLIT("  "), &p->status));
  return 0;
err:
  return -1;
}

static int upb_textprinter_endfield(upb_textprinter *p) {
  if(p->single_line) {
    CHECK(upb_bytesink_putstr(p->bytesink, UPB_STRLIT(" "), &p->status));
  } else {
    CHECK(upb_bytesink_putstr(p->bytesink, UPB_STRLIT("\n"), &p->status));
  }
  return 0;
err:
  return -1;
}

static upb_flow_t upb_textprinter_value(void *_p, upb_value fval,
                                        upb_value val) {
  upb_textprinter *p = _p;
  upb_fielddef *f = upb_value_getfielddef(fval);
  upb_textprinter_indent(p);
  CHECK(upb_bytesink_printf(p->bytesink, &p->status, UPB_STRFMT ": ", UPB_STRARG(f->name)));
#define CASE(fmtstr, member) \
    CHECK(upb_bytesink_printf(p->bytesink, &p->status, fmtstr, upb_value_get ## member(val))); break;
  switch(f->type) {
    // TODO: figure out what we should really be doing for these
    // floating-point formats.
    case UPB_TYPE(DOUBLE):
      CHECK(upb_bytesink_printf(p->bytesink, &p->status, "%.*g", DBL_DIG, upb_value_getdouble(val))); break;
    case UPB_TYPE(FLOAT):
      CHECK(upb_bytesink_printf(p->bytesink, &p->status, "%.*g", FLT_DIG+2, upb_value_getfloat(val))); break;
    case UPB_TYPE(INT64):
    case UPB_TYPE(SFIXED64):
    case UPB_TYPE(SINT64):
      CASE("%" PRId64, int64)
    case UPB_TYPE(UINT64):
    case UPB_TYPE(FIXED64):
      CASE("%" PRIu64, uint64)
    case UPB_TYPE(UINT32):
    case UPB_TYPE(FIXED32):
      CASE("%" PRIu32, uint32);
    case UPB_TYPE(ENUM): {
      upb_enumdef *enum_def = upb_downcast_enumdef(f->def);
      upb_string *enum_label =
          upb_enumdef_iton(enum_def, upb_value_getint32(val));
      if (enum_label) {
        // We found a corresponding string for this enum.  Otherwise we fall
        // through to the int32 code path.
        CHECK(upb_bytesink_putstr(p->bytesink, enum_label, &p->status));
        break;
      }
    }
    case UPB_TYPE(INT32):
    case UPB_TYPE(SFIXED32):
    case UPB_TYPE(SINT32):
      CASE("%" PRId32, int32)
    case UPB_TYPE(BOOL):
      CASE("%hhu", bool);
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES):
      CHECK(upb_bytesink_putstr(p->bytesink, UPB_STRLIT("\""), &p->status));
      CHECK(upb_textprinter_putescaped(p, upb_value_getstr(val),
                                       f->type == UPB_TYPE(STRING)));
      CHECK(upb_bytesink_putstr(p->bytesink, UPB_STRLIT("\""), &p->status));
      break;
  }
  upb_textprinter_endfield(p);
  return UPB_CONTINUE;
err:
  return UPB_BREAK;
}

static upb_sflow_t upb_textprinter_startsubmsg(void *_p, upb_value fval) {
  upb_textprinter *p = _p;
  upb_fielddef *f = upb_value_getfielddef(fval);
  upb_textprinter_indent(p);
  bool ret = upb_bytesink_printf(p->bytesink, &p->status,
                                 UPB_STRFMT " {", UPB_STRARG(f->name));
  if (!ret) return UPB_SBREAK;
  if (!p->single_line)
    upb_bytesink_putstr(p->bytesink, UPB_STRLIT("\n"), &p->status);
  p->indent_depth++;
  return UPB_CONTINUE_WITH(_p);
}

static upb_flow_t upb_textprinter_endsubmsg(void *_p, upb_value fval) {
  (void)fval;
  upb_textprinter *p = _p;
  p->indent_depth--;
  upb_textprinter_indent(p);
  upb_bytesink_putstr(p->bytesink, UPB_STRLIT("}"), &p->status);
  upb_textprinter_endfield(p);
  return UPB_CONTINUE;
}

upb_textprinter *upb_textprinter_new() {
  upb_textprinter *p = malloc(sizeof(*p));
  return p;
}

void upb_textprinter_free(upb_textprinter *p) {
  free(p);
}

void upb_textprinter_reset(upb_textprinter *p, upb_bytesink *sink,
                           bool single_line) {
  p->bytesink = sink;
  p->single_line = single_line;
  p->indent_depth = 0;
}

upb_mhandlers *upb_textprinter_reghandlers(upb_handlers *h, upb_msgdef *m) {
  upb_handlerset hset = {
    NULL,  // startmsg
    NULL,  // endmsg
    upb_textprinter_value,
    upb_textprinter_startsubmsg,
    upb_textprinter_endsubmsg,
    NULL,  // startseq
    NULL,  // endseq
  };
  return upb_handlers_reghandlerset(h, m, &hset);
}
