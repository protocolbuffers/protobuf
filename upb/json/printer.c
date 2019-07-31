/*
** This currently uses snprintf() to format primitives, and could be optimized
** further.
*/

#include "upb/json/printer.h"

#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "upb/port_def.inc"

struct upb_json_printer {
  upb_sink input_;
  /* BytesSink closure. */
  void *subc_;
  upb_bytessink output_;

  /* We track the depth so that we know when to emit startstr/endstr on the
   * output. */
  int depth_;

  /* Have we emitted the first element? This state is necessary to emit commas
   * without leaving a trailing comma in arrays/maps. We keep this state per
   * frame depth.
   *
   * Why max_depth * 2? UPB_MAX_HANDLER_DEPTH counts depth as nested messages.
   * We count frames (contexts in which we separate elements by commas) as both
   * repeated fields and messages (maps), and the worst case is a
   * message->repeated field->submessage->repeated field->... nesting. */
  bool first_elem_[UPB_MAX_HANDLER_DEPTH * 2];

  /* To print timestamp, printer needs to cache its seconds and nanos values
   * and convert them when ending timestamp message. See comments of
   * printer_sethandlers_timestamp for more detail. */
  int64_t seconds;
  int32_t nanos;
};

/* StringPiece; a pointer plus a length. */
typedef struct {
  char *ptr;
  size_t len;
} strpc;

void freestrpc(void *ptr) {
  strpc *pc = ptr;
  upb_gfree(pc->ptr);
  upb_gfree(pc);
}

typedef struct {
  bool preserve_fieldnames;
} upb_json_printercache;

/* Convert fielddef name to JSON name and return as a string piece. */
strpc *newstrpc(upb_handlers *h, const upb_fielddef *f,
                bool preserve_fieldnames) {
  /* TODO(haberman): handle malloc failure. */
  strpc *ret = upb_gmalloc(sizeof(*ret));
  if (preserve_fieldnames) {
    ret->ptr = upb_gstrdup(upb_fielddef_name(f));
    ret->len = strlen(ret->ptr);
  } else {
    size_t len;
    ret->len = upb_fielddef_getjsonname(f, NULL, 0);
    ret->ptr = upb_gmalloc(ret->len);
    len = upb_fielddef_getjsonname(f, ret->ptr, ret->len);
    UPB_ASSERT(len == ret->len);
    ret->len--;  /* NULL */
  }

  upb_handlers_addcleanup(h, ret, freestrpc);
  return ret;
}

/* Convert a null-terminated const char* to a string piece. */
strpc *newstrpc_str(upb_handlers *h, const char * str) {
  strpc * ret = upb_gmalloc(sizeof(*ret));
  ret->ptr = upb_gstrdup(str);
  ret->len = strlen(str);
  upb_handlers_addcleanup(h, ret, freestrpc);
  return ret;
}

/* ------------ JSON string printing: values, maps, arrays ------------------ */

static void print_data(
    upb_json_printer *p, const char *buf, unsigned int len) {
  /* TODO: Will need to change if we support pushback from the sink. */
  size_t n = upb_bytessink_putbuf(p->output_, p->subc_, buf, len, NULL);
  UPB_ASSERT(n == len);
}

static void print_comma(upb_json_printer *p) {
  if (!p->first_elem_[p->depth_]) {
    print_data(p, ",", 1);
  }
  p->first_elem_[p->depth_] = false;
}

/* Helpers that print properly formatted elements to the JSON output stream. */

/* Used for escaping control chars in strings. */
static const char kControlCharLimit = 0x20;

UPB_INLINE bool is_json_escaped(char c) {
  /* See RFC 4627. */
  unsigned char uc = (unsigned char)c;
  return uc < kControlCharLimit || uc == '"' || uc == '\\';
}

