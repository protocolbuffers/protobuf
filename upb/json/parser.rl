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

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include "upb/json/parser.h"
#include "upb/pb/encoder.h"

#include "upb/port_def.inc"

#define UPB_JSON_MAX_DEPTH 64

/* Type of value message */
enum {
  VALUE_NULLVALUE   = 0,
  VALUE_NUMBERVALUE = 1,
  VALUE_STRINGVALUE = 2,
  VALUE_BOOLVALUE   = 3,
  VALUE_STRUCTVALUE = 4,
  VALUE_LISTVALUE   = 5
};

/* Forward declare */
static bool is_top_level(upb_json_parser *p);
static bool is_wellknown_msg(upb_json_parser *p, upb_wellknowntype_t type);
static bool is_wellknown_field(upb_json_parser *p, upb_wellknowntype_t type);

static bool is_number_wrapper_object(upb_json_parser *p);
static bool does_number_wrapper_start(upb_json_parser *p);
static bool does_number_wrapper_end(upb_json_parser *p);

static bool is_string_wrapper_object(upb_json_parser *p);
static bool does_string_wrapper_start(upb_json_parser *p);
static bool does_string_wrapper_end(upb_json_parser *p);

static bool does_fieldmask_start(upb_json_parser *p);
static bool does_fieldmask_end(upb_json_parser *p);
static void start_fieldmask_object(upb_json_parser *p);
static void end_fieldmask_object(upb_json_parser *p);

static void start_wrapper_object(upb_json_parser *p);
static void end_wrapper_object(upb_json_parser *p);

static void start_value_object(upb_json_parser *p, int value_type);
static void end_value_object(upb_json_parser *p);

static void start_listvalue_object(upb_json_parser *p);
static void end_listvalue_object(upb_json_parser *p);

static void start_structvalue_object(upb_json_parser *p);
static void end_structvalue_object(upb_json_parser *p);

static void start_object(upb_json_parser *p);
static void end_object(upb_json_parser *p);

static void start_any_object(upb_json_parser *p, const char *ptr);
static bool end_any_object(upb_json_parser *p, const char *ptr);

static bool start_subobject(upb_json_parser *p);
static void end_subobject(upb_json_parser *p);

static void start_member(upb_json_parser *p);
static void end_member(upb_json_parser *p);
static bool end_membername(upb_json_parser *p);

static void start_any_member(upb_json_parser *p, const char *ptr);
static void end_any_member(upb_json_parser *p, const char *ptr);
static bool end_any_membername(upb_json_parser *p);

size_t parse(void *closure, const void *hd, const char *buf, size_t size,
             const upb_bufhandle *handle);
static bool end(void *closure, const void *hd);

static const char eof_ch = 'e';

/* stringsink */
typedef struct {
  upb_byteshandler handler;
  upb_bytessink sink;
  char *ptr;
  size_t len, size;
} upb_stringsink;


static void *stringsink_start(void *_sink, const void *hd, size_t size_hint) {
  upb_stringsink *sink = _sink;
  sink->len = 0;
  UPB_UNUSED(hd);
  UPB_UNUSED(size_hint);
  return sink;
}

static size_t stringsink_string(void *_sink, const void *hd, const char *ptr,
                                size_t len, const upb_bufhandle *handle) {
  upb_stringsink *sink = _sink;
  size_t new_size = sink->size;

  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  while (sink->len + len > new_size) {
    new_size *= 2;
  }

  if (new_size != sink->size) {
    sink->ptr = realloc(sink->ptr, new_size);
    sink->size = new_size;
  }

  memcpy(sink->ptr + sink->len, ptr, len);
  sink->len += len;

  return len;
}

void upb_stringsink_init(upb_stringsink *sink) {
  upb_byteshandler_init(&sink->handler);
  upb_byteshandler_setstartstr(&sink->handler, stringsink_start, NULL);
  upb_byteshandler_setstring(&sink->handler, stringsink_string, NULL);

  upb_bytessink_reset(&sink->sink, &sink->handler, sink);

  sink->size = 32;
  sink->ptr = malloc(sink->size);
  sink->len = 0;
}

void upb_stringsink_uninit(upb_stringsink *sink) { free(sink->ptr); }

typedef struct {
  /* For encoding Any value field in binary format. */
  upb_handlercache *encoder_handlercache;
  upb_stringsink stringsink;

  /* For decoding Any value field in json format. */
  upb_json_codecache *parser_codecache;
  upb_sink sink;
  upb_json_parser *parser;

  /* Mark the range of uninterpreted values in json input before type url. */
  const char *before_type_url_start;
  const char *before_type_url_end;

  /* Mark the range of uninterpreted values in json input after type url. */
  const char *after_type_url_start;
} upb_jsonparser_any_frame;

typedef struct {
  upb_sink sink;

  /* The current message in which we're parsing, and the field whose value we're
   * expecting next. */
  const upb_msgdef *m;
  const upb_fielddef *f;

  /* The table mapping json name to fielddef for this message. */
  const upb_strtable *name_table;

  /* We are in a repeated-field context. We need this flag to decide whether to
   * handle the array as a normal repeated field or a
   * google.protobuf.ListValue/google.protobuf.Value. */
  bool is_repeated;

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

  /* We are in an Any message context. This flag is set when parsing the Any
   * message and indicates to all field parsers (subobjects, strings, numbers,
   * and bools) that the parsed field should be serialized as binary data or
   * cached (type url not found yet). */
  bool is_any;

  /* The type of packed message in Any. */
  upb_jsonparser_any_frame *any_frame;

  /* True if the field to be parsed is unknown. */
  bool is_unknown_field;
} upb_jsonparser_frame;

static void init_frame(upb_jsonparser_frame* frame) {
  frame->m = NULL;
  frame->f = NULL;
  frame->name_table = NULL;
  frame->is_repeated = false;
  frame->is_map = false;
  frame->is_mapentry = false;
  frame->mapfield = NULL;
  frame->is_any = false;
  frame->any_frame = NULL;
  frame->is_unknown_field = false;
}

struct upb_json_parser {
  upb_arena *arena;
  const upb_json_parsermethod *method;
  upb_bytessink input_;

  /* Stack to track the JSON scopes we are in. */
  upb_jsonparser_frame stack[UPB_JSON_MAX_DEPTH];
  upb_jsonparser_frame *top;
  upb_jsonparser_frame *limit;

  upb_status *status;

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

  /* For resolve type url in Any. */
  const upb_symtab *symtab;

  /* Whether to proceed if unknown field is met. */
  bool ignore_json_unknown;

  /* Cache for parsing timestamp due to base and zone are handled in different
   * handlers. */
  struct tm tm;
};

static upb_jsonparser_frame* start_jsonparser_frame(upb_json_parser *p) {
  upb_jsonparser_frame *inner;
  inner = p->top + 1;
  init_frame(inner);
  return inner;
}

struct upb_json_codecache {
  upb_arena *arena;
  upb_inttable methods;   /* upb_msgdef* -> upb_json_parsermethod* */
};

struct upb_json_parsermethod {
  const upb_json_codecache *cache;
  upb_byteshandler input_handler_;

  /* Maps json_name -> fielddef */
  upb_strtable name_table;
};

#define PARSER_CHECK_RETURN(x) if (!(x)) return false

static upb_jsonparser_any_frame *json_parser_any_frame_new(
    upb_json_parser *p) {
  upb_jsonparser_any_frame *frame;

  frame = upb_arena_malloc(p->arena, sizeof(upb_jsonparser_any_frame));

  frame->encoder_handlercache = upb_pb_encoder_newcache();
  frame->parser_codecache = upb_json_codecache_new();
  frame->parser = NULL;
  frame->before_type_url_start = NULL;
  frame->before_type_url_end = NULL;
  frame->after_type_url_start = NULL;

  upb_stringsink_init(&frame->stringsink);

  return frame;
}

