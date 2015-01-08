/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2014 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A parser that uses the Ragel State Machine Compiler to generate
 * the finite automata.
 *
 * Ragel only natively handles regular languages, but we can manually
 * program it a bit to handle context-free languages like JSON, by using
 * the "fcall" and "fret" constructs.
 *
 * This parser can handle the basics, but needs several things to be fleshed
 * out:
 *
 * - handling of unicode escape sequences (including high surrogate pairs).
 * - properly check and report errors for unknown fields, stack overflow,
 *   improper array nesting (or lack of nesting).
 * - handling of base64 sequences with padding characters.
 * - handling of push-back (non-success returns from sink functions).
 * - handling of keys/escape-sequences/etc that span input buffers.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "upb/json/parser.h"

#define PARSER_CHECK_RETURN(x) if (!(x)) return false

// Used to signal that a capture has been suspended.
static char suspend_capture;

static upb_selector_t getsel_for_handlertype(upb_json_parser *p,
                                             upb_handlertype_t type) {
  upb_selector_t sel;
  bool ok = upb_handlers_getselector(p->top->f, type, &sel);
  UPB_ASSERT_VAR(ok, ok);
  return sel;
}

static upb_selector_t parser_getsel(upb_json_parser *p) {
  return getsel_for_handlertype(
      p, upb_handlers_getprimitivehandlertype(p->top->f));
}

static bool check_stack(upb_json_parser *p) {
  if ((p->top + 1) == p->limit) {
    upb_status_seterrmsg(p->status, "Nesting too deep");
    return false;
  }

  return true;
}

// There are GCC/Clang built-ins for overflow checking which we could start
// using if there was any performance benefit to it.

static bool checked_add(size_t a, size_t b, size_t *c) {
  if (SIZE_MAX - a < b) return false;
  *c = a + b;
  return true;
}

static size_t saturating_multiply(size_t a, size_t b) {
  // size_t is unsigned, so this is defined behavior even on overflow.
  size_t ret = a * b;
  if (b != 0 && ret / b != a) {
    ret = SIZE_MAX;
  }
  return ret;
}


/* Base64 decoding ************************************************************/

// TODO(haberman): make this streaming.

static const signed char b64table[] = {
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      62/*+*/, -1,      -1,      -1,      63/*/ */,
  52/*0*/, 53/*1*/, 54/*2*/, 55/*3*/, 56/*4*/, 57/*5*/, 58/*6*/, 59/*7*/,
  60/*8*/, 61/*9*/, -1,      -1,      -1,      -1,      -1,      -1,
  -1,       0/*A*/,  1/*B*/,  2/*C*/,  3/*D*/,  4/*E*/,  5/*F*/,  6/*G*/,
  07/*H*/,  8/*I*/,  9/*J*/, 10/*K*/, 11/*L*/, 12/*M*/, 13/*N*/, 14/*O*/,
  15/*P*/, 16/*Q*/, 17/*R*/, 18/*S*/, 19/*T*/, 20/*U*/, 21/*V*/, 22/*W*/,
  23/*X*/, 24/*Y*/, 25/*Z*/, -1,      -1,      -1,      -1,      -1,
  -1,      26/*a*/, 27/*b*/, 28/*c*/, 29/*d*/, 30/*e*/, 31/*f*/, 32/*g*/,
  33/*h*/, 34/*i*/, 35/*j*/, 36/*k*/, 37/*l*/, 38/*m*/, 39/*n*/, 40/*o*/,
  41/*p*/, 42/*q*/, 43/*r*/, 44/*s*/, 45/*t*/, 46/*u*/, 47/*v*/, 48/*w*/,
  49/*x*/, 50/*y*/, 51/*z*/, -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1,
  -1,      -1,      -1,      -1,      -1,      -1,      -1,      -1
};

// Returns the table value sign-extended to 32 bits.  Knowing that the upper
// bits will be 1 for unrecognized characters makes it easier to check for
// this error condition later (see below).
int32_t b64lookup(unsigned char ch) { return b64table[ch]; }

// Returns true if the given character is not a valid base64 character or
// padding.
bool nonbase64(unsigned char ch) { return b64lookup(ch) == -1 && ch != '='; }