UPB_INLINE const char* json_nice_escape(char c) {
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

/* Write a properly escaped string chunk. The surrounding quotes are *not*
 * printed; this is so that the caller has the option of emitting the string
 * content in chunks. */
static void putstring(upb_json_printer *p, const char *buf, unsigned int len) {
  const char* unescaped_run = NULL;
  unsigned int i;
  for (i = 0; i < len; i++) {
    char c = buf[i];
    /* Handle escaping. */
    if (is_json_escaped(c)) {
      /* Use a "nice" escape, like \n, if one exists for this character. */
      const char* escape = json_nice_escape(c);
      /* If we don't have a specific 'nice' escape code, use a \uXXXX-style
       * escape. */
      char escape_buf[8];
      if (!escape) {
        unsigned char byte = (unsigned char)c;
        _upb_snprintf(escape_buf, sizeof(escape_buf), "\\u%04x", (int)byte);
        escape = escape_buf;
      }

      /* N.B. that we assume that the input encoding is equal to the output
       * encoding (both UTF-8 for  now), so for chars >= 0x20 and != \, ", we
       * can simply pass the bytes through. */

      /* If there's a current run of unescaped chars, print that run first. */
      if (unescaped_run) {
        print_data(p, unescaped_run, &buf[i] - unescaped_run);
        unescaped_run = NULL;
      }
      /* Then print the escape code. */
      print_data(p, escape, strlen(escape));
    } else {
      /* Add to the current unescaped run of characters. */
      if (unescaped_run == NULL) {
        unescaped_run = &buf[i];
      }
    }
  }

  /* If the string ended in a run of unescaped characters, print that last run. */
  if (unescaped_run) {
    print_data(p, unescaped_run, &buf[len] - unescaped_run);
  }
}

#define CHKLENGTH(x) if (!(x)) return -1;

/* Helpers that format floating point values according to our custom formats.
 * Right now we use %.8g and %.17g for float/double, respectively, to match
 * proto2::util::JsonFormat's defaults.  May want to change this later. */

const char neginf[] = "\"-Infinity\"";
const char inf[] = "\"Infinity\"";

static size_t fmt_double(double val, char* buf, size_t length) {
  if (val == UPB_INFINITY) {
    CHKLENGTH(length >= strlen(inf));
    strcpy(buf, inf);
    return strlen(inf);
  } else if (val == -UPB_INFINITY) {
    CHKLENGTH(length >= strlen(neginf));
    strcpy(buf, neginf);
    return strlen(neginf);
  } else {
    size_t n = _upb_snprintf(buf, length, "%.17g", val);
    CHKLENGTH(n > 0 && n < length);
    return n;
  }
}

static size_t fmt_float(float val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%.8g", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_bool(bool val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%s", (val ? "true" : "false"));
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_int64_as_number(long long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%lld", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_uint64_as_number(
    unsigned long long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%llu", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_int64_as_string(long long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "\"%lld\"", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_uint64_as_string(
    unsigned long long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "\"%llu\"", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

/* Print a map key given a field name. Called by scalar field handlers and by
 * startseq for repeated fields. */
static bool putkey(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  const strpc *key = handler_data;
  print_comma(p);
  print_data(p, "\"", 1);
  putstring(p, key->ptr, key->len);
  print_data(p, "\":", 2);
  return true;
}

#define CHKFMT(val) if ((val) == (size_t)-1) return false;
#define CHK(val)    if (!(val)) return false;

#define TYPE_HANDLERS(type, fmt_func)                                        \
  static bool put##type(void *closure, const void *handler_data, type val) { \
    upb_json_printer *p = closure;                                           \
    char data[64];                                                           \
    size_t length = fmt_func(val, data, sizeof(data));                       \
    UPB_UNUSED(handler_data);                                                \
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
                              type val) {                                    \
    upb_json_printer *p = closure;                                           \
    print_comma(p);                                                          \
    CHK(put##type(closure, handler_data, val));                              \
    return true;                                                             \
  }

#define TYPE_HANDLERS_MAPKEY(type, fmt_func)                                 \
  static bool putmapkey_##type(void *closure, const void *handler_data,      \
                            type val) {                                      \
    upb_json_printer *p = closure;                                           \
    char data[64];                                                           \
    size_t length = fmt_func(val, data, sizeof(data));                       \
    UPB_UNUSED(handler_data);                                                \
    print_data(p, "\"", 1);                                                  \
    print_data(p, data, length);                                             \
    print_data(p, "\":", 2);                                                 \
    return true;                                                             \
  }

TYPE_HANDLERS(double,   fmt_double)
TYPE_HANDLERS(float,    fmt_float)
TYPE_HANDLERS(bool,     fmt_bool)
TYPE_HANDLERS(int32_t,  fmt_int64_as_number)
TYPE_HANDLERS(uint32_t, fmt_int64_as_number)
TYPE_HANDLERS(int64_t,  fmt_int64_as_string)
TYPE_HANDLERS(uint64_t, fmt_uint64_as_string)

/* double and float are not allowed to be map keys. */
TYPE_HANDLERS_MAPKEY(bool,     fmt_bool)
TYPE_HANDLERS_MAPKEY(int32_t,  fmt_int64_as_number)
TYPE_HANDLERS_MAPKEY(uint32_t, fmt_int64_as_number)
TYPE_HANDLERS_MAPKEY(int64_t,  fmt_int64_as_number)
TYPE_HANDLERS_MAPKEY(uint64_t, fmt_uint64_as_number)

#undef TYPE_HANDLERS
#undef TYPE_HANDLERS_MAPKEY

typedef struct {
  void *keyname;
  const upb_enumdef *enumdef;
} EnumHandlerData;

static bool scalar_enum(void *closure, const void *handler_data,
                        int32_t val) {
  const EnumHandlerData *hd = handler_data;
  upb_json_printer *p = closure;
  const char *symbolic_name;

  CHK(putkey(closure, hd->keyname));

  symbolic_name = upb_enumdef_iton(hd->enumdef, val);
  if (symbolic_name) {
    print_data(p, "\"", 1);
    putstring(p, symbolic_name, strlen(symbolic_name));
    print_data(p, "\"", 1);
  } else {
    putint32_t(closure, NULL, val);
  }

  return true;
}

static void print_enum_symbolic_name(upb_json_printer *p,
                                     const upb_enumdef *def,
                                     int32_t val) {
  const char *symbolic_name = upb_enumdef_iton(def, val);
  if (symbolic_name) {
    print_data(p, "\"", 1);
    putstring(p, symbolic_name, strlen(symbolic_name));
    print_data(p, "\"", 1);
  } else {
    putint32_t(p, NULL, val);
  }
}

static bool repeated_enum(void *closure, const void *handler_data,
                          int32_t val) {
  const EnumHandlerData *hd = handler_data;
  upb_json_printer *p = closure;
  print_comma(p);

  print_enum_symbolic_name(p, hd->enumdef, val);

  return true;
}

static bool mapvalue_enum(void *closure, const void *handler_data,
                          int32_t val) {
  const EnumHandlerData *hd = handler_data;
  upb_json_printer *p = closure;

  print_enum_symbolic_name(p, hd->enumdef, val);

  return true;
}

static void *scalar_startsubmsg(void *closure, const void *handler_data) {
  return putkey(closure, handler_data) ? closure : UPB_BREAK;
}

static void *repeated_startsubmsg(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_comma(p);
  return closure;
}

static void start_frame(upb_json_printer *p) {
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "{", 1);
}

static void end_frame(upb_json_printer *p) {
  print_data(p, "}", 1);
  p->depth_--;
}

static bool printer_startmsg(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  if (p->depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  start_frame(p);
  return true;
}

static bool printer_endmsg(void *closure, const void *handler_data, upb_status *s) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(s);
  end_frame(p);
  if (p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }
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
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "]", 1);
  p->depth_--;
  return true;
}

