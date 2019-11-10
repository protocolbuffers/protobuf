/*
 * upb::pb::TextPrinter
 *
 * OPT: This is not optimized at all.  It uses printf() which parses the format
 * string every time, and it allocates memory for every put.
 */

#include "upb/pb/textprinter.h"

#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "upb/sink.h"

#include "upb/port_def.inc"

struct upb_textprinter {
  upb_sink input_;
  upb_bytessink output_;
  int indent_depth_;
  bool single_line_;
  void *subc;
};

#define CHECK(x) if ((x) < 0) goto err;

static const char *shortname(const char *longname) {
  const char *last = strrchr(longname, '.');
  return last ? last + 1 : longname;
}

static int indent(upb_textprinter *p) {
  int i;
  if (!p->single_line_)
    for (i = 0; i < p->indent_depth_; i++)
      upb_bytessink_putbuf(p->output_, p->subc, "  ", 2, NULL);
  return 0;
}

static int endfield(upb_textprinter *p) {
  const char ch = (p->single_line_ ? ' ' : '\n');
  upb_bytessink_putbuf(p->output_, p->subc, &ch, 1, NULL);
  return 0;
}

static int putescaped(upb_textprinter *p, const char *buf, size_t len,
                      bool preserve_utf8) {
  /* Based on CEscapeInternal() from Google's protobuf release. */
  char dstbuf[4096], *dst = dstbuf, *dstend = dstbuf + sizeof(dstbuf);
  const char *end = buf + len;

  /* I think hex is prettier and more useful, but proto2 uses octal; should
   * investigate whether it can parse hex also. */
  const bool use_hex = false;
  bool last_hex_escape = false; /* true if last output char was \xNN */

  for (; buf < end; buf++) {
    bool is_hex_escape;

    if (dstend - dst < 4) {
      upb_bytessink_putbuf(p->output_, p->subc, dstbuf, dst - dstbuf, NULL);
      dst = dstbuf;
    }

    is_hex_escape = false;
    switch (*buf) {
      case '\n': *(dst++) = '\\'; *(dst++) = 'n';  break;
      case '\r': *(dst++) = '\\'; *(dst++) = 'r';  break;
      case '\t': *(dst++) = '\\'; *(dst++) = 't';  break;
      case '\"': *(dst++) = '\\'; *(dst++) = '\"'; break;
      case '\'': *(dst++) = '\\'; *(dst++) = '\''; break;
      case '\\': *(dst++) = '\\'; *(dst++) = '\\'; break;
      default:
        /* Note that if we emit \xNN and the buf character after that is a hex
         * digit then that digit must be escaped too to prevent it being
         * interpreted as part of the character code by C. */
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
  /* Flush remaining data. */
  upb_bytessink_putbuf(p->output_, p->subc, dstbuf, dst - dstbuf, NULL);
  return 0;
}

bool putf(upb_textprinter *p, const char *fmt, ...) {
  va_list args;
  va_list args_copy;
  char *str;
  int written;
  int len;
  bool ok;

  va_start(args, fmt);

  /* Run once to get the length of the string. */
  _upb_va_copy(args_copy, args);
  len = _upb_vsnprintf(NULL, 0, fmt, args_copy);
  va_end(args_copy);

  /* + 1 for NULL terminator (vsprintf() requires it even if we don't). */
  str = upb_gmalloc(len + 1);
  if (!str) return false;
  written = vsprintf(str, fmt, args);
  va_end(args);
  UPB_ASSERT(written == len);

  ok = upb_bytessink_putbuf(p->output_, p->subc, str, len, NULL);
  upb_gfree(str);
  return ok;
}


/* handlers *******************************************************************/

static bool textprinter_startmsg(void *c, const void *hd) {
  upb_textprinter *p = c;
  UPB_UNUSED(hd);
  if (p->indent_depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc);
  }
  return true;
}

static bool textprinter_endmsg(void *c, const void *hd, upb_status *s) {
  upb_textprinter *p = c;
  UPB_UNUSED(hd);
  UPB_UNUSED(s);
  if (p->indent_depth_ == 0) {
    upb_bytessink_end(p->output_);
  }
  return true;
}

#define TYPE(name, ctype, fmt) \
  static bool textprinter_put ## name(void *closure, const void *handler_data, \
                                      ctype val) {                             \
    upb_textprinter *p = closure;                                              \
    const upb_fielddef *f = handler_data;                                      \
    CHECK(indent(p));                                                          \
    putf(p, "%s: " fmt, upb_fielddef_name(f), val);                            \
    CHECK(endfield(p));                                                        \
    return true;                                                               \
  err:                                                                         \
    return false;                                                              \
}

static bool textprinter_putbool(void *closure, const void *handler_data,
                                bool val) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = handler_data;
  CHECK(indent(p));
  putf(p, "%s: %s", upb_fielddef_name(f), val ? "true" : "false");
  CHECK(endfield(p));
  return true;
err:
  return false;
}

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY_MACROVAL(x) STRINGIFY_HELPER(x)

TYPE(int32,  int32_t,  "%" PRId32)
TYPE(int64,  int64_t,  "%" PRId64)
TYPE(uint32, uint32_t, "%" PRIu32)
TYPE(uint64, uint64_t, "%" PRIu64)
TYPE(float,  float,    "%." STRINGIFY_MACROVAL(FLT_DIG) "g")
TYPE(double, double,   "%." STRINGIFY_MACROVAL(DBL_DIG) "g")

#undef TYPE