static void json_parser_any_frame_set_payload_type(
    upb_json_parser *p,
    upb_jsonparser_any_frame *frame,
    const upb_msgdef *payload_type) {
  const upb_handlers *h;
  const upb_json_parsermethod *parser_method;
  upb_pb_encoder *encoder;

  /* Initialize encoder. */
  h = upb_handlercache_get(frame->encoder_handlercache, payload_type);
  encoder = upb_pb_encoder_create(p->arena, h, frame->stringsink.sink);

  /* Initialize parser. */
  parser_method = upb_json_codecache_get(frame->parser_codecache, payload_type);
  upb_sink_reset(&frame->sink, h, encoder);
  frame->parser =
      upb_json_parser_create(p->arena, parser_method, p->symtab, frame->sink,
                             p->status, p->ignore_json_unknown);
}

static void json_parser_any_frame_free(upb_jsonparser_any_frame *frame) {
  upb_handlercache_free(frame->encoder_handlercache);
  upb_json_codecache_free(frame->parser_codecache);
  upb_stringsink_uninit(&frame->stringsink);
}

static bool json_parser_any_frame_has_type_url(
  upb_jsonparser_any_frame *frame) {
  return frame->parser != NULL;
}

static bool json_parser_any_frame_has_value_before_type_url(
  upb_jsonparser_any_frame *frame) {
  return frame->before_type_url_start != frame->before_type_url_end;
}

static bool json_parser_any_frame_has_value_after_type_url(
  upb_jsonparser_any_frame *frame) {
  return frame->after_type_url_start != NULL;
}

static bool json_parser_any_frame_has_value(
  upb_jsonparser_any_frame *frame) {
  return json_parser_any_frame_has_value_before_type_url(frame) ||
         json_parser_any_frame_has_value_after_type_url(frame);
}

static void json_parser_any_frame_set_before_type_url_end(
    upb_jsonparser_any_frame *frame,
    const char *ptr) {
  if (frame->parser == NULL) {
    frame->before_type_url_end = ptr;
  }
}

static void json_parser_any_frame_set_after_type_url_start_once(
    upb_jsonparser_any_frame *frame,
    const char *ptr) {
  if (json_parser_any_frame_has_type_url(frame) &&
      frame->after_type_url_start == NULL) {
    frame->after_type_url_start = ptr;
  }
}

/* Used to signal that a capture has been suspended. */
static char suspend_capture;

static upb_selector_t getsel_for_handlertype(upb_json_parser *p,
                                             upb_handlertype_t type) {
  upb_selector_t sel;
  bool ok = upb_handlers_getselector(p->top->f, type, &sel);
  UPB_ASSUME(ok);
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

static void set_name_table(upb_json_parser *p, upb_jsonparser_frame *frame) {
  upb_value v;
  const upb_json_codecache *cache = p->method->cache;
  bool ok;
  const upb_json_parsermethod *method;

  ok = upb_inttable_lookupptr(&cache->methods, frame->m, &v);
  UPB_ASSUME(ok);
  method = upb_value_getconstptr(v);

  frame->name_table = &method->name_table;
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
      upb_status_seterrf(p->status,
                         "Base64 input for bytes field not a multiple of 4: %s",
                         upb_fielddef_name(p->top->f));
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
    upb_sink_putstring(p->top->sink, sel, output, 3, NULL);
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
    upb_sink_putstring(p->top->sink, sel, &output, 1, NULL);
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
    upb_sink_putstring(p->top->sink, sel, output, 2, NULL);
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

  mem = upb_arena_realloc(p->arena, p->accumulate_buf, old_size, new_size);
  if (!mem) {
    upb_status_seterrmsg(p->status, "Out of memory allocating buffer.");
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
    upb_status_seterrmsg(p->status, "Integer overflow.");
    return false;
  }

  if (need > p->accumulate_buf_size && !accumulate_realloc(p, need)) {
    return false;
  }

  if (p->accumulated != p->accumulate_buf) {
    if (p->accumulated_len) {
      memcpy(p->accumulate_buf, p->accumulated, p->accumulated_len);
    }
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
          p->status, "Internal error: unexpected state MULTIPART_INACTIVE");
      return false;

    case MULTIPART_ACCUMULATE:
      if (!accumulate_append(p, buf, len, can_alias)) {
        return false;
      }
      break;

    case MULTIPART_PUSHEAGERLY: {
      const upb_bufhandle *handle = can_alias ? p->handle : NULL;
      upb_sink_putstring(p->top->sink, p->string_selector, buf, len, handle);
      break;
    }
  }

  return true;
}

/* Note: this invalidates the accumulate buffer!  Call only after reading its
 * contents. */
static void multipart_end(upb_json_parser *p) {
  /* This is false sometimes. Probably a bug of some sort, but this code is
   * intended for deletion soon. */
  /* UPB_ASSERT(p->multipart_state != MULTIPART_INACTIVE); */
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

static bool start_number(upb_json_parser *p, const char *ptr) {
  if (is_top_level(p)) {
    if (is_number_wrapper_object(p)) {
      start_wrapper_object(p);
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
      start_value_object(p, VALUE_NUMBERVALUE);
    } else {
      return false;
    }
  } else if (does_number_wrapper_start(p)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_wrapper_object(p);
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_VALUE)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_value_object(p, VALUE_NUMBERVALUE);
  }

  multipart_startaccum(p);
  capture_begin(p, ptr);
  return true;
}

static bool parse_number(upb_json_parser *p, bool is_quoted);

static bool end_number_nontop(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }

  if (p->top->f == NULL) {
    multipart_end(p);
    return true;
  }

  return parse_number(p, false);
}

static bool end_number(upb_json_parser *p, const char *ptr) {
  if (!end_number_nontop(p, ptr)) {
    return false;
  }

  if (does_number_wrapper_end(p)) {
    end_wrapper_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
    end_value_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  return true;
}

/* |buf| is NULL-terminated. |buf| itself will never include quotes;
 * |is_quoted| tells us whether this text originally appeared inside quotes. */
static bool parse_number_from_buffer(upb_json_parser *p, const char *buf,
                                     bool is_quoted) {
  size_t len = strlen(buf);
  const char *bufend = buf + len;
  char *end;
  upb_fieldtype_t type = upb_fielddef_type(p->top->f);
  double val;
  double dummy;
  double inf = INFINITY;

  errno = 0;

  if (len == 0 || buf[0] == ' ') {
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
        upb_sink_putint32(p->top->sink, parser_getsel(p), (int32_t)val);
        return true;
      }
      UPB_UNREACHABLE();
    }
    case UPB_TYPE_UINT32: {
      unsigned long val = strtoul(buf, &end, 0);
      if (end != bufend) {
        break;
      } else if (val > UINT32_MAX || errno == ERANGE) {
        return false;
      } else {
        upb_sink_putuint32(p->top->sink, parser_getsel(p), (uint32_t)val);
        return true;
      }
      UPB_UNREACHABLE();
    }
    /* XXX: We can't handle [u]int64 properly on 32-bit machines because
     * strto[u]ll isn't in C89. */
    case UPB_TYPE_INT64: {
      long val = strtol(buf, &end, 0);
      if (errno == ERANGE || end != bufend) {
        break;
      } else {
        upb_sink_putint64(p->top->sink, parser_getsel(p), val);
        return true;
      }
      UPB_UNREACHABLE();
    }
    case UPB_TYPE_UINT64: {
      unsigned long val = strtoul(p->accumulated, &end, 0);
      if (end != bufend) {
        break;
      } else if (errno == ERANGE) {
        return false;
      } else {
        upb_sink_putuint64(p->top->sink, parser_getsel(p), val);
        return true;
      }
      UPB_UNREACHABLE();
    }
    default:
      break;
  }

  if (type != UPB_TYPE_DOUBLE && type != UPB_TYPE_FLOAT && is_quoted) {
    /* Quoted numbers for integer types are not allowed to be in double form. */
    return false;
  }

  if (len == strlen("Infinity") && strcmp(buf, "Infinity") == 0) {
    /* C89 does not have an INFINITY macro. */
    val = inf;
  } else if (len == strlen("-Infinity") && strcmp(buf, "-Infinity") == 0) {
    val = -inf;
  } else {
    val = strtod(buf, &end);
    if (errno == ERANGE || end != bufend) {
      return false;
    }
  }

  switch (type) {
#define CASE(capitaltype, smalltype, ctype, min, max)                     \
    case UPB_TYPE_ ## capitaltype: {                                      \
      if (modf(val, &dummy) != 0 || val > max || val < min) {             \
        return false;                                                     \
      } else {                                                            \
        upb_sink_put ## smalltype(p->top->sink, parser_getsel(p),        \
                                  (ctype)val);                            \
        return true;                                                      \
      }                                                                   \
      break;                                                              \
    }
    case UPB_TYPE_ENUM:
    CASE(INT32, int32, int32_t, INT32_MIN, INT32_MAX);
    CASE(INT64, int64, int64_t, INT64_MIN, INT64_MAX);
    CASE(UINT32, uint32, uint32_t, 0, UINT32_MAX);
    CASE(UINT64, uint64, uint64_t, 0, UINT64_MAX);
