
#include "upb/json_decode.h"

#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "upb/encode.h"
#include "upb/reflection.h"

/* Special header, must be included last. */
#include "upb/port_def.inc"

typedef struct {
  const char *ptr, *end;
  upb_arena *arena;  /* TODO: should we have a tmp arena for tmp data? */
  const upb_symtab *any_pool;
  int depth;
  upb_status *status;
  jmp_buf err;
  int line;
  const char *line_begin;
  bool is_first;
  int options;
  const upb_fielddef *debug_field;
} jsondec;

enum { JD_OBJECT, JD_ARRAY, JD_STRING, JD_NUMBER, JD_TRUE, JD_FALSE, JD_NULL };

/* Forward declarations of mutually-recursive functions. */
static void jsondec_wellknown(jsondec *d, upb_msg *msg, const upb_msgdef *m);
static upb_msgval jsondec_value(jsondec *d, const upb_fielddef *f);
static void jsondec_wellknownvalue(jsondec *d, upb_msg *msg,
                                   const upb_msgdef *m);
static void jsondec_object(jsondec *d, upb_msg *msg, const upb_msgdef *m);

static bool jsondec_streql(upb_strview str, const char *lit) {
  return str.size == strlen(lit) && memcmp(str.data, lit, str.size) == 0;
}

static bool jsondec_isnullvalue(const upb_fielddef *f) {
  return upb_fielddef_type(f) == UPB_TYPE_ENUM &&
         strcmp(upb_enumdef_fullname(upb_fielddef_enumsubdef(f)),
                "google.protobuf.NullValue") == 0;
}

static bool jsondec_isvalue(const upb_fielddef *f) {
  return (upb_fielddef_type(f) == UPB_TYPE_MESSAGE &&
          upb_msgdef_wellknowntype(upb_fielddef_msgsubdef(f)) ==
              UPB_WELLKNOWN_VALUE) ||
         jsondec_isnullvalue(f);
}

UPB_NORETURN static void jsondec_err(jsondec *d, const char *msg) {
  upb_status_seterrf(d->status, "Error parsing JSON @%d:%d: %s", d->line,
                     (int)(d->ptr - d->line_begin), msg);
  UPB_LONGJMP(d->err, 1);
}

UPB_NORETURN static void jsondec_errf(jsondec *d, const char *fmt, ...) {
  va_list argp;
  upb_status_seterrf(d->status, "Error parsing JSON @%d:%d: ", d->line,
                     (int)(d->ptr - d->line_begin));
  va_start(argp, fmt);
  upb_status_vappenderrf(d->status, fmt, argp);
  va_end(argp);
  UPB_LONGJMP(d->err, 1);
}

static void jsondec_skipws(jsondec *d) {
  while (d->ptr != d->end) {
    switch (*d->ptr) {
      case '\n':
        d->line++;
        d->line_begin = d->ptr;
        /* Fallthrough. */
      case '\r':
      case '\t':
      case ' ':
        d->ptr++;
        break;
      default:
        return;
    }
  }
  jsondec_err(d, "Unexpected EOF");
}

static bool jsondec_tryparsech(jsondec *d, char ch) {
  if (d->ptr == d->end || *d->ptr != ch) return false;
  d->ptr++;
  return true;
}

static void jsondec_parselit(jsondec *d, const char *lit) {
  size_t avail = d->end - d->ptr;
  size_t len = strlen(lit);
  if (avail < len || memcmp(d->ptr, lit, len) != 0) {
    jsondec_errf(d, "Expected: '%s'", lit);
  }
  d->ptr += len;
}

static void jsondec_wsch(jsondec *d, char ch) {
  jsondec_skipws(d);
  if (!jsondec_tryparsech(d, ch)) {
    jsondec_errf(d, "Expected: '%c'", ch);
  }
}

static void jsondec_true(jsondec *d) { jsondec_parselit(d, "true"); }
static void jsondec_false(jsondec *d) { jsondec_parselit(d, "false"); }
static void jsondec_null(jsondec *d) { jsondec_parselit(d, "null"); }

static void jsondec_entrysep(jsondec *d) {
  jsondec_skipws(d);
  jsondec_parselit(d, ":");
}

static int jsondec_rawpeek(jsondec *d) {
  switch (*d->ptr) {
    case '{':
      return JD_OBJECT;
    case '[':
      return JD_ARRAY;
    case '"':
      return JD_STRING;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return JD_NUMBER;
    case 't':
      return JD_TRUE;
    case 'f':
      return JD_FALSE;
    case 'n':
      return JD_NULL;
    default:
      jsondec_errf(d, "Unexpected character: '%c'", *d->ptr);
  }
}

/* JSON object/array **********************************************************/

/* These are used like so:
 *
 * jsondec_objstart(d);
 * while (jsondec_objnext(d)) {
 *   ...
 * }
 * jsondec_objend(d) */

static int jsondec_peek(jsondec *d) {
  jsondec_skipws(d);
  return jsondec_rawpeek(d);
}

static void jsondec_push(jsondec *d) {
  if (--d->depth < 0) {
    jsondec_err(d, "Recursion limit exceeded");
  }
  d->is_first = true;
}

static bool jsondec_seqnext(jsondec *d, char end_ch) {
  bool is_first = d->is_first;
  d->is_first = false;
  jsondec_skipws(d);
  if (*d->ptr == end_ch) return false;
  if (!is_first) jsondec_parselit(d, ",");
  return true;
}

static void jsondec_arrstart(jsondec *d) {
  jsondec_push(d);
  jsondec_wsch(d, '[');
}

static void jsondec_arrend(jsondec *d) {
  d->depth++;
  jsondec_wsch(d, ']');
}

static bool jsondec_arrnext(jsondec *d) {
  return jsondec_seqnext(d, ']');
}