static bool base64_push(upb_json_parser *p, upb_selector_t sel, const char *ptr,
                        size_t len) {
  const char *limit = ptr + len;
  for (; ptr < limit; ptr += 4) {
    if (limit - ptr < 4) {
      upb_status_seterrf(p->status,
                         "Base64 input for bytes field not a multiple of 4: %s",
                         upb_fielddef_name(p->top->f));
      return false;
    }

    uint32_t val = b64lookup(ptr[0]) << 18 |
                   b64lookup(ptr[1]) << 12 |
                   b64lookup(ptr[2]) << 6  |
                   b64lookup(ptr[3]);

    // Test the upper bit; returns true if any of the characters returned -1.
    if (val & 0x80000000) {
      goto otherchar;
    }

    char output[3];
    output[0] = val >> 16;
    output[1] = (val >> 8) & 0xff;
    output[2] = val & 0xff;
    upb_sink_putstring(&p->top->sink, sel, output, 3, NULL);
  }
  return true;

otherchar:
  if (nonbase64(ptr[0]) || nonbase64(ptr[1]) || nonbase64(ptr[2]) ||
      nonbase64(ptr[3]) ) {
    upb_status_seterrf(p->status,
                       "Non-base64 characters in bytes field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  } if (ptr[2] == '=') {
    // Last group contains only two input bytes, one output byte.
    if (ptr[0] == '=' || ptr[1] == '=' || ptr[3] != '=') {
      goto badpadding;
    }

    uint32_t val = b64lookup(ptr[0]) << 18 |
                   b64lookup(ptr[1]) << 12;

    assert(!(val & 0x80000000));
    char output = val >> 16;
    upb_sink_putstring(&p->top->sink, sel, &output, 1, NULL);
    return true;
  } else {
    // Last group contains only three input bytes, two output bytes.
    if (ptr[0] == '=' || ptr[1] == '=' || ptr[2] == '=') {
      goto badpadding;
    }

    uint32_t val = b64lookup(ptr[0]) << 18 |
                   b64lookup(ptr[1]) << 12 |
                   b64lookup(ptr[2]) << 6;

    char output[2];
    output[0] = val >> 16;
    output[1] = (val >> 8) & 0xff;
    upb_sink_putstring(&p->top->sink, sel, output, 2, NULL);
    return true;
  }

badpadding:
  upb_status_seterrf(p->status,
                     "Incorrect base64 padding for field: %s (%.*s)",
                     upb_fielddef_name(p->top->f),
                     4, ptr);
  return false;
}


/* Accumulate buffer **********************************************************/

// Functionality for accumulating a buffer.
//
// Some parts of the parser need an entire value as a contiguous string.  For
// example, to look up a member name in a hash table, or to turn a string into
// a number, the relevant library routines need the input string to be in
// contiguous memory, even if the value spanned two or more buffers in the
// input.  These routines handle that.
//
// In the common case we can just point to the input buffer to get this
// contiguous string and avoid any actual copy.  So we optimistically begin
// this way.  But there are a few cases where we must instead copy into a
// separate buffer:
//
//   1. The string was not contiguous in the input (it spanned buffers).
//
//   2. The string included escape sequences that need to be interpreted to get
//      the true value in a contiguous buffer.

static void assert_accumulate_empty(upb_json_parser *p) {
  UPB_UNUSED(p);
  assert(p->accumulated == NULL);
  assert(p->accumulated_len == 0);
}

static void accumulate_clear(upb_json_parser *p) {
  p->accumulated = NULL;
  p->accumulated_len = 0;
}

// Used internally by accumulate_append().
static bool accumulate_realloc(upb_json_parser *p, size_t need) {
  size_t new_size = UPB_MAX(p->accumulate_buf_size, 128);
  while (new_size < need) {
    new_size = saturating_multiply(new_size, 2);
  }

  void *mem = realloc(p->accumulate_buf, new_size);
  if (!mem) {
    upb_status_seterrmsg(p->status, "Out of memory allocating buffer.");
    return false;
  }

  p->accumulate_buf = mem;
  p->accumulate_buf_size = new_size;
  return true;
}

// Logically appends the given data to the append buffer.
// If "can_alias" is true, we will try to avoid actually copying, but the buffer
// must be valid until the next accumulate_append() call (if any).
static bool accumulate_append(upb_json_parser *p, const char *buf, size_t len,
                              bool can_alias) {
  if (!p->accumulated && can_alias) {
    p->accumulated = buf;
    p->accumulated_len = len;
    return true;
  }

  size_t need;
  if (!checked_add(p->accumulated_len, len, &need)) {
    upb_status_seterrmsg(p->status, "Integer overflow.");
    return false;
  }

  if (need > p->accumulate_buf_size && !accumulate_realloc(p, need)) {
    return false;
  }

  if (p->accumulated != p->accumulate_buf) {
    memcpy(p->accumulate_buf, p->accumulated, p->accumulated_len);
    p->accumulated = p->accumulate_buf;
  }

  memcpy(p->accumulate_buf + p->accumulated_len, buf, len);
  p->accumulated_len += len;
  return true;
}

// Returns a pointer to the data accumulated since the last accumulate_clear()
// call, and writes the length to *len.  This with point either to the input
// buffer or a temporary accumulate buffer.
static const char *accumulate_getptr(upb_json_parser *p, size_t *len) {
  assert(p->accumulated);
  *len = p->accumulated_len;
  return p->accumulated;
}


/* Mult-part text data ********************************************************/

// When we have text data in the input, it can often come in multiple segments.
// For example, there may be some raw string data followed by an escape
// sequence.  The two segments are processed with different logic.  Also buffer
// seams in the input can cause multiple segments.
//
// As we see segments, there are two main cases for how we want to process them:
//
//  1. we want to push the captured input directly to string handlers.
//
//  2. we need to accumulate all the parts into a contiguous buffer for further
//     processing (field name lookup, string->number conversion, etc).

// This is the set of states for p->multipart_state.
enum {
  // We are not currently processing multipart data.
  MULTIPART_INACTIVE = 0,

  // We are processing multipart data by accumulating it into a contiguous
  // buffer.
  MULTIPART_ACCUMULATE = 1,

  // We are processing multipart data by pushing each part directly to the
  // current string handlers.
  MULTIPART_PUSHEAGERLY = 2
};

// Start a multi-part text value where we accumulate the data for processing at
// the end.
static void multipart_startaccum(upb_json_parser *p) {
  assert_accumulate_empty(p);
  assert(p->multipart_state == MULTIPART_INACTIVE);
  p->multipart_state = MULTIPART_ACCUMULATE;
}

// Start a multi-part text value where we immediately push text data to a string
// value with the given selector.
static void multipart_start(upb_json_parser *p, upb_selector_t sel) {
  assert_accumulate_empty(p);
  assert(p->multipart_state == MULTIPART_INACTIVE);
  p->multipart_state = MULTIPART_PUSHEAGERLY;
  p->string_selector = sel;
}

static bool multipart_text(upb_json_parser *p, const char *buf, size_t len,
                           bool can_alias) {
  switch (p->multipart_state) {
    case MULTIPART_INACTIVE:
      upb_status_seterrmsg(
          p->status, "Internal error: unexpected state MULTIPART_INACTIVE");
      return false;

    case MULTIPART_ACCUMULATE:
      if (!accumulate_append(p, buf, len, can_alias)) {
        return false;
      }
      break;

    case MULTIPART_PUSHEAGERLY: {
      const upb_bufhandle *handle = can_alias ? p->handle : NULL;
      upb_sink_putstring(&p->top->sink, p->string_selector, buf, len, handle);
      break;
    }
  }

  return true;
}

// Note: this invalidates the accumulate buffer!  Call only after reading its
// contents.
static void multipart_end(upb_json_parser *p) {
  assert(p->multipart_state != MULTIPART_INACTIVE);
  p->multipart_state = MULTIPART_INACTIVE;
  accumulate_clear(p);
}


/* Input capture **************************************************************/

// Functionality for capturing a region of the input as text.  Gracefully
// handles the case where a buffer seam occurs in the middle of the captured
// region.

static void capture_begin(upb_json_parser *p, const char *ptr) {
  assert(p->multipart_state != MULTIPART_INACTIVE);
  assert(p->capture == NULL);
  p->capture = ptr;
}

static bool capture_end(upb_json_parser *p, const char *ptr) {
  assert(p->capture);
  if (multipart_text(p, p->capture, ptr - p->capture, true)) {
    p->capture = NULL;
    return true;
  } else {
    return false;
  }
}

// This is called at the end of each input buffer (ie. when we have hit a
// buffer seam).  If we are in the middle of capturing the input, this
// processes the unprocessed capture region.
static void capture_suspend(upb_json_parser *p, const char **ptr) {
  if (!p->capture) return;

  if (multipart_text(p, p->capture, *ptr - p->capture, false)) {
    // We use this as a signal that we were in the middle of capturing, and
    // that capturing should resume at the beginning of the next buffer.
    //
    // We can't use *ptr here, because we have no guarantee that this pointer
    // will be valid when we resume (if the underlying memory is freed, then
    // using the pointer at all, even to compare to NULL, is likely undefined
    // behavior).
    p->capture = &suspend_capture;
  } else {
    // Need to back up the pointer to the beginning of the capture, since
    // we were not able to actually preserve it.
    *ptr = p->capture;
  }
}

static void capture_resume(upb_json_parser *p, const char *ptr) {
  if (p->capture) {
    assert(p->capture == &suspend_capture);
    p->capture = ptr;
  }
}


/* Callbacks from the parser **************************************************/

// These are the functions called directly from the parser itself.
// We define these in the same order as their declarations in the parser.

static char escape_char(char in) {
  switch (in) {
    case 'r': return '\r';
    case 't': return '\t';
    case 'n': return '\n';
    case 'f': return '\f';
    case 'b': return '\b';
    case '/': return '/';
    case '"': return '"';
    case '\\': return '\\';
    default:
      assert(0);
      return 'x';
  }
}

static bool escape(upb_json_parser *p, const char *ptr) {
  char ch = escape_char(*ptr);
  return multipart_text(p, &ch, 1, false);
}

static void start_hex(upb_json_parser *p) {
  p->digit = 0;
}

static void hexdigit(upb_json_parser *p, const char *ptr) {
  char ch = *ptr;

  p->digit <<= 4;

  if (ch >= '0' && ch <= '9') {
    p->digit += (ch - '0');
  } else if (ch >= 'a' && ch <= 'f') {
    p->digit += ((ch - 'a') + 10);
  } else {
    assert(ch >= 'A' && ch <= 'F');
    p->digit += ((ch - 'A') + 10);
  }
}

static bool end_hex(upb_json_parser *p) {
  uint32_t codepoint = p->digit;

  // emit the codepoint as UTF-8.
  char utf8[3]; // support \u0000 -- \uFFFF -- need only three bytes.
  int length = 0;
  if (codepoint <= 0x7F) {
    utf8[0] = codepoint;
    length = 1;
  } else if (codepoint <= 0x07FF) {
    utf8[1] = (codepoint & 0x3F) | 0x80;
    codepoint >>= 6;
    utf8[0] = (codepoint & 0x1F) | 0xC0;
    length = 2;
  } else /* codepoint <= 0xFFFF */ {
    utf8[2] = (codepoint & 0x3F) | 0x80;
    codepoint >>= 6;
    utf8[1] = (codepoint & 0x3F) | 0x80;
    codepoint >>= 6;
    utf8[0] = (codepoint & 0x0F) | 0xE0;
    length = 3;
  }
  // TODO(haberman): Handle high surrogates: if codepoint is a high surrogate
  // we have to wait for the next escape to get the full code point).

  return multipart_text(p, utf8, length, false);
}

static void start_text(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_text(upb_json_parser *p, const char *ptr) {
  return capture_end(p, ptr);
}

static void start_number(upb_json_parser *p, const char *ptr) {
  multipart_startaccum(p);
  capture_begin(p, ptr);
}

static bool end_number(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }

  // strtol() and friends unfortunately do not support specifying the length of
  // the input string, so we need to force a copy into a NULL-terminated buffer.
  if (!multipart_text(p, "\0", 1, false)) {
    return false;
  }

  size_t len;
  const char *buf = accumulate_getptr(p, &len);
  const char *myend = buf + len - 1;  // One for NULL.
  char *end;

  switch (upb_fielddef_type(p->top->f)) {
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32: {
      long val = strtol(p->accumulated, &end, 0);
      if (val > INT32_MAX || val < INT32_MIN || errno == ERANGE || end != myend)
        goto err;
      else
        upb_sink_putint32(&p->top->sink, parser_getsel(p), val);
      break;
    }
    case UPB_TYPE_INT64: {
      long long val = strtoll(p->accumulated, &end, 0);
      if (val > INT64_MAX || val < INT64_MIN || errno == ERANGE || end != myend)
        goto err;
      else
        upb_sink_putint64(&p->top->sink, parser_getsel(p), val);
      break;
    }
    case UPB_TYPE_UINT32: {
      unsigned long val = strtoul(p->accumulated, &end, 0);
      if (val > UINT32_MAX || errno == ERANGE || end != myend)
        goto err;
      else
        upb_sink_putuint32(&p->top->sink, parser_getsel(p), val);
      break;
    }
    case UPB_TYPE_UINT64: {
      unsigned long long val = strtoull(p->accumulated, &end, 0);
      if (val > UINT64_MAX || errno == ERANGE || end != myend)
        goto err;
      else
        upb_sink_putuint64(&p->top->sink, parser_getsel(p), val);
      break;
    }
    case UPB_TYPE_DOUBLE: {
      double val = strtod(p->accumulated, &end);
      if (errno == ERANGE || end != myend)
        goto err;
      else
        upb_sink_putdouble(&p->top->sink, parser_getsel(p), val);
      break;
    }
    case UPB_TYPE_FLOAT: {
      float val = strtof(p->accumulated, &end);
      if (errno == ERANGE || end != myend)
        goto err;
      else
        upb_sink_putfloat(&p->top->sink, parser_getsel(p), val);
      break;
    }
    default:
      assert(false);
  }

  multipart_end(p);
  return true;

err:
  upb_status_seterrf(p->status, "error parsing number: %s", buf);
  multipart_end(p);
  return false;
}

static bool parser_putbool(upb_json_parser *p, bool val) {
  if (upb_fielddef_type(p->top->f) != UPB_TYPE_BOOL) {
    upb_status_seterrf(p->status,
                       "Boolean value specified for non-bool field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }

  bool ok = upb_sink_putbool(&p->top->sink, parser_getsel(p), val);
  UPB_ASSERT_VAR(ok, ok);
  return true;
}

static bool start_stringval(upb_json_parser *p) {
  assert(p->top->f);

  if (upb_fielddef_isstring(p->top->f)) {
    if (!check_stack(p)) return false;

    // Start a new parser frame: parser frames correspond one-to-one with
    // handler frames, and string events occur in a sub-frame.
    upb_jsonparser_frame *inner = p->top + 1;
    upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
    upb_sink_startstr(&p->top->sink, sel, 0, &inner->sink);
    inner->m = p->top->m;
    inner->f = p->top->f;
    p->top = inner;

    if (upb_fielddef_type(p->top->f) == UPB_TYPE_STRING) {
      // For STRING fields we push data directly to the handlers as it is
      // parsed.  We don't do this yet for BYTES fields, because our base64
      // decoder is not streaming.
      //
      // TODO(haberman): make base64 decoding streaming also.
      multipart_start(p, getsel_for_handlertype(p, UPB_HANDLER_STRING));
      return true;
    } else {
      multipart_startaccum(p);
      return true;
    }
  } else if (upb_fielddef_type(p->top->f) == UPB_TYPE_ENUM) {
    // No need to push a frame -- symbolic enum names in quotes remain in the
    // current parser frame.
    //
    // Enum string values must accumulate so we can look up the value in a table
    // once it is complete.
    multipart_startaccum(p);
    return true;
  } else {
    upb_status_seterrf(p->status,
                       "String specified for non-string/non-enum field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }
}

static bool end_stringval(upb_json_parser *p) {
  bool ok = true;

  switch (upb_fielddef_type(p->top->f)) {
    case UPB_TYPE_BYTES:
      if (!base64_push(p, getsel_for_handlertype(p, UPB_HANDLER_STRING),
                       p->accumulated, p->accumulated_len)) {
        return false;
      }
      // Fall through.

    case UPB_TYPE_STRING: {
      upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
      upb_sink_endstr(&p->top->sink, sel);
      p->top--;
      break;
    }

    case UPB_TYPE_ENUM: {
      // Resolve enum symbolic name to integer value.
      const upb_enumdef *enumdef =
          (const upb_enumdef*)upb_fielddef_subdef(p->top->f);

      size_t len;
      const char *buf = accumulate_getptr(p, &len);

      int32_t int_val = 0;
      ok = upb_enumdef_ntoi(enumdef, buf, len, &int_val);

      if (ok) {
        upb_selector_t sel = parser_getsel(p);
        upb_sink_putint32(&p->top->sink, sel, int_val);
      } else {
        upb_status_seterrf(p->status, "Enum value unknown: '%.*s'", len, buf);
      }

      break;
    }

    default:
      assert(false);
      upb_status_seterrmsg(p->status, "Internal error in JSON decoder");
      ok = false;
      break;
  }

  multipart_end(p);
  return ok;
}

static void start_member(upb_json_parser *p) {
  assert(!p->top->f);
  multipart_startaccum(p);
}

static bool end_member(upb_json_parser *p) {
  assert(!p->top->f);
  size_t len;
  const char *buf = accumulate_getptr(p, &len);

  const upb_fielddef *f = upb_msgdef_ntof(p->top->m, buf, len);

  if (!f) {
    // TODO(haberman): Ignore unknown fields if requested/configured to do so.
    upb_status_seterrf(p->status, "No such field: %.*s\n", (int)len, buf);
    return false;
  }

  p->top->f = f;
  multipart_end(p);

  return true;
}

static void clear_member(upb_json_parser *p) { p->top->f = NULL; }

static bool start_subobject(upb_json_parser *p) {
  assert(p->top->f);

  if (!upb_fielddef_issubmsg(p->top->f)) {
    upb_status_seterrf(p->status,
                       "Object specified for non-message/group field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }

  if (!check_stack(p)) return false;

  upb_jsonparser_frame *inner = p->top + 1;

  upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSUBMSG);
  upb_sink_startsubmsg(&p->top->sink, sel, &inner->sink);
  inner->m = upb_fielddef_msgsubdef(p->top->f);
  inner->f = NULL;
  p->top = inner;

  return true;
}

static void end_subobject(upb_json_parser *p) {
  p->top--;
  upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSUBMSG);
  upb_sink_endsubmsg(&p->top->sink, sel);
}

static bool start_array(upb_json_parser *p) {
  assert(p->top->f);

  if (!upb_fielddef_isseq(p->top->f)) {
    upb_status_seterrf(p->status,
                       "Array specified for non-repeated field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }

  if (!check_stack(p)) return false;

  upb_jsonparser_frame *inner = p->top + 1;
  upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSEQ);
  upb_sink_startseq(&p->top->sink, sel, &inner->sink);
  inner->m = p->top->m;
  inner->f = p->top->f;
  p->top = inner;

  return true;
}

static void end_array(upb_json_parser *p) {
  assert(p->top > p->stack);

  p->top--;
  upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSEQ);
  upb_sink_endseq(&p->top->sink, sel);
}

static void start_object(upb_json_parser *p) {
  upb_sink_startmsg(&p->top->sink);
}

static void end_object(upb_json_parser *p) {
  upb_status status;
  upb_sink_endmsg(&p->top->sink, &status);
}


#define CHECK_RETURN_TOP(x) if (!(x)) goto error


/* The actual parser **********************************************************/

// What follows is the Ragel parser itself.  The language is specified in Ragel
// and the actions call our C functions above.
//
// Ragel has an extensive set of functionality, and we use only a small part of
// it.  There are many action types but we only use a few:
//
//   ">" -- transition into a machine
//   "%" -- transition out of a machine
//   "@" -- transition into a final state of a machine.
//
// "@" transitions are tricky because a machine can transition into a final
// state repeatedly.  But in some cases we know this can't happen, for example
// a string which is delimited by a final '"' can only transition into its
// final state once, when the closing '"' is seen.

%%{
  machine json;

  ws = space*;

  integer  = "0" | /[1-9]/ /[0-9]/*;
  decimal  = "." /[0-9]/+;
  exponent = /[eE]/ /[+\-]/? /[0-9]/+;

  number_machine :=
      ("-"? integer decimal? exponent?)
      <: any >{ fhold; fret; };
  number  = /[0-9\-]/ >{ fhold; fcall number_machine; };

  text =
    /[^\\"]/+
      >{ start_text(parser, p); }
      %{ CHECK_RETURN_TOP(end_text(parser, p)); }
    ;

  unicode_char =
    "\\u"
    /[0-9A-Fa-f]/{4}
      >{ start_hex(parser); }
      ${ hexdigit(parser, p); }
      %{ CHECK_RETURN_TOP(end_hex(parser)); }
    ;

  escape_char  =
    "\\"
    /[rtbfn"\/\\]/
      >{ CHECK_RETURN_TOP(escape(parser, p)); }
    ;

  string_machine :=
    (text | unicode_char | escape_char)**
    '"'
      @{ fhold; fret; }
    ;

  string       = '"' @{ fcall string_machine; } '"';

  value2 = ^(space | "]" | "}") >{ fhold; fcall value_machine; } ;

  member =
    ws
    string
      >{ start_member(parser); }
      @{ CHECK_RETURN_TOP(end_member(parser)); }
    ws ":" ws
    value2
      %{ clear_member(parser); }
    ws;

  object =
    "{"
    ws
      >{ start_object(parser); }
    (member ("," member)*)?
    "}"
      >{ end_object(parser); }
    ;

  element = ws value2 ws;
  array   =
    "["
      >{ CHECK_RETURN_TOP(start_array(parser)); }
    ws
    (element ("," element)*)?
    "]"
      >{ end_array(parser); }
    ;

  value =
    number
      >{ start_number(parser, p); }
      %{ CHECK_RETURN_TOP(end_number(parser, p)); }
    | string
      >{ CHECK_RETURN_TOP(start_stringval(parser)); }
      @{ CHECK_RETURN_TOP(end_stringval(parser)); }
    | "true"
      %{ CHECK_RETURN_TOP(parser_putbool(parser, true)); }
    | "false"
      %{ CHECK_RETURN_TOP(parser_putbool(parser, false)); }
    | "null"
      %{ /* null value */ }
    | object
      >{ CHECK_RETURN_TOP(start_subobject(parser)); }
      %{ end_subobject(parser); }
    | array;

  value_machine :=
    value
    <: any >{ fhold; fret; } ;

  main := ws object ws;
}%%

%% write data;

size_t parse(void *closure, const void *hd, const char *buf, size_t size,
             const upb_bufhandle *handle) {
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  upb_json_parser *parser = closure;
  parser->handle = handle;

  // Variables used by Ragel's generated code.
  int cs = parser->current_state;
  int *stack = parser->parser_stack;
  int top = parser->parser_top;

  const char *p = buf;
  const char *pe = buf + size;

  capture_resume(parser, buf);

  %% write exec;

  if (p != pe) {
    upb_status_seterrf(parser->status, "Parse error at %s\n", p);
  } else {
    capture_suspend(parser, &p);
  }

error:
  // Save parsing state back to parser.
  parser->current_state = cs;
  parser->parser_top = top;

  return p - buf;
}

bool end(void *closure, const void *hd) {
  UPB_UNUSED(closure);
  UPB_UNUSED(hd);
  return true;
}


/* Public API *****************************************************************/

void upb_json_parser_init(upb_json_parser *p, upb_status *status) {
  p->limit = p->stack + UPB_JSON_MAX_DEPTH;
  p->accumulate_buf = NULL;
  p->accumulate_buf_size = 0;
  upb_byteshandler_init(&p->input_handler_);
  upb_byteshandler_setstring(&p->input_handler_, parse, NULL);
  upb_byteshandler_setendstr(&p->input_handler_, end, NULL);
  upb_bytessink_reset(&p->input_, &p->input_handler_, p);
  p->status = status;
}

void upb_json_parser_uninit(upb_json_parser *p) {
  upb_byteshandler_uninit(&p->input_handler_);
  free(p->accumulate_buf);
}

void upb_json_parser_reset(upb_json_parser *p) {
  p->top = p->stack;
  p->top->f = NULL;

  int cs;
  int top;
  // Emit Ragel initialization of the parser.
  %% write init;
  p->current_state = cs;
  p->parser_top = top;
  accumulate_clear(p);
  p->multipart_state = MULTIPART_INACTIVE;
  p->capture = NULL;
}

void upb_json_parser_resetoutput(upb_json_parser *p, upb_sink *sink) {
  upb_json_parser_reset(p);
  upb_sink_reset(&p->top->sink, sink->handlers, sink->closure);
  p->top->m = upb_handlers_msgdef(sink->handlers);
  p->accumulated = NULL;
}

upb_bytessink *upb_json_parser_input(upb_json_parser *p) {
  return &p->input_;
}
