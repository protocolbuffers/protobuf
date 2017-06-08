
#line 1 "upb/json/parser.rl"
/*
** upb::json::Parser (upb_json_parser)
**
** A parser that uses the Ragel State Machine Compiler to generate
** the finite automata.
**
** Ragel only natively handles regular languages, but we can manually
** program it a bit to handle context-free languages like JSON, by using
** the "fcall" and "fret" constructs.
**
** This parser can handle the basics, but needs several things to be fleshed
** out:
**
** - handling of unicode escape sequences (including high surrogate pairs).
** - properly check and report errors for unknown fields, stack overflow,
**   improper array nesting (or lack of nesting).
** - handling of base64 sequences with padding characters.
** - handling of push-back (non-success returns from sink functions).
** - handling of keys/escape-sequences/etc that span input buffers.
*/

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "upb/json/parser.h"

#define UPB_JSON_MAX_DEPTH 64

typedef struct {
  upb_sink sink;

  /* The current message in which we're parsing, and the field whose value we're
   * expecting next. */
  const upb_msgdef *m;
  const upb_fielddef *f;

  /* The table mapping json name to fielddef for this message. */
  upb_strtable *name_table;

  /* We are in a repeated-field context, ready to emit mapentries as
   * submessages. This flag alters the start-of-object (open-brace) behavior to
   * begin a sequence of mapentry messages rather than a single submessage. */
  bool is_map;

  /* We are in a map-entry message context. This flag is set when parsing the
   * value field of a single map entry and indicates to all value-field parsers
   * (subobjects, strings, numbers, and bools) that the map-entry submessage
   * should end as soon as the value is parsed. */
  bool is_mapentry;

  /* If |is_map| or |is_mapentry| is true, |mapfield| refers to the parent
   * message's map field that we're currently parsing. This differs from |f|
   * because |f| is the field in the *current* message (i.e., the map-entry
   * message itself), not the parent's field that leads to this map. */
  const upb_fielddef *mapfield;
} upb_jsonparser_frame;

struct upb_json_parser {
  upb_env *env;
  const upb_json_parsermethod *method;
  upb_bytessink input_;

  /* Stack to track the JSON scopes we are in. */
  upb_jsonparser_frame stack[UPB_JSON_MAX_DEPTH];
  upb_jsonparser_frame *top;
  upb_jsonparser_frame *limit;

  upb_status status;

  /* Ragel's internal parsing stack for the parsing state machine. */
  int current_state;
  int parser_stack[UPB_JSON_MAX_DEPTH];
  int parser_top;

  /* The handle for the current buffer. */
  const upb_bufhandle *handle;

  /* Accumulate buffer.  See details in parser.rl. */
  const char *accumulated;
  size_t accumulated_len;
  char *accumulate_buf;
  size_t accumulate_buf_size;

  /* Multi-part text data.  See details in parser.rl. */
  int multipart_state;
  upb_selector_t string_selector;

  /* Input capture.  See details in parser.rl. */
  const char *capture;

  /* Intermediate result of parsing a unicode escape sequence. */
  uint32_t digit;
};

struct upb_json_parsermethod {
  upb_refcounted base;

  upb_byteshandler input_handler_;

  /* Mainly for the purposes of refcounting, so all the fielddefs we point
   * to stay alive. */
  const upb_msgdef *msg;

  /* Keys are upb_msgdef*, values are upb_strtable (json_name -> fielddef) */
  upb_inttable name_tables;
};

#define PARSER_CHECK_RETURN(x) if (!(x)) return false

/* Used to signal that a capture has been suspended. */
static char suspend_capture;

static upb_selector_t getsel_for_handlertype(upb_json_parser *p,
                                             upb_handlertype_t type) {
  upb_selector_t sel;
  bool ok = upb_handlers_getselector(p->top->f, type, &sel);
  UPB_ASSERT(ok);
  return sel;
}

static upb_selector_t parser_getsel(upb_json_parser *p) {
  return getsel_for_handlertype(
      p, upb_handlers_getprimitivehandlertype(p->top->f));
}

