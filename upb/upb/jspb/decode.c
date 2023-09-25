// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/upb/jspb/decode.h"

#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "upb/upb/base/descriptor_constants.h"
#include "upb/upb/base/status.h"
#include "upb/upb/base/string_view.h"
#include "upb/upb/collections/array.h"
#include "upb/upb/collections/map.h"
#include "upb/upb/lex/atoi.h"
#include "upb/upb/lex/unicode.h"
#include "upb/upb/mem/arena.h"
#include "upb/upb/message/internal/accessors.h"
#include "upb/upb/message/message.h"
#include "upb/upb/message/types.h"
#include "upb/upb/mini_table/enum.h"
#include "upb/upb/mini_table/extension_registry.h"
#include "upb/upb/mini_table/field.h"
#include "upb/upb/mini_table/internal/field.h"
#include "upb/upb/mini_table/message.h"

// Must be last.
#include "upb/upb/port/def.inc"

typedef struct {
  const char *ptr, *end;
  upb_Arena* arena; /* TODO: should we have a tmp arena for tmp data? */
  const upb_ExtensionRegistry* extreg;
  int depth;
  upb_Status* status;
  jmp_buf err;
  int line;
  const char* line_begin;
  bool is_first;
  int options;
} jspbdec;

enum { JD_OBJECT, JD_ARRAY, JD_STRING, JD_NUMBER, JD_TRUE, JD_FALSE, JD_NULL };

/* Forward declarations of mutually-recursive functions. */
static upb_MessageValue jspbdec_single_value(jspbdec* d, const upb_MiniTable* m,
                                             const upb_MiniTableField* f);
static upb_MessageValue jspbdec_value(jspbdec* d, const upb_MiniTable* m,
                                      const upb_MiniTableField* f);

static bool jspbdec_streql(upb_StringView str, const char* lit) {
  return str.size == strlen(lit) && memcmp(str.data, lit, str.size) == 0;
}

UPB_NORETURN static void jspbdec_err(jspbdec* d, const char* msg) {
  upb_Status_SetErrorFormat(d->status, "Error parsing JSON @%d:%d: %s", d->line,
                            (int)(d->ptr - d->line_begin), msg);
  UPB_LONGJMP(d->err, 1);
}

UPB_PRINTF(2, 3)
UPB_NORETURN static void jspbdec_errf(jspbdec* d, const char* fmt, ...) {
  va_list argp;
  upb_Status_SetErrorFormat(d->status, "Error parsing JSON @%d:%d: ", d->line,
                            (int)(d->ptr - d->line_begin));
  va_start(argp, fmt);
  upb_Status_VAppendErrorFormat(d->status, fmt, argp);
  va_end(argp);
  UPB_LONGJMP(d->err, 1);
}

// Advances d->ptr until the next non-whitespace character or to the end of
// the buffer.
static void jspbdec_consumews(jspbdec* d) {
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
}

// Advances d->ptr until the next non-whitespace character. Postcondition that
// d->ptr is pointing at a valid non-whitespace character.
static void jspbdec_skipws(jspbdec* d) {
  jspbdec_consumews(d);
  if (d->ptr == d->end) {
    jspbdec_err(d, "Unexpected EOF");
  }
}

static bool jspbdec_tryparsech(jspbdec* d, char ch) {
  if (d->ptr == d->end || *d->ptr != ch) return false;
  d->ptr++;
  return true;
}

static void jspbdec_parselit(jspbdec* d, const char* lit) {
  size_t avail = d->end - d->ptr;
  size_t len = strlen(lit);
  if (avail < len || memcmp(d->ptr, lit, len) != 0) {
    jspbdec_errf(d, "Expected str: '%s'", lit);
  }
  d->ptr += len;
}

static void jspbdec_wsch(jspbdec* d, char ch) {
  jspbdec_skipws(d);
  if (!jspbdec_tryparsech(d, ch)) {
    jspbdec_errf(d, "Expected: '%c' got %c", ch, *d->ptr);
  }
}