/* Output a symbolic value from the enum if found, else just print as int32. */
static bool textprinter_putenum(void *closure, const void *handler_data,
                                int32_t val) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = handler_data;
  const upb_enumdef *enum_def = upb_fielddef_enumsubdef(f);
  const char *label = upb_enumdef_iton(enum_def, val);
  if (label) {
    indent(p);
    putf(p, "%s: %s", upb_fielddef_name(f), label);
    endfield(p);
  } else {
    if (!textprinter_putint32(closure, handler_data, val))
      return false;
  }
  return true;
}

static void *textprinter_startstr(void *closure, const void *handler_data,
                      size_t size_hint) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = handler_data;
  UPB_UNUSED(size_hint);
  indent(p);
  putf(p, "%s: \"", upb_fielddef_name(f));
  return p;
}

static bool textprinter_endstr(void *closure, const void *handler_data) {
  upb_textprinter *p = closure;
  UPB_UNUSED(handler_data);
  putf(p, "\"");
  endfield(p);
  return true;
}

static size_t textprinter_putstr(void *closure, const void *hd, const char *buf,
                                 size_t len, const upb_bufhandle *handle) {
  upb_textprinter *p = closure;
  const upb_fielddef *f = hd;
  UPB_UNUSED(handle);
  CHECK(putescaped(p, buf, len, upb_fielddef_type(f) == UPB_TYPE_STRING));
  return len;
err:
  return 0;
}

static void *textprinter_startsubmsg(void *closure, const void *handler_data) {
  upb_textprinter *p = closure;
  const char *name = handler_data;
  CHECK(indent(p));
  putf(p, "%s {%c", name, p->single_line_ ? ' ' : '\n');
  p->indent_depth_++;
  return p;
err:
  return UPB_BREAK;
}

static bool textprinter_endsubmsg(void *closure, const void *handler_data) {
  upb_textprinter *p = closure;
  UPB_UNUSED(handler_data);
  p->indent_depth_--;
  CHECK(indent(p));
  upb_bytessink_putbuf(p->output_, p->subc, "}", 1, NULL);
  CHECK(endfield(p));
  return true;
err:
  return false;
}

static void onmreg(const void *c, upb_handlers *h) {
  const upb_msgdef *m = upb_handlers_msgdef(h);
  upb_msg_field_iter i;
  UPB_UNUSED(c);

  upb_handlers_setstartmsg(h, textprinter_startmsg, NULL);
  upb_handlers_setendmsg(h, textprinter_endmsg, NULL);

  for(upb_msg_field_begin(&i, m);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    upb_fielddef *f = upb_msg_iter_field(&i);
    upb_handlerattr attr = UPB_HANDLERATTR_INIT;
    attr.handler_data = f;
    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_INT32:
        upb_handlers_setint32(h, f, textprinter_putint32, &attr);
        break;
      case UPB_TYPE_INT64:
        upb_handlers_setint64(h, f, textprinter_putint64, &attr);
        break;
      case UPB_TYPE_UINT32:
        upb_handlers_setuint32(h, f, textprinter_putuint32, &attr);
        break;
      case UPB_TYPE_UINT64:
        upb_handlers_setuint64(h, f, textprinter_putuint64, &attr);
        break;
      case UPB_TYPE_FLOAT:
        upb_handlers_setfloat(h, f, textprinter_putfloat, &attr);
        break;
      case UPB_TYPE_DOUBLE:
        upb_handlers_setdouble(h, f, textprinter_putdouble, &attr);
        break;
      case UPB_TYPE_BOOL:
        upb_handlers_setbool(h, f, textprinter_putbool, &attr);
        break;
      case UPB_TYPE_STRING:
      case UPB_TYPE_BYTES:
        upb_handlers_setstartstr(h, f, textprinter_startstr, &attr);
        upb_handlers_setstring(h, f, textprinter_putstr, &attr);
        upb_handlers_setendstr(h, f, textprinter_endstr, &attr);
        break;
      case UPB_TYPE_MESSAGE: {
        const char *name =
            upb_fielddef_descriptortype(f) == UPB_DESCRIPTOR_TYPE_GROUP
                ? shortname(upb_msgdef_fullname(upb_fielddef_msgsubdef(f)))
                : upb_fielddef_name(f);
        attr.handler_data = name;
        upb_handlers_setstartsubmsg(h, f, textprinter_startsubmsg, &attr);
        upb_handlers_setendsubmsg(h, f, textprinter_endsubmsg, &attr);
        break;
      }
      case UPB_TYPE_ENUM:
        upb_handlers_setint32(h, f, textprinter_putenum, &attr);
        break;
    }
  }
}

static void textprinter_reset(upb_textprinter *p, bool single_line) {
  p->single_line_ = single_line;
  p->indent_depth_ = 0;
}


/* Public API *****************************************************************/

upb_textprinter *upb_textprinter_create(upb_arena *arena, const upb_handlers *h,
                                        upb_bytessink output) {
  upb_textprinter *p = upb_arena_malloc(arena, sizeof(upb_textprinter));
  if (!p) return NULL;

  p->output_ = output;
  upb_sink_reset(&p->input_, h, p);
  textprinter_reset(p, false);

  return p;
}

upb_handlercache *upb_textprinter_newcache(void) {
  return upb_handlercache_new(&onmreg, NULL);
}

upb_sink upb_textprinter_input(upb_textprinter *p) { return p->input_; }

void upb_textprinter_setsingleline(upb_textprinter *p, bool single_line) {
  p->single_line_ = single_line;
}
