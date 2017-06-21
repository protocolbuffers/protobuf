/*
** This currently uses snprintf() to format primitives, and could be optimized
** further.
*/

#include "upb/json/printer.h"

#include <string.h>
#include <stdint.h>

struct upb_json_printer {
  upb_sink input_;
  /* BytesSink closure. */
  void *subc_;
  upb_bytessink *output_;

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
  if (val == (1.0 / 0.0)) {
    CHKLENGTH(length >= strlen(inf));
    strcpy(buf, inf);
    return strlen(inf);
  } else if (val == (-1.0 / 0.0)) {
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

static size_t fmt_int64(long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%ld", val);
  CHKLENGTH(n > 0 && n < length);
  return n;
}

static size_t fmt_uint64(unsigned long long val, char* buf, size_t length) {
  size_t n = _upb_snprintf(buf, length, "%llu", val);
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
    print_data(p, "\"", 1);                                                  \
    CHK(put##type(closure, handler_data, val));                              \
    print_data(p, "\":", 2);                                                 \
    return true;                                                             \
  }

TYPE_HANDLERS(double,   fmt_double)
TYPE_HANDLERS(float,    fmt_float)
TYPE_HANDLERS(bool,     fmt_bool)
TYPE_HANDLERS(int32_t,  fmt_int64)
TYPE_HANDLERS(uint32_t, fmt_int64)
TYPE_HANDLERS(int64_t,  fmt_int64)
TYPE_HANDLERS(uint64_t, fmt_uint64)

/* double and float are not allowed to be map keys. */
TYPE_HANDLERS_MAPKEY(bool,     fmt_bool)
TYPE_HANDLERS_MAPKEY(int32_t,  fmt_int64)
TYPE_HANDLERS_MAPKEY(uint32_t, fmt_int64)
TYPE_HANDLERS_MAPKEY(int64_t,  fmt_int64)
TYPE_HANDLERS_MAPKEY(uint64_t, fmt_uint64)

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

  while (remaining > 2) {
    /* TODO(haberman): handle encoded lengths > sizeof(data) */
    UPB_ASSERT((limit - to) >= 4);

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
  print_data(p, "\"", 1);
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
  hd->enumdef = (const upb_enumdef *)upb_fielddef_subdef(f);
  hd->keyname = newstrpc(h, f, preserve_fieldnames);
  upb_handlers_addcleanup(h, hd, upb_gfree);
  upb_handlerattr_sethandlerdata(attr, hd);
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

  upb_handlerattr empty_attr = UPB_HANDLERATTR_INITIALIZER;

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
      upb_handlerattr enum_attr = UPB_HANDLERATTR_INITIALIZER;
      set_enum_hd(h, value_field, preserve_fieldnames, &enum_attr);
      upb_handlers_setint32(h, value_field, mapvalue_enum, &enum_attr);
      upb_handlerattr_uninit(&enum_attr);
      break;
    }
    case UPB_TYPE_MESSAGE:
      /* No handler necessary -- the submsg handlers will print the message
       * as appropriate. */
      break;
  }

  upb_handlerattr_uninit(&empty_attr);
}

void printer_sethandlers(const void *closure, upb_handlers *h) {
  const upb_msgdef *md = upb_handlers_msgdef(h);
  bool is_mapentry = upb_msgdef_mapentry(md);
  upb_handlerattr empty_attr = UPB_HANDLERATTR_INITIALIZER;
  upb_msg_field_iter i;
  const bool *preserve_fieldnames_ptr = closure;
  const bool preserve_fieldnames = *preserve_fieldnames_ptr;

  if (is_mapentry) {
    /* mapentry messages are sufficiently different that we handle them
     * separately. */
    printer_sethandlers_mapentry(closure, preserve_fieldnames, h);
    return;
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

    upb_handlerattr name_attr = UPB_HANDLERATTR_INITIALIZER;
    upb_handlerattr_sethandlerdata(&name_attr,
                                   newstrpc(h, f, preserve_fieldnames));

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
        upb_handlerattr enum_attr = UPB_HANDLERATTR_INITIALIZER;
        set_enum_hd(h, f, preserve_fieldnames, &enum_attr);

        if (upb_fielddef_isseq(f)) {
          upb_handlers_setint32(h, f, repeated_enum, &enum_attr);
        } else {
          upb_handlers_setint32(h, f, scalar_enum, &enum_attr);
        }

        upb_handlerattr_uninit(&enum_attr);
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

    upb_handlerattr_uninit(&name_attr);
  }

  upb_handlerattr_uninit(&empty_attr);
#undef TYPE
}

static void json_printer_reset(upb_json_printer *p) {
  p->depth_ = 0;
}


/* Public API *****************************************************************/

upb_json_printer *upb_json_printer_create(upb_env *e, const upb_handlers *h,
                                          upb_bytessink *output) {
#ifndef NDEBUG
  size_t size_before = upb_env_bytesallocated(e);
#endif

  upb_json_printer *p = upb_env_malloc(e, sizeof(upb_json_printer));
  if (!p) return NULL;

  p->output_ = output;
  json_printer_reset(p);
  upb_sink_reset(&p->input_, h, p);

  /* If this fails, increase the value in printer.h. */
  UPB_ASSERT_DEBUGVAR(upb_env_bytesallocated(e) - size_before <=
                      UPB_JSON_PRINTER_SIZE);
  return p;
}

upb_sink *upb_json_printer_input(upb_json_printer *p) {
  return &p->input_;
}

const upb_handlers *upb_json_printer_newhandlers(const upb_msgdef *md,
                                                 bool preserve_fieldnames,
                                                 const void *owner) {
  return upb_handlers_newfrozen(
      md, owner, printer_sethandlers, &preserve_fieldnames);
}
