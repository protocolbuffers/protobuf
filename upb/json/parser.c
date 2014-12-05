
#line 1 "upb/json/parser.rl"
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

#line 568 "upb/json/parser.rl"



#line 486 "upb/json/parser.c"
static const char _json_actions[] = {
	0, 1, 0, 1, 2, 1, 3, 1, 
	4, 1, 5, 1, 6, 1, 7, 1, 
	9, 1, 11, 1, 12, 1, 13, 1, 
	14, 1, 15, 1, 16, 1, 24, 1, 
	26, 2, 3, 7, 2, 5, 2, 2, 
	5, 7, 2, 10, 8, 2, 12, 14, 
	2, 13, 14, 2, 17, 1, 2, 18, 
	26, 2, 19, 8, 2, 20, 26, 2, 
	21, 26, 2, 22, 26, 2, 23, 26, 
	2, 25, 26, 3, 13, 10, 8
};

static const unsigned char _json_key_offsets[] = {
	0, 0, 4, 9, 14, 18, 22, 27, 
	32, 37, 41, 45, 48, 51, 53, 57, 
	61, 63, 65, 70, 72, 74, 83, 89, 
	95, 101, 107, 109, 118, 118, 118, 123, 
	128, 133, 133, 134, 135, 136, 137, 137, 
	138, 139, 140, 140, 141, 142, 143, 143, 
	148, 153, 157, 161, 166, 171, 176, 180, 
	180, 183, 183, 183
};

static const char _json_trans_keys[] = {
	32, 123, 9, 13, 32, 34, 125, 9, 
	13, 32, 34, 125, 9, 13, 32, 58, 
	9, 13, 32, 58, 9, 13, 32, 93, 
	125, 9, 13, 32, 44, 125, 9, 13, 
	32, 44, 125, 9, 13, 32, 34, 9, 
	13, 45, 48, 49, 57, 48, 49, 57, 
	46, 69, 101, 48, 57, 69, 101, 48, 
	57, 43, 45, 48, 57, 48, 57, 48, 
	57, 46, 69, 101, 48, 57, 34, 92, 
	34, 92, 34, 47, 92, 98, 102, 110, 
	114, 116, 117, 48, 57, 65, 70, 97, 
	102, 48, 57, 65, 70, 97, 102, 48, 
	57, 65, 70, 97, 102, 48, 57, 65, 
	70, 97, 102, 34, 92, 34, 45, 91, 
	102, 110, 116, 123, 48, 57, 32, 93, 
	125, 9, 13, 32, 44, 93, 9, 13, 
	32, 93, 125, 9, 13, 97, 108, 115, 
	101, 117, 108, 108, 114, 117, 101, 32, 
	34, 125, 9, 13, 32, 34, 125, 9, 
	13, 32, 58, 9, 13, 32, 58, 9, 
	13, 32, 93, 125, 9, 13, 32, 44, 
	125, 9, 13, 32, 44, 125, 9, 13, 
	32, 34, 9, 13, 32, 9, 13, 0
};

static const char _json_single_lengths[] = {
	0, 2, 3, 3, 2, 2, 3, 3, 
	3, 2, 2, 1, 3, 0, 2, 2, 
	0, 0, 3, 2, 2, 9, 0, 0, 
	0, 0, 2, 7, 0, 0, 3, 3, 
	3, 0, 1, 1, 1, 1, 0, 1, 
	1, 1, 0, 1, 1, 1, 0, 3, 
	3, 2, 2, 3, 3, 3, 2, 0, 
	1, 0, 0, 0
};

static const char _json_range_lengths[] = {
	0, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 0, 1, 1, 1, 
	1, 1, 1, 0, 0, 0, 3, 3, 
	3, 3, 0, 1, 0, 0, 1, 1, 
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 1, 1, 1, 1, 1, 1, 0, 
	1, 0, 0, 0
};

static const short _json_index_offsets[] = {
	0, 0, 4, 9, 14, 18, 22, 27, 
	32, 37, 41, 45, 48, 52, 54, 58, 
	62, 64, 66, 71, 74, 77, 87, 91, 
	95, 99, 103, 106, 115, 116, 117, 122, 
	127, 132, 133, 135, 137, 139, 141, 142, 
	144, 146, 148, 149, 151, 153, 155, 156, 
	161, 166, 170, 174, 179, 184, 189, 193, 
	194, 197, 198, 199
};

static const char _json_indicies[] = {
	0, 2, 0, 1, 3, 4, 5, 3, 
	1, 6, 7, 8, 6, 1, 9, 10, 
	9, 1, 11, 12, 11, 1, 12, 1, 
	1, 12, 13, 14, 15, 16, 14, 1, 
	17, 18, 8, 17, 1, 18, 7, 18, 
	1, 19, 20, 21, 1, 20, 21, 1, 
	23, 24, 24, 22, 25, 1, 24, 24, 
	25, 22, 26, 26, 27, 1, 27, 1, 
	27, 22, 23, 24, 24, 21, 22, 29, 
	30, 28, 32, 33, 31, 34, 34, 34, 
	34, 34, 34, 34, 34, 35, 1, 36, 
	36, 36, 1, 37, 37, 37, 1, 38, 
	38, 38, 1, 39, 39, 39, 1, 41, 
	42, 40, 43, 44, 45, 46, 47, 48, 
	49, 44, 1, 50, 51, 53, 54, 1, 
	53, 52, 55, 56, 54, 55, 1, 56, 
	1, 1, 56, 52, 57, 58, 1, 59, 
	1, 60, 1, 61, 1, 62, 63, 1, 
	64, 1, 65, 1, 66, 67, 1, 68, 
	1, 69, 1, 70, 71, 72, 73, 71, 
	1, 74, 75, 76, 74, 1, 77, 78, 
	77, 1, 79, 80, 79, 1, 80, 1, 
	1, 80, 81, 82, 83, 84, 82, 1, 
	85, 86, 76, 85, 1, 86, 75, 86, 
	1, 87, 88, 88, 1, 1, 1, 1, 
	0
};

