
#include "upb/json_encode.h"

#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "upb/decode.h"
#include "upb/reflection.h"

/* Must be last. */
#include "upb/port_def.inc"

typedef struct {
  char *buf, *ptr, *end;
  size_t overflow;
  int indent_depth;
  int options;
  const upb_symtab *ext_pool;
  jmp_buf err;
  upb_status *status;
  upb_arena *arena;
} jsonenc;

static void jsonenc_msg(jsonenc *e, const upb_msg *msg, const upb_msgdef *m);
static void jsonenc_scalar(jsonenc *e, upb_msgval val, const upb_fielddef *f);
static void jsonenc_msgfield(jsonenc *e, const upb_msg *msg,
                             const upb_msgdef *m);
static void jsonenc_msgfields(jsonenc *e, const upb_msg *msg,
                              const upb_msgdef *m);
static void jsonenc_value(jsonenc *e, const upb_msg *msg, const upb_msgdef *m);

UPB_NORETURN static void jsonenc_err(jsonenc *e, const char *msg) {
  upb_status_seterrmsg(e->status, msg);
  longjmp(e->err, 1);
}

UPB_NORETURN static void jsonenc_errf(jsonenc *e, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  upb_status_vseterrf(e->status, fmt, argp);
  va_end(argp);
  longjmp(e->err, 1);
}

static upb_arena *jsonenc_arena(jsonenc *e) {
  /* Create lazily, since it's only needed for Any */
  if (!e->arena) {
    e->arena = upb_arena_new();
  }
  return e->arena;
}

static void jsonenc_putbytes(jsonenc *e, const void *data, size_t len) {
  size_t have = e->end - e->ptr;
  if (UPB_LIKELY(have >= len)) {
    memcpy(e->ptr, data, len);
    e->ptr += len;
  } else {
    if (have) memcpy(e->ptr, data, have);
    e->ptr += have;
    e->overflow += (len - have);
  }
}

static void jsonenc_putstr(jsonenc *e, const char *str) {
  jsonenc_putbytes(e, str, strlen(str));
}

static void jsonenc_printf(jsonenc *e, const char *fmt, ...) {
  size_t n;
  size_t have = e->end - e->ptr;
  va_list args;

  va_start(args, fmt);
  n = vsnprintf(e->ptr, have, fmt, args);
  va_end(args);

  if (UPB_LIKELY(have > n)) {
    e->ptr += n;
  } else {
    e->ptr += have;
    e->overflow += (n - have);
  }
}

static void jsonenc_nanos(jsonenc *e, int32_t nanos) {
  int digits = 9;

  if (nanos == 0) return;
  if (nanos < 0 || nanos >= 1000000000) {
    jsonenc_err(e, "error formatting timestamp as JSON: invalid nanos");
  }

  while (nanos % 1000 == 0) {
    nanos /= 1000;
    digits -= 3;
  }

  jsonenc_printf(e, ".%0.*" PRId32, digits, nanos);
}

static void jsonenc_timestamp(jsonenc *e, const upb_msg *msg,
                              const upb_msgdef *m) {
  const upb_fielddef *seconds_f = upb_msgdef_itof(m, 1);
  const upb_fielddef *nanos_f = upb_msgdef_itof(m, 2);
  int64_t seconds = upb_msg_get(msg, seconds_f).int64_val;
  int32_t nanos = upb_msg_get(msg, nanos_f).int32_val;
  int L, N, I, J, K, hour, min, sec;

  if (seconds < -62135596800) {
    jsonenc_err(e,
                "error formatting timestamp as JSON: minimum acceptable value "
                "is 0001-01-01T00:00:00Z");
  } else if (seconds > 253402300799) {
    jsonenc_err(e,
                "error formatting timestamp as JSON: maximum acceptable value "
                "is 9999-12-31T23:59:59Z");
  }

  /* Julian Day -> Y/M/D, Algorithm from:
   * Fliegel, H. F., and Van Flandern, T. C., "A Machine Algorithm for
   *   Processing Calendar Dates," Communications of the Association of
   *   Computing Machines, vol. 11 (1968), p. 657.  */
  L = (int)(seconds / 86400) + 68569 + 2440588;
  N = 4 * L / 146097;
  L = L - (146097 * N + 3) / 4;
  I = 4000 * (L + 1) / 1461001;
  L = L - 1461 * I / 4 + 31;
  J = 80 * L / 2447;
  K = L - 2447 * J / 80;
  L = J / 11;
  J = J + 2 - 12 * L;
  I = 100 * (N - 49) + I + L;

  sec = seconds % 60;
  min = (seconds / 60) % 60;
  hour = (seconds / 3600) % 24;

  jsonenc_printf(e, "\"%04d-%02d-%02dT%02d:%02d:%02d", I, J, K, hour, min, sec);
  jsonenc_nanos(e, nanos);
  jsonenc_putstr(e, "Z\"");
}