static void jspbdec_true(jspbdec* d) { jspbdec_parselit(d, "true"); }
static void jspbdec_false(jspbdec* d) { jspbdec_parselit(d, "false"); }
static void jspbdec_null(jspbdec* d) { jspbdec_parselit(d, "null"); }
static void jspbdec_quote(jspbdec* d) { jspbdec_parselit(d, "\""); }

static void jspbdec_entrysep(jspbdec* d) {
  jspbdec_skipws(d);
  jspbdec_parselit(d, ":");
}

static int jspbdec_rawpeek(jspbdec* d) {
  if (d->ptr == d->end) {
    jspbdec_err(d, "Unexpected EOF");
  }
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
      jspbdec_errf(d, "Unexpected character: '%c'", *d->ptr);
  }
}

static int jspbdec_peek(jspbdec* d) {
  jspbdec_consumews(d);
  return jspbdec_rawpeek(d);
}

/* JSON object/array **********************************************************/

/* These are used like so:
 *
 * jspbdec_objstart(d);
 * while (jspbdec_objnext(d)) {
 *   ...
 * }
 * jspbdec_objend(d) */

static void jspbdec_push(jspbdec* d) {
  if (--d->depth < 0) {
    jspbdec_err(d, "Recursion limit exceeded");
  }
  d->is_first = true;
}

static bool jspbdec_seqnext(jspbdec* d, char end_ch) {
  bool is_first = d->is_first;
  d->is_first = false;
  jspbdec_skipws(d);
  if (*d->ptr == end_ch) return false;
  if (!is_first) jspbdec_parselit(d, ",");
  return true;
}

static void jspbdec_arrstart(jspbdec* d) {
  jspbdec_push(d);
  jspbdec_wsch(d, '[');
}

static void jspbdec_arrend(jspbdec* d) {
  d->depth++;
  jspbdec_wsch(d, ']');
}

static bool jspbdec_arrnext(jspbdec* d) { return jspbdec_seqnext(d, ']'); }

static void jspbdec_objstart(jspbdec* d) {
  jspbdec_push(d);
  jspbdec_wsch(d, '{');
}

static void jspbdec_objend(jspbdec* d) {
  d->depth++;
  jspbdec_wsch(d, '}');
}

static bool jspbdec_objnext(jspbdec* d) {
  if (!jspbdec_seqnext(d, '}')) return false;
  if (jspbdec_peek(d) != JD_STRING) {
    jspbdec_err(d, "Object must start with string");
  }
  return true;
}

/* JSON number ****************************************************************/

static bool jspbdec_tryskipdigits(jspbdec* d) {
  const char* start = d->ptr;

  while (d->ptr < d->end) {
    if (*d->ptr < '0' || *d->ptr > '9') {
      break;
    }
    d->ptr++;
  }

  return d->ptr != start;
}

static void jspbdec_skipdigits(jspbdec* d) {
  if (!jspbdec_tryskipdigits(d)) {
    jspbdec_err(d, "Expected one or more digits");
  }
}

static double jspbdec_number(jspbdec* d) {
  const char* start = d->ptr;

  UPB_ASSERT(jspbdec_rawpeek(d) == JD_NUMBER);

  /* Skip over the syntax of a number, as specified by JSON. */
  if (*d->ptr == '-') d->ptr++;

  if (jspbdec_tryparsech(d, '0')) {
    if (jspbdec_tryskipdigits(d)) {
      jspbdec_err(d, "number cannot have leading zero");
    }
  } else {
    jspbdec_skipdigits(d);
  }

  if (d->ptr == d->end) goto parse;
  if (jspbdec_tryparsech(d, '.')) {
    jspbdec_skipdigits(d);
  }
  if (d->ptr == d->end) goto parse;

  if (*d->ptr == 'e' || *d->ptr == 'E') {
    d->ptr++;
    if (d->ptr == d->end) {
      jspbdec_err(d, "Unexpected EOF in number");
    }
    if (*d->ptr == '+' || *d->ptr == '-') {
      d->ptr++;
    }
    jspbdec_skipdigits(d);
  }

parse:
  errno = 0;
  {
    // Having verified the syntax of a JSON number, use strtod() to parse
    // (strtod() accepts a superset of JSON syntax)
    // Copy the number into a null-terminated scratch buffer since strtod
    // expects a null-terminated string.
    char nullz[64];
    size_t len = d->ptr - start;
    if (len > sizeof(nullz) - 1) {
      jspbdec_err(d, "Max allowed number length 64 characters");
    }
    memcpy(nullz, start, len);
    nullz[len] = '\0';

    char* end;
    double val = strtod(nullz, &end);
    UPB_ASSERT(end - nullz == len);
    if (val > DBL_MAX || val < -DBL_MAX) {
      jspbdec_err(d, "Number out of range");
    }
    return val;
  }
}