static void jsondec_objstart(jsondec *d) {
  jsondec_push(d);
  jsondec_wsch(d, '{');
}

static void jsondec_objend(jsondec *d) {
  d->depth++;
  jsondec_wsch(d, '}');
}

static bool jsondec_objnext(jsondec *d) {
  if (!jsondec_seqnext(d, '}')) return false;
  if (jsondec_peek(d) != JD_STRING) {
    jsondec_err(d, "Object must start with string");
  }
  return true;
}

/* JSON number ****************************************************************/

static bool jsondec_tryskipdigits(jsondec *d) {
  const char *start = d->ptr;

  while (d->ptr < d->end) {
    if (*d->ptr < '0' || *d->ptr > '9') {
      break;
    }
    d->ptr++;
  }

  return d->ptr != start;
}

static void jsondec_skipdigits(jsondec *d) {
  if (!jsondec_tryskipdigits(d)) {
    jsondec_err(d, "Expected one or more digits");
  }
}

static double jsondec_number(jsondec *d) {
  const char *start = d->ptr;

  assert(jsondec_rawpeek(d) == JD_NUMBER);

  /* Skip over the syntax of a number, as specified by JSON. */
  if (*d->ptr == '-') d->ptr++;

  if (jsondec_tryparsech(d, '0')) {
    if (jsondec_tryskipdigits(d)) {
      jsondec_err(d, "number cannot have leading zero");
    }
  } else {
    jsondec_skipdigits(d);
  }

  if (d->ptr == d->end) goto parse;
  if (jsondec_tryparsech(d, '.')) {
    jsondec_skipdigits(d);
  }
  if (d->ptr == d->end) goto parse;

  if (*d->ptr == 'e' || *d->ptr == 'E') {
    d->ptr++;
    if (d->ptr == d->end) {
      jsondec_err(d, "Unexpected EOF in number");
    }
    if (*d->ptr == '+' || *d->ptr == '-') {
      d->ptr++;
    }
    jsondec_skipdigits(d);
  }

parse:
  /* Having verified the syntax of a JSON number, use strtod() to parse
   * (strtod() accepts a superset of JSON syntax). */
  errno = 0;
  {
    char* end;
    double val = strtod(start, &end);
    assert(end == d->ptr);

    /* Currently the min/max-val conformance tests fail if we check this.  Does
     * this mean the conformance tests are wrong or strtod() is wrong, or
     * something else?  Investigate further. */
    /*
    if (errno == ERANGE) {
      jsondec_err(d, "Number out of range");
    }
    */

    if (val > DBL_MAX || val < -DBL_MAX) {
      jsondec_err(d, "Number out of range");
    }

    return val;
  }
}

/* JSON string ****************************************************************/

static char jsondec_escape(jsondec *d) {
  switch (*d->ptr++) {
    case '"':
      return '\"';
    case '\\':
      return '\\';
    case '/':
      return '/';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    default:
      jsondec_err(d, "Invalid escape char");
  }
}

static uint32_t jsondec_codepoint(jsondec *d) {
  uint32_t cp = 0;
  const char *end;

  if (d->end - d->ptr < 4) {
    jsondec_err(d, "EOF inside string");
  }

  end = d->ptr + 4;
  while (d->ptr < end) {
    char ch = *d->ptr++;
    if (ch >= '0' && ch <= '9') {
      ch -= '0';
    } else if (ch >= 'a' && ch <= 'f') {
      ch = ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'F') {
      ch = ch - 'A' + 10;
    } else {
      jsondec_err(d, "Invalid hex digit");
    }
    cp = (cp << 4) | ch;
  }

  return cp;
}

/* Parses a \uXXXX unicode escape (possibly a surrogate pair). */
static size_t jsondec_unicode(jsondec *d, char* out) {
  uint32_t cp = jsondec_codepoint(d);
  if (cp >= 0xd800 && cp <= 0xdbff) {
    /* Surrogate pair: two 16-bit codepoints become a 32-bit codepoint. */
    uint32_t high = cp;
    uint32_t low;
    jsondec_parselit(d, "\\u");
    low = jsondec_codepoint(d);
    if (low < 0xdc00 || low > 0xdfff) {
      jsondec_err(d, "Invalid low surrogate");
    }
    cp = (high & 0x3ff) << 10;
    cp |= (low & 0x3ff);
    cp += 0x10000;
  } else if (cp >= 0xdc00 && cp <= 0xdfff) {
    jsondec_err(d, "Unpaired low surrogate");
  }

  /* Write to UTF-8 */
  if (cp <= 0x7f) {
    out[0] = cp;
    return 1;
  } else if (cp <= 0x07FF) {
    out[0] = ((cp >> 6) & 0x1F) | 0xC0;
    out[1] = ((cp >> 0) & 0x3F) | 0x80;
    return 2;
  } else if (cp <= 0xFFFF) {
    out[0] = ((cp >> 12) & 0x0F) | 0xE0;
    out[1] = ((cp >> 6) & 0x3F) | 0x80;
    out[2] = ((cp >> 0) & 0x3F) | 0x80;
    return 3;
  } else if (cp < 0x10FFFF) {
    out[0] = ((cp >> 18) & 0x07) | 0xF0;
    out[1] = ((cp >> 12) & 0x3f) | 0x80;
    out[2] = ((cp >> 6) & 0x3f) | 0x80;
    out[3] = ((cp >> 0) & 0x3f) | 0x80;
    return 4;
  } else {
    jsondec_err(d, "Invalid codepoint");
  }
}