static void *startmap(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  CHK(putkey(closure, handler_data));
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "{", 1);
  return closure;
}

static bool endmap(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "}", 1);
  p->depth_--;
  return true;
}

static size_t putstr(void *closure, const void *handler_data, const char *str,
                     size_t len, const upb_bufhandle *handle) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(handle);
  putstring(p, str, len);
  return len;
}

/* This has to Base64 encode the bytes, because JSON has no "bytes" type. */
static size_t putbytes(void *closure, const void *handler_data, const char *str,
                       size_t len, const upb_bufhandle *handle) {
  upb_json_printer *p = closure;

  /* This is the regular base64, not the "web-safe" version. */
  static const char base64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  /* Base64-encode. */
  char data[16000];
  const char *limit = data + sizeof(data);
  const unsigned char *from = (const unsigned char*)str;
  char *to = data;
  size_t remaining = len;
  size_t bytes;

  UPB_UNUSED(handler_data);
  UPB_UNUSED(handle);

  print_data(p, "\"", 1);

  while (remaining > 2) {
    if (limit - to < 4) {
      bytes = to - data;
      putstring(p, data, bytes);
      to = data;
    }

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

  bytes = to - data;
  putstring(p, data, bytes);
  print_data(p, "\"", 1);
  return len;
}

static void *scalar_startstr(void *closure, const void *handler_data,
                             size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  CHK(putkey(closure, handler_data));
  print_data(p, "\"", 1);
  return p;
}

static size_t scalar_str(void *closure, const void *handler_data,
                         const char *str, size_t len,
                         const upb_bufhandle *handle) {
  CHK(putstr(closure, handler_data, str, len, handle));
  return len;
}

static bool scalar_endstr(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "\"", 1);
  return true;
}

static void *repeated_startstr(void *closure, const void *handler_data,
                               size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  print_comma(p);
  print_data(p, "\"", 1);
  return p;
}

static size_t repeated_str(void *closure, const void *handler_data,
                           const char *str, size_t len,
                           const upb_bufhandle *handle) {
  CHK(putstr(closure, handler_data, str, len, handle));
  return len;
}

static bool repeated_endstr(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "\"", 1);
  return true;
}

static void *mapkeyval_startstr(void *closure, const void *handler_data,
                                size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  print_data(p, "\"", 1);
  return p;
}

static size_t mapkey_str(void *closure, const void *handler_data,
                         const char *str, size_t len,
                         const upb_bufhandle *handle) {
  CHK(putstr(closure, handler_data, str, len, handle));
  return len;
}

static bool mapkey_endstr(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "\":", 2);
  return true;
}