#undef CASE

    case UPB_TYPE_DOUBLE:
      upb_sink_putdouble(p->top->sink, parser_getsel(p), val);
      return true;
    case UPB_TYPE_FLOAT:
      if ((val > FLT_MAX || val < -FLT_MAX) && val != inf && val != -inf) {
        return false;
      } else {
        upb_sink_putfloat(p->top->sink, parser_getsel(p), val);
        return true;
      }
    default:
      return false;
  }
}

static bool parse_number(upb_json_parser *p, bool is_quoted) {
  size_t len;
  const char *buf;

  /* strtol() and friends unfortunately do not support specifying the length of
   * the input string, so we need to force a copy into a NULL-terminated buffer. */
  if (!multipart_text(p, "\0", 1, false)) {
    return false;
  }

  buf = accumulate_getptr(p, &len);

  if (parse_number_from_buffer(p, buf, is_quoted)) {
    multipart_end(p);
    return true;
  } else {
    upb_status_seterrf(p->status, "error parsing number: %s", buf);
    multipart_end(p);
    return false;
  }
}

static bool parser_putbool(upb_json_parser *p, bool val) {
  bool ok;

  if (p->top->f == NULL) {
    return true;
  }

  if (upb_fielddef_type(p->top->f) != UPB_TYPE_BOOL) {
    upb_status_seterrf(p->status,
                       "Boolean value specified for non-bool field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }

  ok = upb_sink_putbool(p->top->sink, parser_getsel(p), val);
  UPB_ASSERT(ok);

  return true;
}

static bool end_bool(upb_json_parser *p, bool val) {
  if (is_top_level(p)) {
    if (is_wellknown_msg(p, UPB_WELLKNOWN_BOOLVALUE)) {
      start_wrapper_object(p);
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
      start_value_object(p, VALUE_BOOLVALUE);
    } else {
      return false;
    }
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_BOOLVALUE)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_wrapper_object(p);
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_VALUE)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_value_object(p, VALUE_BOOLVALUE);
  }

  if (p->top->is_unknown_field) {
    return true;
  }

  if (!parser_putbool(p, val)) {
    return false;
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_BOOLVALUE)) {
    end_wrapper_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
    end_value_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  return true;
}

static bool end_null(upb_json_parser *p) {
  const char *zero_ptr = "0";

  if (is_top_level(p)) {
    if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
      start_value_object(p, VALUE_NULLVALUE);
    } else {
      return true;
    }
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_VALUE)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_value_object(p, VALUE_NULLVALUE);
  } else {
    return true;
  }

  /* Fill null_value field. */
  multipart_startaccum(p);
  capture_begin(p, zero_ptr);
  capture_end(p, zero_ptr + 1);
  parse_number(p, false);

  end_value_object(p);
  if (!is_top_level(p)) {
    end_subobject(p);
  }

  return true;
}

static bool start_any_stringval(upb_json_parser *p) {
  multipart_startaccum(p);
  return true;
}