static void jsondec_resize(jsondec *d, char **buf, char **end, char **buf_end) {
  size_t oldsize = *buf_end - *buf;
  size_t len = *end - *buf;
  size_t size = UPB_MAX(8, 2 * oldsize);

  *buf = upb_arena_realloc(d->arena, *buf, len, size);
  if (!*buf) jsondec_err(d, "Out of memory");

  *end = *buf + len;
  *buf_end = *buf + size;
}

static upb_strview jsondec_string(jsondec *d) {
  char *buf = NULL;
  char *end = NULL;
  char *buf_end = NULL;

  jsondec_skipws(d);

  if (*d->ptr++ != '"') {
    jsondec_err(d, "Expected string");
  }

  while (d->ptr < d->end) {
    char ch = *d->ptr++;

    if (end == buf_end) {
      jsondec_resize(d, &buf, &end, &buf_end);
    }

    switch (ch) {
      case '"': {
        upb_strview ret;
        ret.data = buf;
        ret.size = end - buf;
        *end = '\0';  /* Needed for possible strtod(). */
        return ret;
      }
      case '\\':
        if (d->ptr == d->end) goto eof;
        if (*d->ptr == 'u') {
          d->ptr++;
          if (buf_end - end < 4) {
            /* Allow space for maximum-sized code point (4 bytes). */
            jsondec_resize(d, &buf, &end, &buf_end);
          }
          end += jsondec_unicode(d, end);
        } else {
          *end++ = jsondec_escape(d);
        }
        break;
      default:
        if ((unsigned char)*d->ptr < 0x20) {
          jsondec_err(d, "Invalid char in JSON string");
        }
        *end++ = ch;
        break;
    }
  }

eof:
  jsondec_err(d, "EOF inside string");
}

static void jsondec_skipval(jsondec *d) {
  switch (jsondec_peek(d)) {
    case JD_OBJECT:
      jsondec_objstart(d);
      while (jsondec_objnext(d)) {
        jsondec_string(d);
        jsondec_entrysep(d);
        jsondec_skipval(d);
      }
      jsondec_objend(d);
      break;
    case JD_ARRAY:
      jsondec_arrstart(d);
      while (jsondec_arrnext(d)) {
        jsondec_skipval(d);
      }
      jsondec_arrend(d);
      break;
    case JD_TRUE:
      jsondec_true(d);
      break;
    case JD_FALSE:
      jsondec_false(d);
      break;
    case JD_NULL:
      jsondec_null(d);
      break;
    case JD_STRING:
      jsondec_string(d);
      break;
    case JD_NUMBER:
      jsondec_number(d);
      break;
  }
}

/* Base64 decoding for bytes fields. ******************************************/

static unsigned int jsondec_base64_tablelookup(const char ch) {
  /* Table includes the normal base64 chars plus the URL-safe variant. */
  const signed char table[256] = {
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       62 /*+*/, -1,       62 /*-*/, -1,       63 /*/ */, 52 /*0*/,
      53 /*1*/, 54 /*2*/, 55 /*3*/, 56 /*4*/, 57 /*5*/, 58 /*6*/,  59 /*7*/,
      60 /*8*/, 61 /*9*/, -1,       -1,       -1,       -1,        -1,
      -1,       -1,       0 /*A*/,  1 /*B*/,  2 /*C*/,  3 /*D*/,   4 /*E*/,
      5 /*F*/,  6 /*G*/,  07 /*H*/, 8 /*I*/,  9 /*J*/,  10 /*K*/,  11 /*L*/,
      12 /*M*/, 13 /*N*/, 14 /*O*/, 15 /*P*/, 16 /*Q*/, 17 /*R*/,  18 /*S*/,
      19 /*T*/, 20 /*U*/, 21 /*V*/, 22 /*W*/, 23 /*X*/, 24 /*Y*/,  25 /*Z*/,
      -1,       -1,       -1,       -1,       63 /*_*/, -1,        26 /*a*/,
      27 /*b*/, 28 /*c*/, 29 /*d*/, 30 /*e*/, 31 /*f*/, 32 /*g*/,  33 /*h*/,
      34 /*i*/, 35 /*j*/, 36 /*k*/, 37 /*l*/, 38 /*m*/, 39 /*n*/,  40 /*o*/,
      41 /*p*/, 42 /*q*/, 43 /*r*/, 44 /*s*/, 45 /*t*/, 46 /*u*/,  47 /*v*/,
      48 /*w*/, 49 /*x*/, 50 /*y*/, 51 /*z*/, -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1};

  /* Sign-extend return value so high bit will be set on any unexpected char. */
  return table[(unsigned)ch];
}

static char *jsondec_partialbase64(jsondec *d, const char *ptr, const char *end,
                                   char *out) {
  int32_t val = -1;

  switch (end - ptr) {
    case 2:
      val = jsondec_base64_tablelookup(ptr[0]) << 18 |
            jsondec_base64_tablelookup(ptr[1]) << 12;
      out[0] = val >> 16;
      out += 1;
      break;
    case 3:
      val = jsondec_base64_tablelookup(ptr[0]) << 18 |
            jsondec_base64_tablelookup(ptr[1]) << 12 |
            jsondec_base64_tablelookup(ptr[2]) << 6;
      out[0] = val >> 16;
      out[1] = (val >> 8) & 0xff;
      out += 2;
      break;
  }

  if (val < 0) {
    jsondec_err(d, "Corrupt base64");
  }

  return out;
}