static bool mapvalue_endstr(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  print_data(p, "\"", 1);
  return true;
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

static size_t mapkey_bytes(void *closure, const void *handler_data,
                           const char *str, size_t len,
                           const upb_bufhandle *handle) {
  upb_json_printer *p = closure;
  CHK(putbytes(closure, handler_data, str, len, handle));
  print_data(p, ":", 1);
  return len;
}

static void set_enum_hd(upb_handlers *h,
                        const upb_fielddef *f,
                        bool preserve_fieldnames,
                        upb_handlerattr *attr) {
  EnumHandlerData *hd = upb_gmalloc(sizeof(EnumHandlerData));
  hd->enumdef = upb_fielddef_enumsubdef(f);
  hd->keyname = newstrpc(h, f, preserve_fieldnames);
  upb_handlers_addcleanup(h, hd, upb_gfree);
  attr->handler_data = hd;
}

/* Set up handlers for a mapentry submessage (i.e., an individual key/value pair
 * in a map).
 *
 * TODO: Handle missing key, missing value, out-of-order key/value, or repeated
 * key or value cases properly. The right way to do this is to allocate a
 * temporary structure at the start of a mapentry submessage, store key and
 * value data in it as key and value handlers are called, and then print the
 * key/value pair once at the end of the submessage. If we don't do this, we
 * should at least detect the case and throw an error. However, so far all of
 * our sources that emit mapentry messages do so canonically (with one key
 * field, and then one value field), so this is not a pressing concern at the
 * moment. */
void printer_sethandlers_mapentry(const void *closure, bool preserve_fieldnames,
                                  upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);

  /* A mapentry message is printed simply as '"key": value'. Rather than
   * special-case key and value for every type below, we just handle both
   * fields explicitly here. */
  const upb_fielddef* key_field = upb_msgdef_itof(md, UPB_MAPENTRY_KEY);
  const upb_fielddef* value_field = upb_msgdef_itof(md, UPB_MAPENTRY_VALUE);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  UPB_UNUSED(closure);

  switch (upb_fielddef_type(key_field)) {
    case UPB_TYPE_INT32:
      upb_handlers_setint32(h, key_field, putmapkey_int32_t, &empty_attr);
      break;
    case UPB_TYPE_INT64:
      upb_handlers_setint64(h, key_field, putmapkey_int64_t, &empty_attr);
      break;
    case UPB_TYPE_UINT32:
      upb_handlers_setuint32(h, key_field, putmapkey_uint32_t, &empty_attr);
      break;
    case UPB_TYPE_UINT64:
      upb_handlers_setuint64(h, key_field, putmapkey_uint64_t, &empty_attr);
      break;
    case UPB_TYPE_BOOL:
      upb_handlers_setbool(h, key_field, putmapkey_bool, &empty_attr);
      break;
    case UPB_TYPE_STRING:
      upb_handlers_setstartstr(h, key_field, mapkeyval_startstr, &empty_attr);
      upb_handlers_setstring(h, key_field, mapkey_str, &empty_attr);
      upb_handlers_setendstr(h, key_field, mapkey_endstr, &empty_attr);
      break;
    case UPB_TYPE_BYTES:
      upb_handlers_setstring(h, key_field, mapkey_bytes, &empty_attr);
      break;
    default:
      UPB_ASSERT(false);
      break;
  }

  switch (upb_fielddef_type(value_field)) {
    case UPB_TYPE_INT32:
      upb_handlers_setint32(h, value_field, putint32_t, &empty_attr);
      break;
    case UPB_TYPE_INT64:
      upb_handlers_setint64(h, value_field, putint64_t, &empty_attr);
      break;
    case UPB_TYPE_UINT32:
      upb_handlers_setuint32(h, value_field, putuint32_t, &empty_attr);
      break;
    case UPB_TYPE_UINT64:
      upb_handlers_setuint64(h, value_field, putuint64_t, &empty_attr);
      break;
    case UPB_TYPE_BOOL:
      upb_handlers_setbool(h, value_field, putbool, &empty_attr);
      break;
    case UPB_TYPE_FLOAT:
      upb_handlers_setfloat(h, value_field, putfloat, &empty_attr);
      break;
    case UPB_TYPE_DOUBLE:
      upb_handlers_setdouble(h, value_field, putdouble, &empty_attr);
      break;
    case UPB_TYPE_STRING:
      upb_handlers_setstartstr(h, value_field, mapkeyval_startstr, &empty_attr);
      upb_handlers_setstring(h, value_field, putstr, &empty_attr);
      upb_handlers_setendstr(h, value_field, mapvalue_endstr, &empty_attr);
      break;
    case UPB_TYPE_BYTES:
      upb_handlers_setstring(h, value_field, putbytes, &empty_attr);
      break;
    case UPB_TYPE_ENUM: {
      upb_handlerattr enum_attr = UPB_HANDLERATTR_INIT;
      set_enum_hd(h, value_field, preserve_fieldnames, &enum_attr);
      upb_handlers_setint32(h, value_field, mapvalue_enum, &enum_attr);
      break;
    }
    case UPB_TYPE_MESSAGE:
      /* No handler necessary -- the submsg handlers will print the message
       * as appropriate. */
      break;
  }
}

static bool putseconds(void *closure, const void *handler_data,
                       int64_t seconds) {
  upb_json_printer *p = closure;
  p->seconds = seconds;
  UPB_UNUSED(handler_data);
  return true;
}

static bool putnanos(void *closure, const void *handler_data,
                     int32_t nanos) {
  upb_json_printer *p = closure;
  p->nanos = nanos;
  UPB_UNUSED(handler_data);
  return true;
}

static void *scalar_startstr_nokey(void *closure, const void *handler_data,
                                   size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  print_data(p, "\"", 1);
  return p;
}

static size_t putstr_nokey(void *closure, const void *handler_data,
                           const char *str, size_t len,
                           const upb_bufhandle *handle) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(handle);
  print_data(p, "\"", 1);
  putstring(p, str, len);
  print_data(p, "\"", 1);
  return len + 2;
}

static void *startseq_nokey(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "[", 1);
  return closure;
}

static void *startseq_fieldmask(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  return closure;
}

static bool endseq_fieldmask(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  p->depth_--;
  return true;
}

static void *repeated_startstr_fieldmask(
    void *closure, const void *handler_data,
    size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(size_hint);
  print_comma(p);
  return p;
}