static void jsonenc_duration(jsonenc *e, const upb_msg *msg, const upb_msgdef *m) {
  const upb_fielddef *seconds_f = upb_msgdef_itof(m, 1);
  const upb_fielddef *nanos_f = upb_msgdef_itof(m, 2);
  int64_t seconds = upb_msg_get(msg, seconds_f).int64_val;
  int32_t nanos = upb_msg_get(msg, nanos_f).int32_val;

  if (seconds > 315576000000 || seconds < -315576000000 ||
      (seconds < 0) != (nanos < 0)) {
    jsonenc_err(e, "bad duration");
  }

  if (nanos < 0) {
    nanos = -nanos;
  }

  jsonenc_printf(e, "\"%" PRId64, seconds);
  jsonenc_nanos(e, nanos);
  jsonenc_putstr(e, "s\"");
}

static void jsonenc_enum(int32_t val, const upb_fielddef *f, jsonenc *e) {
  const upb_enumdef *e_def = upb_fielddef_enumsubdef(f);

  if (strcmp(upb_enumdef_fullname(e_def), "google.protobuf.NullValue") == 0) {
    jsonenc_putstr(e, "null");
  } else {
    const char *name = upb_enumdef_iton(e_def, val);

    if (name) {
      jsonenc_printf(e, "\"%s\"", name);
    } else {
      jsonenc_printf(e, "%" PRId32, val);
    }
  }
}

static void jsonenc_bytes(jsonenc *e, upb_strview str) {
  /* This is the regular base64, not the "web-safe" version. */
  static const char base64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const unsigned char *ptr = (unsigned char*)str.data;
  const unsigned char *end = ptr + str.size;
  char buf[4];

  jsonenc_putstr(e, "\"");

  while (end - ptr >= 3) {
    buf[0] = base64[ptr[0] >> 2];
    buf[1] = base64[((ptr[0] & 0x3) << 4) | (ptr[1] >> 4)];
    buf[2] = base64[((ptr[1] & 0xf) << 2) | (ptr[2] >> 6)];
    buf[3] = base64[ptr[2] & 0x3f];
    jsonenc_putbytes(e, buf, 4);
    ptr += 3;
  }

  switch (end - ptr) {
    case 2:
      buf[0] = base64[ptr[0] >> 2];
      buf[1] = base64[((ptr[0] & 0x3) << 4) | (ptr[1] >> 4)];
      buf[2] = base64[(ptr[1] & 0xf) << 2];
      buf[3] = '=';
      jsonenc_putbytes(e, buf, 4);
      break;
    case 1:
      buf[0] = base64[ptr[0] >> 2];
      buf[1] = base64[((ptr[0] & 0x3) << 4)];
      buf[2] = '=';
      buf[3] = '=';
      jsonenc_putbytes(e, buf, 4);
      break;
  }

  jsonenc_putstr(e, "\"");
}

static void jsonenc_stringbody(jsonenc *e, upb_strview str) {
  const char *ptr = str.data;
  const char *end = ptr + str.size;

  while (ptr < end) {
    switch (*ptr) {
      case '\n':
        jsonenc_putstr(e, "\\n");
        break;
      case '\r':
        jsonenc_putstr(e, "\\r");
        break;
      case '\t':
        jsonenc_putstr(e, "\\t");
        break;
      case '\"':
        jsonenc_putstr(e, "\\\"");
        break;
      case '\f':
        jsonenc_putstr(e, "\\f");
        break;
      case '\b':
        jsonenc_putstr(e, "\\b");
        break;
      case '\\':
        jsonenc_putstr(e, "\\\\");
        break;
      default:
        if ((uint8_t)*ptr < 0x20) {
          jsonenc_printf(e, "\\u%04x", (int)(uint8_t)*ptr);
        } else {
          /* This could be a non-ASCII byte.  We rely on the string being valid
           * UTF-8. */
          jsonenc_putbytes(e, ptr, 1);
        }
        break;
    }
    ptr++;
  }
}