static size_t jsondec_base64(jsondec *d, upb_strview str) {
  /* We decode in place. This is safe because this is a new buffer (not
   * aliasing the input) and because base64 decoding shrinks 4 bytes into 3. */
  char *out = (char*)str.data;
  const char *ptr = str.data;
  const char *end = ptr + str.size;
  const char *end4 = ptr + (str.size & -4);  /* Round down to multiple of 4. */

  for (; ptr < end4; ptr += 4, out += 3) {
    int val = jsondec_base64_tablelookup(ptr[0]) << 18 |
              jsondec_base64_tablelookup(ptr[1]) << 12 |
              jsondec_base64_tablelookup(ptr[2]) << 6 |
              jsondec_base64_tablelookup(ptr[3]) << 0;

    if (val < 0) {
      /* Junk chars or padding. Remove trailing padding, if any. */
      if (end - ptr == 4 && ptr[3] == '=') {
        if (ptr[2] == '=') {
          end -= 2;
        } else {
          end -= 1;
        }
      }
      break;
    }

    out[0] = val >> 16;
    out[1] = (val >> 8) & 0xff;
    out[2] = val & 0xff;
  }

  if (ptr < end) {
    /* Process remaining chars. We do not require padding. */
    out = jsondec_partialbase64(d, ptr, end, out);
  }

  return out - str.data;
}

/* Low-level integer parsing **************************************************/

/* We use these hand-written routines instead of strto[u]l() because the "long
 * long" variants aren't in c89. Also our version allows setting a ptr limit. */

static const char *jsondec_buftouint64(jsondec *d, const char *ptr,
                                       const char *end, uint64_t *val) {
  uint64_t u64 = 0;
  while (ptr < end) {
    unsigned ch = *ptr - '0';
    if (ch >= 10) break;
    if (u64 > UINT64_MAX / 10 || u64 * 10 > UINT64_MAX - ch) {
      jsondec_err(d, "Integer overflow");
    }
    u64 *= 10;
    u64 += ch;
    ptr++;
  }

  *val = u64;
  return ptr;
}

static const char *jsondec_buftoint64(jsondec *d, const char *ptr,
                                      const char *end, int64_t *val) {
  bool neg = false;
  uint64_t u64;

  if (ptr != end && *ptr == '-') {
    ptr++;
    neg = true;
  }

  ptr = jsondec_buftouint64(d, ptr, end, &u64);
  if (u64 > (uint64_t)INT64_MAX + neg) {
    jsondec_err(d, "Integer overflow");
  }

  *val = neg ? -u64 : u64;
  return ptr;
}

static uint64_t jsondec_strtouint64(jsondec *d, upb_strview str) {
  const char *end = str.data + str.size;
  uint64_t ret;
  if (jsondec_buftouint64(d, str.data, end, &ret) != end) {
    jsondec_err(d, "Non-number characters in quoted integer");
  }
  return ret;
}

static int64_t jsondec_strtoint64(jsondec *d, upb_strview str) {
  const char *end = str.data + str.size;
  int64_t ret;
  if (jsondec_buftoint64(d, str.data, end, &ret) != end) {
    jsondec_err(d, "Non-number characters in quoted integer");
  }
  return ret;
}

/* Primitive value types ******************************************************/

/* Parse INT32 or INT64 value. */
static upb_msgval jsondec_int(jsondec *d, const upb_fielddef *f) {
  upb_msgval val;

  switch (jsondec_peek(d)) {
    case JD_NUMBER: {
      double dbl = jsondec_number(d);
      if (dbl > 9223372036854774784.0 || dbl < -9223372036854775808.0) {
        jsondec_err(d, "JSON number is out of range.");
      }
      val.int64_val = dbl;  /* must be guarded, overflow here is UB */
      if (val.int64_val != dbl) {
        jsondec_errf(d, "JSON number was not integral (%d != %" PRId64 ")", dbl,
                     val.int64_val);
      }
      break;
    }
    case JD_STRING: {
      upb_strview str = jsondec_string(d);
      val.int64_val = jsondec_strtoint64(d, str);
      break;
    }
    default:
      jsondec_err(d, "Expected number or string");
  }

  if (upb_fielddef_type(f) == UPB_TYPE_INT32) {
    if (val.int64_val > INT32_MAX || val.int64_val < INT32_MIN) {
      jsondec_err(d, "Integer out of range.");
    }
    val.int32_val = (int32_t)val.int64_val;
  }

  return val;
}

/* Parse UINT32 or UINT64 value. */
static upb_msgval jsondec_uint(jsondec *d, const upb_fielddef *f) {
  upb_msgval val;

  switch (jsondec_peek(d)) {
    case JD_NUMBER: {
      double dbl = jsondec_number(d);
      if (dbl > 18446744073709549568.0 || dbl < 0) {
        jsondec_err(d, "JSON number is out of range.");
      }
      val.uint64_val = dbl;  /* must be guarded, overflow here is UB */
      if (val.uint64_val != dbl) {
        jsondec_errf(d, "JSON number was not integral (%d != %" PRIu64 ")", dbl,
                     val.uint64_val);
      }
      break;
    }
    case JD_STRING: {
      upb_strview str = jsondec_string(d);
      val.uint64_val = jsondec_strtouint64(d, str);
      break;
    }
    default:
      jsondec_err(d, "Expected number or string");
  }

  if (upb_fielddef_type(f) == UPB_TYPE_UINT32) {
    if (val.uint64_val > UINT32_MAX) {
      jsondec_err(d, "Integer out of range.");
    }
    val.uint32_val = (uint32_t)val.uint64_val;
  }

  return val;
}

/* Parse DOUBLE or FLOAT value. */
static upb_msgval jsondec_double(jsondec *d, const upb_fielddef *f) {
  upb_strview str;
  upb_msgval val;

  switch (jsondec_peek(d)) {
    case JD_NUMBER:
      val.double_val = jsondec_number(d);
      break;
    case JD_STRING:
      str = jsondec_string(d);
      if (jsondec_streql(str, "NaN")) {
        val.double_val = NAN;
      } else if (jsondec_streql(str, "Infinity")) {
        val.double_val = INFINITY;
      } else if (jsondec_streql(str, "-Infinity")) {
        val.double_val = -INFINITY;
      } else {
        val.double_val = strtod(str.data, NULL);
      }
      break;
    default:
      jsondec_err(d, "Expected number or string");
  }

  if (upb_fielddef_type(f) == UPB_TYPE_FLOAT) {
    if (val.double_val != INFINITY && val.double_val != -INFINITY &&
        (val.double_val > FLT_MAX || val.double_val < -FLT_MAX)) {
      jsondec_err(d, "Float out of range");
    }
    val.float_val = val.double_val;
  }

  return val;
}