static size_t repeated_str_fieldmask(
    void *closure, const void *handler_data,
    const char *str, size_t len,
    const upb_bufhandle *handle) {
  const char* limit = str + len;
  bool upper = false;
  size_t result_len = 0;
  for (; str < limit; str++) {
    if (*str == '_') {
      upper = true;
      continue;
    }
    if (upper && *str >= 'a' && *str <= 'z') {
      char upper_char = toupper(*str);
      CHK(putstr(closure, handler_data, &upper_char, 1, handle));
    } else {
      CHK(putstr(closure, handler_data, str, 1, handle));
    }
    upper = false;
    result_len++;
  }
  return result_len;
}

static void *startmap_nokey(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  p->depth_++;
  p->first_elem_[p->depth_] = true;
  print_data(p, "{", 1);
  return closure;
}

static bool putnull(void *closure, const void *handler_data,
                    int32_t null) {
  upb_json_printer *p = closure;
  print_data(p, "null", 4);
  UPB_UNUSED(handler_data);
  UPB_UNUSED(null);
  return true;
}

static bool printer_startdurationmsg(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  if (p->depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  return true;
}

#define UPB_DURATION_MAX_JSON_LEN 23
#define UPB_DURATION_MAX_NANO_LEN 9

static bool printer_enddurationmsg(void *closure, const void *handler_data,
                                   upb_status *s) {
  upb_json_printer *p = closure;
  char buffer[UPB_DURATION_MAX_JSON_LEN];
  size_t base_len;
  size_t curr;
  size_t i;

  memset(buffer, 0, UPB_DURATION_MAX_JSON_LEN);

  if (p->seconds < -315576000000) {
    upb_status_seterrf(s, "error parsing duration: "
                          "minimum acceptable value is "
                          "-315576000000");
    return false;
  }

  if (p->seconds > 315576000000) {
    upb_status_seterrf(s, "error serializing duration: "
                          "maximum acceptable value is "
                          "315576000000");
    return false;
  }

  _upb_snprintf(buffer, sizeof(buffer), "%ld", (long)p->seconds);
  base_len = strlen(buffer);

  if (p->nanos != 0) {
    char nanos_buffer[UPB_DURATION_MAX_NANO_LEN + 3];
    _upb_snprintf(nanos_buffer, sizeof(nanos_buffer), "%.9f",
                  p->nanos / 1000000000.0);
    /* Remove trailing 0. */
    for (i = UPB_DURATION_MAX_NANO_LEN + 2;
         nanos_buffer[i] == '0'; i--) {
      nanos_buffer[i] = 0;
    }
    strcpy(buffer + base_len, nanos_buffer + 1);
  }

  curr = strlen(buffer);
  strcpy(buffer + curr, "s");

  p->seconds = 0;
  p->nanos = 0;

  print_data(p, "\"", 1);
  print_data(p, buffer, strlen(buffer));
  print_data(p, "\"", 1);

  if (p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }

  UPB_UNUSED(handler_data);
  return true;
}

static bool printer_starttimestampmsg(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  if (p->depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  return true;
}

#define UPB_TIMESTAMP_MAX_JSON_LEN 31
#define UPB_TIMESTAMP_BEFORE_NANO_LEN 19
#define UPB_TIMESTAMP_MAX_NANO_LEN 9

static bool printer_endtimestampmsg(void *closure, const void *handler_data,
                                    upb_status *s) {
  upb_json_printer *p = closure;
  char buffer[UPB_TIMESTAMP_MAX_JSON_LEN];
  time_t time = p->seconds;
  size_t curr;
  size_t i;
  size_t year_length =
      strftime(buffer, UPB_TIMESTAMP_MAX_JSON_LEN, "%Y", gmtime(&time));

  if (p->seconds < -62135596800) {
    upb_status_seterrf(s, "error parsing timestamp: "
                          "minimum acceptable value is "
                          "0001-01-01T00:00:00Z");
    return false;
  }

  if (p->seconds > 253402300799) {
    upb_status_seterrf(s, "error parsing timestamp: "
                          "maximum acceptable value is "
                          "9999-12-31T23:59:59Z");
    return false;
  }

  /* strftime doesn't guarantee 4 digits for year. Prepend 0 by ourselves. */
  for (i = 0; i < 4 - year_length; i++) {
    buffer[i] = '0';
  }

  strftime(buffer + (4 - year_length), UPB_TIMESTAMP_MAX_JSON_LEN,
           "%Y-%m-%dT%H:%M:%S", gmtime(&time));
  if (p->nanos != 0) {
    char nanos_buffer[UPB_TIMESTAMP_MAX_NANO_LEN + 3];
    _upb_snprintf(nanos_buffer, sizeof(nanos_buffer), "%.9f",
                  p->nanos / 1000000000.0);
    /* Remove trailing 0. */
    for (i = UPB_TIMESTAMP_MAX_NANO_LEN + 2;
         nanos_buffer[i] == '0'; i--) {
      nanos_buffer[i] = 0;
    }
    strcpy(buffer + UPB_TIMESTAMP_BEFORE_NANO_LEN, nanos_buffer + 1);
  }

  curr = strlen(buffer);
  strcpy(buffer + curr, "Z");

  p->seconds = 0;
  p->nanos = 0;

  print_data(p, "\"", 1);
  print_data(p, buffer, strlen(buffer));
  print_data(p, "\"", 1);

  if (p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }

  UPB_UNUSED(handler_data);
  UPB_UNUSED(s);
  return true;
}

static bool printer_startmsg_noframe(void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  if (p->depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  return true;
}

static bool printer_endmsg_noframe(
    void *closure, const void *handler_data, upb_status *s) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(s);
  if (p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }
  return true;
}

static bool printer_startmsg_fieldmask(
    void *closure, const void *handler_data) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  if (p->depth_ == 0) {
    upb_bytessink_start(p->output_, 0, &p->subc_);
  }
  print_data(p, "\"", 1);
  return true;
}

static bool printer_endmsg_fieldmask(
    void *closure, const void *handler_data, upb_status *s) {
  upb_json_printer *p = closure;
  UPB_UNUSED(handler_data);
  UPB_UNUSED(s);
  print_data(p, "\"", 1);
  if (p->depth_ == 0) {
    upb_bytessink_end(p->output_);
  }
  return true;
}

static void *scalar_startstr_onlykey(
    void *closure, const void *handler_data, size_t size_hint) {
  upb_json_printer *p = closure;
  UPB_UNUSED(size_hint);
  CHK(putkey(closure, handler_data));
  return p;
}

/* Set up handlers for an Any submessage. */
void printer_sethandlers_any(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);

  const upb_fielddef* type_field = upb_msgdef_itof(md, UPB_ANY_TYPE);
  const upb_fielddef* value_field = upb_msgdef_itof(md, UPB_ANY_VALUE);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  /* type_url's json name is "@type" */
  upb_handlerattr type_name_attr = UPB_HANDLERATTR_INIT;
  upb_handlerattr value_name_attr = UPB_HANDLERATTR_INIT;
  strpc *type_url_json_name = newstrpc_str(h, "@type");
  strpc *value_json_name = newstrpc_str(h, "value");

  type_name_attr.handler_data = type_url_json_name;
  value_name_attr.handler_data = value_json_name;

  /* Set up handlers. */
  upb_handlers_setstartmsg(h, printer_startmsg, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg, &empty_attr);

  upb_handlers_setstartstr(h, type_field, scalar_startstr, &type_name_attr);
  upb_handlers_setstring(h, type_field, scalar_str, &empty_attr);
  upb_handlers_setendstr(h, type_field, scalar_endstr, &empty_attr);

  /* This is not the full and correct JSON encoding for the Any value field. It
   * requires further processing by the wrapper code based on the type URL.
   */
  upb_handlers_setstartstr(h, value_field, scalar_startstr_onlykey,
                           &value_name_attr);

  UPB_UNUSED(closure);
}

