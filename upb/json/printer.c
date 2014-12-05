/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2014 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * This currently uses snprintf() to format primitives, and could be optimized
 * further.
 */

#include "upb/json/printer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// StringPiece; a pointer plus a length.
typedef struct {
  const char *ptr;
  size_t len;
} strpc;

strpc *newstrpc(upb_handlers *h, const upb_fielddef *f) {
  strpc *ret = malloc(sizeof(*ret));
  ret->ptr = upb_fielddef_name(f);
  ret->len = strlen(ret->ptr);
  upb_handlers_addcleanup(h, ret, free);
  return ret;
}

// ------------ JSON string printing: values, maps, arrays --------------------

static void print_data(
    upb_json_printer *p, const char *buf, unsigned int len) {
  // TODO: Will need to change if we support pushback from the sink.
  size_t n = upb_bytessink_putbuf(p->output_, p->subc_, buf, len, NULL);
  UPB_ASSERT_VAR(n, n == len);
}

static void print_comma(upb_json_printer *p) {
  if (!p->first_elem_[p->depth_]) {
    print_data(p, ",", 1);
  }
  p->first_elem_[p->depth_] = false;
}

// Helpers that print properly formatted elements to the JSON output stream.

// Used for escaping control chars in strings.
static const char kControlCharLimit = 0x20;

static inline bool is_json_escaped(char c) {
  // See RFC 4627.
  return c < kControlCharLimit || c == '"' || c == '\\';
}

static inline char* json_nice_escape(char c) {
  switch (c) {
    case '"':  return "\\\"";
    case '\\': return "\\\\";
    case '\b': return "\\b";
    case '\f': return "\\f";
    case '\n': return "\\n";
    case '\r': return "\\r";
    case '\t': return "\\t";
    default:   return NULL;
  }
}

// Write a properly quoted and escaped string.
static void putstring(upb_json_printer *p, const char *buf, unsigned int len) {
  print_data(p, "\"", 1);

  const char* unescaped_run = NULL;
  for (unsigned int i = 0; i < len; i++) {
    char c = buf[i];
    // Handle escaping.
    if (is_json_escaped(c)) {
      // Use a "nice" escape, like \n, if one exists for this character.
      const char* escape = json_nice_escape(c);
      // If we don't have a specific 'nice' escape code, use a \uXXXX-style
      // escape.
      char escape_buf[8];
      if (!escape) {
        snprintf(escape_buf, sizeof(escape_buf), "\\u%04x", (int)c);
        escape = escape_buf;
      }

      // N.B. that we assume that the input encoding is equal to the output
      // encoding (both UTF-8 for  now), so for chars >= 0x20 and != \, ", we
      // can simply pass the bytes through.

      // If there's a current run of unescaped chars, print that run first.
      if (unescaped_run) {
        print_data(p, unescaped_run, &buf[i] - unescaped_run);
        unescaped_run = NULL;
      }
      // Then print the escape code.
      print_data(p, escape, strlen(escape));
    } else {
      // Add to the current unescaped run of characters.
      if (unescaped_run == NULL) {
        unescaped_run = &buf[i];
      }
    }
  }

  // If the string ended in a run of unescaped characters, print that last run.
  if (unescaped_run) {
    print_data(p, unescaped_run, &buf[len] - unescaped_run);
  }

  print_data(p, "\"", 1);
}

#define CHKLENGTH(x) if (!(x)) return -1;

// Helpers that format floating point values according to our custom formats.
// Right now we use %.8g and %.17g for float/double, respectively, to match
// proto2::util::JsonFormat's defaults.  May want to change this later.