static void jsonenc_string(jsonenc *e, upb_strview str) {
  jsonenc_putstr(e, "\"");
  jsonenc_stringbody(e, str);
  jsonenc_putstr(e, "\"");
}

static void jsonenc_double(jsonenc *e, const char *fmt, double val) {
  if (val == INFINITY) {
    jsonenc_putstr(e, "\"Infinity\"");
  } else if (val == -INFINITY) {
    jsonenc_putstr(e, "\"-Infinity\"");
  } else if (val != val) {
    jsonenc_putstr(e, "\"NaN\"");
  } else {
    jsonenc_printf(e, fmt, val);
  }
}

static void jsonenc_wrapper(jsonenc *e, const upb_msg *msg,
                            const upb_msgdef *m) {
  const upb_fielddef *val_f = upb_msgdef_itof(m, 1);
  upb_msgval val = upb_msg_get(msg, val_f);
  jsonenc_scalar(e, val, val_f);
}

static const upb_msgdef *jsonenc_getanymsg(jsonenc *e, upb_strview type_url) {
  /* Find last '/', if any. */
  const char *end = type_url.data + type_url.size;
  const char *ptr = end;
  const upb_msgdef *ret;

  if (!e->ext_pool) {
    jsonenc_err(e, "Tried to encode Any, but no symtab was provided");
  }

  if (type_url.size == 0) goto badurl;

  while (true) {
    if (--ptr == type_url.data) {
      /* Type URL must contain at least one '/', with host before. */
      goto badurl;
    }
    if (*ptr == '/') {
      ptr++;
      break;
    }
  }

  ret = upb_symtab_lookupmsg2(e->ext_pool, ptr, end - ptr);

  if (!ret) {
    jsonenc_errf(e, "Couldn't find Any type: %.*s", (int)(end - ptr), ptr);
  }

  return ret;

badurl:
  jsonenc_errf(
      e, "Bad type URL: " UPB_STRVIEW_FORMAT, UPB_STRVIEW_ARGS(type_url));
}

static void jsonenc_any(jsonenc *e, const upb_msg *msg, const upb_msgdef *m) {
  const upb_fielddef *type_url_f = upb_msgdef_itof(m, 1);
  const upb_fielddef *value_f = upb_msgdef_itof(m, 2);
  upb_strview type_url = upb_msg_get(msg, type_url_f).str_val;
  upb_strview value = upb_msg_get(msg, value_f).str_val;
  const upb_msgdef *any_m = jsonenc_getanymsg(e, type_url);
  const upb_msglayout *any_layout = upb_msgdef_layout(any_m);
  upb_arena *arena = jsonenc_arena(e);
  upb_msg *any = upb_msg_new(any_m, arena);

  if (!upb_decode(value.data, value.size, any, any_layout, arena)) {
    jsonenc_err(e, "Error decoding message in Any");
  }

  jsonenc_putstr(e, "{\"@type\":");
  jsonenc_string(e, type_url);
  jsonenc_putstr(e, ",");

  if (upb_msgdef_wellknowntype(any_m) == UPB_WELLKNOWN_UNSPECIFIED) {
    /* Regular messages: {"@type": "...","foo": 1, "bar": 2} */
    jsonenc_msgfields(e, any, any_m);
  } else {
    /* Well-known type: {"@type": "...","value": <well-known encoding>} */
    jsonenc_putstr(e, "\"value\":");
    jsonenc_msgfield(e, any, any_m);
  }

  jsonenc_putstr(e, "}");
}

static void jsonenc_putsep(jsonenc *e, const char *str, bool *first) {
  if (*first) {
    *first = false;
  } else {
    jsonenc_putstr(e, str);
  }
}