/* JSON string ****************************************************************/

static char jspbdec_escape(jspbdec* d) {
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
      jspbdec_err(d, "Invalid escape char");
  }
}

static uint32_t jspbdec_codepoint(jspbdec* d) {
  uint32_t cp = 0;
  const char* end;

  if (d->end - d->ptr < 4) {
    jspbdec_err(d, "EOF inside string");
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
      jspbdec_err(d, "Invalid hex digit");
    }
    cp = (cp << 4) | ch;
  }

  return cp;
}

/* Parses a \uXXXX unicode escape (possibly a surrogate pair). */
static size_t jspbdec_unicode(jspbdec* d, char* out) {
  uint32_t cp = jspbdec_codepoint(d);
  if (upb_Unicode_IsHigh(cp)) {
    /* Surrogate pair: two 16-bit codepoints become a 32-bit codepoint. */
    jspbdec_parselit(d, "\\u");
    uint32_t low = jspbdec_codepoint(d);
    if (!upb_Unicode_IsLow(low)) jspbdec_err(d, "Invalid low surrogate");
    cp = upb_Unicode_FromPair(cp, low);
  } else if (upb_Unicode_IsLow(cp)) {
    jspbdec_err(d, "Unpaired low surrogate");
  }

  /* Write to UTF-8 */
  int bytes = upb_Unicode_ToUTF8(cp, out);
  if (bytes == 0) jspbdec_err(d, "Invalid codepoint");
  return bytes;
}

static void jspbdec_resize(jspbdec* d, char** buf, char** end, char** buf_end) {
  size_t oldsize = *buf_end - *buf;
  size_t len = *end - *buf;
  size_t size = UPB_MAX(8, 2 * oldsize);

  *buf = upb_Arena_Realloc(d->arena, *buf, len, size);
  if (!*buf) jspbdec_err(d, "Out of memory");

  *end = *buf + len;
  *buf_end = *buf + size;
}

static upb_StringView jspbdec_string(jspbdec* d) {
  char* buf = NULL;
  char* end = NULL;
  char* buf_end = NULL;

  jspbdec_skipws(d);
  if (d->ptr == d->end || *d->ptr++ != '"') {
    jspbdec_err(d, "Expected string");
  }

  while (d->ptr < d->end) {
    char ch = *d->ptr++;

    if (end == buf_end) {
      jspbdec_resize(d, &buf, &end, &buf_end);
    }

    switch (ch) {
      case '"': {
        upb_StringView ret;
        ret.data = buf;
        ret.size = end - buf;
        *end = '\0'; /* Needed for possible strtod(). */
        return ret;
      }
      case '\\':
        if (d->ptr == d->end) goto eof;
        if (*d->ptr == 'u') {
          d->ptr++;
          if (buf_end - end < 4) {
            /* Allow space for maximum-sized codepoint (4 bytes). */
            jspbdec_resize(d, &buf, &end, &buf_end);
          }
          end += jspbdec_unicode(d, end);
        } else {
          *end++ = jspbdec_escape(d);
        }
        break;
      default:
        if ((unsigned char)ch < 0x20) {
          jspbdec_err(d, "Invalid char in JSON string");
        }
        *end++ = ch;
        break;
    }
  }

eof:
  jspbdec_err(d, "EOF inside string");
}

/* Base64 decoding for bytes fields. ******************************************/