/* Parse STRING or BYTES value. */
static upb_msgval jsondec_strfield(jsondec *d, const upb_fielddef *f) {
  upb_msgval val;
  val.str_val = jsondec_string(d);
  if (upb_fielddef_type(f) == UPB_TYPE_BYTES) {
    val.str_val.size = jsondec_base64(d, val.str_val);
  }
  return val;
}

static upb_msgval jsondec_enum(jsondec *d, const upb_fielddef *f) {
  switch (jsondec_peek(d)) {
    case JD_STRING: {
      const upb_enumdef *e = upb_fielddef_enumsubdef(f);
      upb_strview str = jsondec_string(d);
      upb_msgval val;
      if (!upb_enumdef_ntoi(e, str.data, str.size, &val.int32_val)) {
        if (d->options & UPB_JSONDEC_IGNOREUNKNOWN) {
          val.int32_val = 0;
        } else {
          jsondec_errf(d, "Unknown enumerator: '" UPB_STRVIEW_FORMAT "'",
                       UPB_STRVIEW_ARGS(str));
        }
      }
      return val;
    }
    case JD_NULL: {
      if (jsondec_isnullvalue(f)) {
        upb_msgval val;
        jsondec_null(d);
        val.int32_val = 0;
        return val;
      }
    }
      /* Fallthrough. */
    default:
      return jsondec_int(d, f);
  }
}

static upb_msgval jsondec_bool(jsondec *d, const upb_fielddef *f) {
  bool is_map_key = upb_fielddef_number(f) == 1 &&
                    upb_msgdef_mapentry(upb_fielddef_containingtype(f));
  upb_msgval val;

  if (is_map_key) {
    upb_strview str = jsondec_string(d);
    if (jsondec_streql(str, "true")) {
      val.bool_val = true;
    } else if (jsondec_streql(str, "false")) {
      val.bool_val = false;
    } else {
      jsondec_err(d, "Invalid boolean map key");
    }
  } else {
    switch (jsondec_peek(d)) {
      case JD_TRUE:
        val.bool_val = true;
        jsondec_true(d);
        break;
      case JD_FALSE:
        val.bool_val = false;
        jsondec_false(d);
        break;
      default:
        jsondec_err(d, "Expected true or false");
    }
  }

  return val;
}

/* Composite types (array/message/map) ****************************************/

static void jsondec_array(jsondec *d, upb_msg *msg, const upb_fielddef *f) {
  upb_array *arr = upb_msg_mutable(msg, f, d->arena).array;

  jsondec_arrstart(d);
  while (jsondec_arrnext(d)) {
    upb_msgval elem = jsondec_value(d, f);
    upb_array_append(arr, elem, d->arena);
  }
  jsondec_arrend(d);
}

static void jsondec_map(jsondec *d, upb_msg *msg, const upb_fielddef *f) {
  upb_map *map = upb_msg_mutable(msg, f, d->arena).map;
  const upb_msgdef *entry = upb_fielddef_msgsubdef(f);
  const upb_fielddef *key_f = upb_msgdef_itof(entry, 1);
  const upb_fielddef *val_f = upb_msgdef_itof(entry, 2);

  jsondec_objstart(d);
  while (jsondec_objnext(d)) {
    upb_msgval key, val;
    key = jsondec_value(d, key_f);
    jsondec_entrysep(d);
    val = jsondec_value(d, val_f);
    upb_map_set(map, key, val, d->arena);
  }
  jsondec_objend(d);
}

static void jsondec_tomsg(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  if (upb_msgdef_wellknowntype(m) == UPB_WELLKNOWN_UNSPECIFIED) {
    jsondec_object(d, msg, m);
  } else {
    jsondec_wellknown(d, msg, m);
  }
}

static upb_msgval jsondec_msg(jsondec *d, const upb_fielddef *f) {
  const upb_msgdef *m = upb_fielddef_msgsubdef(f);
  upb_msg *msg = upb_msg_new(m, d->arena);
  upb_msgval val;

  jsondec_tomsg(d, msg, m);
  val.msg_val = msg;
  return val;
}

static void jsondec_field(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  upb_strview name;
  const upb_fielddef *f;
  const upb_fielddef *preserved;

  name = jsondec_string(d);
  jsondec_entrysep(d);
  f = upb_msgdef_lookupjsonname(m, name.data, name.size);

  if (!f) {
    if ((d->options & UPB_JSONDEC_IGNOREUNKNOWN) == 0) {
      jsondec_errf(d, "Unknown field: '" UPB_STRVIEW_FORMAT "'",
                   UPB_STRVIEW_ARGS(name));
    }
    jsondec_skipval(d);
    return;
  }

  if (upb_fielddef_realcontainingoneof(f) &&
      upb_msg_whichoneof(msg, upb_fielddef_containingoneof(f))) {
    jsondec_err(d, "More than one field for this oneof.");
  }

  if (jsondec_peek(d) == JD_NULL && !jsondec_isvalue(f)) {
    /* JSON "null" indicates a default value, so no need to set anything. */
    jsondec_null(d);
    return;
  }

  preserved = d->debug_field;
  d->debug_field = f;

  if (upb_fielddef_ismap(f)) {
    jsondec_map(d, msg, f);
  } else if (upb_fielddef_isseq(f)) {
    jsondec_array(d, msg, f);
  } else if (upb_fielddef_issubmsg(f)) {
    upb_msg *submsg = upb_msg_mutable(msg, f, d->arena).msg;
    const upb_msgdef *subm = upb_fielddef_msgsubdef(f);
    jsondec_tomsg(d, submsg, subm);
  } else {
    upb_msgval val = jsondec_value(d, f);
    upb_msg_set(msg, f, val, d->arena);
  }

  d->debug_field = preserved;
}

