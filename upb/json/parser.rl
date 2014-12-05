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

#define CHECK_RETURN(x) if (!(x)) return false

static upb_selector_t getsel_for_handlertype(upb_json_parser *p,
                                             upb_handlertype_t type) {
  upb_selector_t sel;
  bool ok = upb_handlers_getselector(p->top->f, type, &sel);
  UPB_ASSERT_VAR(ok, ok);
  return sel;
}

static upb_selector_t getsel(upb_json_parser *p) {
  return getsel_for_handlertype(
      p, upb_handlers_getprimitivehandlertype(p->top->f));
}

static void start_member(upb_json_parser *p, const char *ptr) {
  assert(!p->top->f);
  assert(!p->accumulated);
  p->accumulated_len = 0;
}

static bool end_member(upb_json_parser *p, const char *ptr) {
  // TODO(haberman): support keys that span buffers or have escape sequences.
  assert(!p->top->f);
  assert(p->accumulated);
  const upb_fielddef *f =
      upb_msgdef_ntof(p->top->m, p->accumulated, p->accumulated_len);

  if (!f) {
    // TODO(haberman): Ignore unknown fields if requested/configured to do so.
    upb_status_seterrf(p->status, "No such field: %.*s\n",
                       (int)p->accumulated_len, p->accumulated);
    return false;
  }

  p->top->f = f;
  p->accumulated = NULL;

  return true;
}

static void start_object(upb_json_parser *p) {
  upb_sink_startmsg(&p->top->sink);
}

static void end_object(upb_json_parser *p) {
  upb_status status;
  upb_sink_endmsg(&p->top->sink, &status);
}

static bool check_stack(upb_json_parser *p) {
  if ((p->top + 1) == p->limit) {
    upb_status_seterrmsg(p->status, "Nesting too deep");
    return false;
  }

  return true;
}

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

static void clear_member(upb_json_parser *p) { p->top->f = NULL; }

static bool putbool(upb_json_parser *p, bool val) {
  if (upb_fielddef_type(p->top->f) != UPB_TYPE_BOOL) {
    upb_status_seterrf(p->status,
                       "Boolean value specified for non-bool field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }

  bool ok = upb_sink_putbool(&p->top->sink, getsel(p), val);
  UPB_ASSERT_VAR(ok, ok);
  return true;
}

static void start_text(upb_json_parser *p, const char *ptr) {
  p->text_begin = ptr;
}

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

static bool end_text(upb_json_parser *p, const char *ptr) {
  assert(!p->accumulated);  // TODO: handle this case.
  p->accumulated = p->text_begin;
  p->accumulated_len = ptr - p->text_begin;

  if (p->top->f && upb_fielddef_isstring(p->top->f)) {
    // This is a string field (as opposed to a member name).
    upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_STRING);
    if (upb_fielddef_type(p->top->f) == UPB_TYPE_BYTES) {
      CHECK_RETURN(base64_push(p, sel, p->accumulated, p->accumulated_len));
    } else {
      upb_sink_putstring(&p->top->sink, sel, p->accumulated, p->accumulated_len, NULL);
    }
    p->accumulated = NULL;
  }

  return true;
}

static bool start_stringval(upb_json_parser *p, const char *ptr) {
  assert(p->top->f);

  if (!upb_fielddef_isstring(p->top->f)) {
    upb_status_seterrf(p->status,
                       "String specified for non-string field: %s",
                       upb_fielddef_name(p->top->f));
    return false;
  }

  if (!check_stack(p)) return false;

  upb_jsonparser_frame *inner = p->top + 1;  // TODO: check for overflow.
  upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_STARTSTR);
  upb_sink_startstr(&p->top->sink, sel, 0, &inner->sink);
  inner->m = p->top->m;
  inner->f = p->top->f;
  p->top = inner;

  return true;
}

static void end_stringval(upb_json_parser *p, const char *ptr) {
  p->top--;
  upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_ENDSTR);
  upb_sink_endstr(&p->top->sink, sel);
}

static void start_number(upb_json_parser *p, const char *ptr) {
  start_text(p, ptr);
  assert(p->accumulated == NULL);
}

static void end_number(upb_json_parser *p, const char *ptr) {
  end_text(p, ptr);
  const char *myend = p->accumulated + p->accumulated_len;
  char *end;

  switch (upb_fielddef_type(p->top->f)) {
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32: {
      long val = strtol(p->accumulated, &end, 0);
      if (val > INT32_MAX || val < INT32_MIN || errno == ERANGE || end != myend)
        assert(false);
      else
        upb_sink_putint32(&p->top->sink, getsel(p), val);
      break;
    }
    case UPB_TYPE_INT64: {
      long long val = strtoll(p->accumulated, &end, 0);
      if (val > INT64_MAX || val < INT64_MIN || errno == ERANGE || end != myend)
        assert(false);
      else
        upb_sink_putint64(&p->top->sink, getsel(p), val);
      break;
    }
    case UPB_TYPE_UINT32: {
      unsigned long val = strtoul(p->accumulated, &end, 0);
      if (val > UINT32_MAX || errno == ERANGE || end != myend)
        assert(false);
      else
        upb_sink_putuint32(&p->top->sink, getsel(p), val);
      break;
    }
    case UPB_TYPE_UINT64: {
      unsigned long long val = strtoull(p->accumulated, &end, 0);
      if (val > UINT64_MAX || errno == ERANGE || end != myend)
        assert(false);
      else
        upb_sink_putuint64(&p->top->sink, getsel(p), val);
      break;
    }
    case UPB_TYPE_DOUBLE: {
      double val = strtod(p->accumulated, &end);
      if (errno == ERANGE || end != myend)
        assert(false);
      else
        upb_sink_putdouble(&p->top->sink, getsel(p), val);
      break;
    }
    case UPB_TYPE_FLOAT: {
      float val = strtof(p->accumulated, &end);
      if (errno == ERANGE || end != myend)
        assert(false);
      else
        upb_sink_putfloat(&p->top->sink, getsel(p), val);
      break;
    }
    default:
      assert(false);
  }

  p->accumulated = NULL;
}

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