static size_t fmt_double(double val, char* buf, size_t length) {
  size_t n = snprintf(buf, length, "%.17g", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_float(float val, char* buf, size_t length) {
  size_t n = snprintf(buf, length, "%.8g", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_bool(bool val, char* buf, size_t length) {
  size_t n = snprintf(buf, length, "%s", (val ? "true" : "false"));
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_int64(long val, char* buf, size_t length) {
  size_t n = snprintf(buf, length, "%ld", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_uint64(unsigned long long val, char* buf, size_t length) {
  size_t n = snprintf(buf, length, "%llu", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

// Print a map key given a field name. Called by scalar field handlers and by
// startseq for repeated fields.
static bool putkey(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  const strpc *key = handler_data;
  print_comma(p);
  putstring(p, key->ptr, key->len);
  print_data(p, ":", 1);
  return true;
}

#define CHKFMT(val) if ((val) == -1) return false;
#define CHK(val)    if (!(val)) return false;

#define TYPE_HANDLERS(type, fmt_func)                                        \
  static bool put##type(void *closure, const void *handler_data, type val) { \
    upb_json_printer *p = closure;                                           \
    UPB_UNUSED(handler_data);                                                \
    char data[64];                                                           \
    size_t length = fmt_func(val, data, sizeof(data));                       \
    CHKFMT(length);                                                          \
    print_data(p, data, length);                                             \
    return true;                                                             \
  }                                                                          \
  static bool scalar_##type(void *closure, const void *handler_data,         \
                            type val) {                                      \
    CHK(putkey(closure, handler_data));                                      \
    CHK(put##type(closure, handler_data, val));                              \
    return true;                                                             \
  }                                                                          \
  static bool repeated_##type(void *closure, const void *handler_data,       \
                            type val) {                                      \
    upb_json_printer *p = closure;                                           \
    print_comma(p);                                                          \
    CHK(put##type(closure, handler_data, val));                              \
    return true;                                                             \
  }

TYPE_HANDLERS(double,   fmt_double);
TYPE_HANDLERS(float,    fmt_float);
TYPE_HANDLERS(bool,     fmt_bool);
TYPE_HANDLERS(int32_t,  fmt_int64);
TYPE_HANDLERS(uint32_t, fmt_int64);
TYPE_HANDLERS(int64_t,  fmt_int64);
TYPE_HANDLERS(uint64_t, fmt_uint64);

#undef TYPE_HANDLERS

static void *scalar_startsubmsg(void *closure, const void *handler_data) {
  return putkey(closure, handler_data) ? closure : UPB_BREAK;
}

static void *repeated_startsubmsg(void *closure, const void *handler_data) {
  UPB_UNUSED(handler_data);
  upb_json_printer *p = closure;
  print_comma(p);
  return closure;
}

static bool startmap(void *closure, const void *handler_data) {
  UPB_UNUSED(handler_data);
  upb_json_printer *p = closure;
  if (p->depth_++ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  p->first_elem_[p->depth_] = true;
  print_data(p, "{", 1);
  return true;
}

static bool endmap(void *closure, const void *handler_data, upb_status *s) {
  UPB_UNUSED(handler_data);
  UPB_UNUSED(s);
  upb_json_printer *p = closure;
  if (--p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }
  print_data(p, "}", 1);
  return true;
}

static void *startseq(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  CHK(putkey(closure, handler_data));
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "[", 1);
  return closure;
}

static bool endseq(void *closure, const void *handler_data) {
  UPB_UNUSED(handler_data);
  upb_json_printer *p = closure;
  print_data(p, "]", 1);
  p->depth_--;
  return true;
}

static size_t putstr(void *closure, const void *handler_data, const char *str,
                     size_t len, const upb_bufhandle *handle) {
  UPB_UNUSED(handler_data);
  UPB_UNUSED(handle);
  upb_json_printer *p = closure;
  putstring(p, str, len);
  return len;
}

// This has to Base64 encode the bytes, because JSON has no "bytes" type.
static size_t putbytes(void *closure, const void *handler_data, const char *str,
                       size_t len, const upb_bufhandle *handle) {
  UPB_UNUSED(handler_data);
  UPB_UNUSED(handle);
  upb_json_printer *p = closure;

  // This is the regular base64, not the "web-safe" version.
  static const char base64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  // Base64-encode.
  char data[16000];
  const char *limit = data + sizeof(data);
  const unsigned char *from = (const unsigned char*)str;
  char *to = data;
  size_t remaining = len;
  while (remaining > 2) {
    // TODO(haberman): handle encoded lengths > sizeof(data)
    UPB_ASSERT_VAR(limit, (limit - to) >= 4);

    to[0] = base64[from[0] >> 2];
    to[1] = base64[((from[0] & 0x3) << 4) | (from[1] >> 4)];
    to[2] = base64[((from[1] & 0xf) << 2) | (from[2] >> 6)];
    to[3] = base64[from[2] & 0x3f];

    remaining -= 3;
    to += 4;
    from += 3;
  }

  switch (remaining) {
    case 2:
      to[0] = base64[from[0] >> 2];
      to[1] = base64[((from[0] & 0x3) << 4) | (from[1] >> 4)];
      to[2] = base64[(from[1] & 0xf) << 2];
      to[3] = '=';
      to += 4;
      from += 2;
      break;
    case 1:
      to[0] = base64[from[0] >> 2];
      to[1] = base64[((from[0] & 0x3) << 4)];
      to[2] = '=';
      to[3] = '=';
      to += 4;
      from += 1;
      break;
  }

  size_t bytes = to - data;
  putstring(p, data, bytes);
  return len;
}

static size_t scalar_str(void *closure, const void *handler_data,
                         const char *str, size_t len,
                         const upb_bufhandle *handle) {
  CHK(putkey(closure, handler_data));
  CHK(putstr(closure, handler_data, str, len, handle));
  return len;
}

static size_t repeated_str(void *closure, const void *handler_data,
                           const char *str, size_t len,
                           const upb_bufhandle *handle) {
  upb_json_printer *p = closure;
  print_comma(p);
  CHK(putstr(closure, handler_data, str, len, handle));
  return len;
}

static size_t scalar_bytes(void *closure, const void *handler_data,
                           const char *str, size_t len,
                           const upb_bufhandle *handle) {
  CHK(putkey(closure, handler_data));
  CHK(putbytes(closure, handler_data, str, len, handle));
  return len;
}

static size_t repeated_bytes(void *closure, const void *handler_data,
                             const char *str, size_t len,
                             const upb_bufhandle *handle) {
  upb_json_printer *p = closure;
  print_comma(p);
  CHK(putbytes(closure, handler_data, str, len, handle));
  return len;
}

void sethandlers(const void *closure, upb_handlers *h) {
  UPB_UNUSED(closure);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INITIALIZER;
  upb_handlers_setstartmsg(h, startmap, &empty_attr);
  upb_handlers_setendmsg(h, endmap, &empty_attr);

#define TYPE(type, name, ctype)                                    \
  case type:                                                       \
    if (upb_fielddef_isseq(f)) {                                   \
      upb_handlers_set##name(h, f, repeated_##ctype, &empty_attr); \
    } else {                                                       \
      upb_handlers_set##name(h, f, scalar_##ctype, &name_attr);    \
    }                                                              \
    break;

  upb_msg_iter i;
  upb_msg_begin(&i, upb_handlers_msgdef(h));
  for(; !upb_msg_done(&i); upb_msg_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);

    upb_handlerattr name_attr = UPB_HANDLERATTR_INITIALIZER;
    upb_handlerattr_sethandlerdata(&name_attr, newstrpc(h, f));

    if (upb_fielddef_isseq(f)) {
      upb_handlers_setstartseq(h, f, startseq, &name_attr);
      upb_handlers_setendseq(h, f, endseq, &empty_attr);
    }

    switch (upb_fielddef_type(f)) {
      TYPE(UPB_TYPE_FLOAT,  float,  float);
      TYPE(UPB_TYPE_DOUBLE, double, double);
      TYPE(UPB_TYPE_BOOL,   bool,   bool);
      TYPE(UPB_TYPE_ENUM,   int32,  int32_t);
      TYPE(UPB_TYPE_INT32,  int32,  int32_t);
      TYPE(UPB_TYPE_UINT32, uint32, uint32_t);
      TYPE(UPB_TYPE_INT64,  int64,  int64_t);
      TYPE(UPB_TYPE_UINT64, uint64, uint64_t);
      case UPB_TYPE_STRING:
        // XXX: this doesn't support strings that span buffers yet.
        if (upb_fielddef_isseq(f)) {
          upb_handlers_setstring(h, f, repeated_str, &empty_attr);
        } else {
          upb_handlers_setstring(h, f, scalar_str, &name_attr);
        }
        break;
      case UPB_TYPE_BYTES:
        // XXX: this doesn't support strings that span buffers yet.
        if (upb_fielddef_isseq(f)) {
          upb_handlers_setstring(h, f, repeated_bytes, &empty_attr);
        } else {
          upb_handlers_setstring(h, f, scalar_bytes, &name_attr);
        }
        break;
      case UPB_TYPE_MESSAGE:
        if (upb_fielddef_isseq(f)) {
          upb_handlers_setstartsubmsg(h, f, repeated_startsubmsg, &name_attr);
        } else {
          upb_handlers_setstartsubmsg(h, f, scalar_startsubmsg, &name_attr);
        }
        break;
    }

    upb_handlerattr_uninit(&name_attr);
  }

  upb_handlerattr_uninit(&empty_attr);
#undef TYPE
}

/* Public API *****************************************************************/

void upb_json_printer_init(upb_json_printer *p, const upb_handlers *h) {
  p->output_ = NULL;
  p->depth_ = 0;
  upb_sink_reset(&p->input_, h, p);
}

void upb_json_printer_uninit(upb_json_printer *p) {
  UPB_UNUSED(p);
}

void upb_json_printer_reset(upb_json_printer *p) {
  p->depth_ = 0;
}

void upb_json_printer_resetoutput(upb_json_printer *p, upb_bytessink *output) {
  upb_json_printer_reset(p);
  p->output_ = output;
}

upb_sink *upb_json_printer_input(upb_json_printer *p) {
  return &p->input_;
}

const upb_handlers *upb_json_printer_newhandlers(const upb_msgdef *md,
                                                 const void *owner) {
  return upb_handlers_newfrozen(md, owner, sethandlers, NULL);
}