static bool start_stringval(upb_json_parser *p) {
  if (is_top_level(p)) {
    if (is_string_wrapper_object(p) ||
        is_number_wrapper_object(p)) {
      start_wrapper_object(p);
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_FIELDMASK)) {
      start_fieldmask_object(p);
      return true;
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_TIMESTAMP) ||
               is_wellknown_msg(p, UPB_WELLKNOWN_DURATION)) {
      start_object(p);
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
      start_value_object(p, VALUE_STRINGVALUE);
    } else {
      return false;
    }
  } else if (does_string_wrapper_start(p) ||
             does_number_wrapper_start(p)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_wrapper_object(p);
  } else if (does_fieldmask_start(p)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_fieldmask_object(p);
    return true;
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_TIMESTAMP) ||
             is_wellknown_field(p, UPB_WELLKNOWN_DURATION)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_object(p);
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_VALUE)) {
    if (!start_subobject(p)) {
      return false;
    }
    start_value_object(p, VALUE_STRINGVALUE);
  }

  if (p->top->f == NULL) {
    multipart_startaccum(p);
    return true;
  }

  if (p->top->is_any) {
    return start_any_stringval(p);
  }

  if (upb_fielddef_isstring(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    if (!check_stack(p)) return false;

    /* Start a new parser frame: parser frames correspond one-to-one with
     * handler frames, and string events occur in a sub-frame. */
    inner = start_jsonparser_frame(p);
    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
    upb_sink_startstr(p->top->sink, sel, 0, &inner->sink);
    inner->m = p->top->m;
    inner->f = p->top->f;
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
    upb_status_seterrf(p->status,
                       "String specified for bool or submessage field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }
}

static bool end_any_stringval(upb_json_parser *p) {
  size_t len;
  const char *buf = accumulate_getptr(p, &len);

  /* Set type_url */
  upb_selector_t sel;
  upb_jsonparser_frame *inner;
  if (!check_stack(p)) return false;
  inner = p->top + 1;

  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
  upb_sink_startstr(p->top->sink, sel, 0, &inner->sink);
  sel = getsel_for_handlertype(p, UPB_HANDLER_STRING);
  upb_sink_putstring(inner->sink, sel, buf, len, NULL);
  sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
  upb_sink_endstr(inner->sink, sel);

  multipart_end(p);

  /* Resolve type url */
  if (strncmp(buf, "type.googleapis.com/", 20) == 0 && len > 20) {
    const upb_msgdef *payload_type = NULL;
    buf += 20;
    len -= 20;

    payload_type = upb_symtab_lookupmsg2(p->symtab, buf, len);
    if (payload_type == NULL) {
      upb_status_seterrf(
          p->status, "Cannot find packed type: %.*s\n", (int)len, buf);
      return false;
    }

    json_parser_any_frame_set_payload_type(p, p->top->any_frame, payload_type);

    return true;
  } else {
    upb_status_seterrf(
        p->status, "Invalid type url: %.*s\n", (int)len, buf);
    return false;
  }
}

static bool end_stringval_nontop(upb_json_parser *p) {
  bool ok = true;

  if (is_wellknown_msg(p, UPB_WELLKNOWN_TIMESTAMP) ||
      is_wellknown_msg(p, UPB_WELLKNOWN_DURATION)) {
    multipart_end(p);
    return true;
  }

  if (p->top->f == NULL) {
    multipart_end(p);
    return true;
  }

  if (p->top->is_any) {
    return end_any_stringval(p);
  }

  switch (upb_fielddef_type(p->top->f)) {
    case UPB_TYPE_BYTES:
      if (!base64_push(p, getsel_for_handlertype(p, UPB_HANDLER_STRING),
                       p->accumulated, p->accumulated_len)) {
        return false;
      }
      /* Fall through. */

    case UPB_TYPE_STRING: {
      upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
      upb_sink_endstr(p->top->sink, sel);
      p->top--;
      break;
    }

    case UPB_TYPE_ENUM: {
      /* Resolve enum symbolic name to integer value. */
      const upb_enumdef *enumdef = upb_fielddef_enumsubdef(p->top->f);

      size_t len;
      const char *buf = accumulate_getptr(p, &len);

      int32_t int_val = 0;
      ok = upb_enumdef_ntoi(enumdef, buf, len, &int_val);

      if (ok) {
        upb_selector_t sel = parser_getsel(p);
        upb_sink_putint32(p->top->sink, sel, int_val);
      } else {
        if (p->ignore_json_unknown) {
          ok = true;
          /* TODO(teboring): Should also clean this field. */
        } else {
          upb_status_seterrf(p->status, "Enum value unknown: '%.*s'", len, buf);
        }
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
      upb_status_seterrmsg(p->status, "Internal error in JSON decoder");
      ok = false;
      break;
  }

  multipart_end(p);

  return ok;
}

static bool end_stringval(upb_json_parser *p) {
  /* FieldMask's stringvals have been ended when handling them. Only need to
   * close FieldMask here.*/
  if (does_fieldmask_end(p)) {
    end_fieldmask_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  if (!end_stringval_nontop(p)) {
    return false;
  }

  if (does_string_wrapper_end(p) ||
      does_number_wrapper_end(p)) {
    end_wrapper_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
    end_value_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_TIMESTAMP) ||
      is_wellknown_msg(p, UPB_WELLKNOWN_DURATION) ||
      is_wellknown_msg(p, UPB_WELLKNOWN_FIELDMASK)) {
    end_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
    return true;
  }

  return true;
}

static void start_duration_base(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_duration_base(upb_json_parser *p, const char *ptr) {
  size_t len;
  const char *buf;
  char seconds_buf[14];
  char nanos_buf[12];
  char *end;
  int64_t seconds = 0;
  int32_t nanos = 0;
  double val = 0.0;
  const char *seconds_membername = "seconds";
  const char *nanos_membername = "nanos";
  size_t fraction_start;

  if (!capture_end(p, ptr)) {
    return false;
  }

  buf = accumulate_getptr(p, &len);

  memset(seconds_buf, 0, 14);
  memset(nanos_buf, 0, 12);

  /* Find out base end. The maximus duration is 315576000000, which cannot be
   * represented by double without losing precision. Thus, we need to handle
   * fraction and base separately. */
  for (fraction_start = 0; fraction_start < len && buf[fraction_start] != '.';
       fraction_start++);

  /* Parse base */
  memcpy(seconds_buf, buf, fraction_start);
  seconds = strtol(seconds_buf, &end, 10);
  if (errno == ERANGE || end != seconds_buf + fraction_start) {
    upb_status_seterrf(p->status, "error parsing duration: %s",
                       seconds_buf);
    return false;
  }

  if (seconds > 315576000000) {
    upb_status_seterrf(p->status, "error parsing duration: "
                                   "maximum acceptable value is "
                                   "315576000000");
    return false;
  }

  if (seconds < -315576000000) {
    upb_status_seterrf(p->status, "error parsing duration: "
                                   "minimum acceptable value is "
                                   "-315576000000");
    return false;
  }

  /* Parse fraction */
  nanos_buf[0] = '0';
  memcpy(nanos_buf + 1, buf + fraction_start, len - fraction_start);
  val = strtod(nanos_buf, &end);
  if (errno == ERANGE || end != nanos_buf + len - fraction_start + 1) {
    upb_status_seterrf(p->status, "error parsing duration: %s",
                       nanos_buf);
    return false;
  }

  nanos = val * 1000000000;
  if (seconds < 0) nanos = -nanos;

  /* Clean up buffer */
  multipart_end(p);

  /* Set seconds */
  start_member(p);
  capture_begin(p, seconds_membername);
  capture_end(p, seconds_membername + 7);
  end_membername(p);
  upb_sink_putint64(p->top->sink, parser_getsel(p), seconds);
  end_member(p);

  /* Set nanos */
  start_member(p);
  capture_begin(p, nanos_membername);
  capture_end(p, nanos_membername + 5);
  end_membername(p);
  upb_sink_putint32(p->top->sink, parser_getsel(p), nanos);
  end_member(p);

  /* Continue previous arena */
  multipart_startaccum(p);

  return true;
}

static int parse_timestamp_number(upb_json_parser *p) {
  size_t len;
  const char *buf;
  int val;

  /* atoi() and friends unfortunately do not support specifying the length of
   * the input string, so we need to force a copy into a NULL-terminated buffer. */
  multipart_text(p, "\0", 1, false);

  buf = accumulate_getptr(p, &len);
  val = atoi(buf);
  multipart_end(p);
  multipart_startaccum(p);

  return val;
}

static void start_year(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_year(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }
  p->tm.tm_year = parse_timestamp_number(p) - 1900;
  return true;
}

static void start_month(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_month(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }
  p->tm.tm_mon = parse_timestamp_number(p) - 1;
  return true;
}

static void start_day(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_day(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }
  p->tm.tm_mday = parse_timestamp_number(p);
  return true;
}

static void start_hour(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_hour(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }
  p->tm.tm_hour = parse_timestamp_number(p);
  return true;
}

static void start_minute(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_minute(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }
  p->tm.tm_min = parse_timestamp_number(p);
  return true;
}

static void start_second(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_second(upb_json_parser *p, const char *ptr) {
  if (!capture_end(p, ptr)) {
    return false;
  }
  p->tm.tm_sec = parse_timestamp_number(p);
  return true;
}

static void start_timestamp_base(upb_json_parser *p) {
  memset(&p->tm, 0, sizeof(struct tm));
}

static void start_timestamp_fraction(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_timestamp_fraction(upb_json_parser *p, const char *ptr) {
  size_t len;
  const char *buf;
  char nanos_buf[12];
  char *end;
  double val = 0.0;
  int32_t nanos;
  const char *nanos_membername = "nanos";

  memset(nanos_buf, 0, 12);

  if (!capture_end(p, ptr)) {
    return false;
  }

  buf = accumulate_getptr(p, &len);

  if (len > 10) {
    upb_status_seterrf(p->status,
        "error parsing timestamp: at most 9-digit fraction.");
    return false;
  }

  /* Parse nanos */
  nanos_buf[0] = '0';
  memcpy(nanos_buf + 1, buf, len);
  val = strtod(nanos_buf, &end);

  if (errno == ERANGE || end != nanos_buf + len + 1) {
    upb_status_seterrf(p->status, "error parsing timestamp nanos: %s",
                       nanos_buf);
    return false;
  }

  nanos = val * 1000000000;

  /* Clean up previous environment */
  multipart_end(p);

  /* Set nanos */
  start_member(p);
  capture_begin(p, nanos_membername);
  capture_end(p, nanos_membername + 5);
  end_membername(p);
  upb_sink_putint32(p->top->sink, parser_getsel(p), nanos);
  end_member(p);

  /* Continue previous environment */
  multipart_startaccum(p);

  return true;
}

static void start_timestamp_zone(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

/* epoch_days(1970, 1, 1) == 1970-01-01 == 0. */
static int epoch_days(int year, int month, int day) {
  static const uint16_t month_yday[12] = {0,   31,  59,  90,  120, 151,
                                          181, 212, 243, 273, 304, 334};
  uint32_t year_adj = year + 4800;  /* Ensure positive year, multiple of 400. */
  uint32_t febs = year_adj - (month <= 2 ? 1 : 0);  /* Februaries since base. */
  uint32_t leap_days = 1 + (febs / 4) - (febs / 100) + (febs / 400);
  uint32_t days = 365 * year_adj + leap_days + month_yday[month - 1] + day - 1;
  return days - 2472692;  /* Adjust to Unix epoch. */
}

static int64_t upb_timegm(const struct tm *tp) {
  int64_t ret = epoch_days(tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday);
  ret = (ret * 24) + tp->tm_hour;
  ret = (ret * 60) + tp->tm_min;
  ret = (ret * 60) + tp->tm_sec;
  return ret;
}

static bool end_timestamp_zone(upb_json_parser *p, const char *ptr) {
  size_t len;
  const char *buf;
  int hours;
  int64_t seconds;
  const char *seconds_membername = "seconds";

  if (!capture_end(p, ptr)) {
    return false;
  }

  buf = accumulate_getptr(p, &len);

  if (buf[0] != 'Z') {
    if (sscanf(buf + 1, "%2d:00", &hours) != 1) {
      upb_status_seterrf(p->status, "error parsing timestamp offset");
      return false;
    }

    if (buf[0] == '+') {
      hours = -hours;
    }

    p->tm.tm_hour += hours;
  }

  /* Normalize tm */
  seconds = upb_timegm(&p->tm);

  /* Check timestamp boundary */
  if (seconds < -62135596800) {
    upb_status_seterrf(p->status, "error parsing timestamp: "
                                   "minimum acceptable value is "
                                   "0001-01-01T00:00:00Z");
    return false;
  }

  /* Clean up previous environment */
  multipart_end(p);

  /* Set seconds */
  start_member(p);
  capture_begin(p, seconds_membername);
  capture_end(p, seconds_membername + 7);
  end_membername(p);
  upb_sink_putint64(p->top->sink, parser_getsel(p), seconds);
  end_member(p);

  /* Continue previous environment */
  multipart_startaccum(p);

  return true;
}

static void start_fieldmask_path_text(upb_json_parser *p, const char *ptr) {
  capture_begin(p, ptr);
}

static bool end_fieldmask_path_text(upb_json_parser *p, const char *ptr) {
  return capture_end(p, ptr);
}

static bool start_fieldmask_path(upb_json_parser *p) {
  upb_jsonparser_frame *inner;
  upb_selector_t sel;

  if (!check_stack(p)) return false;

  /* Start a new parser frame: parser frames correspond one-to-one with
   * handler frames, and string events occur in a sub-frame. */
  inner = start_jsonparser_frame(p);
  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
  upb_sink_startstr(p->top->sink, sel, 0, &inner->sink);
  inner->m = p->top->m;
  inner->f = p->top->f;
  p->top = inner;

  multipart_startaccum(p);
  return true;
}

static bool lower_camel_push(
    upb_json_parser *p, upb_selector_t sel, const char *ptr, size_t len) {
  const char *limit = ptr + len;
  bool first = true;
  for (;ptr < limit; ptr++) {
    if (*ptr >= 'A' && *ptr <= 'Z' && !first) {
      char lower = tolower(*ptr);
      upb_sink_putstring(p->top->sink, sel, "_", 1, NULL);
      upb_sink_putstring(p->top->sink, sel, &lower, 1, NULL);
    } else {
      upb_sink_putstring(p->top->sink, sel, ptr, 1, NULL);
    }
    first = false;
  }
  return true;
}

static bool end_fieldmask_path(upb_json_parser *p) {
  upb_selector_t sel;

  if (!lower_camel_push(
           p, getsel_for_handlertype(p, UPB_HANDLER_STRING),
           p->accumulated, p->accumulated_len)) {
    return false;
  }

  sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
  upb_sink_endstr(p->top->sink, sel);
  p->top--;

  multipart_end(p);
  return true;
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
    upb_status_seterrmsg(p->status, "mapentry message has no key");
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
        upb_status_seterrmsg(p->status,
                             "Map bool key not 'true' or 'false'");
        return false;
      }
      multipart_end(p);
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      upb_sink subsink;
      upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
      upb_sink_startstr(p->top->sink, sel, len, &subsink);
      sel = getsel_for_handlertype(p, UPB_HANDLER_STRING);
      upb_sink_putstring(subsink, sel, buf, len, NULL);
      sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
      upb_sink_endstr(subsink, sel);
      multipart_end(p);
      break;
    }
    default:
      upb_status_seterrmsg(p->status, "Invalid field type for map key");
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

  inner = start_jsonparser_frame(p);
  p->top->f = mapfield;
  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSUBMSG);
  upb_sink_startsubmsg(p->top->sink, sel, &inner->sink);
  inner->m = mapentrymsg;
  inner->mapfield = mapfield;

  /* Don't set this to true *yet* -- we reuse parsing handlers below to push
   * the key field value to the sink, and these handlers will pop the frame
   * if they see is_mapentry (when invoked by the parser state machine, they
   * would have just seen the map-entry value, not key). */
  inner->is_mapentry = false;
  p->top = inner;

  /* send STARTMSG in submsg frame. */
  upb_sink_startmsg(p->top->sink);

  parse_mapentry_key(p);

  /* Set up the value field to receive the map-entry value. */
  p->top->f = upb_msgdef_itof(p->top->m, UPB_MAPENTRY_VALUE);
  p->top->is_mapentry = true;  /* set up to pop frame after value is parsed. */
  p->top->mapfield = mapfield;
  if (p->top->f == NULL) {
    upb_status_seterrmsg(p->status, "mapentry message has no value");
    return false;
  }

  return true;
}

static bool end_membername(upb_json_parser *p) {
  UPB_ASSERT(!p->top->f);

  if (!p->top->m) {
    p->top->is_unknown_field = true;
    multipart_end(p);
    return true;
  }

  if (p->top->is_any) {
    return end_any_membername(p);
  } else if (p->top->is_map) {
    return handle_mapentry(p);
  } else {
    size_t len;
    const char *buf = accumulate_getptr(p, &len);
    upb_value v;

    if (upb_strtable_lookup2(p->top->name_table, buf, len, &v)) {
      p->top->f = upb_value_getconstptr(v);
      multipart_end(p);

      return true;
    } else if (p->ignore_json_unknown) {
      p->top->is_unknown_field = true;
      multipart_end(p);
      return true;
    } else {
      upb_status_seterrf(p->status, "No such field: %.*s\n", (int)len, buf);
      return false;
    }
  }
}

static bool end_any_membername(upb_json_parser *p) {
  size_t len;
  const char *buf = accumulate_getptr(p, &len);
  upb_value v;

  if (len == 5 && strncmp(buf, "@type", len) == 0) {
    upb_strtable_lookup2(p->top->name_table, "type_url", 8, &v);
    p->top->f = upb_value_getconstptr(v);
    multipart_end(p);
    return true;
  } else {
    p->top->is_unknown_field = true;
    multipart_end(p);
    return true;
  }
}

static void end_member(upb_json_parser *p) {
  /* If we just parsed a map-entry value, end that frame too. */
  if (p->top->is_mapentry) {
    upb_selector_t sel;
    bool ok;
    const upb_fielddef *mapfield;

    UPB_ASSERT(p->top > p->stack);
    /* send ENDMSG on submsg. */
    upb_sink_endmsg(p->top->sink, p->status);
    mapfield = p->top->mapfield;

    /* send ENDSUBMSG in repeated-field-of-mapentries frame. */
    p->top--;
    ok = upb_handlers_getselector(mapfield, UPB_HANDLER_ENDSUBMSG, &sel);
    UPB_ASSUME(ok);
    upb_sink_endsubmsg(p->top->sink, (p->top + 1)->sink, sel);
  }

  p->top->f = NULL;
  p->top->is_unknown_field = false;
}

static void start_any_member(upb_json_parser *p, const char *ptr) {
  start_member(p);
  json_parser_any_frame_set_after_type_url_start_once(p->top->any_frame, ptr);
}

static void end_any_member(upb_json_parser *p, const char *ptr) {
  json_parser_any_frame_set_before_type_url_end(p->top->any_frame, ptr);
  end_member(p);
}

static bool start_subobject(upb_json_parser *p) {
  if (p->top->is_unknown_field) {
    if (!check_stack(p)) return false;

    p->top = start_jsonparser_frame(p);
    return true;
  }

  if (upb_fielddef_ismap(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    /* Beginning of a map. Start a new parser frame in a repeated-field
     * context. */
    if (!check_stack(p)) return false;

    inner = start_jsonparser_frame(p);
    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSEQ);
    upb_sink_startseq(p->top->sink, sel, &inner->sink);
    inner->m = upb_fielddef_msgsubdef(p->top->f);
    inner->mapfield = p->top->f;
    inner->is_map = true;
    p->top = inner;

    return true;
  } else if (upb_fielddef_issubmsg(p->top->f)) {
    upb_jsonparser_frame *inner;
    upb_selector_t sel;

    /* Beginning of a subobject. Start a new parser frame in the submsg
     * context. */
    if (!check_stack(p)) return false;

    inner = start_jsonparser_frame(p);
    sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSUBMSG);
    upb_sink_startsubmsg(p->top->sink, sel, &inner->sink);
    inner->m = upb_fielddef_msgsubdef(p->top->f);
    set_name_table(p, inner);
    p->top = inner;

    if (is_wellknown_msg(p, UPB_WELLKNOWN_ANY)) {
      p->top->is_any = true;
      p->top->any_frame = json_parser_any_frame_new(p);
    } else {
      p->top->is_any = false;
      p->top->any_frame = NULL;
    }

    return true;
  } else {
    upb_status_seterrf(p->status,
                       "Object specified for non-message/group field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }
}

static bool start_subobject_full(upb_json_parser *p) {
  if (is_top_level(p)) {
    if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
      start_value_object(p, VALUE_STRUCTVALUE);
      if (!start_subobject(p)) return false;
      start_structvalue_object(p);
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_STRUCT)) {
      start_structvalue_object(p);
    } else {
      return true;
    }
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_STRUCT)) {
    if (!start_subobject(p)) return false;
    start_structvalue_object(p);
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_VALUE)) {
    if (!start_subobject(p)) return false;
    start_value_object(p, VALUE_STRUCTVALUE);
    if (!start_subobject(p)) return false;
    start_structvalue_object(p);
  }

  return start_subobject(p);
}

static void end_subobject(upb_json_parser *p) {
  if (is_top_level(p)) {
    return;
  }

  if (p->top->is_map) {
    upb_selector_t sel;
    p->top--;
    sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSEQ);
    upb_sink_endseq(p->top->sink, sel);
  } else {
    upb_selector_t sel;
    bool is_unknown = p->top->m == NULL;
    p->top--;
    if (!is_unknown) {
      sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSUBMSG);
      upb_sink_endsubmsg(p->top->sink, (p->top + 1)->sink, sel);
    }
  }
}

