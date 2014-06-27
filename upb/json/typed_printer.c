/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2014 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Uses YAJL at the moment; this is not exposed publicly and will likely change
 * in the future.
 *
 */

#include "upb/json/typed_printer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <yajl/yajl_gen.h>

static void doprint(void *_p, const char *buf, unsigned int len) {
  upb_json_typedprinter *p = _p;
  // YAJL doesn't support returning an error status here, so we can't properly
  // support clients who return a value other than "len" here.
  size_t n = upb_bytessink_putbuf(p->output_, p->subc_, buf, len, NULL);
  UPB_ASSERT_VAR(n, n == len);
}

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

#define CHKYAJL(x) if ((x) != yajl_gen_status_ok) return false;
#define CHK(x) if (!(x)) return false;

// Wrapper for yajl_gen_string that takes "const char*" instead of "const
// unsigned char*".
static yajl_gen_status yajl_gen_string2(yajl_gen yajl, const char *ptr,
                                        size_t len) {
  return yajl_gen_string(yajl, (const unsigned char *)ptr, len);
}

// Wrappers for yajl_gen_number() that formats floating point values
// according to our custom formats.  Right now we use %.8g and %.17g
// for float/double, respectively, to match proto2::util::JsonFormat's
// defaults.  May want to change this later.

static yajl_gen_status upbyajl_gen_double(yajl_gen yajl, double val) {
  char data[64];
  int n = snprintf(data, sizeof(data), "%.17g", val);
  CHK(n > 0 && n < sizeof(data));
  return yajl_gen_number(yajl, data, n);
}

static yajl_gen_status upbyajl_gen_float(yajl_gen yajl, float val) {
  char data[64];
  int n = snprintf(data, sizeof(data), "%.8g", val);
  CHK(n > 0 && n < sizeof(data));
  return yajl_gen_number(yajl, data, n);
}

static yajl_gen_status upbyajl_gen_uint64(yajl_gen yajl,
                                          unsigned long long val) {
  char data[64];
  int n = snprintf(data, sizeof(data), "%llu", val);
  CHK(n > 0 && n < sizeof(data));
  return yajl_gen_number(yajl, data, n);
}

static bool putkey(void *closure, const void *handler_data) {
  upb_json_typedprinter *p = closure;
  const strpc *key = handler_data;
  CHKYAJL(yajl_gen_string2(p->yajl_gen_, key->ptr, key->len));
  return true;
}