static void jsondec_object(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  jsondec_objstart(d);
  while (jsondec_objnext(d)) {
    jsondec_field(d, msg, m);
  }
  jsondec_objend(d);
}

static upb_msgval jsondec_value(jsondec *d, const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_BOOL:
      return jsondec_bool(d, f);
    case UPB_TYPE_FLOAT:
    case UPB_TYPE_DOUBLE:
      return jsondec_double(d, f);
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
      return jsondec_uint(d, f);
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
      return jsondec_int(d, f);
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      return jsondec_strfield(d, f);
    case UPB_TYPE_ENUM:
      return jsondec_enum(d, f);
    case UPB_TYPE_MESSAGE:
      return jsondec_msg(d, f);
    default:
      UPB_UNREACHABLE();
  }
}

/* Well-known types ***********************************************************/

static int jsondec_tsdigits(jsondec *d, const char **ptr, size_t digits,
                            const char *after) {
  uint64_t val;
  const char *p = *ptr;
  const char *end = p + digits;
  size_t after_len = after ? strlen(after) : 0;

  UPB_ASSERT(digits <= 9);  /* int can't overflow. */

  if (jsondec_buftouint64(d, p, end, &val) != end ||
      (after_len && memcmp(end, after, after_len) != 0)) {
    jsondec_err(d, "Malformed timestamp");
  }

  UPB_ASSERT(val < INT_MAX);

  *ptr = end + after_len;
  return (int)val;
}

static int jsondec_nanos(jsondec *d, const char **ptr, const char *end) {
  uint64_t nanos = 0;
  const char *p = *ptr;

  if (p != end && *p == '.') {
    const char *nano_end = jsondec_buftouint64(d, p + 1, end, &nanos);
    int digits = (int)(nano_end - p - 1);
    int exp_lg10 = 9 - digits;
    if (digits > 9) {
      jsondec_err(d, "Too many digits for partial seconds");
    }
    while (exp_lg10--) nanos *= 10;
    *ptr = nano_end;
  }

  UPB_ASSERT(nanos < INT_MAX);

  return (int)nanos;
}

/* jsondec_epochdays(1970, 1, 1) == 1970-01-01 == 0. */
int jsondec_epochdays(int y, int m, int d) {
  const uint32_t year_base = 4800;    /* Before min year, multiple of 400. */
  const uint32_t m_adj = m - 3;       /* March-based month. */
  const uint32_t carry = m_adj > (uint32_t)m ? 1 : 0;
  const uint32_t adjust = carry ? 12 : 0;
  const uint32_t y_adj = y + year_base - carry;
  const uint32_t month_days = ((m_adj + adjust) * 62719 + 769) / 2048;
  const uint32_t leap_days = y_adj / 4 - y_adj / 100 + y_adj / 400;
  return y_adj * 365 + leap_days + month_days + (d - 1) - 2472632;
}

static int64_t jsondec_unixtime(int y, int m, int d, int h, int min, int s) {
  return (int64_t)jsondec_epochdays(y, m, d) * 86400 + h * 3600 + min * 60 + s;
}

static void jsondec_timestamp(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  upb_msgval seconds;
  upb_msgval nanos;
  upb_strview str = jsondec_string(d);
  const char *ptr = str.data;
  const char *end = ptr + str.size;

  if (str.size < 20) goto malformed;

  {
    /* 1972-01-01T01:00:00 */
    int year = jsondec_tsdigits(d, &ptr, 4, "-");
    int mon = jsondec_tsdigits(d, &ptr, 2, "-");
    int day = jsondec_tsdigits(d, &ptr, 2, "T");
    int hour = jsondec_tsdigits(d, &ptr, 2, ":");
    int min = jsondec_tsdigits(d, &ptr, 2, ":");
    int sec = jsondec_tsdigits(d, &ptr, 2, NULL);

    seconds.int64_val = jsondec_unixtime(year, mon, day, hour, min, sec);
  }

  nanos.int32_val = jsondec_nanos(d, &ptr, end);

  {
    /* [+-]08:00 or Z */
    int ofs = 0;
    bool neg = false;

    if (ptr == end) goto malformed;

    switch (*ptr++) {
      case '-':
        neg = true;
        /* fallthrough */
      case '+':
        if ((end - ptr) != 5) goto malformed;
        ofs = jsondec_tsdigits(d, &ptr, 2, ":00");
        ofs *= 60 * 60;
        seconds.int64_val += (neg ? ofs : -ofs);
        break;
      case 'Z':
        if (ptr != end) goto malformed;
        break;
      default:
        goto malformed;
    }
  }

  if (seconds.int64_val < -62135596800) {
    jsondec_err(d, "Timestamp out of range");
  }

  upb_msg_set(msg, upb_msgdef_itof(m, 1), seconds, d->arena);
  upb_msg_set(msg, upb_msgdef_itof(m, 2), nanos, d->arena);
  return;

malformed:
  jsondec_err(d, "Malformed timestamp");
}