static void escape(upb_json_parser *p, const char *ptr) {
  char ch = escape_char(*ptr);
  upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_STRING);
  upb_sink_putstring(&p->top->sink, sel, &ch, 1, NULL);
}

static uint8_t hexdigit(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  } else if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  } else {
    assert(ch >= 'A' && ch <= 'F');
    return ch - 'A' + 10;
  }
}

static void start_hex(upb_json_parser *p, const char *ptr) {
  start_text(p, ptr);
}

static void hex(upb_json_parser *p, const char *end) {
  const char *start = p->text_begin;
  assert(end - start == 4);
  uint16_t codepoint =
      (hexdigit(start[0]) << 12) |
      (hexdigit(start[1]) << 8) |
      (hexdigit(start[2]) << 4) |
      hexdigit(start[3]);
  // emit the codepoint as UTF-8.
  char utf8[3]; // support \u0000 -- \uFFFF -- need only three bytes.
  int length = 0;
  if (codepoint < 0x7F) {
    utf8[0] = codepoint;
    length = 1;
  } else if (codepoint < 0x07FF) {
    utf8[1] = (codepoint & 0x3F) | 0x80;
    codepoint >>= 6;
    utf8[0] = (codepoint & 0x1F) | 0xC0;
    length = 2;
  } else /* codepoint < 0xFFFF */ {
    utf8[2] = (codepoint & 0x3F) | 0x80;
    codepoint >>= 6;
    utf8[1] = (codepoint & 0x3F) | 0x80;
    codepoint >>= 6;
    utf8[0] = (codepoint & 0x0F) | 0xE0;
    length = 3;
  }
  // TODO(haberman): Handle high surrogates: if codepoint is a high surrogate
  // we have to wait for the next escape to get the full code point).

  upb_selector_t sel = getsel_for_handlertype(p, UPB_HANDLER_STRING);
  upb_sink_putstring(&p->top->sink, sel, utf8, length, NULL);
}

#define CHECK_RETURN_TOP(x) if (!(x)) goto error

// What follows is the Ragel parser itself.  The language is specified in Ragel
// and the actions call our C functions above.
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
      >{ start_hex(parser, p); }
      %{ hex(parser, p); }
    ;

  escape_char  =
    "\\"
    /[rtbfn"\/\\]/
      >{ escape(parser, p); }
    ;

  string_machine := (text | unicode_char | escape_char)** '"' @{ fret; } ;
  string       = '"' @{ fcall string_machine; };

  value2 = ^(space | "]" | "}") >{ fhold; fcall value_machine; } ;

  member =
    ws
    string
      >{ start_member(parser, p); }
      %{ CHECK_RETURN_TOP(end_member(parser, p)); }
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
      %{ end_number(parser, p); }
    | string
      >{ CHECK_RETURN_TOP(start_stringval(parser, p)); }
      %{ end_stringval(parser, p); }
    | "true"
      %{ CHECK_RETURN_TOP(putbool(parser, true)); }
    | "false"
      %{ CHECK_RETURN_TOP(putbool(parser, false)); }
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
  upb_json_parser *parser = closure;

  // Variables used by Ragel's generated code.
  int cs = parser->current_state;
  int *stack = parser->parser_stack;
  int top = parser->parser_top;

  const char *p = buf;
  const char *pe = buf + size;

  %% write exec;

  if (p != pe) {
    upb_status_seterrf(parser->status, "Parse error at %s\n", p);
  }

error:
  // Save parsing state back to parser.
  parser->current_state = cs;
  parser->parser_top = top;

  return p - buf;
}

bool end(void *closure, const void *hd) {
  return true;
}

void upb_json_parser_init(upb_json_parser *p, upb_status *status) {
  p->limit = p->stack + UPB_JSON_MAX_DEPTH;
  upb_byteshandler_init(&p->input_handler_);
  upb_byteshandler_setstring(&p->input_handler_, parse, NULL);
  upb_byteshandler_setendstr(&p->input_handler_, end, NULL);
  upb_bytessink_reset(&p->input_, &p->input_handler_, p);
  p->status = status;
}

void upb_json_parser_uninit(upb_json_parser *p) {
  upb_byteshandler_uninit(&p->input_handler_);
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
  p->text_begin = NULL;
  p->accumulated = NULL;
  p->accumulated_len = 0;
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