/* Set up handlers for a fieldmask submessage. */
void printer_sethandlers_fieldmask(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  const upb_fielddef* f = upb_msgdef_itof(md, 1);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  upb_handlers_setstartseq(h, f, startseq_fieldmask, &empty_attr);
  upb_handlers_setendseq(h, f, endseq_fieldmask, &empty_attr);

  upb_handlers_setstartmsg(h, printer_startmsg_fieldmask, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg_fieldmask, &empty_attr);

  upb_handlers_setstartstr(h, f, repeated_startstr_fieldmask, &empty_attr);
  upb_handlers_setstring(h, f, repeated_str_fieldmask, &empty_attr);

  UPB_UNUSED(closure);
}

/* Set up handlers for a duration submessage. */
void printer_sethandlers_duration(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);

  const upb_fielddef* seconds_field =
      upb_msgdef_itof(md, UPB_DURATION_SECONDS);
  const upb_fielddef* nanos_field =
      upb_msgdef_itof(md, UPB_DURATION_NANOS);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  upb_handlers_setstartmsg(h, printer_startdurationmsg, &empty_attr);
  upb_handlers_setint64(h, seconds_field, putseconds, &empty_attr);
  upb_handlers_setint32(h, nanos_field, putnanos, &empty_attr);
  upb_handlers_setendmsg(h, printer_enddurationmsg, &empty_attr);

  UPB_UNUSED(closure);
}

/* Set up handlers for a timestamp submessage. Instead of printing fields
 * separately, the json representation of timestamp follows RFC 3339 */
void printer_sethandlers_timestamp(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);

  const upb_fielddef* seconds_field =
      upb_msgdef_itof(md, UPB_TIMESTAMP_SECONDS);
  const upb_fielddef* nanos_field =
      upb_msgdef_itof(md, UPB_TIMESTAMP_NANOS);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  upb_handlers_setstartmsg(h, printer_starttimestampmsg, &empty_attr);
  upb_handlers_setint64(h, seconds_field, putseconds, &empty_attr);
  upb_handlers_setint32(h, nanos_field, putnanos, &empty_attr);
  upb_handlers_setendmsg(h, printer_endtimestampmsg, &empty_attr);

  UPB_UNUSED(closure);
}