static void end_subobject_full(upb_json_parser *p) {
  end_subobject(p);

  if (is_wellknown_msg(p, UPB_WELLKNOWN_STRUCT)) {
    end_structvalue_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
    end_value_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
  }
}

static bool start_array(upb_json_parser *p) {
  upb_jsonparser_frame *inner;
  upb_selector_t sel;

  if (is_top_level(p)) {
    if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
      start_value_object(p, VALUE_LISTVALUE);
      if (!start_subobject(p)) return false;
      start_listvalue_object(p);
    } else if (is_wellknown_msg(p, UPB_WELLKNOWN_LISTVALUE)) {
      start_listvalue_object(p);
    } else {
      return false;
    }
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_LISTVALUE) &&
             (!upb_fielddef_isseq(p->top->f) ||
              p->top->is_repeated)) {
    if (!start_subobject(p)) return false;
    start_listvalue_object(p);
  } else if (is_wellknown_field(p, UPB_WELLKNOWN_VALUE) &&
             (!upb_fielddef_isseq(p->top->f) ||
              p->top->is_repeated)) {
    if (!start_subobject(p)) return false;
    start_value_object(p, VALUE_LISTVALUE);
    if (!start_subobject(p)) return false;
    start_listvalue_object(p);
  }

  if (p->top->is_unknown_field) {
    inner = start_jsonparser_frame(p);
    inner->is_unknown_field = true;
    p->top = inner;

    return true;
  }

  if (!upb_fielddef_isseq(p->top->f)) {
    upb_status_seterrf(p->status,
                       "Array specified for non-repeated field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }

  if (!check_stack(p)) return false;

  inner = start_jsonparser_frame(p);
  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSEQ);
  upb_sink_startseq(p->top->sink, sel, &inner->sink);
  inner->m = p->top->m;
  inner->f = p->top->f;
  inner->is_repeated = true;
  p->top = inner;

  return true;
}