static void jsonenc_fieldpath(jsonenc *e, upb_strview path) {
  const char *ptr = path.data;
  const char *end = ptr + path.size;

  while (ptr < end) {
    char ch = *ptr;

    if (ch >= 'A' && ch <= 'Z') {
      jsonenc_err(e, "Field mask element may not have upper-case letter.");
    } else if (ch == '_') {
      if (ptr == end - 1 || *(ptr + 1) < 'a' || *(ptr + 1) > 'z') {
        jsonenc_err(e, "Underscore must be followed by a lowercase letter.");
      }
      ch = *++ptr - 32;
    }

    jsonenc_putbytes(e, &ch, 1);
    ptr++;
  }
}

static void jsonenc_fieldmask(jsonenc *e, const upb_msg *msg,
                              const upb_msgdef *m) {
  const upb_fielddef *paths_f = upb_msgdef_itof(m, 1);
  const upb_array *paths = upb_msg_get(msg, paths_f).array_val;
  bool first = true;
  size_t i, n = 0;

  if (paths) n = upb_array_size(paths);

  jsonenc_putstr(e, "\"");

  for (i = 0; i < n; i++) {
    jsonenc_putsep(e, ",", &first);
    jsonenc_fieldpath(e, upb_array_get(paths, i).str_val);
  }

  jsonenc_putstr(e, "\"");
}

static void jsonenc_struct(jsonenc *e, const upb_msg *msg,
                           const upb_msgdef *m) {
  const upb_fielddef *fields_f = upb_msgdef_itof(m, 1);
  const upb_map *fields = upb_msg_get(msg, fields_f).map_val;
  const upb_msgdef *entry_m = upb_fielddef_msgsubdef(fields_f);
  const upb_fielddef *value_f = upb_msgdef_itof(entry_m, 2);
  size_t iter = UPB_MAP_BEGIN;
  bool first = true;

  jsonenc_putstr(e, "{");

  if (fields) {
    while (upb_mapiter_next(fields, &iter)) {
      upb_msgval key = upb_mapiter_key(fields, iter);
      upb_msgval val = upb_mapiter_value(fields, iter);

      jsonenc_putsep(e, ",", &first);
      jsonenc_string(e, key.str_val);
      jsonenc_putstr(e, ":");
      jsonenc_value(e, val.msg_val, upb_fielddef_msgsubdef(value_f));
    }
  }

  jsonenc_putstr(e, "}");
}

static void jsonenc_listvalue(jsonenc *e, const upb_msg *msg,
                              const upb_msgdef *m) {
  const upb_fielddef *values_f = upb_msgdef_itof(m, 1);
  const upb_msgdef *values_m = upb_fielddef_msgsubdef(values_f);
  const upb_array *values = upb_msg_get(msg, values_f).array_val;
  size_t i;
  bool first = true;

  jsonenc_putstr(e, "[");

  if (values) {
    const size_t size = upb_array_size(values);
    for (i = 0; i < size; i++) {
      upb_msgval elem = upb_array_get(values, i);

      jsonenc_putsep(e, ",", &first);
      jsonenc_value(e, elem.msg_val, values_m);
    }
  }

  jsonenc_putstr(e, "]");
}

static void jsonenc_value(jsonenc *e, const upb_msg *msg, const upb_msgdef *m) {
  /* TODO(haberman): do we want a reflection method to get oneof case? */
  size_t iter = UPB_MSG_BEGIN;
  const upb_fielddef *f;
  upb_msgval val;

  if (!upb_msg_next(msg, m, NULL,  &f, &val, &iter)) {
    jsonenc_err(e, "No value set in Value proto");
  }

  switch (upb_fielddef_number(f)) {
    case 1:
      jsonenc_putstr(e, "null");
      break;
    case 2:
      jsonenc_double(e, "%.17g", val.double_val);
      break;
    case 3:
      jsonenc_string(e, val.str_val);
      break;
    case 4:
      jsonenc_putstr(e, val.bool_val ? "true" : "false");
      break;
    case 5:
      jsonenc_struct(e, val.msg_val, upb_fielddef_msgsubdef(f));
      break;
    case 6:
      jsonenc_listvalue(e, val.msg_val, upb_fielddef_msgsubdef(f));
      break;
  }
}