static const char _json_trans_targs[] = {
	1, 0, 2, 3, 4, 56, 3, 4, 
	56, 5, 6, 5, 6, 7, 8, 9, 
	56, 8, 9, 11, 12, 18, 57, 13, 
	15, 14, 16, 17, 20, 58, 21, 20, 
	58, 21, 19, 22, 23, 24, 25, 26, 
	20, 58, 21, 28, 29, 30, 34, 39, 
	43, 47, 59, 59, 31, 30, 33, 31, 
	32, 59, 35, 36, 37, 38, 59, 40, 
	41, 42, 59, 44, 45, 46, 59, 48, 
	49, 55, 48, 49, 55, 50, 51, 50, 
	51, 52, 53, 54, 55, 53, 54, 59, 
	56
};

static const char _json_trans_actions[] = {
	0, 0, 0, 21, 75, 48, 0, 42, 
	23, 17, 17, 0, 0, 15, 19, 19, 
	45, 0, 0, 0, 0, 0, 1, 0, 
	0, 0, 0, 0, 3, 13, 0, 0, 
	33, 5, 11, 0, 7, 0, 0, 0, 
	36, 39, 9, 57, 51, 25, 0, 0, 
	0, 29, 60, 54, 15, 0, 27, 0, 
	0, 31, 0, 0, 0, 0, 66, 0, 
	0, 0, 69, 0, 0, 0, 63, 21, 
	75, 48, 0, 42, 23, 17, 17, 0, 
	0, 15, 19, 19, 45, 0, 0, 72, 
	0
};

static const int json_start = 1;
static const int json_first_final = 56;
static const int json_error = 0;

static const int json_en_number_machine = 10;
static const int json_en_string_machine = 19;
static const int json_en_value_machine = 27;
static const int json_en_main = 1;


#line 571 "upb/json/parser.rl"

size_t parse(void *closure, const void *hd, const char *buf, size_t size,
             const upb_bufhandle *handle) {
  upb_json_parser *parser = closure;

  // Variables used by Ragel's generated code.
  int cs = parser->current_state;
  int *stack = parser->parser_stack;
  int top = parser->parser_top;

  const char *p = buf;
  const char *pe = buf + size;

  
#line 654 "upb/json/parser.c"
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
#line 489 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
	case 1:
#line 490 "upb/json/parser.rl"
	{ p--; {stack[top++] = cs; cs = 10; goto _again;} }
	break;
	case 2:
#line 494 "upb/json/parser.rl"
	{ start_text(parser, p); }
	break;
	case 3:
#line 495 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_text(parser, p)); }
	break;
	case 4:
#line 501 "upb/json/parser.rl"
	{ start_hex(parser, p); }
	break;
	case 5:
#line 502 "upb/json/parser.rl"
	{ hex(parser, p); }
	break;
	case 6:
#line 508 "upb/json/parser.rl"
	{ escape(parser, p); }
	break;
	case 7:
#line 511 "upb/json/parser.rl"
	{ {cs = stack[--top]; goto _again;} }
	break;
	case 8:
#line 512 "upb/json/parser.rl"
	{ {stack[top++] = cs; cs = 19; goto _again;} }
	break;
	case 9:
#line 514 "upb/json/parser.rl"
	{ p--; {stack[top++] = cs; cs = 27; goto _again;} }
	break;
	case 10:
#line 519 "upb/json/parser.rl"
	{ start_member(parser, p); }
	break;
	case 11:
#line 520 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(end_member(parser, p)); }
	break;
	case 12:
#line 523 "upb/json/parser.rl"
	{ clear_member(parser); }
	break;
	case 13:
#line 529 "upb/json/parser.rl"
	{ start_object(parser); }
	break;
	case 14:
#line 532 "upb/json/parser.rl"
	{ end_object(parser); }
	break;
	case 15:
#line 538 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_array(parser)); }
	break;
	case 16:
#line 542 "upb/json/parser.rl"
	{ end_array(parser); }
	break;
	case 17:
#line 547 "upb/json/parser.rl"
	{ start_number(parser, p); }
	break;
	case 18:
#line 548 "upb/json/parser.rl"
	{ end_number(parser, p); }
	break;
	case 19:
#line 550 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_stringval(parser, p)); }
	break;
	case 20:
#line 551 "upb/json/parser.rl"
	{ end_stringval(parser, p); }
	break;
	case 21:
#line 553 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(putbool(parser, true)); }
	break;
	case 22:
#line 555 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(putbool(parser, false)); }
	break;
	case 23:
#line 557 "upb/json/parser.rl"
	{ /* null value */ }
	break;
	case 24:
#line 559 "upb/json/parser.rl"
	{ CHECK_RETURN_TOP(start_subobject(parser)); }
	break;
	case 25:
#line 560 "upb/json/parser.rl"
	{ end_subobject(parser); }
	break;
	case 26:
#line 565 "upb/json/parser.rl"
	{ p--; {cs = stack[--top]; goto _again;} }
	break;
#line 836 "upb/json/parser.c"
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

#line 585 "upb/json/parser.rl"

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
  
#line 888 "upb/json/parser.c"
	{
	cs = json_start;
	top = 0;
	}

#line 623 "upb/json/parser.rl"
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