static void end_array(upb_json_parser *p) {
  upb_selector_t sel;

  UPB_ASSERT(p->top > p->stack);

  p->top--;

  if (p->top->is_unknown_field) {
    return;
  }

  sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSEQ);
  upb_sink_endseq(p->top->sink, sel);

  if (is_wellknown_msg(p, UPB_WELLKNOWN_LISTVALUE)) {
    end_listvalue_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
  }

  if (is_wellknown_msg(p, UPB_WELLKNOWN_VALUE)) {
    end_value_object(p);
    if (!is_top_level(p)) {
      end_subobject(p);
    }
  }
}

static void start_object(upb_json_parser *p) {
  if (!p->top->is_map && p->top->m != NULL) {
    upb_sink_startmsg(p->top->sink);
  }
}

static void end_object(upb_json_parser *p) {
  if (!p->top->is_map && p->top->m != NULL) {
    upb_sink_endmsg(p->top->sink, p->status);
  }
}

static void start_any_object(upb_json_parser *p, const char *ptr) {
  start_object(p);
  p->top->any_frame->before_type_url_start = ptr;
  p->top->any_frame->before_type_url_end = ptr;
}

static bool end_any_object(upb_json_parser *p, const char *ptr) {
  const char *value_membername = "value";
  bool is_well_known_packed = false;
  const char *packed_end = ptr + 1;
  upb_selector_t sel;
  upb_jsonparser_frame *inner;

  if (json_parser_any_frame_has_value(p->top->any_frame) &&
      !json_parser_any_frame_has_type_url(p->top->any_frame)) {
    upb_status_seterrmsg(p->status, "No valid type url");
    return false;
  }

  /* Well known types data is represented as value field. */
  if (upb_msgdef_wellknowntype(p->top->any_frame->parser->top->m) !=
          UPB_WELLKNOWN_UNSPECIFIED) {
    is_well_known_packed = true;

    if (json_parser_any_frame_has_value_before_type_url(p->top->any_frame)) {
      p->top->any_frame->before_type_url_start =
          memchr(p->top->any_frame->before_type_url_start, ':',
                 p->top->any_frame->before_type_url_end -
                 p->top->any_frame->before_type_url_start);
      if (p->top->any_frame->before_type_url_start == NULL) {
        upb_status_seterrmsg(p->status, "invalid data for well known type.");
        return false;
      }
      p->top->any_frame->before_type_url_start++;
    }

    if (json_parser_any_frame_has_value_after_type_url(p->top->any_frame)) {
      p->top->any_frame->after_type_url_start =
          memchr(p->top->any_frame->after_type_url_start, ':',
                 (ptr + 1) -
                 p->top->any_frame->after_type_url_start);
      if (p->top->any_frame->after_type_url_start == NULL) {
        upb_status_seterrmsg(p->status, "Invalid data for well known type.");
        return false;
      }
      p->top->any_frame->after_type_url_start++;
      packed_end = ptr;
    }
  }

  if (json_parser_any_frame_has_value_before_type_url(p->top->any_frame)) {
    if (!parse(p->top->any_frame->parser, NULL,
               p->top->any_frame->before_type_url_start,
               p->top->any_frame->before_type_url_end -
               p->top->any_frame->before_type_url_start, NULL)) {
      return false;
    }
  } else {
    if (!is_well_known_packed) {
      if (!parse(p->top->any_frame->parser, NULL, "{", 1, NULL)) {
        return false;
      }
    }
  }

  if (json_parser_any_frame_has_value_before_type_url(p->top->any_frame) &&
      json_parser_any_frame_has_value_after_type_url(p->top->any_frame)) {
    if (!parse(p->top->any_frame->parser, NULL, ",", 1, NULL)) {
      return false;
    }
  }

  if (json_parser_any_frame_has_value_after_type_url(p->top->any_frame)) {
    if (!parse(p->top->any_frame->parser, NULL,
               p->top->any_frame->after_type_url_start,
               packed_end - p->top->any_frame->after_type_url_start, NULL)) {
      return false;
    }
  } else {
    if (!is_well_known_packed) {
      if (!parse(p->top->any_frame->parser, NULL, "}", 1, NULL)) {
        return false;
      }
    }
  }

  if (!end(p->top->any_frame->parser, NULL)) {
    return false;
  }

  p->top->is_any = false;

  /* Set value */
  start_member(p);
  capture_begin(p, value_membername);
  capture_end(p, value_membername + 5);
  end_membername(p);

  if (!check_stack(p)) return false;
  inner = p->top + 1;

  sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
  upb_sink_startstr(p->top->sink, sel, 0, &inner->sink);
  sel = getsel_for_handlertype(p, UPB_HANDLER_STRING);
  upb_sink_putstring(inner->sink, sel, p->top->any_frame->stringsink.ptr,
                     p->top->any_frame->stringsink.len, NULL);
  sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
  upb_sink_endstr(inner->sink, sel);

  end_member(p);

  end_object(p);

  /* Deallocate any parse frame. */
  json_parser_any_frame_free(p->top->any_frame);

  return true;
}