static void jsondec_duration(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  upb_msgval seconds;
  upb_msgval nanos;
  upb_strview str = jsondec_string(d);
  const char *ptr = str.data;
  const char *end = ptr + str.size;
  const int64_t max = (uint64_t)3652500 * 86400;

  /* "3.000000001s", "3s", etc. */
  ptr = jsondec_buftoint64(d, ptr, end, &seconds.int64_val);
  nanos.int32_val = jsondec_nanos(d, &ptr, end);

  if (end - ptr != 1 || *ptr != 's') {
    jsondec_err(d, "Malformed duration");
  }

  if (seconds.int64_val < -max || seconds.int64_val > max) {
    jsondec_err(d, "Duration out of range");
  }

  if (seconds.int64_val < 0) {
    nanos.int32_val = - nanos.int32_val;
  }

  upb_msg_set(msg, upb_msgdef_itof(m, 1), seconds, d->arena);
  upb_msg_set(msg, upb_msgdef_itof(m, 2), nanos, d->arena);
}

static void jsondec_listvalue(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  const upb_fielddef *values_f = upb_msgdef_itof(m, 1);
  const upb_msgdef *value_m = upb_fielddef_msgsubdef(values_f);
  upb_array *values = upb_msg_mutable(msg, values_f, d->arena).array;

  jsondec_arrstart(d);
  while (jsondec_arrnext(d)) {
    upb_msg *value_msg = upb_msg_new(value_m, d->arena);
    upb_msgval value;
    value.msg_val = value_msg;
    upb_array_append(values, value, d->arena);
    jsondec_wellknownvalue(d, value_msg, value_m);
  }
  jsondec_arrend(d);
}

static void jsondec_struct(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  const upb_fielddef *fields_f = upb_msgdef_itof(m, 1);
  const upb_msgdef *entry_m = upb_fielddef_msgsubdef(fields_f);
  const upb_fielddef *value_f = upb_msgdef_itof(entry_m, 2);
  const upb_msgdef *value_m = upb_fielddef_msgsubdef(value_f);
  upb_map *fields = upb_msg_mutable(msg, fields_f, d->arena).map;

  jsondec_objstart(d);
  while (jsondec_objnext(d)) {
    upb_msgval key, value;
    upb_msg *value_msg = upb_msg_new(value_m, d->arena);
    key.str_val = jsondec_string(d);
    value.msg_val = value_msg;
    upb_map_set(fields, key, value, d->arena);
    jsondec_entrysep(d);
    jsondec_wellknownvalue(d, value_msg, value_m);
  }
  jsondec_objend(d);
}

static void jsondec_wellknownvalue(jsondec *d, upb_msg *msg,
                                   const upb_msgdef *m) {
  upb_msgval val;
  const upb_fielddef *f;
  upb_msg *submsg;

  switch (jsondec_peek(d)) {
    case JD_NUMBER:
      /* double number_value = 2; */
      f = upb_msgdef_itof(m, 2);
      val.double_val = jsondec_number(d);
      break;
    case JD_STRING:
      /* string string_value = 3; */
      f = upb_msgdef_itof(m, 3);
      val.str_val = jsondec_string(d);
      break;
    case JD_FALSE:
      /* bool bool_value = 4; */
      f = upb_msgdef_itof(m, 4);
      val.bool_val = false;
      jsondec_false(d);
      break;
    case JD_TRUE:
      /* bool bool_value = 4; */
      f = upb_msgdef_itof(m, 4);
      val.bool_val = true;
      jsondec_true(d);
      break;
    case JD_NULL:
      /* NullValue null_value = 1; */
      f = upb_msgdef_itof(m, 1);
      val.int32_val = 0;
      jsondec_null(d);
      break;
    /* Note: these cases return, because upb_msg_mutable() is enough. */
    case JD_OBJECT:
      /* Struct struct_value = 5; */
      f = upb_msgdef_itof(m, 5);
      submsg = upb_msg_mutable(msg, f, d->arena).msg;
      jsondec_struct(d, submsg, upb_fielddef_msgsubdef(f));
      return;
    case JD_ARRAY:
      /* ListValue list_value = 6; */
      f = upb_msgdef_itof(m, 6);
      submsg = upb_msg_mutable(msg, f, d->arena).msg;
      jsondec_listvalue(d, submsg, upb_fielddef_msgsubdef(f));
      return;
    default:
      UPB_UNREACHABLE();
  }

  upb_msg_set(msg, f, val, d->arena);
}

static upb_strview jsondec_mask(jsondec *d, const char *buf, const char *end) {
  /* FieldMask fields grow due to inserted '_' characters, so we can't do the
   * transform in place. */
  const char *ptr = buf;
  upb_strview ret;
  char *out;

  ret.size = end - ptr;
  while (ptr < end) {
    ret.size += (*ptr >= 'A' && *ptr <= 'Z');
    ptr++;
  }

  out = upb_arena_malloc(d->arena, ret.size);
  ptr = buf;
  ret.data = out;

  while (ptr < end) {
    char ch = *ptr++;
    if (ch >= 'A' && ch <= 'Z') {
      *out++ = '_';
      *out++ = ch + 32;
    } else if (ch == '_') {
      jsondec_err(d, "field mask may not contain '_'");
    } else {
      *out++ = ch;
    }
  }

  return ret;
}

static void jsondec_fieldmask(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  /* repeated string paths = 1; */
  const upb_fielddef *paths_f = upb_msgdef_itof(m, 1);
  upb_array *arr = upb_msg_mutable(msg, paths_f, d->arena).array;
  upb_strview str = jsondec_string(d);
  const char *ptr = str.data;
  const char *end = ptr + str.size;
  upb_msgval val;

  while (ptr < end) {
    const char *elem_end = memchr(ptr, ',', end - ptr);
    if (elem_end) {
      val.str_val = jsondec_mask(d, ptr, elem_end);
      ptr = elem_end + 1;
    } else {
      val.str_val = jsondec_mask(d, ptr, end);
      ptr = end;
    }
    upb_array_append(arr, val, d->arena);
  }
}