static void jsonenc_msgfield(jsonenc *e, const upb_msg *msg,
                             const upb_msgdef *m) {
  switch (upb_msgdef_wellknowntype(m)) {
    case UPB_WELLKNOWN_UNSPECIFIED:
      jsonenc_msg(e, msg, m);
      break;
    case UPB_WELLKNOWN_ANY:
      jsonenc_any(e, msg, m);
      break;
    case UPB_WELLKNOWN_FIELDMASK:
      jsonenc_fieldmask(e, msg, m);
      break;
    case UPB_WELLKNOWN_DURATION:
      jsonenc_duration(e, msg, m);
      break;
    case UPB_WELLKNOWN_TIMESTAMP:
      jsonenc_timestamp(e, msg, m);
      break;
    case UPB_WELLKNOWN_DOUBLEVALUE:
    case UPB_WELLKNOWN_FLOATVALUE:
    case UPB_WELLKNOWN_INT64VALUE:
    case UPB_WELLKNOWN_UINT64VALUE:
    case UPB_WELLKNOWN_INT32VALUE:
    case UPB_WELLKNOWN_UINT32VALUE:
    case UPB_WELLKNOWN_STRINGVALUE:
    case UPB_WELLKNOWN_BYTESVALUE:
    case UPB_WELLKNOWN_BOOLVALUE:
      jsonenc_wrapper(e, msg, m);
      break;
    case UPB_WELLKNOWN_VALUE:
      jsonenc_value(e, msg, m);
      break;
    case UPB_WELLKNOWN_LISTVALUE:
      jsonenc_listvalue(e, msg, m);
      break;
    case UPB_WELLKNOWN_STRUCT:
      jsonenc_struct(e, msg, m);
      break;
  }
}

static void jsonenc_scalar(jsonenc *e, upb_msgval val, const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_BOOL:
      jsonenc_putstr(e, val.bool_val ? "true" : "false");
      break;
    case UPB_TYPE_FLOAT:
      jsonenc_double(e, "%.9g", val.float_val);
      break;
    case UPB_TYPE_DOUBLE:
      jsonenc_double(e, "%.17g", val.double_val);
      break;
    case UPB_TYPE_INT32:
      jsonenc_printf(e, "%" PRId32, val.int32_val);
      break;
    case UPB_TYPE_UINT32:
      jsonenc_printf(e, "%" PRIu32, val.uint32_val);
      break;
    case UPB_TYPE_INT64:
      jsonenc_printf(e, "\"%" PRId64 "\"", val.int64_val);
      break;
    case UPB_TYPE_UINT64:
      jsonenc_printf(e, "\"%" PRIu64 "\"", val.uint64_val);
      break;
    case UPB_TYPE_STRING:
      jsonenc_string(e, val.str_val);
      break;
    case UPB_TYPE_BYTES:
      jsonenc_bytes(e, val.str_val);
      break;
    case UPB_TYPE_ENUM:
      jsonenc_enum(val.int32_val, f, e);
      break;
    case UPB_TYPE_MESSAGE:
      jsonenc_msgfield(e, val.msg_val, upb_fielddef_msgsubdef(f));
      break;
  }
}

static void jsonenc_mapkey(jsonenc *e, upb_msgval val, const upb_fielddef *f) {
  jsonenc_putstr(e, "\"");

  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_BOOL:
      jsonenc_putstr(e, val.bool_val ? "true" : "false");
      break;
    case UPB_TYPE_INT32:
      jsonenc_printf(e, "%" PRId32, val.int32_val);
      break;
    case UPB_TYPE_UINT32:
      jsonenc_printf(e, "%" PRIu32, val.uint32_val);
      break;
    case UPB_TYPE_INT64:
      jsonenc_printf(e, "%" PRId64, val.int64_val);
      break;
    case UPB_TYPE_UINT64:
      jsonenc_printf(e, "%" PRIu64, val.uint64_val);
      break;
    case UPB_TYPE_STRING:
      jsonenc_stringbody(e, val.str_val);
      break;
    default:
      UPB_UNREACHABLE();
  }

  jsonenc_putstr(e, "\":");
}