static bool is_string_wrapper(const upb_msgdef *m) {
  upb_wellknowntype_t type = upb_msgdef_wellknowntype(m);
  return type == UPB_WELLKNOWN_STRINGVALUE ||
         type == UPB_WELLKNOWN_BYTESVALUE;
}

static bool is_fieldmask(const upb_msgdef *m) {
  upb_wellknowntype_t type = upb_msgdef_wellknowntype(m);
  return type == UPB_WELLKNOWN_FIELDMASK;
}

static void start_fieldmask_object(upb_json_parser *p) {
  const char *membername = "paths";

  start_object(p);

  /* Set up context for parsing value */
  start_member(p);
  capture_begin(p, membername);
  capture_end(p, membername + 5);
  end_membername(p);

  start_array(p);
}

static void end_fieldmask_object(upb_json_parser *p) {
  end_array(p);
  end_member(p);
  end_object(p);
}

static void start_wrapper_object(upb_json_parser *p) {
  const char *membername = "value";

  start_object(p);

  /* Set up context for parsing value */
  start_member(p);
  capture_begin(p, membername);
  capture_end(p, membername + 5);
  end_membername(p);
}

static void end_wrapper_object(upb_json_parser *p) {
  end_member(p);
  end_object(p);
}

static void start_value_object(upb_json_parser *p, int value_type) {
  const char *nullmember = "null_value";
  const char *numbermember = "number_value";
  const char *stringmember = "string_value";
  const char *boolmember = "bool_value";
  const char *structmember = "struct_value";
  const char *listmember = "list_value";
  const char *membername = "";

  switch (value_type) {
    case VALUE_NULLVALUE:
      membername = nullmember;
      break;
    case VALUE_NUMBERVALUE:
      membername = numbermember;
      break;
    case VALUE_STRINGVALUE:
      membername = stringmember;
      break;
    case VALUE_BOOLVALUE:
      membername = boolmember;
      break;
    case VALUE_STRUCTVALUE:
      membername = structmember;
      break;
    case VALUE_LISTVALUE:
      membername = listmember;
      break;
  }

  start_object(p);

  /* Set up context for parsing value */
  start_member(p);
  capture_begin(p, membername);
  capture_end(p, membername + strlen(membername));
  end_membername(p);
}

static void end_value_object(upb_json_parser *p) {
  end_member(p);
  end_object(p);
}

static void start_listvalue_object(upb_json_parser *p) {
  const char *membername = "values";

  start_object(p);

  /* Set up context for parsing value */
  start_member(p);
  capture_begin(p, membername);
  capture_end(p, membername + strlen(membername));
  end_membername(p);
}

static void end_listvalue_object(upb_json_parser *p) {
  end_member(p);
  end_object(p);
}

static void start_structvalue_object(upb_json_parser *p) {
  const char *membername = "fields";

  start_object(p);

  /* Set up context for parsing value */
  start_member(p);
  capture_begin(p, membername);
  capture_end(p, membername + strlen(membername));
  end_membername(p);
}

static void end_structvalue_object(upb_json_parser *p) {
  end_member(p);
  end_object(p);
}

static bool is_top_level(upb_json_parser *p) {
  return p->top == p->stack && p->top->f == NULL && !p->top->is_unknown_field;
}

static bool is_wellknown_msg(upb_json_parser *p, upb_wellknowntype_t type) {
  return p->top->m != NULL && upb_msgdef_wellknowntype(p->top->m) == type;
}

static bool is_wellknown_field(upb_json_parser *p, upb_wellknowntype_t type) {
  return p->top->f != NULL &&
         upb_fielddef_issubmsg(p->top->f) &&
         (upb_msgdef_wellknowntype(upb_fielddef_msgsubdef(p->top->f))
              == type);
}

static bool does_number_wrapper_start(upb_json_parser *p) {
  return p->top->f != NULL &&
         upb_fielddef_issubmsg(p->top->f) &&
         upb_msgdef_isnumberwrapper(upb_fielddef_msgsubdef(p->top->f));
}

static bool does_number_wrapper_end(upb_json_parser *p) {
  return p->top->m != NULL && upb_msgdef_isnumberwrapper(p->top->m);
}

static bool is_number_wrapper_object(upb_json_parser *p) {
  return p->top->m != NULL && upb_msgdef_isnumberwrapper(p->top->m);
}

static bool does_string_wrapper_start(upb_json_parser *p) {
  return p->top->f != NULL &&
         upb_fielddef_issubmsg(p->top->f) &&
         is_string_wrapper(upb_fielddef_msgsubdef(p->top->f));
}

static bool does_string_wrapper_end(upb_json_parser *p) {
  return p->top->m != NULL && is_string_wrapper(p->top->m);
}

static bool is_string_wrapper_object(upb_json_parser *p) {
  return p->top->m != NULL && is_string_wrapper(p->top->m);
}

static bool does_fieldmask_start(upb_json_parser *p) {
  return p->top->f != NULL &&
         upb_fielddef_issubmsg(p->top->f) &&
         is_fieldmask(upb_fielddef_msgsubdef(p->top->f));
}

