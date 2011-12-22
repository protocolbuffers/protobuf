/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 *
 * An exhaustive set of tests for parsing both valid and invalid protobuf
 * input, with buffer breaks in arbitrary places.
 *
 * Tests to add:
 * - unknown field handler called appropriately
 * - unknown fields can be inserted in random places
 * - fuzzing of valid input
 * - resource limits (max stack depth, max string len)
 * - testing of groups
 * - more throrough testing of sequences
 * - test skipping of submessages
 * - test suspending the decoder
 * - buffers that are close enough to the end of the address space that
 *   pointers overflow (this might be difficult).
 * - a few "kitchen sink" examples (one proto that uses all types, lots
 *   of submsg/sequences, etc.
 */

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "upb/handlers.h"
#include "upb/pb/decoder.h"
#include "upb/pb/varint.h"
#include "upb/upb.h"
#include "upb_test.h"

typedef struct {
  char *buf;
  size_t len;
} buffer;

// Mem is initialized to NULL.
buffer *buffer_new(size_t len) {
  buffer *buf = malloc(sizeof(*buf));
  buf->buf = malloc(len);
  buf->len = len;
  memset(buf->buf, 0, buf->len);
  return buf;
}

buffer *buffer_new2(const void *data, size_t len) {
  buffer *buf = buffer_new(len);
  memcpy(buf->buf, data, len);
  return buf;
}

buffer *buffer_new3(const char *data) {
  return buffer_new2(data, strlen(data));
}

buffer *buffer_dup(buffer *buf) { return buffer_new2(buf->buf, buf->len); }

void buffer_free(buffer *buf) {
  free(buf->buf);
  free(buf);
}

void buffer_appendf(buffer *buf, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t size = buf->len;
  buf->len += upb_vrprintf(&buf->buf, &size, buf->len, fmt, args);
  va_end(args);
}

void buffer_cat(buffer *buf, buffer *buf2) {
  size_t newlen = buf->len + buf2->len;
  buf->buf = realloc(buf->buf, newlen);
  memcpy(buf->buf + buf->len, buf2->buf, buf2->len);
  buf->len = newlen;
  buffer_free(buf2);
}

bool buffer_eql(buffer *buf, buffer *buf2) {
  return buf->len == buf2->len && memcmp(buf->buf, buf2->buf, buf->len) == 0;
}


/* Routines for building arbitrary protos *************************************/

buffer *cat(buffer *arg1, ...) {
  va_list ap;
  buffer *arg;
  va_start(ap, arg1);
  while ((arg = va_arg(ap, buffer*)) != NULL) {
    buffer_cat(arg1, arg);
  }
  va_end(ap);
  return arg1;
}

buffer *varint(uint64_t x) {
  buffer *buf = buffer_new(UPB_PB_VARINT_MAX_LEN + 1);
  buf->len = upb_vencode64(x, buf->buf);
  return buf;
}

// TODO: proper byte-swapping for big-endian machines.
buffer *fixed32(void *data) { return buffer_new2(data, 4); }
buffer *fixed64(void *data) { return buffer_new2(data, 8); }

buffer *delim(buffer *buf) { return cat( varint(buf->len), buf, NULL ); }
buffer *uint32(uint32_t u32) { return fixed32(&u32); }
buffer *uint64(uint64_t u64) { return fixed64(&u64); }
buffer *flt(float f) { return fixed32(&f); }
buffer *dbl(double d) { return fixed64(&d); }
buffer *zz32(int32_t x) { return varint(upb_zzenc_32(x)); }
buffer *zz64(int64_t x) { return varint(upb_zzenc_64(x)); }

buffer *tag(uint32_t fieldnum, char wire_type) {
  return varint((fieldnum << 3) | wire_type);
}

buffer *submsg(uint32_t fn, buffer *buf) {
  return cat( tag(fn, UPB_WIRE_TYPE_DELIMITED), delim(buf), NULL );
}


/* A set of handlers that covers all .proto types *****************************/

// The handlers simply append to a string indicating what handlers were called.
// This string is similar to protobuf text format but fields are referred to by
// number instead of name and sequences are explicitly delimited.

#define VALUE_HANDLER(member, fmt) \
  upb_flow_t value_ ## member(void *closure, upb_value fval, upb_value val) { \
    buffer_appendf(closure, "%" PRIu32 ":%" fmt "; ",                         \
                   upb_value_getuint32(fval), upb_value_get ## member(val));  \
    return UPB_CONTINUE;                                                      \
  }

VALUE_HANDLER(uint32, PRIu32)
VALUE_HANDLER(uint64, PRIu64)
VALUE_HANDLER(int32, PRId32)
VALUE_HANDLER(int64, PRId64)
VALUE_HANDLER(float, "g")
VALUE_HANDLER(double, "g")

upb_flow_t value_bool(void *closure, upb_value fval, upb_value val) {
  buffer_appendf(closure, "%" PRIu32 ":%s; ",
                 upb_value_getuint32(fval),
                 upb_value_getbool(val) ? "true" : "false");
  return UPB_CONTINUE;
}

upb_flow_t value_string(void *closure, upb_value fval, upb_value val) {
  // Note: won't work with strings that contain NULL.
  char *str = upb_byteregion_strdup(upb_value_getbyteregion(val));
  buffer_appendf(closure, "%" PRIu32 ":%s; ", upb_value_getuint32(fval), str);
  free(str);
  return UPB_CONTINUE;
}

upb_sflow_t startsubmsg(void *closure, upb_value fval) {
  buffer_appendf(closure, "%" PRIu32 ":{ ", upb_value_getuint32(fval));
  return UPB_CONTINUE_WITH(closure);
}

upb_flow_t endsubmsg(void *closure, upb_value fval) {
  (void)fval;
  buffer_appendf(closure, "} ");
  return UPB_CONTINUE;
}

upb_sflow_t startseq(void *closure, upb_value fval) {
  buffer_appendf(closure, "%" PRIu32 ":[ ", upb_value_getuint32(fval));
  return UPB_CONTINUE_WITH(closure);
}

upb_flow_t endseq(void *closure, upb_value fval) {
  (void)fval;
  buffer_appendf(closure, "] ");
  return UPB_CONTINUE;
}

void doreg(upb_mhandlers *m, uint32_t num, upb_fieldtype_t type, bool repeated,
           upb_value_handler *handler) {
  upb_fhandlers *f = upb_mhandlers_newfhandlers(m, num, type, repeated);
  ASSERT(f);
  upb_fhandlers_setvalue(f, handler);
  upb_fhandlers_setstartseq(f, &startseq);
  upb_fhandlers_setendseq(f, &endseq);
  upb_fhandlers_setfval(f, upb_value_uint32(num));
}

// The repeated field number to correspond to the given non-repeated field
// number.
uint32_t rep_fn(uint32_t fn) {
  return (UPB_MAX_FIELDNUMBER - 1000) + fn;
}

#define NOP_FIELD 40
#define UNKNOWN_FIELD 666

void reg(upb_mhandlers *m, upb_fieldtype_t type, upb_value_handler *handler) {
  // We register both a repeated and a non-repeated field for every type.
  // For the non-repeated field we make the field number the same as the
  // type.  For the repeated field we make it a function of the type.
  doreg(m, type, type, false, handler);
  doreg(m, rep_fn(type), type, true, handler);
}

void reg_subm(upb_mhandlers *m, uint32_t num, upb_fieldtype_t type,
              bool repeated) {
  upb_fhandlers *f =
      upb_mhandlers_newfhandlers_subm(m, num, type, repeated, m);
  ASSERT(f);
  upb_fhandlers_setstartseq(f, &startseq);
  upb_fhandlers_setendseq(f, &endseq);
  upb_fhandlers_setstartsubmsg(f, &startsubmsg);
  upb_fhandlers_setendsubmsg(f, &endsubmsg);
  upb_fhandlers_setfval(f, upb_value_uint32(num));
}

void reghandlers(upb_mhandlers *m) {
  // Register handlers for each type.
  reg(m, UPB_TYPE(DOUBLE),   &value_double);
  reg(m, UPB_TYPE(FLOAT),    &value_float);
  reg(m, UPB_TYPE(INT64),    &value_int64);
  reg(m, UPB_TYPE(UINT64),   &value_uint64);
  reg(m, UPB_TYPE(INT32) ,   &value_int32);
  reg(m, UPB_TYPE(FIXED64),  &value_uint64);
  reg(m, UPB_TYPE(FIXED32),  &value_uint32);
  reg(m, UPB_TYPE(BOOL),     &value_bool);
  reg(m, UPB_TYPE(STRING),   &value_string);
  reg(m, UPB_TYPE(BYTES),    &value_string);
  reg(m, UPB_TYPE(UINT32),   &value_uint32);
  reg(m, UPB_TYPE(ENUM),     &value_int32);
  reg(m, UPB_TYPE(SFIXED32), &value_int32);
  reg(m, UPB_TYPE(SFIXED64), &value_int64);
  reg(m, UPB_TYPE(SINT32),   &value_int32);
  reg(m, UPB_TYPE(SINT64),   &value_int64);

  // Register submessage/group handlers that are self-recursive
  // to this type, eg: message M { optional M m = 1; }
  reg_subm(m, UPB_TYPE(MESSAGE),         UPB_TYPE(MESSAGE), false);
  reg_subm(m, UPB_TYPE(GROUP),           UPB_TYPE(GROUP),   false);
  reg_subm(m, rep_fn(UPB_TYPE(MESSAGE)), UPB_TYPE(MESSAGE), true);
  reg_subm(m, rep_fn(UPB_TYPE(GROUP)),   UPB_TYPE(GROUP),   true);

  // Register a no-op string field so we can pad the proto wherever we want.
  upb_mhandlers_newfhandlers(m, NOP_FIELD, UPB_TYPE(STRING), false);
}


/* Custom bytesrc that can insert buffer seams in arbitrary places ************/

typedef struct {
  upb_bytesrc bytesrc;
  const char *str;
  size_t len, seam1, seam2;
  upb_byteregion byteregion;
} upb_seamsrc;

size_t upb_seamsrc_avail(const upb_seamsrc *src, size_t ofs) {
  if (ofs < src->seam1) return src->seam1 - ofs;
  if (ofs < src->seam2) return src->seam2 - ofs;
  return src->len - ofs;
}

upb_bytesuccess_t upb_seamsrc_fetch(void *_src, uint64_t ofs, size_t *read) {
  upb_seamsrc *src = _src;
  assert(ofs < src->len);
  if (ofs == src->len) {
    upb_status_seteof(&src->bytesrc.status);
    return UPB_BYTE_EOF;
  }
  *read = upb_seamsrc_avail(src, ofs);
  return UPB_BYTE_OK;
}

void upb_seamsrc_copy(const void *_src, uint64_t ofs,
                      size_t len, char *dst) {
  const upb_seamsrc *src = _src;
  assert(ofs + len <= src->len);
  memcpy(dst, src->str + ofs, len);
}

void upb_seamsrc_discard(void *src, uint64_t ofs) {
  (void)src;
  (void)ofs;
}

const char *upb_seamsrc_getptr(const void *_s, uint64_t ofs, size_t *len) {
  const upb_seamsrc *src = _s;
  *len = upb_seamsrc_avail(src, ofs);
  return src->str + ofs;
}

void upb_seamsrc_init(upb_seamsrc *s, const char *str, size_t len) {
  static upb_bytesrc_vtbl vtbl = {
    &upb_seamsrc_fetch,
    &upb_seamsrc_discard,
    &upb_seamsrc_copy,
    &upb_seamsrc_getptr,
  };
  upb_bytesrc_init(&s->bytesrc, &vtbl);
  s->seam1 = 0;
  s->seam2 = 0;
  s->str = str;
  s->len = len;
  s->byteregion.bytesrc = &s->bytesrc;
  s->byteregion.toplevel = true;
  s->byteregion.start = 0;
  s->byteregion.end = len;
}

void upb_seamsrc_resetseams(upb_seamsrc *s, size_t seam1, size_t seam2) {
  ASSERT(seam1 <= seam2);
  s->seam1 = seam1;
  s->seam2 = seam2;
  s->byteregion.discard = 0;
  s->byteregion.fetch = 0;
}

void upb_seamsrc_uninit(upb_seamsrc *s) { (void)s; }

upb_bytesrc *upb_seamsrc_bytesrc(upb_seamsrc *s) {
  return &s->bytesrc;
}

// Returns the top-level upb_byteregion* for this seamsrc.  Invalidated when
// the seamsrc is reset.
upb_byteregion *upb_seamsrc_allbytes(upb_seamsrc *s) {
  return &s->byteregion;
}


/* Running of test cases ******************************************************/

upb_decoderplan *plan;

void run_decoder(buffer *proto, buffer *expected_output) {
  upb_seamsrc src;
  upb_seamsrc_init(&src, proto->buf, proto->len);
  upb_decoder d;
  upb_decoder_init(&d);
  upb_decoder_resetplan(&d, plan, 0);
  for (size_t i = 0; i < proto->len; i++) {
    for (size_t j = i; j < proto->len; j++) {
      upb_seamsrc_resetseams(&src, i, j);
      upb_byteregion *input = upb_seamsrc_allbytes(&src);
      buffer *output = buffer_new(0);
      upb_decoder_resetinput(&d, input, output);
      upb_success_t success = UPB_SUSPENDED;
      while (success == UPB_SUSPENDED)
        success = upb_decoder_decode(&d);
      ASSERT(upb_ok(upb_decoder_status(&d)) == (success == UPB_OK));
      if (expected_output) {
        ASSERT(success == UPB_OK);
        // The input should be fully consumed.
        ASSERT(upb_byteregion_fetchofs(input) == upb_byteregion_endofs(input));
        ASSERT(upb_byteregion_discardofs(input) ==
               upb_byteregion_endofs(input));
        if (!buffer_eql(output, expected_output)) {
          fprintf(stderr, "Text mismatch: '%s' vs '%s'\n",
                  output->buf, expected_output->buf);
        }
        ASSERT(strcmp(output->buf, expected_output->buf) == 0);
      } else {
        ASSERT(success == UPB_ERROR);
      }
      buffer_free(output);
    }
  }
  upb_seamsrc_uninit(&src);
  upb_decoder_uninit(&d);
  buffer_free(proto);
}

void assert_successful_parse_at_eof(buffer *proto, const char *expected_fmt,
                                    va_list args) {
  buffer *expected_text = buffer_new(0);
  size_t size = expected_text->len;
  expected_text->len += upb_vrprintf(&expected_text->buf, &size,
                                     expected_text->len, expected_fmt, args);
  run_decoder(proto, expected_text);
  buffer_free(expected_text);
}

void assert_does_not_parse_at_eof(buffer *proto) {
  run_decoder(proto, NULL);
}

void assert_successful_parse(buffer *proto, const char *expected_fmt, ...) {
  // The JIT is only used for data >=20 bytes from end-of-buffer, so
  // repeat once with no-op padding data at the end of buffer.
  va_list args, args2;
  va_start(args, expected_fmt);
  va_copy(args2, args);
  assert_successful_parse_at_eof(buffer_dup(proto), expected_fmt, args);
  assert_successful_parse_at_eof(
      cat( proto,
           tag(NOP_FIELD, UPB_WIRE_TYPE_DELIMITED), delim(buffer_new(30)),
           NULL ),
      expected_fmt, args2);
  va_end(args);
  va_end(args2);
}

void assert_does_not_parse(buffer *proto) {
  // The JIT is only used for data >=20 bytes from end-of-buffer, so
  // repeat once with no-op padding data at the end of buffer.
  assert_does_not_parse_at_eof(buffer_dup(proto));
  assert_does_not_parse_at_eof(
      cat( proto,
           tag(NOP_FIELD, UPB_WIRE_TYPE_DELIMITED), delim( buffer_new(30)),
           NULL ));
}


/* The actual tests ***********************************************************/

void test_premature_eof_for_type(upb_fieldtype_t type) {
  // Incomplete values for each wire type.
  static const char *incompletes[] = {
    "\x80",    // UPB_WIRE_TYPE_VARINT
    "abcdefg", // UPB_WIRE_TYPE_64BIT
    "\x80",    // UPB_WIRE_TYPE_DELIMITED (partial length)
    NULL,      // UPB_WIRE_TYPE_START_GROUP (no value required)
    NULL,      // UPB_WIRE_TYPE_END_GROUP (no value required)
    "abc"      // UPB_WIRE_TYPE_32BIT
  };

  uint32_t fieldnum = type;
  uint32_t rep_fieldnum = rep_fn(type);
  int wire_type = upb_types[type].native_wire_type;
  const char *incomplete = incompletes[wire_type];

  // EOF before a known non-repeated value.
  assert_does_not_parse_at_eof(tag(fieldnum, wire_type));

  // EOF before a known repeated value.
  assert_does_not_parse_at_eof(tag(rep_fieldnum, wire_type));

  // EOF before an unknown value.
  assert_does_not_parse_at_eof(tag(UNKNOWN_FIELD, wire_type));

  // EOF inside a known non-repeated value.
  assert_does_not_parse_at_eof(
      cat( tag(fieldnum, wire_type), buffer_new3(incomplete), NULL ));

  // EOF inside a known repeated value.
  assert_does_not_parse_at_eof(
      cat( tag(rep_fieldnum, wire_type), buffer_new3(incomplete), NULL ));

  // EOF inside an unknown value.
  assert_does_not_parse_at_eof(
      cat( tag(UNKNOWN_FIELD, wire_type), buffer_new3(incomplete), NULL ));

  if (wire_type == UPB_WIRE_TYPE_DELIMITED) {
    // EOF in the middle of delimited data for known non-repeated value.
    assert_does_not_parse_at_eof(
        cat( tag(fieldnum, wire_type), varint(1), NULL ));

    // EOF in the middle of delimited data for known repeated value.
    assert_does_not_parse_at_eof(
        cat( tag(rep_fieldnum, wire_type), varint(1), NULL ));

    // EOF in the middle of delimited data for unknown value.
    assert_does_not_parse_at_eof(
        cat( tag(UNKNOWN_FIELD, wire_type), varint(1), NULL ));

    if (type == UPB_TYPE(MESSAGE)) {
      // Submessage ends in the middle of a value.
      buffer *incomplete_submsg =
          cat ( tag(UPB_TYPE(INT32), UPB_WIRE_TYPE_VARINT),
                buffer_new3(incompletes[UPB_WIRE_TYPE_VARINT]), NULL );
      assert_does_not_parse(
          cat( tag(fieldnum, UPB_WIRE_TYPE_DELIMITED),
               varint(incomplete_submsg->len),
               incomplete_submsg, NULL ));
    }
  } else {
    // Packed region ends in the middle of a value.
    assert_does_not_parse(
        cat( tag(rep_fieldnum, UPB_WIRE_TYPE_DELIMITED),
             varint(strlen(incomplete)),
             buffer_new3(incomplete), NULL ));

    // EOF in the middle of packed region.
    assert_does_not_parse_at_eof(
        cat( tag(rep_fieldnum, UPB_WIRE_TYPE_DELIMITED), varint(1), NULL ));
  }
}

// "33" and "66" are just two random values that all numeric types can
// represent.
void test_valid_data_for_type(upb_fieldtype_t type,
                              buffer *enc33, buffer *enc66) {
  uint32_t fieldnum = type;
  uint32_t rep_fieldnum = rep_fn(type);
  int wire_type = upb_types[type].native_wire_type;

  // Non-repeated
  assert_successful_parse(
      cat( tag(fieldnum, wire_type), buffer_dup(enc33),
           tag(fieldnum, wire_type), buffer_dup(enc66), NULL ),
      "%u:33; %u:66; ", fieldnum, fieldnum);

  // Non-packed repeated.
  assert_successful_parse(
      cat( tag(rep_fieldnum, wire_type), buffer_dup(enc33),
           tag(rep_fieldnum, wire_type), buffer_dup(enc66), NULL ),
      "%u:[ %u:33; %u:66; ] ", rep_fieldnum, rep_fieldnum, rep_fieldnum);

  // Packed repeated.
  assert_successful_parse(
      cat( tag(rep_fieldnum, UPB_WIRE_TYPE_DELIMITED),
           delim(cat( buffer_dup(enc33), buffer_dup(enc66), NULL )), NULL ),
      "%u:[ %u:33; %u:66; ] ", rep_fieldnum, rep_fieldnum, rep_fieldnum);

  buffer_free(enc33);
  buffer_free(enc66);
}

void test_valid_data_for_signed_type(upb_fieldtype_t type,
                                     buffer *enc33, buffer *enc66) {
  uint32_t fieldnum = type;
  uint32_t rep_fieldnum = rep_fn(type);
  int wire_type = upb_types[type].native_wire_type;

  // Non-repeated
  assert_successful_parse(
      cat( tag(fieldnum, wire_type), buffer_dup(enc33),
           tag(fieldnum, wire_type), buffer_dup(enc66), NULL ),
      "%u:33; %u:-66; ", fieldnum, fieldnum);

  // Non-packed repeated.
  assert_successful_parse(
      cat( tag(rep_fieldnum, wire_type), buffer_dup(enc33),
           tag(rep_fieldnum, wire_type), buffer_dup(enc66), NULL ),
      "%u:[ %u:33; %u:-66; ] ", rep_fieldnum, rep_fieldnum, rep_fieldnum);

  // Packed repeated.
  assert_successful_parse(
      cat( tag(rep_fieldnum, UPB_WIRE_TYPE_DELIMITED),
           delim(cat( buffer_dup(enc33), buffer_dup(enc66), NULL )), NULL ),
      "%u:[ %u:33; %u:-66; ] ", rep_fieldnum, rep_fieldnum, rep_fieldnum);

  buffer_free(enc33);
  buffer_free(enc66);
}

// Test that invalid protobufs are properly detected (without crashing) and
// have an error reported.  Field numbers match registered handlers above.
void test_invalid() {
  test_premature_eof_for_type(UPB_TYPE(DOUBLE));
  test_premature_eof_for_type(UPB_TYPE(FLOAT));
  test_premature_eof_for_type(UPB_TYPE(INT64));
  test_premature_eof_for_type(UPB_TYPE(UINT64));
  test_premature_eof_for_type(UPB_TYPE(INT32));
  test_premature_eof_for_type(UPB_TYPE(FIXED64));
  test_premature_eof_for_type(UPB_TYPE(FIXED32));
  test_premature_eof_for_type(UPB_TYPE(BOOL));
  test_premature_eof_for_type(UPB_TYPE(STRING));
  test_premature_eof_for_type(UPB_TYPE(BYTES));
  test_premature_eof_for_type(UPB_TYPE(UINT32));
  test_premature_eof_for_type(UPB_TYPE(ENUM));
  test_premature_eof_for_type(UPB_TYPE(SFIXED32));
  test_premature_eof_for_type(UPB_TYPE(SFIXED64));
  test_premature_eof_for_type(UPB_TYPE(SINT32));
  test_premature_eof_for_type(UPB_TYPE(SINT64));

  // EOF inside a tag's varint.
  assert_does_not_parse_at_eof( buffer_new3("\x80") );

  // EOF inside a known group.
  assert_does_not_parse_at_eof( tag(4, UPB_WIRE_TYPE_START_GROUP) );

  // EOF inside an unknown group.
  assert_does_not_parse_at_eof( tag(UNKNOWN_FIELD, UPB_WIRE_TYPE_START_GROUP) );

  // End group that we are not currently in.
  assert_does_not_parse( tag(4, UPB_WIRE_TYPE_END_GROUP) );

  // Field number is 0.
  assert_does_not_parse(
      cat( tag(0, UPB_WIRE_TYPE_DELIMITED), varint(0), NULL ));

  // Field number is too large.
  assert_does_not_parse(
      cat( tag(UPB_MAX_FIELDNUMBER + 1, UPB_WIRE_TYPE_DELIMITED),
           varint(0), NULL ));

  // Test exceeding the resource limit of stack depth.
  buffer *buf = buffer_new3("");
  for (int i = 0; i < UPB_MAX_NESTING; i++) {
    buf = submsg(UPB_TYPE(MESSAGE), buf);
  }
  assert_does_not_parse(buf);

  // Staying within the stack limit should work properly.
  buf = buffer_new3("");
  buffer *textbuf = buffer_new3("");
  int total = UPB_MAX_NESTING - 1;
  for (int i = 0; i < total; i++) {
    buf = submsg(UPB_TYPE(MESSAGE), buf);
    buffer_appendf(textbuf, "%u:{ ", UPB_TYPE(MESSAGE));
  }
  for (int i = 0; i < total; i++) {
    buffer_appendf(textbuf, "} ");
  }
  assert_successful_parse(buf, "%s", textbuf->buf);
  buffer_free(textbuf);
}

void test_valid() {
  test_valid_data_for_signed_type(UPB_TYPE(DOUBLE), dbl(33), dbl(-66));
  test_valid_data_for_signed_type(UPB_TYPE(FLOAT), flt(33), flt(-66));
  test_valid_data_for_signed_type(UPB_TYPE(INT64), varint(33), varint(-66));
  test_valid_data_for_signed_type(UPB_TYPE(INT32), varint(33), varint(-66));
  test_valid_data_for_signed_type(UPB_TYPE(ENUM), varint(33), varint(-66));
  test_valid_data_for_signed_type(UPB_TYPE(SFIXED32), uint32(33), uint32(-66));
  test_valid_data_for_signed_type(UPB_TYPE(SFIXED64), uint64(33), uint64(-66));
  test_valid_data_for_signed_type(UPB_TYPE(SINT32), zz32(33), zz32(-66));
  test_valid_data_for_signed_type(UPB_TYPE(SINT64), zz64(33), zz64(-66));

  test_valid_data_for_type(UPB_TYPE(UINT64), varint(33), varint(66));
  test_valid_data_for_type(UPB_TYPE(UINT32), varint(33), varint(66));
  test_valid_data_for_type(UPB_TYPE(FIXED64), uint64(33), uint64(66));
  test_valid_data_for_type(UPB_TYPE(FIXED32), uint32(33), uint32(66));

  // Submessage tests.
  uint32_t msg_fn = UPB_TYPE(MESSAGE);
  assert_successful_parse(
      submsg(msg_fn, submsg(msg_fn, submsg(msg_fn, buffer_new3("")))),
      "%u:{ %u:{ %u:{ } } } ", msg_fn, msg_fn, msg_fn);

  uint32_t repm_fn = rep_fn(UPB_TYPE(MESSAGE));
  assert_successful_parse(
      submsg(repm_fn, submsg(repm_fn, buffer_new3(""))),
      "%u:[ %u:{ %u:[ %u:{ } ] } ] ", repm_fn, repm_fn, repm_fn, repm_fn);
}

void run_tests() {
  test_invalid();
  test_valid();
}

int main() {
  // Construct decoder plan.
  upb_handlers *h = upb_handlers_new();
  reghandlers(upb_handlers_newmhandlers(h));

  // Test without JIT.
  plan = upb_decoderplan_new(h, false);
  run_tests();
  upb_decoderplan_unref(plan);

  // Test JIT.
  plan = upb_decoderplan_new(h, true);
  run_tests();
  upb_decoderplan_unref(plan);

  plan = NULL;
  printf("All tests passed, %d assertions.\n", num_assertions);
  upb_handlers_unref(h);
  return 0;
}