static bool check_stack(upb_json_parser *p) {
  if ((p->top + 1) == p->limit) {
    upb_status_seterrmsg(&p->status, "Nesting too deep");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  return true;
}

static void set_name_table(upb_json_parser *p, upb_jsonparser_frame *frame) {
  upb_value v;
  bool ok = upb_inttable_lookupptr(&p->method->name_tables, frame->m, &v);
  UPB_ASSERT(ok);
  frame->name_table = upb_value_getptr(v);
}

/* There are GCC/Clang built-ins for overflow checking which we could start
 * using if there was any performance benefit to it. */

static bool checked_add(size_t a, size_t b, size_t *c) {
  if (SIZE_MAX - a < b) return false;
  *c = a + b;
  return true;
}

static size_t saturating_multiply(size_t a, size_t b) {
  /* size_t is unsigned, so this is defined behavior even on overflow. */
  size_t ret = a * b;
  if (b != 0 && ret / b != a) {
    ret = SIZE_MAX;
  }
  return ret;
}


/* Base64 decoding ************************************************************/

/* TODO(haberman): make this streaming. */

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

/* Returns the table value sign-extended to 32 bits.  Knowing that the upper
 * bits will be 1 for unrecognized characters makes it easier to check for
 * this error condition later (see below). */
int32_t b64lookup(unsigned char ch) { return b64table[ch]; }

/* Returns true if the given character is not a valid base64 character or
 * padding. */
bool nonbase64(unsigned char ch) { return b64lookup(ch) == -1 && ch != '='; }

static bool base64_push(upb_json_parser *p, upb_selector_t sel, const char *ptr,
                        size_t len) {
  const char *limit = ptr + len;
  for (; ptr < limit; ptr += 4) {
    uint32_t val;
    char output[3];

    if (limit - ptr < 4) {
      upb_status_seterrf(&p->status,
                         "Base64 input for bytes field not a multiple of 4: %s",
                         upb_fielddef_name(p->top->f));
      upb_env_reporterror(p->env, &p->status);
      return false;
    }

    val = b64lookup(ptr[0]) << 18 |
          b64lookup(ptr[1]) << 12 |
          b64lookup(ptr[2]) << 6  |
          b64lookup(ptr[3]);

    /* Test the upper bit; returns true if any of the characters returned -1. */
    if (val & 0x80000000) {
      goto otherchar;
    }

    output[0] = val >> 16;
    output[1] = (val >> 8) & 0xff;
    output[2] = val & 0xff;
    upb_sink_putstring(&p->top->sink, sel, output, 3, NULL);
  }
  return true;

otherchar:
  if (nonbase64(ptr[0]) || nonbase64(ptr[1]) || nonbase64(ptr[2]) ||
      nonbase64(ptr[3]) ) {
    upb_status_seterrf(&p->status,
                       "Non-base64 characters in bytes field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  } if (ptr[2] == '=') {
    uint32_t val;
    char output;

    /* Last group contains only two input bytes, one output byte. */
    if (ptr[0] == '=' || ptr[1] == '=' || ptr[3] != '=') {
      goto badpadding;
    }

    val = b64lookup(ptr[0]) << 18 |
          b64lookup(ptr[1]) << 12;

    UPB_ASSERT(!(val & 0x80000000));
    output = val >> 16;
    upb_sink_putstring(&p->top->sink, sel, &output, 1, NULL);
    return true;
  } else {
    uint32_t val;
    char output[2];

    /* Last group contains only three input bytes, two output bytes. */
    if (ptr[0] == '=' || ptr[1] == '=' || ptr[2] == '=') {
      goto badpadding;
    }

    val = b64lookup(ptr[0]) << 18 |
          b64lookup(ptr[1]) << 12 |
          b64lookup(ptr[2]) << 6;

    output[0] = val >> 16;
    output[1] = (val >> 8) & 0xff;
    upb_sink_putstring(&p->top->sink, sel, output, 2, NULL);
    return true;
  }

badpadding:
  upb_status_seterrf(&p->status,
                     "Incorrect base64 padding for field: %s (%.*s)",
                     upb_fielddef_name(p->top->f),
                     4, ptr);
  upb_env_reporterror(p->env, &p->status);
  return false;
}


/* Accumulate buffer **********************************************************/

/* Functionality for accumulating a buffer.
 *
 * Some parts of the parser need an entire value as a contiguous string.  For
 * example, to look up a member name in a hash table, or to turn a string into
 * a number, the relevant library routines need the input string to be in
 * contiguous memory, even if the value spanned two or more buffers in the
 * input.  These routines handle that.
 *
 * In the common case we can just point to the input buffer to get this
 * contiguous string and avoid any actual copy.  So we optimistically begin
 * this way.  But there are a few cases where we must instead copy into a
 * separate buffer:
 *
 *   1. The string was not contiguous in the input (it spanned buffers).
 *
 *   2. The string included escape sequences that need to be interpreted to get
 *      the true value in a contiguous buffer. */

static void assert_accumulate_empty(upb_json_parser *p) {
  UPB_ASSERT(p->accumulated == NULL);
  UPB_ASSERT(p->accumulated_len == 0);
}

static void accumulate_clear(upb_json_parser *p) {
  p->accumulated = NULL;
  p->accumulated_len = 0;
}

/* Used internally by accumulate_append(). */
static bool accumulate_realloc(upb_json_parser *p, size_t need) {
  void *mem;
  size_t old_size = p->accumulate_buf_size;
  size_t new_size = UPB_MAX(old_size, 128);
  while (new_size < need) {
    new_size = saturating_multiply(new_size, 2);
  }

  mem = upb_env_realloc(p->env, p->accumulate_buf, old_size, new_size);
  if (!mem) {
    upb_status_seterrmsg(&p->status, "Out of memory allocating buffer.");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  p->accumulate_buf = mem;
  p->accumulate_buf_size = new_size;
  return true;
}

/* Logically appends the given data to the append buffer.
 * If "can_alias" is true, we will try to avoid actually copying, but the buffer
 * must be valid until the next accumulate_append() call (if any). */
static bool accumulate_append(upb_json_parser *p, const char *buf, size_t len,
                              bool can_alias) {
  size_t need;

  if (!p->accumulated && can_alias) {
    p->accumulated = buf;
    p->accumulated_len = len;
    return true;
  }

  if (!checked_add(p->accumulated_len, len, &need)) {
    upb_status_seterrmsg(&p->status, "Integer overflow.");
    upb_env_reporterror(p->env, &p->status);
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

/* Returns a pointer to the data accumulated since the last accumulate_clear()
 * call, and writes the length to *len.  This with point either to the input
 * buffer or a temporary accumulate buffer. */
static const char *accumulate_getptr(upb_json_parser *p, size_t *len) {
  UPB_ASSERT(p->accumulated);
  *len = p->accumulated_len;
  return p->accumulated;
}


/* Mult-part text data ********************************************************/

/* When we have text data in the input, it can often come in multiple segments.
 * For example, there may be some raw string data followed by an escape
 * sequence.  The two segments are processed with different logic.  Also buffer
 * seams in the input can cause multiple segments.
 *
 * As we see segments, there are two main cases for how we want to process them:
 *
 *  1. we want to push the captured input directly to string handlers.
 *
 *  2. we need to accumulate all the parts into a contiguous buffer for further
 *     processing (field name lookup, string->number conversion, etc). */

/* This is the set of states for p->multipart_state. */
enum {
  /* We are not currently processing multipart data. */
  MULTIPART_INACTIVE = 0,

  /* We are processing multipart data by accumulating it into a contiguous
   * buffer. */
  MULTIPART_ACCUMULATE = 1,

  /* We are processing multipart data by pushing each part directly to the
   * current string handlers. */
  MULTIPART_PUSHEAGERLY = 2
};

/* Start a multi-part text value where we accumulate the data for processing at
 * the end. */
static void multipart_startaccum(upb_json_parser *p) {
  assert_accumulate_empty(p);
  UPB_ASSERT(p->multipart_state == MULTIPART_INACTIVE);
  p->multipart_state = MULTIPART_ACCUMULATE;
}

/* Start a multi-part text value where we immediately push text data to a string
 * value with the given selector. */
static void multipart_start(upb_json_parser *p, upb_selector_t sel) {
  assert_accumulate_empty(p);
  UPB_ASSERT(p->multipart_state == MULTIPART_INACTIVE);
  p->multipart_state = MULTIPART_PUSHEAGERLY;
  p->string_selector = sel;
}

static bool multipart_text(upb_json_parser *p, const char *buf, size_t len,
                           bool can_alias) {
  switch (p->multipart_state) {
    case MULTIPART_INACTIVE:
      upb_status_seterrmsg(
          &p->status, "Internal error: unexpected state MULTIPART_INACTIVE");
      upb_env_reporterror(p->env, &p->status);
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

/* Note: this invalidates the accumulate buffer!  Call only after reading its
 * contents. */
static void multipart_end(upb_json_parser *p) {
  UPB_ASSERT(p->multipart_state != MULTIPART_INACTIVE);
  p->multipart_state = MULTIPART_INACTIVE;
  accumulate_clear(p);
}


/* Input capture **************************************************************/

/* Functionality for capturing a region of the input as text.  Gracefully
 * handles the case where a buffer seam occurs in the middle of the captured
 * region. */

static void capture_begin(upb_json_parser *p, const char *ptr) {
  UPB_ASSERT(p->multipart_state != MULTIPART_INACTIVE);
  UPB_ASSERT(p->capture == NULL);
  p->capture = ptr;
}

static bool capture_end(upb_json_parser *p, const char *ptr) {
  UPB_ASSERT(p->capture);
  if (multipart_text(p, p->capture, ptr - p->capture, true)) {
    p->capture = NULL;
    return true;
  } else {
    return false;
  }
}

/* This is called at the end of each input buffer (ie. when we have hit a
 * buffer seam).  If we are in the middle of capturing the input, this
 * processes the unprocessed capture region. */
static void capture_suspend(upb_json_parser *p, const char **ptr) {
  if (!p->capture) return;

  if (multipart_text(p, p->capture, *ptr - p->capture, false)) {
    /* We use this as a signal that we were in the middle of capturing, and
     * that capturing should resume at the beginning of the next buffer.
     * 
     * We can't use *ptr here, because we have no guarantee that this pointer
     * will be valid when we resume (if the underlying memory is freed, then
     * using the pointer at all, even to compare to NULL, is likely undefined
     * behavior). */
    p->capture = &suspend_capture;
  } else {
    /* Need to back up the pointer to the beginning of the capture, since
     * we were not able to actually preserve it. */
    *ptr = p->capture;
  }
}

static void capture_resume(upb_json_parser *p, const char *ptr) {
  if (p->capture) {
    UPB_ASSERT(p->capture == &suspend_capture);
    p->capture = ptr;
  }
}


/* Callbacks from the parser **************************************************/

/* These are the functions called directly from the parser itself.
 * We define these in the same order as their declarations in the parser. */

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
      UPB_ASSERT(0);
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
    UPB_ASSERT(ch >= 'A' && ch <= 'F');
    p->digit += ((ch - 'A') + 10);
  }
}

static bool end_hex(upb_json_parser *p) {
  uint32_t codepoint = p->digit;

  /* emit the codepoint as UTF-8. */
  char utf8[3]; /* support \u0000 -- \uFFFF -- need only three bytes. */
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
  /* TODO(haberman): Handle high surrogates: if codepoint is a high surrogate
   * we have to wait for the next escape to get the full code point). */

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

static bool parse_number(upb_json_parser *p, bool is_quoted);

static bool end_number(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }

  return parse_number(p, false);
}

static bool parse_number_from_buffer(upb_json_parser *p, const char *buf,
                                     const char *bufend, bool is_quoted) {
  size_t len = bufend - buf;
  char *end;
  upb_fieldtype_t type = upb_fielddef_type(p->top->f);
  double val;
  double dummy;

  if (buf[0] == ' ') {
    return false;
  }

  /* For integer types, first try parsing with integer-specific routines.
   * If these succeed, they will be more accurate for int64/uint64 than
   * strtod().
   */
  switch (type) {
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32: {
      long val = strtol(buf, &end, 0);
      if (errno == ERANGE || end != bufend) {
        break;
      } else if (val > INT32_MAX || val < INT32_MIN) {
        return false;
      } else {
        upb_sink_putint32(&p->top->sink, parser_getsel(p), val);
        return true;
      }
    }
    case UPB_TYPE_UINT32: {
      unsigned long val = strtoul(buf, &end, 0);
      if (end != bufend) {
        break;
      } else if (val > UINT32_MAX || errno == ERANGE) {
        return false;
      } else {
        upb_sink_putuint32(&p->top->sink, parser_getsel(p), val);
        return true;
      }
    }
    /* XXX: We can't handle [u]int64 properly on 32-bit machines because
     * strto[u]ll isn't in C89. */
    case UPB_TYPE_INT64: {
      long val = strtol(buf, &end, 0);
      if (errno == ERANGE || end != bufend) {
        break;
      } else {
        upb_sink_putint64(&p->top->sink, parser_getsel(p), val);
        return true;
      }
    }
    case UPB_TYPE_UINT64: {
      unsigned long val = strtoul(p->accumulated, &end, 0);
      if (end != bufend) {
        break;
      } else if (errno == ERANGE) {
        return false;
      } else {
        upb_sink_putuint64(&p->top->sink, parser_getsel(p), val);
        return true;
      }
    }
    default:
      break;
  }

  if (type != UPB_TYPE_DOUBLE && type != UPB_TYPE_FLOAT && is_quoted) {
    /* Quoted numbers shouldn't support double forms for integer types. */
    return false;
  }

  if (len == strlen("Infinity") && strcmp(buf, "Infinity") == 0) {
    /* C89 does not have an INFINITY macro. */
    val = 1.0 / 0.0;
  } else if (len == strlen("-Infinity") && strcmp(buf, "-Infinity") == 0) {
    val = -1.0 / 0.0;
  } else {
    val = strtod(buf, &end);
    if (errno == ERANGE || end != bufend) {
      return false;
    }
  }

  switch (type) {
#define CASE(capitaltype, smalltype, min, max)                            \
    case UPB_TYPE_ ## capitaltype: {                                      \
      if (modf(val, &dummy) != 0 || val > max || val < min) {             \
        return false;                                                     \
      } else {                                                            \
        upb_sink_put ## smalltype(&p->top->sink, parser_getsel(p), val);  \
        return true;                                                      \
      }                                                                   \
      break;                                                              \
    }
    case UPB_TYPE_ENUM:
    CASE(INT32, int32, INT32_MIN, INT32_MAX);
    CASE(INT64, int64, INT64_MIN, INT64_MAX);
    CASE(UINT32, uint32, 0, UINT32_MAX);
    CASE(UINT64, uint64, 0, UINT64_MAX);
#undef CASE

    case UPB_TYPE_DOUBLE:
      upb_sink_putdouble(&p->top->sink, parser_getsel(p), val);
      return true;
    case UPB_TYPE_FLOAT:
      if (false /*val > FLT_MAX || val < -FLT_MAX*/) {
        return false;
      } else {
        upb_sink_putfloat(&p->top->sink, parser_getsel(p), val);
        return true;
      }
    default:
      return false;
  }
}

static bool parse_number(upb_json_parser *p, bool is_quoted) {
  size_t len;
  const char *buf;
  const char *bufend;

  /* strtol() and friends unfortunately do not support specifying the length of
   * the input string, so we need to force a copy into a NULL-terminated buffer. */
  if (!multipart_text(p, "\0", 1, false)) {
    return false;
  }

  buf = accumulate_getptr(p, &len);
  bufend = buf + len - 1;  /* One for NULL. */
  errno = 0;

  if (parse_number_from_buffer(p, buf, bufend, is_quoted)) {
    multipart_end(p);
    return true;
  } else {
    upb_status_seterrf(&p->status, "error parsing number: %s", buf);
    upb_env_reporterror(p->env, &p->status);
    multipart_end(p);
    return false;
  }
}

static bool parser_putbool(upb_json_parser *p, bool val) {
  bool ok;

  if (upb_fielddef_type(p->top->f) != UPB_TYPE_BOOL) {
    upb_status_seterrf(&p->status,
                       "Boolean value specified for non-bool field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  ok = upb_sink_putbool(&p->top->sink, parser_getsel(p), val);
  UPB_ASSERT(ok);

  return true;
}

static bool start_stringval(upb_json_parser *p) {
  UPB_ASSERT(p->top->f);

  if (upb_fielddef_isstring(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    if (!check_stack(p)) return false;

    /* Start a new parser frame: parser frames correspond one-to-one with
     * handler frames, and string events occur in a sub-frame. */
    inner = p->top + 1;
    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
    upb_sink_startstr(&p->top->sink, sel, 0, &inner->sink);
    inner->m = p->top->m;
    inner->f = p->top->f;
    inner->name_table = NULL;
    inner->is_map = false;
    inner->is_mapentry = false;
    p->top = inner;

    if (upb_fielddef_type(p->top->f) == UPB_TYPE_STRING) {
      /* For STRING fields we push data directly to the handlers as it is
       * parsed.  We don't do this yet for BYTES fields, because our base64
       * decoder is not streaming.
       *
       * TODO(haberman): make base64 decoding streaming also. */
      multipart_start(p, getsel_for_handlertype(p, UPB_HANDLER_STRING));
      return true;
    } else {
      multipart_startaccum(p);
      return true;
    }
  } else if (upb_fielddef_type(p->top->f) != UPB_TYPE_BOOL &&
             upb_fielddef_type(p->top->f) != UPB_TYPE_MESSAGE) {
    /* No need to push a frame -- numeric values in quotes remain in the
     * current parser frame.  These values must accmulate so we can convert
     * them all at once at the end. */
    multipart_startaccum(p);
    return true;
  } else {
    upb_status_seterrf(&p->status,
                       "String specified for bool or submessage field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
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
      /* Fall through. */

    case UPB_TYPE_STRING: {
      upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
      p->top--;
      upb_sink_endstr(&p->top->sink, sel);
      break;
    }

    case UPB_TYPE_ENUM: {
      /* Resolve enum symbolic name to integer value. */
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
        upb_status_seterrf(&p->status, "Enum value unknown: '%.*s'", len, buf);
        upb_env_reporterror(p->env, &p->status);
      }

      break;
    }

    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_FLOAT:
      ok = parse_number(p, true);
      break;

    default:
      UPB_ASSERT(false);
      upb_status_seterrmsg(&p->status, "Internal error in JSON decoder");
      upb_env_reporterror(p->env, &p->status);
      ok = false;
      break;
  }

  multipart_end(p);

  return ok;
}

static void start_member(upb_json_parser *p) {
  UPB_ASSERT(!p->top->f);
  multipart_startaccum(p);
}

/* Helper: invoked during parse_mapentry() to emit the mapentry message's key
 * field based on the current contents of the accumulate buffer. */
static bool parse_mapentry_key(upb_json_parser *p) {

  size_t len;
  const char *buf = accumulate_getptr(p, &len);

  /* Emit the key field. We do a bit of ad-hoc parsing here because the
   * parser state machine has already decided that this is a string field
   * name, and we are reinterpreting it as some arbitrary key type. In
   * particular, integer and bool keys are quoted, so we need to parse the
   * quoted string contents here. */

  p->top->f = upb_msgdef_itof(p->top->m, UPB_MAPENTRY_KEY);
  if (p->top->f == NULL) {
    upb_status_seterrmsg(&p->status, "mapentry message has no key");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }
  switch (upb_fielddef_type(p->top->f)) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_UINT64:
      /* Invoke end_number. The accum buffer has the number's text already. */
      if (!parse_number(p, true)) {
        return false;
      }
      break;
    case UPB_TYPE_BOOL:
      if (len == 4 && !strncmp(buf, "true", 4)) {
        if (!parser_putbool(p, true)) {
          return false;
        }
      } else if (len == 5 && !strncmp(buf, "false", 5)) {
        if (!parser_putbool(p, false)) {
          return false;
        }
      } else {
        upb_status_seterrmsg(&p->status,
                             "Map bool key not 'true' or 'false'");
        upb_env_reporterror(p->env, &p->status);
        return false;
      }
      multipart_end(p);
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      upb_sink subsink;
      upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
      upb_sink_startstr(&p->top->sink, sel, len, &subsink);
      sel = getsel_for_handlertype(p, UPB_HANDLER_STRING);
      upb_sink_putstring(&subsink, sel, buf, len, NULL);
      sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
      upb_sink_endstr(&p->top->sink, sel);
      multipart_end(p);
      break;
    }
    default:
      upb_status_seterrmsg(&p->status, "Invalid field type for map key");
      upb_env_reporterror(p->env, &p->status);
      return false;
  }

  return true;
}

/* Helper: emit one map entry (as a submessage in the map field sequence). This
 * is invoked from end_membername(), at the end of the map entry's key string,
 * with the map key in the accumulate buffer. It parses the key from that
 * buffer, emits the handler calls to start the mapentry submessage (setting up
 * its subframe in the process), and sets up state in the subframe so that the
 * value parser (invoked next) will emit the mapentry's value field and then
 * end the mapentry message. */

static bool handle_mapentry(upb_json_parser *p) {
  const upb_fielddef *mapfield;
  const upb_msgdef *mapentrymsg;
  upb_jsonparser_frame *inner;
  upb_selector_t sel;

  /* Map entry: p->top->sink is the seq frame, so we need to start a frame
   * for the mapentry itself, and then set |f| in that frame so that the map
   * value field is parsed, and also set a flag to end the frame after the
   * map-entry value is parsed. */
  if (!check_stack(p)) return false;

  mapfield = p->top->mapfield;
  mapentrymsg = upb_fielddef_msgsubdef(mapfield);

  inner = p->top + 1;
  p->top->f = mapfield;
  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSUBMSG);
  upb_sink_startsubmsg(&p->top->sink, sel, &inner->sink);
  inner->m = mapentrymsg;
  inner->name_table = NULL;
  inner->mapfield = mapfield;
  inner->is_map = false;

  /* Don't set this to true *yet* -- we reuse parsing handlers below to push
   * the key field value to the sink, and these handlers will pop the frame
   * if they see is_mapentry (when invoked by the parser state machine, they
   * would have just seen the map-entry value, not key). */
  inner->is_mapentry = false;
  p->top = inner;

  /* send STARTMSG in submsg frame. */
  upb_sink_startmsg(&p->top->sink);

  parse_mapentry_key(p);

  /* Set up the value field to receive the map-entry value. */
  p->top->f = upb_msgdef_itof(p->top->m, UPB_MAPENTRY_VALUE);
  p->top->is_mapentry = true;  /* set up to pop frame after value is parsed. */
  p->top->mapfield = mapfield;
  if (p->top->f == NULL) {
    upb_status_seterrmsg(&p->status, "mapentry message has no value");
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  return true;
}

static bool end_membername(upb_json_parser *p) {
  UPB_ASSERT(!p->top->f);

  if (p->top->is_map) {
    return handle_mapentry(p);
  } else {
    size_t len;
    const char *buf = accumulate_getptr(p, &len);
    upb_value v;

    if (upb_strtable_lookup2(p->top->name_table, buf, len, &v)) {
      p->top->f = upb_value_getconstptr(v);
      multipart_end(p);

      return true;
    } else {
      /* TODO(haberman): Ignore unknown fields if requested/configured to do
       * so. */
      upb_status_seterrf(&p->status, "No such field: %.*s\n", (int)len, buf);
      upb_env_reporterror(p->env, &p->status);
      return false;
    }
  }
}

static void end_member(upb_json_parser *p) {
  /* If we just parsed a map-entry value, end that frame too. */
  if (p->top->is_mapentry) {
    upb_status s = UPB_STATUS_INIT;
    upb_selector_t sel;
    bool ok;
    const upb_fielddef *mapfield;

    UPB_ASSERT(p->top > p->stack);
    /* send ENDMSG on submsg. */
    upb_sink_endmsg(&p->top->sink, &s);
    mapfield = p->top->mapfield;

    /* send ENDSUBMSG in repeated-field-of-mapentries frame. */
    p->top--;
    ok = upb_handlers_getselector(mapfield, UPB_HANDLER_ENDSUBMSG, &sel);
    UPB_ASSERT(ok);
    upb_sink_endsubmsg(&p->top->sink, sel);
  }

  p->top->f = NULL;
}

static bool start_subobject(upb_json_parser *p) {
  UPB_ASSERT(p->top->f);

  if (upb_fielddef_ismap(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    /* Beginning of a map. Start a new parser frame in a repeated-field
     * context. */
    if (!check_stack(p)) return false;

    inner = p->top + 1;
    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSEQ);
    upb_sink_startseq(&p->top->sink, sel, &inner->sink);
    inner->m = upb_fielddef_msgsubdef(p->top->f);
    inner->name_table = NULL;
    inner->mapfield = p->top->f;
    inner->f = NULL;
    inner->is_map = true;
    inner->is_mapentry = false;
    p->top = inner;

    return true;
  } else if (upb_fielddef_issubmsg(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    /* Beginning of a subobject. Start a new parser frame in the submsg
     * context. */
    if (!check_stack(p)) return false;

    inner = p->top + 1;

    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSUBMSG);
    upb_sink_startsubmsg(&p->top->sink, sel, &inner->sink);
    inner->m = upb_fielddef_msgsubdef(p->top->f);
    set_name_table(p, inner);
    inner->f = NULL;
    inner->is_map = false;
    inner->is_mapentry = false;
    p->top = inner;

    return true;
  } else {
    upb_status_seterrf(&p->status,
                       "Object specified for non-message/group field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  }
}

static void end_subobject(upb_json_parser *p) {
  if (p->top->is_map) {
    upb_selector_t sel;
    p->top--;
    sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSEQ);
    upb_sink_endseq(&p->top->sink, sel);
  } else {
    upb_selector_t sel;
    p->top--;
    sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSUBMSG);
    upb_sink_endsubmsg(&p->top->sink, sel);
  }
}

static bool start_array(upb_json_parser *p) {
  upb_jsonparser_frame *inner;
  upb_selector_t sel;

  UPB_ASSERT(p->top->f);

  if (!upb_fielddef_isseq(p->top->f)) {
    upb_status_seterrf(&p->status,
                       "Array specified for non-repeated field: %s",
                       upb_fielddef_name(p->top->f));
    upb_env_reporterror(p->env, &p->status);
    return false;
  }

  if (!check_stack(p)) return false;

  inner = p->top + 1;
  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSEQ);
  upb_sink_startseq(&p->top->sink, sel, &inner->sink);
  inner->m = p->top->m;
  inner->name_table = NULL;
  inner->f = p->top->f;
  inner->is_map = false;
  inner->is_mapentry = false;
  p->top = inner;

  return true;
}

static void end_array(upb_json_parser *p) {
  upb_selector_t sel;

  UPB_ASSERT(p->top > p->stack);

  p->top--;
  sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSEQ);
  upb_sink_endseq(&p->top->sink, sel);
}

static void start_object(upb_json_parser *p) {
  if (!p->top->is_map) {
    upb_sink_startmsg(&p->top->sink);
  }
}

static void end_object(upb_json_parser *p) {
  if (!p->top->is_map) {
    upb_status status;
    upb_status_clear(&status);
    upb_sink_endmsg(&p->top->sink, &status);
    if (!upb_ok(&status)) {
      upb_env_reporterror(p->env, &status);
    }
  }
}


#define CHECK_RETURN_TOP(x) if (!(x)) goto error


/* The actual parser **********************************************************/

/* What follows is the Ragel parser itself.  The language is specified in Ragel
 * and the actions call our C functions above.
 *
 * Ragel has an extensive set of functionality, and we use only a small part of
 * it.  There are many action types but we only use a few:
 *
 *   ">" -- transition into a machine
 *   "%" -- transition out of a machine
 *   "@" -- transition into a final state of a machine.
 *
 * "@" transitions are tricky because a machine can transition into a final
 * state repeatedly.  But in some cases we know this can't happen, for example
 * a string which is delimited by a final '"' can only transition into its
 * final state once, when the closing '"' is seen. */


#line 1306 "upb/json/parser.rl"



#line 1218 "upb/json/parser.c"
static const char _json_actions[] = {
	0, 1, 0, 1, 2, 1, 3, 1, 
	5, 1, 6, 1, 7, 1, 8, 1, 
	10, 1, 12, 1, 13, 1, 14, 1, 
	15, 1, 16, 1, 17, 1, 21, 1, 
	25, 1, 27, 2, 3, 8, 2, 4, 
	5, 2, 6, 2, 2, 6, 8, 2, 
	11, 9, 2, 13, 15, 2, 14, 15, 
	2, 18, 1, 2, 19, 27, 2, 20, 
	9, 2, 22, 27, 2, 23, 27, 2, 
	24, 27, 2, 26, 27, 3, 14, 11, 
	9
};

static const unsigned char _json_key_offsets[] = {
	0, 0, 4, 9, 14, 15, 19, 24, 
	29, 34, 38, 42, 45, 48, 50, 54, 
	58, 60, 62, 67, 69, 71, 80, 86, 
	92, 98, 104, 106, 115, 116, 116, 116, 
	121, 126, 131, 132, 133, 134, 135, 135, 
	136, 137, 138, 138, 139, 140, 141, 141, 
	146, 151, 152, 156, 161, 166, 171, 175, 
	175, 178, 178, 178
};

static const char _json_trans_keys[] = {
	32, 123, 9, 13, 32, 34, 125, 9, 
	13, 32, 34, 125, 9, 13, 34, 32, 
	58, 9, 13, 32, 93, 125, 9, 13, 
	32, 44, 125, 9, 13, 32, 44, 125, 
	9, 13, 32, 34, 9, 13, 45, 48, 
	49, 57, 48, 49, 57, 46, 69, 101, 
	48, 57, 69, 101, 48, 57, 43, 45, 
	48, 57, 48, 57, 48, 57, 46, 69, 
	101, 48, 57, 34, 92, 34, 92, 34, 
	47, 92, 98, 102, 110, 114, 116, 117, 
	48, 57, 65, 70, 97, 102, 48, 57, 
	65, 70, 97, 102, 48, 57, 65, 70, 
	97, 102, 48, 57, 65, 70, 97, 102, 
	34, 92, 34, 45, 91, 102, 110, 116, 
	123, 48, 57, 34, 32, 93, 125, 9, 
	13, 32, 44, 93, 9, 13, 32, 93, 
	125, 9, 13, 97, 108, 115, 101, 117, 
	108, 108, 114, 117, 101, 32, 34, 125, 
	9, 13, 32, 34, 125, 9, 13, 34, 
	32, 58, 9, 13, 32, 93, 125, 9, 
	13, 32, 44, 125, 9, 13, 32, 44, 
	125, 9, 13, 32, 34, 9, 13, 32, 
	9, 13, 0
};

static const char _json_single_lengths[] = {
	0, 2, 3, 3, 1, 2, 3, 3, 
	3, 2, 2, 1, 3, 0, 2, 2, 
	0, 0, 3, 2, 2, 9, 0, 0, 
	0, 0, 2, 7, 1, 0, 0, 3, 
	3, 3, 1, 1, 1, 1, 0, 1, 
	1, 1, 0, 1, 1, 1, 0, 3, 
	3, 1, 2, 3, 3, 3, 2, 0, 
	1, 0, 0, 0
};

static const char _json_range_lengths[] = {
	0, 1, 1, 1, 0, 1, 1, 1, 
	1, 1, 1, 1, 0, 1, 1, 1, 
	1, 1, 1, 0, 0, 0, 3, 3, 
	3, 3, 0, 1, 0, 0, 0, 1, 
	1, 1, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 0, 1, 1, 1, 1, 1, 0, 
	1, 0, 0, 0
};

static const short _json_index_offsets[] = {
	0, 0, 4, 9, 14, 16, 20, 25, 
	30, 35, 39, 43, 46, 50, 52, 56, 
	60, 62, 64, 69, 72, 75, 85, 89, 
	93, 97, 101, 104, 113, 115, 116, 117, 
	122, 127, 132, 134, 136, 138, 140, 141, 
	143, 145, 147, 148, 150, 152, 154, 155, 
	160, 165, 167, 171, 176, 181, 186, 190, 
	191, 194, 195, 196
};

static const char _json_indicies[] = {
	0, 2, 0, 1, 3, 4, 5, 3, 
	1, 6, 7, 8, 6, 1, 9, 1, 
	10, 11, 10, 1, 11, 1, 1, 11, 
	12, 13, 14, 15, 13, 1, 16, 17, 
	8, 16, 1, 17, 7, 17, 1, 18, 
	19, 20, 1, 19, 20, 1, 22, 23, 
	23, 21, 24, 1, 23, 23, 24, 21, 
	25, 25, 26, 1, 26, 1, 26, 21, 
	22, 23, 23, 20, 21, 28, 29, 27, 
	31, 32, 30, 33, 33, 33, 33, 33, 
	33, 33, 33, 34, 1, 35, 35, 35, 
	1, 36, 36, 36, 1, 37, 37, 37, 
	1, 38, 38, 38, 1, 40, 41, 39, 
	42, 43, 44, 45, 46, 47, 48, 43, 
	1, 49, 1, 50, 51, 53, 54, 1, 
	53, 52, 55, 56, 54, 55, 1, 56, 
	1, 1, 56, 52, 57, 1, 58, 1, 
	59, 1, 60, 1, 61, 62, 1, 63, 
	1, 64, 1, 65, 66, 1, 67, 1, 
	68, 1, 69, 70, 71, 72, 70, 1, 
	73, 74, 75, 73, 1, 76, 1, 77, 
	78, 77, 1, 78, 1, 1, 78, 79, 
	80, 81, 82, 80, 1, 83, 84, 75, 
	83, 1, 84, 74, 84, 1, 85, 86, 
	86, 1, 1, 1, 1, 0
};

static const char _json_trans_targs[] = {
	1, 0, 2, 3, 4, 56, 3, 4, 
	56, 5, 5, 6, 7, 8, 9, 56, 
	8, 9, 11, 12, 18, 57, 13, 15, 
	14, 16, 17, 20, 58, 21, 20, 58, 
	21, 19, 22, 23, 24, 25, 26, 20, 
	58, 21, 28, 30, 31, 34, 39, 43, 
	47, 29, 59, 59, 32, 31, 29, 32, 
	33, 35, 36, 37, 38, 59, 40, 41, 
	42, 59, 44, 45, 46, 59, 48, 49, 
	55, 48, 49, 55, 50, 50, 51, 52, 
	53, 54, 55, 53, 54, 59, 56
};

static const char _json_trans_actions[] = {
	0, 0, 0, 21, 77, 53, 0, 47, 
	23, 17, 0, 0, 15, 19, 19, 50, 
	0, 0, 0, 0, 0, 1, 0, 0, 
	0, 0, 0, 3, 13, 0, 0, 35, 
	5, 11, 0, 38, 7, 7, 7, 41, 
	44, 9, 62, 56, 25, 0, 0, 0, 
	31, 29, 33, 59, 15, 0, 27, 0, 
	0, 0, 0, 0, 0, 68, 0, 0, 
	0, 71, 0, 0, 0, 65, 21, 77, 
	53, 0, 47, 23, 17, 0, 0, 15, 
	19, 19, 50, 0, 0, 74, 0
};

static const int json_start = 1;

static const int json_en_number_machine = 10;
static const int json_en_string_machine = 19;
static const int json_en_value_machine = 27;
static const int json_en_main = 1;


#line 1309 "upb/json/parser.rl"

size_t parse(void *closure, const void *hd, const char *buf, size_t size,
             const upb_bufhandle *handle) {
  upb_json_parser *parser = closure;

  /* Variables used by Ragel's generated code. */
  int cs = parser->current_state;
  int *stack = parser->parser_stack;
  int top = parser->parser_top;

  const char *p = buf;
  const char *pe = buf + size;

  parser->handle = handle;

  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  capture_resume(parser, buf);

  
#line 1389 "upb/json/parser.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _json_trans_keys + _json_key_offsets[cs];
	_trans = _json_index_offsets[cs];

	_klen = _json_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _json_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _json_indicies[_trans];
	cs = _json_trans_targs[_trans];

	if ( _json_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _json_actions + _json_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 1221 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
	case 1:
#line 1222 "upb/json/parser.rl"
	{ p--; {stack[top++] = cs; cs = 10; goto _again;} }
	break;
	case 2:
#line 1226 "upb/json/parser.rl"
	{ start_text(parser, p); }
	break;
	case 3:
#line 1227 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_text(parser, p)); }
	break;
	case 4:
#line 1233 "upb/json/parser.rl"
	{ start_hex(parser); }
	break;
	case 5:
#line 1234 "upb/json/parser.rl"
	{ hexdigit(parser, p); }
	break;
	case 6:
#line 1235 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_hex(parser)); }
	break;
	case 7:
#line 1241 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(escape(parser, p)); }
	break;
	case 8:
#line 1247 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
	case 9:
#line 1250 "upb/json/parser.rl"
	{ {stack[top++] = cs; cs = 19; goto _again;} }
	break;
	case 10:
#line 1252 "upb/json/parser.rl"
	{ p--; {stack[top++] = cs; cs = 27; goto _again;} }
	break;
	case 11:
#line 1257 "upb/json/parser.rl"
	{ start_member(parser); }
	break;
	case 12:
#line 1258 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_membername(parser)); }
	break;
	case 13:
#line 1261 "upb/json/parser.rl"
	{ end_member(parser); }
	break;
	case 14:
#line 1267 "upb/json/parser.rl"
	{ start_object(parser); }
	break;
	case 15:
#line 1270 "upb/json/parser.rl"
	{ end_object(parser); }
	break;
	case 16:
#line 1276 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_array(parser)); }
	break;
	case 17:
#line 1280 "upb/json/parser.rl"
	{ end_array(parser); }
	break;
	case 18:
#line 1285 "upb/json/parser.rl"
	{ start_number(parser, p); }
	break;
	case 19:
#line 1286 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_number(parser, p)); }
	break;
	case 20:
#line 1288 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_stringval(parser)); }
	break;
	case 21:
#line 1289 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_stringval(parser)); }
	break;
	case 22:
#line 1291 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(parser_putbool(parser, true)); }
	break;
	case 23:
#line 1293 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(parser_putbool(parser, false)); }
	break;
	case 24:
#line 1295 "upb/json/parser.rl"
	{ /* null value */ }
	break;
	case 25:
#line 1297 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_subobject(parser)); }
	break;
	case 26:
#line 1298 "upb/json/parser.rl"
	{ end_subobject(parser); }
	break;
	case 27:
#line 1303 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
#line 1575 "upb/json/parser.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 1330 "upb/json/parser.rl"

  if (p != pe) {
    upb_status_seterrf(&parser->status, "Parse error at '%.*s'\n", pe - p, p);
    upb_env_reporterror(parser->env, &parser->status);
  } else {
    capture_suspend(parser, &p);
  }

error:
  /* Save parsing state back to parser. */
  parser->current_state = cs;
  parser->parser_top = top;

  return p - buf;
}

bool end(void *closure, const void *hd) {
  UPB_UNUSED(closure);
  UPB_UNUSED(hd);

  /* Prevent compile warning on unused static constants. */
  UPB_UNUSED(json_start);
  UPB_UNUSED(json_en_number_machine);
  UPB_UNUSED(json_en_string_machine);
  UPB_UNUSED(json_en_value_machine);
  UPB_UNUSED(json_en_main);
  return true;
}

static void json_parser_reset(upb_json_parser *p) {
  int cs;
  int top;

  p->top = p->stack;
  p->top->f = NULL;
  p->top->is_map = false;
  p->top->is_mapentry = false;

  /* Emit Ragel initialization of the parser. */
  
#line 1629 "upb/json/parser.c"
	{
	cs = json_start;
	top = 0;
	}

#line 1370 "upb/json/parser.rl"
  p->current_state = cs;
  p->parser_top = top;
  accumulate_clear(p);
  p->multipart_state = MULTIPART_INACTIVE;
  p->capture = NULL;
  p->accumulated = NULL;
  upb_status_clear(&p->status);
}

static void visit_json_parsermethod(const upb_refcounted *r,
                                    upb_refcounted_visit *visit,
                                    void *closure) {
  const upb_json_parsermethod *method = (upb_json_parsermethod*)r;
  visit(r, upb_msgdef_upcast2(method->msg), closure);
}

static void free_json_parsermethod(upb_refcounted *r) {
  upb_json_parsermethod *method = (upb_json_parsermethod*)r;

  upb_inttable_iter i;
  upb_inttable_begin(&i, &method->name_tables);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_value val = upb_inttable_iter_value(&i);
    upb_strtable *t = upb_value_getptr(val);
    upb_strtable_uninit(t);
    upb_gfree(t);
  }

  upb_inttable_uninit(&method->name_tables);

  upb_gfree(r);
}

static void add_jsonname_table(upb_json_parsermethod *m, const upb_msgdef* md) {
  upb_msg_field_iter i;
  upb_strtable *t;

  /* It would be nice to stack-allocate this, but protobufs do not limit the
   * length of fields to any reasonable limit. */
  char *buf = NULL;
  size_t len = 0;

  if (upb_inttable_lookupptr(&m->name_tables, md, NULL)) {
    return;
  }

  /* TODO(haberman): handle malloc failure. */
  t = upb_gmalloc(sizeof(*t));
  upb_strtable_init(t, UPB_CTYPE_CONSTPTR);
  upb_inttable_insertptr(&m->name_tables, md, upb_value_ptr(t));

  for(upb_msg_field_begin(&i, md);
      !upb_msg_field_done(&i);
      upb_msg_field_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);

    /* Add an entry for the JSON name. */
    size_t field_len = upb_fielddef_getjsonname(f, buf, len);
    if (field_len > len) {
      size_t len2;
      buf = upb_grealloc(buf, 0, field_len);
      len = field_len;
      len2 = upb_fielddef_getjsonname(f, buf, len);
      UPB_ASSERT(len == len2);
    }
    upb_strtable_insert(t, buf, upb_value_constptr(f));

    if (strcmp(buf, upb_fielddef_name(f)) != 0) {
      /* Since the JSON name is different from the regular field name, add an
       * entry for the raw name (compliant proto3 JSON parsers must accept
       * both). */
      upb_strtable_insert(t, upb_fielddef_name(f), upb_value_constptr(f));
    }

    if (upb_fielddef_issubmsg(f)) {
      add_jsonname_table(m, upb_fielddef_msgsubdef(f));
    }
  }

  upb_gfree(buf);
}

/* Public API *****************************************************************/

upb_json_parser *upb_json_parser_create(upb_env *env,
                                        const upb_json_parsermethod *method,
                                        upb_sink *output) {
#ifndef NDEBUG
  const size_t size_before = upb_env_bytesallocated(env);
#endif
  upb_json_parser *p = upb_env_malloc(env, sizeof(upb_json_parser));
  if (!p) return false;

  p->env = env;
  p->method = method;
  p->limit = p->stack + UPB_JSON_MAX_DEPTH;
  p->accumulate_buf = NULL;
  p->accumulate_buf_size = 0;
  upb_bytessink_reset(&p->input_, &method->input_handler_, p);

  json_parser_reset(p);
  upb_sink_reset(&p->top->sink, output->handlers, output->closure);
  p->top->m = upb_handlers_msgdef(output->handlers);
  set_name_table(p, p->top);

  /* If this fails, uncomment and increase the value in parser.h. */
  /* fprintf(stderr, "%zd\n", upb_env_bytesallocated(env) - size_before); */
  UPB_ASSERT_DEBUGVAR(upb_env_bytesallocated(env) - size_before <=
                      UPB_JSON_PARSER_SIZE);
  return p;
}

upb_bytessink *upb_json_parser_input(upb_json_parser *p) {
  return &p->input_;
}

upb_json_parsermethod *upb_json_parsermethod_new(const upb_msgdef* md,
                                                 const void* owner) {
  static const struct upb_refcounted_vtbl vtbl = {visit_json_parsermethod,
                                                  free_json_parsermethod};
  upb_json_parsermethod *ret = upb_gmalloc(sizeof(*ret));
  upb_refcounted_init(upb_json_parsermethod_upcast_mutable(ret), &vtbl, owner);

  ret->msg = md;
  upb_ref2(md, ret);

  upb_byteshandler_init(&ret->input_handler_);
  upb_byteshandler_setstring(&ret->input_handler_, parse, ret);
  upb_byteshandler_setendstr(&ret->input_handler_, end, ret);

  upb_inttable_init(&ret->name_tables, UPB_CTYPE_PTR);

  add_jsonname_table(ret, md);

  return ret;
}

const upb_byteshandler *upb_json_parsermethod_inputhandler(
    const upb_json_parsermethod *m) {
  return &m->input_handler_;
}