#define TYPE_HANDLERS(type, yajlfunc)                                        \
  static bool put##type(void *closure, const void *handler_data, type val) { \
    upb_json_typedprinter *p = closure;                                      \
    UPB_UNUSED(handler_data);                                                \
    CHKYAJL(yajlfunc(p->yajl_gen_, val));                                    \
    return true;                                                             \
  }                                                                          \
  static bool scalar_##type(void *closure, const void *handler_data,         \
                            type val) {                                      \
    CHK(putkey(closure, handler_data));                                      \
    CHK(put##type(closure, handler_data, val));                              \
    return true;                                                             \
  }

TYPE_HANDLERS(double,   upbyajl_gen_double);
TYPE_HANDLERS(float,    upbyajl_gen_float);
TYPE_HANDLERS(bool,     yajl_gen_bool);
TYPE_HANDLERS(int32_t,  yajl_gen_integer);
TYPE_HANDLERS(uint32_t, yajl_gen_integer);
TYPE_HANDLERS(int64_t,  yajl_gen_integer);
TYPE_HANDLERS(uint64_t, upbyajl_gen_uint64);

#undef TYPE_HANDLERS

static void *startsubmsg(void *closure, const void *handler_data) {
  return putkey(closure, handler_data) ? closure : UPB_BREAK;
}

static bool startmap(void *closure, const void *handler_data) {
  UPB_UNUSED(handler_data);
  upb_json_typedprinter *p = closure;
  if (p->depth_++ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  CHKYAJL(yajl_gen_map_open(p->yajl_gen_));
  return true;
}

static bool endmap(void *closure, const void *handler_data, upb_status *s) {
  UPB_UNUSED(handler_data);
  UPB_UNUSED(s);
  upb_json_typedprinter *p = closure;
  if (--p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }
  CHKYAJL(yajl_gen_map_close(p->yajl_gen_));
  return true;
}

static void *startseq(void *closure, const void *handler_data) {
  upb_json_typedprinter *p = closure;
  CHK(putkey(closure, handler_data));
  CHKYAJL(yajl_gen_array_open(p->yajl_gen_));
  return closure;
}

static bool endseq(void *closure, const void *handler_data) {
  UPB_UNUSED(handler_data);
  upb_json_typedprinter *p = closure;
  CHKYAJL(yajl_gen_array_close(p->yajl_gen_));
  return true;
}

static size_t putstr(void *closure, const void *handler_data, const char *str,
                     size_t len, const upb_bufhandle *handle) {
  UPB_UNUSED(handle);
  upb_json_typedprinter *p = closure;
  CHKYAJL(yajl_gen_string2(p->yajl_gen_, str, len));
  return len;
}

// This has to Base64 encode the bytes, because JSON has no "bytes" type.
static size_t putbytes(void *closure, const void *handler_data, const char *str,
                       size_t len, const upb_bufhandle *handle) {
  UPB_UNUSED(handle);
  upb_json_typedprinter *p = closure;

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
    to[1] = base64[((from[0] & 0x3) << 4) + (from[1] >> 4)];
    to[2] = base64[((from[1] & 0xf) << 2) + (from[2] >> 6)];
    to[3] = base64[from[2] & 0x3f];

    remaining -= 3;
    to += 4;
    from += 3;
  }

  size_t bytes = to - data;
  if (yajl_gen_string2(p->yajl_gen_, data, bytes) != yajl_gen_status_ok) {
    return 0;
  }
  return len;
}

static size_t scalar_str(void *closure, const void *handler_data,
                         const char *str, size_t len,
                         const upb_bufhandle *handle) {
  CHK(putkey(closure, handler_data));
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

void sethandlers(const void *closure, upb_handlers *h) {
  UPB_UNUSED(closure);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INITIALIZER;
  upb_handlers_setstartmsg(h, startmap, &empty_attr);
  upb_handlers_setendmsg(h, endmap, &empty_attr);

#define TYPE(type, name, ctype)                                 \
  case type:                                                    \
    if (upb_fielddef_isseq(f)) {                                \
      upb_handlers_set##name(h, f, put##ctype, &empty_attr);    \
    } else {                                                    \
      upb_handlers_set##name(h, f, scalar_##ctype, &name_attr); \
    }                                                           \
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
          upb_handlers_setstring(h, f, putstr, &empty_attr);
        } else {
          upb_handlers_setstring(h, f, scalar_str, &name_attr);
        }
        break;
      case UPB_TYPE_BYTES:
        // XXX: this doesn't support strings that span buffers yet.
        if (upb_fielddef_isseq(f)) {
          upb_handlers_setstring(h, f, putbytes, &empty_attr);
        } else {
          upb_handlers_setstring(h, f, scalar_bytes, &name_attr);
        }
        break;
      case UPB_TYPE_MESSAGE:
        if (!upb_fielddef_isseq(f)) {
          upb_handlers_setstartsubmsg(h, f, startsubmsg, &name_attr);
        }
        break;
    }

    upb_handlerattr_uninit(&name_attr);
  }

  upb_handlerattr_uninit(&empty_attr);
#undef TYPE
}

// YAJL unfortunately does not support stack allocation, nor resetting an
// allocated object, so we have to allocate on the heap and reallocate whenever
// there is a reset.
static void reset(upb_json_typedprinter *p, bool free) {
  if (free) {
    yajl_gen_free(p->yajl_gen_);
  }
  p->yajl_gen_ = yajl_gen_alloc(NULL);
  yajl_gen_config(p->yajl_gen_, yajl_gen_validate_utf8, 0);
  yajl_gen_config(p->yajl_gen_, yajl_gen_print_callback, &doprint, p);
}

/* Public API *****************************************************************/

void upb_json_typedprinter_init(upb_json_typedprinter *p,
                                const upb_handlers *h) {
  p->output_ = NULL;
  p->depth_ = 0;
  reset(p, false);
  upb_sink_reset(&p->input_, h, p);
}

void upb_json_typedprinter_uninit(upb_json_typedprinter *p) {
  yajl_gen_free(p->yajl_gen_);
}

void upb_json_typedprinter_reset(upb_json_typedprinter *p) {
  p->depth_ = 0;
  reset(p, true);
}

void upb_json_typedprinter_resetoutput(upb_json_typedprinter *p,
                                       upb_bytessink *output) {
  upb_json_typedprinter_reset(p);
  p->output_ = output;
}

upb_sink *upb_json_typedprinter_input(upb_json_typedprinter *p) {
  return &p->input_;
}

const upb_handlers *upb_json_typedprinter_newhandlers(const upb_msgdef *md,
                                                      const void *owner) {
  return upb_handlers_newfrozen(md, owner, sethandlers, NULL);
}