void printer_sethandlers_value(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  upb_msg_field_iter i;

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  upb_handlers_setstartmsg(h, printer_startmsg_noframe, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg_noframe, &empty_attr);

  upb_msg_field_begin(&i, md);
  for(; !upb_msg_field_done(&i); upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);

    switch (upb_fielddef_type(f)) {
      case UPB_TYPE_ENUM:
        upb_handlers_setint32(h, f, putnull, &empty_attr);
        break;
      case UPB_TYPE_DOUBLE:
        upb_handlers_setdouble(h, f, putdouble, &empty_attr);
        break;
      case UPB_TYPE_STRING:
        upb_handlers_setstartstr(h, f, scalar_startstr_nokey, &empty_attr);
        upb_handlers_setstring(h, f, scalar_str, &empty_attr);
        upb_handlers_setendstr(h, f, scalar_endstr, &empty_attr);
        break;
      case UPB_TYPE_BOOL:
        upb_handlers_setbool(h, f, putbool, &empty_attr);
        break;
      case UPB_TYPE_MESSAGE:
        break;
      default:
        UPB_ASSERT(false);
        break;
    }
  }

  UPB_UNUSED(closure);
}

#define WRAPPER_SETHANDLERS(wrapper, type, putmethod)                      \
void printer_sethandlers_##wrapper(const void *closure, upb_handlers *h) { \
  const upb_msgdef *md = upb_handlers_msgdef(h);                           \
  const upb_fielddef* f = upb_msgdef_itof(md, 1);                          \
  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;                \
  upb_handlers_setstartmsg(h, printer_startmsg_noframe, &empty_attr);      \
  upb_handlers_setendmsg(h, printer_endmsg_noframe, &empty_attr);          \
  upb_handlers_set##type(h, f, putmethod, &empty_attr);                    \
  UPB_UNUSED(closure);                                                     \
}

WRAPPER_SETHANDLERS(doublevalue, double, putdouble)
WRAPPER_SETHANDLERS(floatvalue,  float,  putfloat)
WRAPPER_SETHANDLERS(int64value,  int64,  putint64_t)
WRAPPER_SETHANDLERS(uint64value, uint64, putuint64_t)
WRAPPER_SETHANDLERS(int32value,  int32,  putint32_t)
WRAPPER_SETHANDLERS(uint32value, uint32, putuint32_t)
WRAPPER_SETHANDLERS(boolvalue,   bool,   putbool)
WRAPPER_SETHANDLERS(stringvalue, string, putstr_nokey)
WRAPPER_SETHANDLERS(bytesvalue,  string, putbytes)

#undef WRAPPER_SETHANDLERS

void printer_sethandlers_listvalue(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  const upb_fielddef* f = upb_msgdef_itof(md, 1);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  upb_handlers_setstartseq(h, f, startseq_nokey, &empty_attr);
  upb_handlers_setendseq(h, f, endseq, &empty_attr);

  upb_handlers_setstartmsg(h, printer_startmsg_noframe, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg_noframe, &empty_attr);

  upb_handlers_setstartsubmsg(h, f, repeated_startsubmsg, &empty_attr);

  UPB_UNUSED(closure);
}

void printer_sethandlers_structvalue(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  const upb_fielddef* f = upb_msgdef_itof(md, 1);

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;

  upb_handlers_setstartseq(h, f, startmap_nokey, &empty_attr);
  upb_handlers_setendseq(h, f, endmap, &empty_attr);

  upb_handlers_setstartmsg(h, printer_startmsg_noframe, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg_noframe, &empty_attr);

  upb_handlers_setstartsubmsg(h, f, repeated_startsubmsg, &empty_attr);

  UPB_UNUSED(closure);
}