static unsigned int jspbdec_base64_tablelookup(const char ch) {
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

static char* jspbdec_partialbase64(jspbdec* d, const char* ptr, const char* end,
                                   char* out) {
  int32_t val = -1;

  switch (end - ptr) {
    case 2:
      val = jspbdec_base64_tablelookup(ptr[0]) << 18 |
            jspbdec_base64_tablelookup(ptr[1]) << 12;
      out[0] = val >> 16;
      out += 1;
      break;
    case 3:
      val = jspbdec_base64_tablelookup(ptr[0]) << 18 |
            jspbdec_base64_tablelookup(ptr[1]) << 12 |
            jspbdec_base64_tablelookup(ptr[2]) << 6;
      out[0] = val >> 16;
      out[1] = (val >> 8) & 0xff;
      out += 2;
      break;
  }

  if (val < 0) {
    jspbdec_err(d, "Corrupt base64");
  }

  return out;
}

static size_t jspbdec_base64(jspbdec* d, upb_StringView str) {
  /* We decode in place. This is safe because this is a new buffer (not
   * aliasing the input) and because base64 decoding shrinks 4 bytes into 3. */
  char* out = (char*)str.data;
  const char* ptr = str.data;
  const char* end = ptr + str.size;
  const char* end4 = ptr + (str.size & -4); /* Round down to multiple of 4. */

  for (; ptr < end4; ptr += 4, out += 3) {
    int val = jspbdec_base64_tablelookup(ptr[0]) << 18 |
              jspbdec_base64_tablelookup(ptr[1]) << 12 |
              jspbdec_base64_tablelookup(ptr[2]) << 6 |
              jspbdec_base64_tablelookup(ptr[3]) << 0;

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
    out = jspbdec_partialbase64(d, ptr, end, out);
  }

  return out - str.data;
}

/* Low-level integer parsing **************************************************/

static const char* jspbdec_buftouint64(jspbdec* d, const char* ptr,
                                       const char* end, uint64_t* val) {
  const char* out = upb_BufToUint64(ptr, end, val);
  if (!out) jspbdec_err(d, "Integer overflow");
  return out;
}

static const char* jspbdec_buftoint64(jspbdec* d, const char* ptr,
                                      const char* end, int64_t* val,
                                      bool* is_neg) {
  const char* out = upb_BufToInt64(ptr, end, val, is_neg);
  if (!out) jspbdec_err(d, "Integer overflow");
  return out;
}

static uint64_t jspbdec_strtouint64(jspbdec* d, upb_StringView str) {
  const char* end = str.data + str.size;
  uint64_t ret;
  if (jspbdec_buftouint64(d, str.data, end, &ret) != end) {
    jspbdec_err(d, "Non-number characters in quoted integer");
  }
  return ret;
}

static int64_t jspbdec_strtoint64(jspbdec* d, upb_StringView str) {
  const char* end = str.data + str.size;
  int64_t ret;
  if (jspbdec_buftoint64(d, str.data, end, &ret, NULL) != end) {
    jspbdec_err(d, "Non-number characters in quoted integer");
  }
  return ret;
}

/* Primitive value types ******************************************************/

/* Parse INT32 or INT64 value. */
static upb_MessageValue jspbdec_int(jspbdec* d, const upb_MiniTableField* f) {
  upb_MessageValue val;

  switch (jspbdec_peek(d)) {
    case JD_NUMBER: {
      double dbl = jspbdec_number(d);

      if (dbl > 9223372036854774784.0 || dbl < -9223372036854775808.0) {
        jspbdec_err(d, "JSON number is out of range.");
      }
      val.int64_val = dbl; /* must be guarded, overflow here is UB */
      if (val.int64_val != dbl) {
        jspbdec_errf(d, "JSON number was not integral (%f != %" PRId64 ")", dbl,
                     val.int64_val);
      }
      break;
    }
    case JD_STRING: {
      upb_StringView str = jspbdec_string(d);
      val.int64_val = jspbdec_strtoint64(d, str);
      break;
    }
    default:
      jspbdec_err(d, "Expected number or string");
  }

  if (upb_MiniTableField_CType(f) == kUpb_CType_Int32 ||
      upb_MiniTableField_CType(f) == kUpb_CType_Enum) {
    if (val.int64_val > INT32_MAX || val.int64_val < INT32_MIN) {
      jspbdec_err(d, "Integer out of range.");
    }
    val.int32_val = (int32_t)val.int64_val;
  }

  return val;
}

/* Parse UINT32 or UINT64 value. */
static upb_MessageValue jspbdec_uint(jspbdec* d, const upb_MiniTableField* f) {
  upb_MessageValue val = {0};

  switch (jspbdec_peek(d)) {
    case JD_NUMBER: {
      double dbl = jspbdec_number(d);
      if (dbl > 18446744073709549568.0 || dbl < 0) {
        jspbdec_err(d, "JSON number is out of range.");
      }
      val.uint64_val = dbl; /* must be guarded, overflow here is UB */
      if (val.uint64_val != dbl) {
        jspbdec_errf(d, "JSON number was not integral (%f != %" PRIu64 ")", dbl,
                     val.uint64_val);
      }
      break;
    }
    case JD_STRING: {
      upb_StringView str = jspbdec_string(d);
      val.uint64_val = jspbdec_strtouint64(d, str);
      break;
    }
    default:
      jspbdec_err(d, "Expected number or string");
  }

  if (upb_MiniTableField_CType(f) == kUpb_CType_UInt32) {
    if (val.uint64_val > UINT32_MAX) {
      jspbdec_err(d, "Integer out of range.");
    }
    val.uint32_val = (uint32_t)val.uint64_val;
  }

  return val;
}

/* Parse DOUBLE or FLOAT value. */
static upb_MessageValue jspbdec_double(jspbdec* d,
                                       const upb_MiniTableField* f) {
  upb_StringView str;
  upb_MessageValue val = {0};

  switch (jspbdec_peek(d)) {
    case JD_NUMBER:
      val.double_val = jspbdec_number(d);
      break;
    case JD_STRING:
      str = jspbdec_string(d);
      if (jspbdec_streql(str, "NaN")) {
        val.double_val = NAN;
      } else if (jspbdec_streql(str, "Infinity")) {
        val.double_val = INFINITY;
      } else if (jspbdec_streql(str, "-Infinity")) {
        val.double_val = -INFINITY;
      } else {
        val.double_val = strtod(str.data, NULL);
      }
      break;
    default:
      jspbdec_err(d, "Expected number or string");
  }

  if (upb_MiniTableField_CType(f) == kUpb_CType_Float) {
    float f = val.double_val;
    if (val.double_val != INFINITY && val.double_val != -INFINITY) {
      if (f == INFINITY || f == -INFINITY) jspbdec_err(d, "Float out of range");
    }
    val.float_val = f;
  }

  return val;
}

/* Parse STRING or BYTES value. */
static upb_MessageValue jspbdec_strfield(jspbdec* d,
                                         const upb_MiniTableField* f) {
  upb_MessageValue val;
  val.str_val = jspbdec_string(d);
  if (upb_MiniTableField_CType(f) == kUpb_CType_Bytes) {
    val.str_val.size = jspbdec_base64(d, val.str_val);
  }
  return val;
}

static upb_MessageValue jspbdec_enum(jspbdec* d, const upb_MiniTable* m,
                                     const upb_MiniTableField* f) {
  upb_MessageValue val = jspbdec_int(d, f);

  UPB_ASSERT(upb_MiniTableField_CType(f) == kUpb_CType_Enum);

  if (upb_MiniTableField_IsClosedEnum(f)) {
    const upb_MiniTableEnum* e = upb_MiniTable_GetSubEnumTable(m, f);
    if (!e) {
      jspbdec_err(d, "Missing MiniTableEnum");
    }
    if (!upb_MiniTableEnum_CheckValue(e, val.int32_val)) {
      val.int32_val = 0;
    }
  }
  return val;
}

static upb_MessageValue jspbdec_bool(jspbdec* d, const upb_MiniTableField* f) {
  upb_MessageValue val;

  switch (jspbdec_peek(d)) {
    case JD_TRUE:
      val.bool_val = true;
      jspbdec_true(d);
      break;
    case JD_FALSE:
      val.bool_val = false;
      jspbdec_false(d);
      break;
    case JD_NUMBER:
      val.bool_val = jspbdec_number(d) != 0;
      break;
    default:
      jspbdec_err(d, "Expected one of: number, true, false");
  }

  return val;
}

/* Composite types (array/message/map) ****************************************/

static upb_Array* jspbdec_array(jspbdec* d, const upb_MiniTable* m,
                                const upb_MiniTableField* f) {
  _upb_MiniTableField_CheckIsArray(f);

  upb_Array* arr = upb_Array_New(d->arena, upb_MiniTableField_CType(f));
  if (!arr) {
    jspbdec_err(d, "Failed to allocate array");
  }
  jspbdec_arrstart(d);
  while (jspbdec_arrnext(d)) {
    upb_MessageValue elem = jspbdec_single_value(d, m, f);
    upb_Array_Append(arr, elem, d->arena);
  }
  jspbdec_arrend(d);
  return arr;
}

static upb_Map* jspbdec_map(jspbdec* d, const upb_MiniTable* m,
                            const upb_MiniTableField* f) {
  _upb_MiniTableField_CheckIsMap(f);

  const upb_MiniTable* entry = upb_MiniTable_GetSubMessageTable(m, f);
  const upb_MiniTableField* key_f = upb_MiniTable_FindFieldByNumber(entry, 1);
  const upb_MiniTableField* val_f = upb_MiniTable_FindFieldByNumber(entry, 2);

  upb_Map* map = upb_Map_New(d->arena, upb_MiniTableField_CType(key_f),
                             upb_MiniTableField_CType(val_f));
  // Maps are represented as an array of [k, v] array-pairs.
  jspbdec_arrstart(d);
  while (jspbdec_arrnext(d)) {
    upb_MessageValue key, val;

    jspbdec_arrstart(d);

    if (!jspbdec_arrnext(d)) {
      jspbdec_err(d, "Key-value pairs must have two values (had 0)");
    }
    key = jspbdec_value(d, m, key_f);

    if (!jspbdec_arrnext(d)) {
      jspbdec_err(d, "Key-value pairs must have two values (had 1)");
    }
    val = jspbdec_value(d, m, val_f);

    jspbdec_arrend(d);

    upb_Map_Set(map, key, val, d->arena);
  }
  jspbdec_arrend(d);
  return map;
}

static void jspbdec_add_sparse_field(jspbdec* d, upb_Message* msg,
                                     const upb_MiniTable* m) {
  double tmp;
  uint32_t field_number;
  const upb_MiniTableField* f;
  upb_MessageValue val;

  // The field key is a quoted integer.
  jspbdec_quote(d);
  if (jspbdec_rawpeek(d) != JD_NUMBER) {
    jspbdec_err(d, "Non-integer field key");
  }
  // TODO: Parse int32 directly instead of parsing double and casting back.
  tmp = jspbdec_number(d);
  if (tmp > INT32_MAX || tmp < 1) {
    jspbdec_err(d, "Integer out of range for field number");
  }
  field_number = (uint32_t)tmp;
  jspbdec_quote(d);

  jspbdec_entrysep(d);

  f = upb_MiniTable_FindFieldByNumber(m, field_number);

  if (!f) {
    // TODO: Put in unknown fields.
    jspbdec_errf(d, "Unknown field hit");
  }

  if (jspbdec_peek(d) == JD_NULL) {
    // TODO: See if null as a value in the sparse object is legal in appsjspb,
    // drop this check if not.
    jspbdec_null(d);
    return;
  }

  val = jspbdec_value(d, m, f);
  _upb_Message_SetField(msg, f, &val, d->arena);
}

static void jspbdesc_msg_sparse(jspbdec* d, upb_Message* msg,
                                const upb_MiniTable* m) {
  jspbdec_objstart(d);
  while (jspbdec_objnext(d)) {
    jspbdec_add_sparse_field(d, msg, m);
  }
  jspbdec_objend(d);
}

static void jspbdec_msg_dense(jspbdec* d, upb_Message* msg,
                              const upb_MiniTable* m) {
  // jspb messages are represented as json arrays. That array first is 0 or more
  // fields densely (array index + 1 is the corresponding field number), then
  // optionally 0 or 1 objects sparsely (object keys are field numbers).
  const upb_MiniTableField* f;
  upb_MessageValue val;
  uint32_t field_number = 0;

  jspbdec_arrstart(d);
  while (jspbdec_arrnext(d)) {
    ++field_number;

    switch (jspbdec_peek(d)) {
      case JD_NULL:
        jspbdec_null(d);  // Continue past any null fields.
        continue;
      case JD_OBJECT:
        jspbdesc_msg_sparse(d, msg, m);
        goto end;  // Sparse representation is only allowed in last position.
      default:
        f = upb_MiniTable_FindFieldByNumber(m, field_number);
        if (!f) {
          // TODO: Put into unknown fields.
          jspbdec_err(d, "Saw a non-null value in an unknown slot");
        }
        val = jspbdec_value(d, m, f);
        _upb_Message_SetField(msg, f, &val, d->arena);
    }
  }

end:
  jspbdec_arrend(d);
}

static upb_MessageValue jspbdec_msg(jspbdec* d, const upb_MiniTable* m,
                                    const upb_MiniTableField* f) {
  upb_Message* msg;
  upb_MessageValue val;
  const upb_MiniTable* sub = upb_MiniTable_GetSubMessageTable(m, f);
  if (!sub) {
    jspbdec_err(d, "Field doesn't have a submessagetable");
  }
  msg = upb_Message_New(sub, d->arena);
  jspbdec_msg_dense(d, msg, sub);
  val.msg_val = msg;
  return val;
}

static upb_MessageValue jspbdec_value(jspbdec* d, const upb_MiniTable* m,
                                      const upb_MiniTableField* f) {
  upb_MessageValue val;

  if (upb_FieldMode_Get(f) == kUpb_FieldMode_Map) {
    val.map_val = jspbdec_map(d, m, f);
  } else if (upb_FieldMode_Get(f) == kUpb_FieldMode_Array) {
    val.array_val = jspbdec_array(d, m, f);
  } else if (upb_IsSubMessage(f)) {
    const upb_MiniTable* sub = upb_MiniTable_GetSubMessageTable(m, f);
    if (!sub) {
      jspbdec_err(d, "failed to get submessage table");
    }

    upb_Message* submsg = upb_Message_New(sub, d->arena);
    if (!submsg) {
      jspbdec_err(d, "Failed to allocate submsg");
    }
    jspbdec_msg_dense(d, submsg, sub);
    val.msg_val = submsg;
  } else {
    val = jspbdec_single_value(d, m, f);
  }

  return val;
}

static upb_MessageValue jspbdec_single_value(jspbdec* d, const upb_MiniTable* m,
                                             const upb_MiniTableField* f) {
  switch (upb_MiniTableField_CType(f)) {
    case kUpb_CType_Bool:
      return jspbdec_bool(d, f);
    case kUpb_CType_Float:
    case kUpb_CType_Double:
      return jspbdec_double(d, f);
    case kUpb_CType_UInt32:
    case kUpb_CType_UInt64:
      return jspbdec_uint(d, f);
    case kUpb_CType_Int32:
    case kUpb_CType_Int64:
      return jspbdec_int(d, f);
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return jspbdec_strfield(d, f);
    case kUpb_CType_Enum:
      return jspbdec_enum(d, m, f);
    case kUpb_CType_Message:
      return jspbdec_msg(d, m, f);
    default:
      UPB_UNREACHABLE();
  }
}

static bool upb_JspbDecoder_Decode(jspbdec* d, upb_Message* msg,
                                   const upb_MiniTable* m) {
  if (UPB_SETJMP(d->err)) {
    return false;
  }

  jspbdec_msg_dense(d, msg, m);

  // Consume any trailing whitespace before seeing if we read the entire input.
  jspbdec_consumews(d);
  return d->ptr == d->end;
}

bool upb_JspbDecode(const char* buf, size_t size, upb_Message* msg,
                    const upb_MiniTable* m, const upb_ExtensionRegistry* extreg,
                    int options, upb_Arena* arena, upb_Status* status) {
  jspbdec d;

  if (size == 0) return false;

  d.ptr = buf;
  d.end = buf + size;
  d.arena = arena;
  d.extreg = extreg;
  d.status = status;
  d.options = options;
  d.depth = 64;
  d.line = 1;
  d.line_begin = d.ptr;
  d.is_first = false;

  return upb_JspbDecoder_Decode(&d, msg, m);
}