static void jsondec_anyfield(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  if (upb_msgdef_wellknowntype(m) == UPB_WELLKNOWN_UNSPECIFIED) {
    /* For regular types: {"@type": "[user type]", "f1": <V1>, "f2": <V2>}
     * where f1, f2, etc. are the normal fields of this type. */
    jsondec_field(d, msg, m);
  } else {
    /* For well-known types: {"@type": "[well-known type]", "value": <X>}
     * where <X> is whatever encoding the WKT normally uses. */
    upb_strview str = jsondec_string(d);
    jsondec_entrysep(d);
    if (!jsondec_streql(str, "value")) {
      jsondec_err(d, "Key for well-known type must be 'value'");
    }
    jsondec_wellknown(d, msg, m);
  }
}

static const upb_msgdef *jsondec_typeurl(jsondec *d, upb_msg *msg,
                                         const upb_msgdef *m) {
  const upb_fielddef *type_url_f = upb_msgdef_itof(m, 1);
  const upb_msgdef *type_m;
  upb_strview type_url = jsondec_string(d);
  const char *end = type_url.data + type_url.size;
  const char *ptr = end;
  upb_msgval val;

  val.str_val = type_url;
  upb_msg_set(msg, type_url_f, val, d->arena);

  /* Find message name after the last '/' */
  while (ptr > type_url.data && *--ptr != '/') {}

  if (ptr == type_url.data || ptr == end) {
    jsondec_err(d, "Type url must have at least one '/' and non-empty host");
  }

  ptr++;
  type_m = upb_symtab_lookupmsg2(d->any_pool, ptr, end - ptr);

  if (!type_m) {
    jsondec_err(d, "Type was not found");
  }

  return type_m;
}

static void jsondec_any(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  /* string type_url = 1;
   * bytes value = 2; */
  const upb_fielddef *value_f = upb_msgdef_itof(m, 2);
  upb_msg *any_msg;
  const upb_msgdef *any_m = NULL;
  const char *pre_type_data = NULL;
  const char *pre_type_end = NULL;
  upb_msgval encoded;

  jsondec_objstart(d);

  /* Scan looking for "@type", which is not necessarily first. */
  while (!any_m && jsondec_objnext(d)) {
    const char *start = d->ptr;
    upb_strview name = jsondec_string(d);
    jsondec_entrysep(d);
    if (jsondec_streql(name, "@type")) {
      any_m = jsondec_typeurl(d, msg, m);
      if (pre_type_data) {
        pre_type_end = start;
        while (*pre_type_end != ',') pre_type_end--;
      }
    } else {
      if (!pre_type_data) pre_type_data = start;
      jsondec_skipval(d);
    }
  }

  if (!any_m) {
    jsondec_err(d, "Any object didn't contain a '@type' field");
  }

  any_msg = upb_msg_new(any_m, d->arena);

  if (pre_type_data) {
    size_t len = pre_type_end - pre_type_data + 1;
    char *tmp = upb_arena_malloc(d->arena, len);
    const char *saved_ptr = d->ptr;
    const char *saved_end = d->end;
    memcpy(tmp, pre_type_data, len - 1);
    tmp[len - 1] = '}';
    d->ptr = tmp;
    d->end = tmp + len;
    d->is_first = true;
    while (jsondec_objnext(d)) {
      jsondec_anyfield(d, any_msg, any_m);
    }
    d->ptr = saved_ptr;
    d->end = saved_end;
  }

  while (jsondec_objnext(d)) {
    jsondec_anyfield(d, any_msg, any_m);
  }

  jsondec_objend(d);

  encoded.str_val.data = upb_encode(any_msg, upb_msgdef_layout(any_m), d->arena,
                                    &encoded.str_val.size);
  upb_msg_set(msg, value_f, encoded, d->arena);
}

static void jsondec_wrapper(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  const upb_fielddef *value_f = upb_msgdef_itof(m, 1);
  upb_msgval val = jsondec_value(d, value_f);
  upb_msg_set(msg, value_f, val, d->arena);
}

static void jsondec_wellknown(jsondec *d, upb_msg *msg, const upb_msgdef *m) {
  switch (upb_msgdef_wellknowntype(m)) {
    case UPB_WELLKNOWN_ANY:
      jsondec_any(d, msg, m);
      break;
    case UPB_WELLKNOWN_FIELDMASK:
      jsondec_fieldmask(d, msg, m);
      break;
    case UPB_WELLKNOWN_DURATION:
      jsondec_duration(d, msg, m);
      break;
    case UPB_WELLKNOWN_TIMESTAMP:
      jsondec_timestamp(d, msg, m);
      break;
    case UPB_WELLKNOWN_VALUE:
      jsondec_wellknownvalue(d, msg, m);
      break;
    case UPB_WELLKNOWN_LISTVALUE:
      jsondec_listvalue(d, msg, m);
      break;
    case UPB_WELLKNOWN_STRUCT:
      jsondec_struct(d, msg, m);
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
      jsondec_wrapper(d, msg, m);
      break;
    default:
      UPB_UNREACHABLE();
  }
}

bool upb_json_decode(const char *buf, size_t size, upb_msg *msg,
                     const upb_msgdef *m, const upb_symtab *any_pool,
                     int options, upb_arena *arena, upb_status *status) {
  jsondec d;
  d.ptr = buf;
  d.end = buf + size;
  d.arena = arena;
  d.any_pool = any_pool;
  d.status = status;
  d.options = options;
  d.depth = 64;
  d.line = 1;
  d.line_begin = d.ptr;
  d.debug_field = NULL;
  d.is_first = false;

  if (UPB_SETJMP(d.err)) return false;

  jsondec_tomsg(&d, msg, m);
  return true;
}