void printer_sethandlers(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  bool is_mapentry = upb_msgdef_mapentry(md);
  upb_handlerattr empty_attr = UPB_HANDLERATTR_INIT;
  upb_msg_field_iter i;
  const upb_json_printercache *cache = closure;
  const bool preserve_fieldnames = cache->preserve_fieldnames;

  if (is_mapentry) {
    /* mapentry messages are sufficiently different that we handle them
     * separately. */
    printer_sethandlers_mapentry(closure, preserve_fieldnames, h);
    return;
  }

  switch (upb_msgdef_wellknowntype(md)) {
    case UPB_WELLKNOWN_UNSPECIFIED:
      break;
    case UPB_WELLKNOWN_ANY:
      printer_sethandlers_any(closure, h);
      return;
    case UPB_WELLKNOWN_FIELDMASK:
      printer_sethandlers_fieldmask(closure, h);
      return;
    case UPB_WELLKNOWN_DURATION:
      printer_sethandlers_duration(closure, h);
      return;
    case UPB_WELLKNOWN_TIMESTAMP:
      printer_sethandlers_timestamp(closure, h);
      return;
    case UPB_WELLKNOWN_VALUE:
      printer_sethandlers_value(closure, h);
      return;
    case UPB_WELLKNOWN_LISTVALUE:
      printer_sethandlers_listvalue(closure, h);
      return;
    case UPB_WELLKNOWN_STRUCT:
      printer_sethandlers_structvalue(closure, h);
      return;
#define WRAPPER(wellknowntype, name)        \
  case wellknowntype:                       \
    printer_sethandlers_##name(closure, h); \
    return;                                 \

    WRAPPER(UPB_WELLKNOWN_DOUBLEVALUE, doublevalue);
    WRAPPER(UPB_WELLKNOWN_FLOATVALUE, floatvalue);
    WRAPPER(UPB_WELLKNOWN_INT64VALUE, int64value);
    WRAPPER(UPB_WELLKNOWN_UINT64VALUE, uint64value);
    WRAPPER(UPB_WELLKNOWN_INT32VALUE, int32value);
    WRAPPER(UPB_WELLKNOWN_UINT32VALUE, uint32value);
    WRAPPER(UPB_WELLKNOWN_BOOLVALUE, boolvalue);
    WRAPPER(UPB_WELLKNOWN_STRINGVALUE, stringvalue);
    WRAPPER(UPB_WELLKNOWN_BYTESVALUE, bytesvalue);

#undef WRAPPER
  }

  upb_handlers_setstartmsg(h, printer_startmsg, &empty_attr);
  upb_handlers_setendmsg(h, printer_endmsg, &empty_attr);

#define TYPE(type, name, ctype)                                               \
  case type:                                                                  \
    if (upb_fielddef_isseq(f)) {                                              \
      upb_handlers_set##name(h, f, repeated_##ctype, &empty_attr);            \
    } else {                                                                  \
      upb_handlers_set##name(h, f, scalar_##ctype, &name_attr);               \
    }                                                                         \
    break;

  upb_msg_field_begin(&i, md);
  for(; !upb_msg_field_done(&i); upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);

    upb_handlerattr name_attr = UPB_HANDLERATTR_INIT;
    name_attr.handler_data = newstrpc(h, f, preserve_fieldnames);

    if (upb_fielddef_ismap(f)) {
      upb_handlers_setstartseq(h, f, startmap, &name_attr);
      upb_handlers_setendseq(h, f, endmap, &name_attr);
    } else if (upb_fielddef_isseq(f)) {
      upb_handlers_setstartseq(h, f, startseq, &name_attr);
      upb_handlers_setendseq(h, f, endseq, &empty_attr);
    }

    switch (upb_fielddef_type(f)) {
      TYPE(UPB_TYPE_FLOAT,  float,  float);
      TYPE(UPB_TYPE_DOUBLE, double, double);
      TYPE(UPB_TYPE_BOOL,   bool,   bool);
      TYPE(UPB_TYPE_INT32,  int32,  int32_t);
      TYPE(UPB_TYPE_UINT32, uint32, uint32_t);
      TYPE(UPB_TYPE_INT64,  int64,  int64_t);
      TYPE(UPB_TYPE_UINT64, uint64, uint64_t);
      case UPB_TYPE_ENUM: {
        /* For now, we always emit symbolic names for enums. We may want an
         * option later to control this behavior, but we will wait for a real
         * need first. */
        upb_handlerattr enum_attr = UPB_HANDLERATTR_INIT;
        set_enum_hd(h, f, preserve_fieldnames, &enum_attr);

        if (upb_fielddef_isseq(f)) {
          upb_handlers_setint32(h, f, repeated_enum, &enum_attr);
        } else {
          upb_handlers_setint32(h, f, scalar_enum, &enum_attr);
        }

        break;
      }
      case UPB_TYPE_STRING:
        if (upb_fielddef_isseq(f)) {
          upb_handlers_setstartstr(h, f, repeated_startstr, &empty_attr);
          upb_handlers_setstring(h, f, repeated_str, &empty_attr);
          upb_handlers_setendstr(h, f, repeated_endstr, &empty_attr);
        } else {
          upb_handlers_setstartstr(h, f, scalar_startstr, &name_attr);
          upb_handlers_setstring(h, f, scalar_str, &empty_attr);
          upb_handlers_setendstr(h, f, scalar_endstr, &empty_attr);
        }
        break;
      case UPB_TYPE_BYTES:
        /* XXX: this doesn't support strings that span buffers yet. The base64
         * encoder will need to be made resumable for this to work properly. */
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
  }

#undef TYPE
}

static void json_printer_reset(upb_json_printer *p) {
  p->depth_ = 0;
}


/* Public API *****************************************************************/

upb_json_printer *upb_json_printer_create(upb_arena *a, const upb_handlers *h,
                                          upb_bytessink output) {
#ifndef NDEBUG
  size_t size_before = upb_arena_bytesallocated(a);
#endif

  upb_json_printer *p = upb_arena_malloc(a, sizeof(upb_json_printer));
  if (!p) return NULL;

  p->output_ = output;
  json_printer_reset(p);
  upb_sink_reset(&p->input_, h, p);
  p->seconds = 0;
  p->nanos = 0;

  /* If this fails, increase the value in printer.h. */
  UPB_ASSERT_DEBUGVAR(upb_arena_bytesallocated(a) - size_before <=
                      UPB_JSON_PRINTER_SIZE);
  return p;
}

upb_sink upb_json_printer_input(upb_json_printer *p) {
  return p->input_;
}

upb_handlercache *upb_json_printer_newcache(bool preserve_proto_fieldnames) {
  upb_json_printercache *cache = upb_gmalloc(sizeof(*cache));
  upb_handlercache *ret = upb_handlercache_new(printer_sethandlers, cache);

  cache->preserve_fieldnames = preserve_proto_fieldnames;
  upb_handlercache_addcleanup(ret, cache, upb_gfree);

  return ret;
}