static bool does_fieldmask_end(upb_json_parser *p) {
  return p->top->m != NULL && is_fieldmask(p->top->m);
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

%%{
  machine json;

  ws = space*;

  integer  = "0" | /[1-9]/ /[0-9]/*;
  decimal  = "." /[0-9]/+;
  exponent = /[eE]/ /[+\-]/? /[0-9]/+;

  number_machine :=
    ("-"? integer decimal? exponent?)
      %/{ fhold; fret; }
    <: any
      >{ fhold; fret; }
    ;
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

  year = 
    (digit digit digit digit)
      >{ start_year(parser, p); }
      %{ CHECK_RETURN_TOP(end_year(parser, p)); }
    ;
  month =
    (digit digit)
      >{ start_month(parser, p); }
      %{ CHECK_RETURN_TOP(end_month(parser, p)); }
    ;
  day =
    (digit digit)
      >{ start_day(parser, p); }
      %{ CHECK_RETURN_TOP(end_day(parser, p)); }
    ;
  hour =
    (digit digit)
      >{ start_hour(parser, p); }
      %{ CHECK_RETURN_TOP(end_hour(parser, p)); }
    ;
  minute =
    (digit digit)
      >{ start_minute(parser, p); }
      %{ CHECK_RETURN_TOP(end_minute(parser, p)); }
    ;
  second =
    (digit digit)
      >{ start_second(parser, p); }
      %{ CHECK_RETURN_TOP(end_second(parser, p)); }
    ;

  duration_machine :=
    ("-"? integer decimal?)
      >{ start_duration_base(parser, p); }
      %{ CHECK_RETURN_TOP(end_duration_base(parser, p)); }
    's"'
      @{ fhold; fret; }
    ;

  timestamp_machine :=
    (year "-" month "-" day "T" hour ":" minute ":" second)
      >{ start_timestamp_base(parser); }
    ("." digit+)?
      >{ start_timestamp_fraction(parser, p); }
      %{ CHECK_RETURN_TOP(end_timestamp_fraction(parser, p)); }
    ([+\-] digit digit ":00" | "Z")
      >{ start_timestamp_zone(parser, p); }
      %{ CHECK_RETURN_TOP(end_timestamp_zone(parser, p)); }
    '"'
      @{ fhold; fret; }
    ;

  fieldmask_path_text =
    /[^",]/+
      >{ start_fieldmask_path_text(parser, p); }
      %{ end_fieldmask_path_text(parser, p); }
    ;

  fieldmask_path =
    fieldmask_path_text
      >{ start_fieldmask_path(parser); }
      %{ end_fieldmask_path(parser); }
    ;

  fieldmask_machine :=
    (fieldmask_path ("," fieldmask_path)*)?
    '"'
      @{ fhold; fret; }
    ;

  string =
    '"'
      @{
        if (is_wellknown_msg(parser, UPB_WELLKNOWN_TIMESTAMP)) {
          fcall timestamp_machine;
        } else if (is_wellknown_msg(parser, UPB_WELLKNOWN_DURATION)) {
          fcall duration_machine;
        } else if (is_wellknown_msg(parser, UPB_WELLKNOWN_FIELDMASK)) {
          fcall fieldmask_machine;
        } else {
          fcall string_machine;
        }
      }
    '"';

  value2 = ^(space | "]" | "}") >{ fhold; fcall value_machine; } ;

  member =
    ws
    string
      >{
        if (is_wellknown_msg(parser, UPB_WELLKNOWN_ANY)) {
          start_any_member(parser, p);
        } else {
          start_member(parser);
        }
      }
      @{ CHECK_RETURN_TOP(end_membername(parser)); }
    ws ":" ws
    value2
      %{
        if (is_wellknown_msg(parser, UPB_WELLKNOWN_ANY)) {
          end_any_member(parser, p);
        } else {
          end_member(parser);
        }
      }
    ws;

  object =
    ("{" ws)
      >{
        if (is_wellknown_msg(parser, UPB_WELLKNOWN_ANY)) {
          start_any_object(parser, p);
        } else {
          start_object(parser);
        }
      }
    (member ("," member)*)?
    "}"
      >{
        if (is_wellknown_msg(parser, UPB_WELLKNOWN_ANY)) {
          CHECK_RETURN_TOP(end_any_object(parser, p));
        } else {
          end_object(parser);
        }
      }
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
      >{ CHECK_RETURN_TOP(start_number(parser, p)); }
      %{ CHECK_RETURN_TOP(end_number(parser, p)); }
    | string
      >{ CHECK_RETURN_TOP(start_stringval(parser)); }
      @{ CHECK_RETURN_TOP(end_stringval(parser)); }
    | "true"
      %{ CHECK_RETURN_TOP(end_bool(parser, true)); }
    | "false"
      %{ CHECK_RETURN_TOP(end_bool(parser, false)); }
    | "null"
      %{ CHECK_RETURN_TOP(end_null(parser)); }
    | object
      >{ CHECK_RETURN_TOP(start_subobject_full(parser)); }
      %{ end_subobject_full(parser); }
    | array;

  value_machine :=
    value
    <: any >{ fhold; fret; } ;

  main := ws value ws;
}%%

%% write data noerror nofinal;

size_t parse(void *closure, const void *hd, const char *buf, size_t size,
             const upb_bufhandle *handle) {
  upb_json_parser *parser = closure;

  /* Variables used by Ragel's generated code. */
  int cs = parser->current_state;
  int *stack = parser->parser_stack;
  int top = parser->parser_top;

  const char *p = buf;
  const char *pe = buf + size;
  const char *eof = &eof_ch;

  parser->handle = handle;

  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  capture_resume(parser, buf);

  %% write exec;

  if (p != pe) {
    upb_status_seterrf(parser->status, "Parse error at '%.*s'\n", pe - p, p);
  } else {
    capture_suspend(parser, &p);
  }

error:
  /* Save parsing state back to parser. */
  parser->current_state = cs;
  parser->parser_top = top;

  return p - buf;
}

static bool end(void *closure, const void *hd) {
  upb_json_parser *parser = closure;

  /* Prevent compile warning on unused static constants. */
  UPB_UNUSED(json_start);
  UPB_UNUSED(json_en_duration_machine);
  UPB_UNUSED(json_en_fieldmask_machine);
  UPB_UNUSED(json_en_number_machine);
  UPB_UNUSED(json_en_string_machine);
  UPB_UNUSED(json_en_timestamp_machine);
  UPB_UNUSED(json_en_value_machine);
  UPB_UNUSED(json_en_main);

  parse(parser, hd, &eof_ch, 0, NULL);

  return parser->current_state >= %%{ write first_final; }%%;
}

static void json_parser_reset(upb_json_parser *p) {
  int cs;
  int top;

  p->top = p->stack;
  init_frame(p->top);

  /* Emit Ragel initialization of the parser. */
  %% write init;
  p->current_state = cs;
  p->parser_top = top;
  accumulate_clear(p);
  p->multipart_state = MULTIPART_INACTIVE;
  p->capture = NULL;
  p->accumulated = NULL;
}

static upb_json_parsermethod *parsermethod_new(upb_json_codecache *c,
                                               const upb_msgdef *md) {
  int i, n;
  upb_alloc *alloc = upb_arena_alloc(c->arena);

  upb_json_parsermethod *m = upb_malloc(alloc, sizeof(*m));

  m->cache = c;

  upb_byteshandler_init(&m->input_handler_);
  upb_byteshandler_setstring(&m->input_handler_, parse, m);
  upb_byteshandler_setendstr(&m->input_handler_, end, m);

  upb_strtable_init2(&m->name_table, UPB_CTYPE_CONSTPTR, 4, alloc);

  /* Build name_table */

  n = upb_msgdef_fieldcount(md);
  for(i = 0; i < n; i++) {
    const upb_fielddef *f = upb_msgdef_field(md, i);
    upb_value v = upb_value_constptr(f);
    const char *name;

    /* Add an entry for the JSON name. */
    name = upb_fielddef_jsonname(f);
    upb_strtable_insert3(&m->name_table, name, strlen(name), v, alloc);

    if (strcmp(name, upb_fielddef_name(f)) != 0) {
      /* Since the JSON name is different from the regular field name, add an
       * entry for the raw name (compliant proto3 JSON parsers must accept
       * both). */
      const char *name = upb_fielddef_name(f);
      upb_strtable_insert3(&m->name_table, name, strlen(name), v, alloc);
    }
  }

  return m;
}

/* Public API *****************************************************************/

upb_json_parser *upb_json_parser_create(upb_arena *arena,
                                        const upb_json_parsermethod *method,
                                        const upb_symtab* symtab,
                                        upb_sink output,
                                        upb_status *status,
                                        bool ignore_json_unknown) {
  upb_json_parser *p = upb_arena_malloc(arena, sizeof(upb_json_parser));
  if (!p) return false;

  p->arena = arena;
  p->method = method;
  p->status = status;
  p->limit = p->stack + UPB_JSON_MAX_DEPTH;
  p->accumulate_buf = NULL;
  p->accumulate_buf_size = 0;
  upb_bytessink_reset(&p->input_, &method->input_handler_, p);

  json_parser_reset(p);
  p->top->sink = output;
  p->top->m = upb_handlers_msgdef(output.handlers);
  if (is_wellknown_msg(p, UPB_WELLKNOWN_ANY)) {
    p->top->is_any = true;
    p->top->any_frame = json_parser_any_frame_new(p);
  } else {
    p->top->is_any = false;
    p->top->any_frame = NULL;
  }
  set_name_table(p, p->top);
  p->symtab = symtab;

  p->ignore_json_unknown = ignore_json_unknown;

  return p;
}

upb_bytessink upb_json_parser_input(upb_json_parser *p) {
  return p->input_;
}

const upb_byteshandler *upb_json_parsermethod_inputhandler(
    const upb_json_parsermethod *m) {
  return &m->input_handler_;
}

upb_json_codecache *upb_json_codecache_new(void) {
  upb_alloc *alloc;
  upb_json_codecache *c;

  c = upb_gmalloc(sizeof(*c));

  c->arena = upb_arena_new();
  alloc = upb_arena_alloc(c->arena);

  upb_inttable_init2(&c->methods, UPB_CTYPE_CONSTPTR, alloc);

  return c;
}

void upb_json_codecache_free(upb_json_codecache *c) {
  upb_arena_free(c->arena);
  upb_gfree(c);
}

const upb_json_parsermethod *upb_json_codecache_get(upb_json_codecache *c,
                                                    const upb_msgdef *md) {
  upb_json_parsermethod *m;
  upb_value v;
  int i, n;
  upb_alloc *alloc = upb_arena_alloc(c->arena);

  if (upb_inttable_lookupptr(&c->methods, md, &v)) {
    return upb_value_getconstptr(v);
  }

  m = parsermethod_new(c, md);
  v = upb_value_constptr(m);

  if (!m) return NULL;
  if (!upb_inttable_insertptr2(&c->methods, md, v, alloc)) return NULL;

  /* Populate parser methods for all submessages, so the name tables will
   * be available during parsing. */
  n = upb_msgdef_fieldcount(md);
  for(i = 0; i < n; i++) {
    const upb_fielddef *f = upb_msgdef_field(md, i);

    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
      const upb_json_parsermethod *sub_method =
          upb_json_codecache_get(c, subdef);

      if (!sub_method) return NULL;
    }
  }

  return m;
}