static void jsonenc_array(jsonenc *e, const upb_array *arr,
                         const upb_fielddef *f) {
  size_t i;
  size_t size = arr ? upb_array_size(arr) : 0;
  bool first = true;

  jsonenc_putstr(e, "[");

  for (i = 0; i < size; i++) {
    jsonenc_putsep(e, ",", &first);
    jsonenc_scalar(e, upb_array_get(arr, i), f);
  }

  jsonenc_putstr(e, "]");
}

static void jsonenc_map(jsonenc *e, const upb_map *map, const upb_fielddef *f) {
  const upb_msgdef *entry = upb_fielddef_msgsubdef(f);
  const upb_fielddef *key_f = upb_msgdef_itof(entry, 1);
  const upb_fielddef *val_f = upb_msgdef_itof(entry, 2);
  size_t iter = UPB_MAP_BEGIN;
  bool first = true;

  jsonenc_putstr(e, "{");

  if (map) {
    while (upb_mapiter_next(map, &iter)) {
      jsonenc_putsep(e, ",", &first);
      jsonenc_mapkey(e, upb_mapiter_key(map, iter), key_f);
      jsonenc_scalar(e, upb_mapiter_value(map, iter), val_f);
    }
  }

  jsonenc_putstr(e, "}");
}

static void jsonenc_fieldval(jsonenc *e, const upb_fielddef *f,
                             upb_msgval val, bool *first) {
  const char *name;

  if (e->options & UPB_JSONENC_PROTONAMES) {
    name = upb_fielddef_name(f);
  } else {
    name = upb_fielddef_jsonname(f);
  }

  jsonenc_putsep(e, ",", first);
  jsonenc_printf(e, "\"%s\":", name);

  if (upb_fielddef_ismap(f)) {
    jsonenc_map(e, val.map_val, f);
  } else if (upb_fielddef_isseq(f)) {
    jsonenc_array(e, val.array_val, f);
  } else {
    jsonenc_scalar(e, val, f);
  }
}

static void jsonenc_msgfields(jsonenc *e, const upb_msg *msg,
                              const upb_msgdef *m) {
  upb_msgval val;
  const upb_fielddef *f;
  bool first = true;

  if (e->options & UPB_JSONENC_EMITDEFAULTS) {
    /* Iterate over all fields. */
    int i = 0;
    int n = upb_msgdef_fieldcount(m);
    for (i = 0; i < n; i++) {
      f = upb_msgdef_field(m, i);
      if (!upb_fielddef_haspresence(f) || upb_msg_has(msg, f)) {
        jsonenc_fieldval(e, f, upb_msg_get(msg, f), &first);
      }
    }
  } else {
    /* Iterate over non-empty fields. */
    size_t iter = UPB_MSG_BEGIN;
    while (upb_msg_next(msg, m, e->ext_pool, &f, &val, &iter)) {
      jsonenc_fieldval(e, f, val, &first);
    }
  }
}

static void jsonenc_msg(jsonenc *e, const upb_msg *msg, const upb_msgdef *m) {
  jsonenc_putstr(e, "{");
  jsonenc_msgfields(e, msg, m);
  jsonenc_putstr(e, "}");
}

static size_t jsonenc_nullz(jsonenc *e, size_t size) {
  size_t ret = e->ptr - e->buf + e->overflow;

  if (size > 0) {
    if (e->ptr == e->end) e->ptr--;
    *e->ptr = '\0';
  }

  return ret;
}

size_t upb_json_encode(const upb_msg *msg, const upb_msgdef *m,
                       const upb_symtab *ext_pool, int options, char *buf,
                       size_t size, upb_status *status) {
  jsonenc e;

  e.buf = buf;
  e.ptr = buf;
  e.end = buf + size;
  e.overflow = 0;
  e.options = options;
  e.ext_pool = ext_pool;
  e.status = status;
  e.arena = NULL;

  if (setjmp(e.err)) return -1;

  jsonenc_msgfield(&e, msg, m);
  if (e.arena) upb_arena_free(e.arena);
  return jsonenc_nullz(&e, size);
}
