/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 *
 * An exhaustive set of tests for parsing both valid and invalid protobuf
 * input, with buffer breaks in arbitrary places.
 *
 * Tests to add:
 * - string/bytes
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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS  // For PRIuS, etc.
#endif

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
#include "third_party/upb/tests/test_decoder_schema.upb.h"

uint32_t filter_hash = 0;

// Copied from decoder.c, since this is not a public interface.
typedef struct {
  uint8_t native_wire_type;
  bool is_numeric;
} upb_decoder_typeinfo;

static const upb_decoder_typeinfo upb_decoder_types[] = {
  {UPB_WIRE_TYPE_END_GROUP,   false},  // ENDGROUP
  {UPB_WIRE_TYPE_64BIT,       true},   // DOUBLE
  {UPB_WIRE_TYPE_32BIT,       true},   // FLOAT
  {UPB_WIRE_TYPE_VARINT,      true},   // INT64
  {UPB_WIRE_TYPE_VARINT,      true},   // UINT64
  {UPB_WIRE_TYPE_VARINT,      true},   // INT32
  {UPB_WIRE_TYPE_64BIT,       true},   // FIXED64
  {UPB_WIRE_TYPE_32BIT,       true},   // FIXED32
  {UPB_WIRE_TYPE_VARINT,      true},   // BOOL
  {UPB_WIRE_TYPE_DELIMITED,   false},  // STRING
  {UPB_WIRE_TYPE_START_GROUP, false},  // GROUP
  {UPB_WIRE_TYPE_DELIMITED,   false},  // MESSAGE
  {UPB_WIRE_TYPE_DELIMITED,   false},  // BYTES
  {UPB_WIRE_TYPE_VARINT,      true},   // UINT32
  {UPB_WIRE_TYPE_VARINT,      true},   // ENUM
  {UPB_WIRE_TYPE_32BIT,       true},   // SFIXED32
  {UPB_WIRE_TYPE_64BIT,       true},   // SFIXED64
  {UPB_WIRE_TYPE_VARINT,      true},   // SINT32
  {UPB_WIRE_TYPE_VARINT,      true},   // SINT64
};


class buffer {
 public:
  buffer(const void *data, size_t len) : len_(0) { append(data, len); }
  explicit buffer(const char *data) : len_(0) { append(data); }
  explicit buffer(size_t len) : len_(len) { memset(buf_, 0, len); }
  buffer(const buffer& buf) : len_(0) { append(buf); }
  buffer() : len_(0) {}

  void append(const void *data, size_t len) {
    ASSERT_NOCOUNT(len + len_ < sizeof(buf_));
    memcpy(buf_ + len_, data, len);
    len_ += len;
    buf_[len_] = NULL;
  }

  void append(const buffer& buf) {
    append(buf.buf_, buf.len_);
  }

  void append(const char *str) {
    append(str, strlen(str));
  }

  void vappendf(const char *fmt, va_list args) {
    size_t avail = sizeof(buf_) - len_;
    size_t size = vsnprintf(buf_ + len_, avail, fmt, args);
    ASSERT_NOCOUNT(avail > size);
    len_ += size;
  }

  void appendf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vappendf(fmt, args);
    va_end(args);
  }

  void assign(const buffer& buf) {
    clear();
    append(buf);
  }

  bool eql(const buffer& other) const {
    return len_ == other.len_ && memcmp(buf_, other.buf_, len_) == 0;
  }

  void clear() { len_ = 0; }
  size_t len() const { return len_; }
  const char *buf() const { return buf_; }

 private:
  // Has to be big enough for the largest string used in the test.
  char buf_[32768];
  size_t len_;
};


/* Routines for building arbitrary protos *************************************/

const buffer empty;

buffer cat(const buffer& a, const buffer& b,
           const buffer& c = empty,
           const buffer& d = empty,
           const buffer& e = empty) {
  buffer ret;
  ret.append(a);
  ret.append(b);
  ret.append(c);
  ret.append(d);
  ret.append(e);
  return ret;
}

buffer varint(uint64_t x) {
  char buf[UPB_PB_VARINT_MAX_LEN];
  size_t len = upb_vencode64(x, buf);
  return buffer(buf, len);
}

// TODO: proper byte-swapping for big-endian machines.
buffer fixed32(void *data) { return buffer(data, 4); }
buffer fixed64(void *data) { return buffer(data, 8); }

buffer delim(const buffer& buf) { return cat(varint(buf.len()), buf); }
buffer uint32(uint32_t u32) { return fixed32(&u32); }
buffer uint64(uint64_t u64) { return fixed64(&u64); }
buffer flt(float f) { return fixed32(&f); }
buffer dbl(double d) { return fixed64(&d); }
buffer zz32(int32_t x) { return varint(upb_zzenc_32(x)); }
buffer zz64(int64_t x) { return varint(upb_zzenc_64(x)); }

buffer tag(uint32_t fieldnum, char wire_type) {
  return varint((fieldnum << 3) | wire_type);
}

buffer submsg(uint32_t fn, const buffer& buf) {
  return cat( tag(fn, UPB_WIRE_TYPE_DELIMITED), delim(buf) );
}


/* A set of handlers that covers all .proto types *****************************/

// The handlers simply append to a string indicating what handlers were called.
// This string is similar to protobuf text format but fields are referred to by
// number instead of name and sequences are explicitly delimited.  We indent
// using the closure depth to test that the stack of closures is properly
// handled.

int closures[UPB_MAX_NESTING];
buffer output;

void indentbuf(buffer *buf, int depth) {
  for (int i = 0; i < depth; i++)
    buf->append("  ", 2);
}

void indent(void *depth) {
  indentbuf(&output, *(int*)depth);
}

#define NUMERIC_VALUE_HANDLER(member, ctype, fmt) \
  bool value_ ## member(void *closure, void *fval, ctype val) {       \
    indent(closure);                                                  \
    uint32_t *num = static_cast<uint32_t*>(fval);                     \
    output.appendf("%" PRIu32 ":%" fmt "\n", *num, val);              \
    return true;                                                      \
  }

NUMERIC_VALUE_HANDLER(uint32, uint32_t, PRIu32)
NUMERIC_VALUE_HANDLER(uint64, uint64_t, PRIu64)
NUMERIC_VALUE_HANDLER(int32,  int32_t,  PRId32)
NUMERIC_VALUE_HANDLER(int64,  int64_t,  PRId64)
NUMERIC_VALUE_HANDLER(float,  float,    "g")
NUMERIC_VALUE_HANDLER(double, double,   "g")

bool value_bool(void *closure, void *fval, bool val) {
  indent(closure);
  uint32_t *num = static_cast<uint32_t*>(fval);
  output.appendf("%" PRIu32 ":%s\n", *num, val ? "true" : "false");
  return true;
}

void* startstr(void *closure, void *fval, size_t size_hint) {
  indent(closure);
  uint32_t *num = static_cast<uint32_t*>(fval);
  output.appendf("%" PRIu32 ":(%zu)\"", *num, size_hint);
  return ((int*)closure) + 1;
}

size_t value_string(void *closure, void *fval, const char *buf, size_t n) {
  output.append(buf, n);
  return n;
}

bool endstr(void *closure, void *fval) {
  UPB_UNUSED(fval);
  output.append("\"\n");
  return true;
}

void* startsubmsg(void *closure, void *fval) {
  indent(closure);
  uint32_t *num = static_cast<uint32_t*>(fval);
  output.appendf("%" PRIu32 ":{\n", *num);
  return ((int*)closure) + 1;
}

bool endsubmsg(void *closure, void *fval) {
  UPB_UNUSED(fval);
  indent(closure);
  output.append("}\n");
  return true;
}

void* startseq(void *closure, void *fval) {
  indent(closure);
  uint32_t *num = static_cast<uint32_t*>(fval);
  output.appendf("%" PRIu32 ":[\n", *num);
  return ((int*)closure) + 1;
}

bool endseq(void *closure, void *fval) {
  UPB_UNUSED(fval);
  indent(closure);
  output.append("]\n");
  return true;
}

bool startmsg(void *closure) {
  indent(closure);
  output.append("<\n");
  return true;
}

void endmsg(void *closure, upb_status *status) {
  (void)status;
  indent(closure);
  output.append(">\n");
}

void free_uint32(void *val) {
  uint32_t *u32 = static_cast<uint32_t*>(val);
  delete u32;
}

template<class T>
void doreg(upb_handlers *h, uint32_t num,
           typename upb::Handlers::Value<T>::Handler *handler) {
  const upb_fielddef *f = upb_msgdef_itof(upb_handlers_msgdef(h), num);
  ASSERT(f);
  ASSERT(h->SetValueHandler<T>(f, handler, new uint32_t(num), free_uint32));
  if (f->IsSequence()) {
    ASSERT(h->SetStartSequenceHandler(
        f, &startseq, new uint32_t(num), free_uint32));
    ASSERT(h->SetEndSequenceHandler(
        f, &endseq, new uint32_t(num), free_uint32));
  }
}

// The repeated field number to correspond to the given non-repeated field
// number.
uint32_t rep_fn(uint32_t fn) {
  return (UPB_MAX_FIELDNUMBER - 1000) + fn;
}

#define NOP_FIELD 40
#define UNKNOWN_FIELD 666

template <class T>
void reg(upb_handlers *h, upb_fieldtype_t type,
         typename upb::Handlers::Value<T>::Handler *handler) {
  // We register both a repeated and a non-repeated field for every type.
  // For the non-repeated field we make the field number the same as the
  // type.  For the repeated field we make it a function of the type.
  doreg<T>(h, type, handler);
  doreg<T>(h, rep_fn(type), handler);
}

void reg_subm(upb_handlers *h, uint32_t num) {
  const upb_fielddef *f = upb_msgdef_itof(upb_handlers_msgdef(h), num);
  ASSERT(f);
  if (f->IsSequence()) {
    ASSERT(h->SetStartSequenceHandler(
        f, &startseq, new uint32_t(num), free_uint32));
    ASSERT(h->SetEndSequenceHandler(
        f, &endseq, new uint32_t(num), free_uint32));
  }
  ASSERT(h->SetStartSubMessageHandler(
      f, &startsubmsg, new uint32_t(num), free_uint32));
  ASSERT(h->SetEndSubMessageHandler(
      f, &endsubmsg, new uint32_t(num), free_uint32));
  ASSERT(upb_handlers_setsubhandlers(h, f, h));
}

void reg_str(upb_handlers *h, uint32_t num) {
  const upb_fielddef *f = upb_msgdef_itof(upb_handlers_msgdef(h), num);
  ASSERT(f);
  if (f->IsSequence()) {
    ASSERT(h->SetStartSequenceHandler(
        f, &startseq, new uint32_t(num), free_uint32));
    ASSERT(h->SetEndSequenceHandler(
        f, &endseq, new uint32_t(num), free_uint32));
  }
  ASSERT(h->SetStartStringHandler(
      f, &startstr, new uint32_t(num), free_uint32));
  ASSERT(h->SetEndStringHandler(
      f, &endstr, new uint32_t(num), free_uint32));
  ASSERT(h->SetStringHandler(
      f, &value_string, new uint32_t(num), free_uint32));
}

void reghandlers(upb_handlers *h) {
  upb_handlers_setstartmsg(h, &startmsg);
  upb_handlers_setendmsg(h, &endmsg);

  // Register handlers for each type.
  reg<double>  (h, UPB_TYPE(DOUBLE),   &value_double);
  reg<float>   (h, UPB_TYPE(FLOAT),    &value_float);
  reg<int64_t> (h, UPB_TYPE(INT64),    &value_int64);
  reg<uint64_t>(h, UPB_TYPE(UINT64),   &value_uint64);
  reg<int32_t> (h, UPB_TYPE(INT32) ,   &value_int32);
  reg<uint64_t>(h, UPB_TYPE(FIXED64),  &value_uint64);
  reg<uint32_t>(h, UPB_TYPE(FIXED32),  &value_uint32);
  reg<bool>    (h, UPB_TYPE(BOOL),     &value_bool);
  reg<uint32_t>(h, UPB_TYPE(UINT32),   &value_uint32);
  reg<int32_t> (h, UPB_TYPE(ENUM),     &value_int32);
  reg<int32_t> (h, UPB_TYPE(SFIXED32), &value_int32);
  reg<int64_t> (h, UPB_TYPE(SFIXED64), &value_int64);
  reg<int32_t> (h, UPB_TYPE(SINT32),   &value_int32);
  reg<int64_t> (h, UPB_TYPE(SINT64),   &value_int64);

  reg_str(h, UPB_TYPE(STRING));
  reg_str(h, UPB_TYPE(BYTES));
  reg_str(h, rep_fn(UPB_TYPE(STRING)));
  reg_str(h, rep_fn(UPB_TYPE(BYTES)));

  // Register submessage/group handlers that are self-recursive
  // to this type, eg: message M { optional M m = 1; }
  reg_subm(h, UPB_TYPE(MESSAGE));
  reg_subm(h, rep_fn(UPB_TYPE(MESSAGE)));

  // For NOP_FIELD we register no handlers, so we can pad a proto freely without
  // changing the output.
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
  upb_seamsrc *src = (upb_seamsrc*)_src;
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
  const upb_seamsrc *src = (const upb_seamsrc*)_src;
  assert(ofs + len <= src->len);
  memcpy(dst, src->str + ofs, len);
}

void upb_seamsrc_discard(void *src, uint64_t ofs) {
  (void)src;
  (void)ofs;
}

const char *upb_seamsrc_getptr(const void *_s, uint64_t ofs, size_t *len) {
  const upb_seamsrc *src = (const upb_seamsrc*)_s;
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
  assert(seam1 <= seam2);
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

uint32_t Hash(const buffer& proto, const buffer* expected_output) {
  uint32_t hash = MurmurHash2(proto.buf(), proto.len(), 0);
  if (expected_output)
    hash = MurmurHash2(expected_output->buf(), expected_output->len(), hash);
  bool hasjit = upb_decoderplan_hasjitcode(plan);
  hash = MurmurHash2(&hasjit, 1, hash);
  return hash;
}

#define LINE(x) x "\n"
void run_decoder(const buffer& proto, const buffer* expected_output) {
  testhash = Hash(proto, expected_output);
  if (filter_hash && testhash != filter_hash) return;
  upb_seamsrc src;
  upb_seamsrc_init(&src, proto.buf(), proto.len());
  upb_decoder d;
  upb_decoder_init(&d);
  upb_decoder_resetplan(&d, plan);
  for (size_t i = 0; i < proto.len(); i++) {
    for (size_t j = i; j < UPB_MIN(proto.len(), i + 5); j++) {
      upb_seamsrc_resetseams(&src, i, j);
      upb_byteregion *input = upb_seamsrc_allbytes(&src);
      output.clear();
      upb_decoder_resetinput(&d, input, &closures[0]);
      upb_success_t success = upb_decoder_decode(&d);
      ASSERT(upb_ok(upb_decoder_status(&d)) == (success == UPB_OK));
      if (expected_output) {
        ASSERT_STATUS(success == UPB_OK, upb_decoder_status(&d));
        // The input should be fully consumed.
        ASSERT(upb_byteregion_fetchofs(input) == upb_byteregion_endofs(input));
        ASSERT(upb_byteregion_discardofs(input) ==
               upb_byteregion_endofs(input));
        if (!output.eql(*expected_output)) {
          fprintf(stderr, "Text mismatch: '%s' vs '%s'\n",
                  output.buf(), expected_output->buf());
        }
        ASSERT(output.eql(*expected_output));
      } else {
        ASSERT(success == UPB_ERROR);
      }
    }
  }
  upb_decoder_uninit(&d);
  upb_seamsrc_uninit(&src);
  testhash = 0;
}

const static buffer thirty_byte_nop = buffer(cat(
    tag(NOP_FIELD, UPB_WIRE_TYPE_DELIMITED), delim(buffer(30)) ));

void assert_successful_parse(const buffer& proto,
                             const char *expected_fmt, ...) {
  buffer expected_text;
  va_list args;
  va_start(args, expected_fmt);
  expected_text.vappendf(expected_fmt, args);
  va_end(args);
  // The JIT is only used for data >=20 bytes from end-of-buffer, so
  // repeat once with no-op padding data at the end of buffer.
  run_decoder(proto, &expected_text);
  run_decoder(cat( proto, thirty_byte_nop ), &expected_text);
}

void assert_does_not_parse_at_eof(const buffer& proto) {
  run_decoder(proto, NULL);
}

void assert_does_not_parse(const buffer& proto) {
  // The JIT is only used for data >=20 bytes from end-of-buffer, so
  // repeat once with no-op padding data at the end of buffer.
  assert_does_not_parse_at_eof(proto);
  assert_does_not_parse_at_eof(cat( proto, thirty_byte_nop ));
}


/* The actual tests ***********************************************************/

void test_premature_eof_for_type(upb_fieldtype_t type) {
  // Incomplete values for each wire type.
  static const buffer incompletes[6] = {
    buffer("\x80"),     // UPB_WIRE_TYPE_VARINT
    buffer("abcdefg"),  // UPB_WIRE_TYPE_64BIT
    buffer("\x80"),     // UPB_WIRE_TYPE_DELIMITED (partial length)
    buffer(),           // UPB_WIRE_TYPE_START_GROUP (no value required)
    buffer(),           // UPB_WIRE_TYPE_END_GROUP (no value required)
    buffer("abc")       // UPB_WIRE_TYPE_32BIT
  };

  uint32_t fieldnum = type;
  uint32_t rep_fieldnum = rep_fn(type);
  int wire_type = upb_decoder_types[type].native_wire_type;
  const buffer& incomplete = incompletes[wire_type];

  // EOF before a known non-repeated value.
  assert_does_not_parse_at_eof(tag(fieldnum, wire_type));

  // EOF before a known repeated value.
  assert_does_not_parse_at_eof(tag(rep_fieldnum, wire_type));

  // EOF before an unknown value.
  assert_does_not_parse_at_eof(tag(UNKNOWN_FIELD, wire_type));

  // EOF inside a known non-repeated value.
  assert_does_not_parse_at_eof(
      cat( tag(fieldnum, wire_type), incomplete ));

  // EOF inside a known repeated value.
  assert_does_not_parse_at_eof(
      cat( tag(rep_fieldnum, wire_type), incomplete ));

  // EOF inside an unknown value.
  assert_does_not_parse_at_eof(
      cat( tag(UNKNOWN_FIELD, wire_type), incomplete ));

  if (wire_type == UPB_WIRE_TYPE_DELIMITED) {
    // EOF in the middle of delimited data for known non-repeated value.
    assert_does_not_parse_at_eof(
        cat( tag(fieldnum, wire_type), varint(1) ));

    // EOF in the middle of delimited data for known repeated value.
    assert_does_not_parse_at_eof(
        cat( tag(rep_fieldnum, wire_type), varint(1) ));

    // EOF in the middle of delimited data for unknown value.
    assert_does_not_parse_at_eof(
        cat( tag(UNKNOWN_FIELD, wire_type), varint(1) ));

    if (type == UPB_TYPE(MESSAGE)) {
      // Submessage ends in the middle of a value.
      buffer incomplete_submsg =
          cat ( tag(UPB_TYPE(INT32), UPB_WIRE_TYPE_VARINT),
                incompletes[UPB_WIRE_TYPE_VARINT] );
      assert_does_not_parse(
          cat( tag(fieldnum, UPB_WIRE_TYPE_DELIMITED),
               varint(incomplete_submsg.len()),
               incomplete_submsg ));
    }
  } else {
    // Packed region ends in the middle of a value.
    assert_does_not_parse(
        cat( tag(rep_fieldnum, UPB_WIRE_TYPE_DELIMITED),
             varint(incomplete.len()),
             incomplete ));

    // EOF in the middle of packed region.
    assert_does_not_parse_at_eof(
        cat( tag(rep_fieldnum, UPB_WIRE_TYPE_DELIMITED), varint(1) ));
  }
}

// "33" and "66" are just two random values that all numeric types can
// represent.
void test_valid_data_for_type(upb_fieldtype_t type,
                              const buffer& enc33, const buffer& enc66) {
  uint32_t fieldnum = type;
  uint32_t rep_fieldnum = rep_fn(type);
  int wire_type = upb_decoder_types[type].native_wire_type;

  // Non-repeated
  assert_successful_parse(
      cat( tag(fieldnum, wire_type), enc33,
           tag(fieldnum, wire_type), enc66 ),
      LINE("<")
      LINE("%u:33")
      LINE("%u:66")
      LINE(">"), fieldnum, fieldnum);

  // Non-packed repeated.
  assert_successful_parse(
      cat( tag(rep_fieldnum, wire_type), enc33,
           tag(rep_fieldnum, wire_type), enc66 ),
      LINE("<")
      LINE("%u:[")
      LINE("  %u:33")
      LINE("  %u:66")
      LINE("]")
      LINE(">"), rep_fieldnum, rep_fieldnum, rep_fieldnum);

  // Packed repeated.
  assert_successful_parse(
      cat( tag(rep_fieldnum, UPB_WIRE_TYPE_DELIMITED),
           delim(cat( enc33, enc66 )) ),
      LINE("<")
      LINE("%u:[")
      LINE("  %u:33")
      LINE("  %u:66")
      LINE("]")
      LINE(">"), rep_fieldnum, rep_fieldnum, rep_fieldnum);
}

void test_valid_data_for_signed_type(upb_fieldtype_t type,
                                     const buffer& enc33, const buffer& enc66) {
  uint32_t fieldnum = type;
  uint32_t rep_fieldnum = rep_fn(type);
  int wire_type = upb_decoder_types[type].native_wire_type;

  // Non-repeated
  assert_successful_parse(
      cat( tag(fieldnum, wire_type), enc33,
           tag(fieldnum, wire_type), enc66 ),
      LINE("<")
      LINE("%u:33")
      LINE("%u:-66")
      LINE(">"), fieldnum, fieldnum);

  // Non-packed repeated.
  assert_successful_parse(
      cat( tag(rep_fieldnum, wire_type), enc33,
           tag(rep_fieldnum, wire_type), enc66 ),
      LINE("<")
      LINE("%u:[")
      LINE("  %u:33")
      LINE("  %u:-66")
      LINE("]")
      LINE(">"), rep_fieldnum, rep_fieldnum, rep_fieldnum);

  // Packed repeated.
  assert_successful_parse(
      cat( tag(rep_fieldnum, UPB_WIRE_TYPE_DELIMITED),
           delim(cat( enc33, enc66 )) ),
      LINE("<")
      LINE("%u:[")
      LINE("  %u:33")
      LINE("  %u:-66")
      LINE("]")
      LINE(">"), rep_fieldnum, rep_fieldnum, rep_fieldnum);
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
  assert_does_not_parse_at_eof( buffer("\x80") );

  // EOF inside a known group.
  assert_does_not_parse_at_eof( tag(4, UPB_WIRE_TYPE_START_GROUP) );

  // EOF inside an unknown group.
  assert_does_not_parse_at_eof( tag(UNKNOWN_FIELD, UPB_WIRE_TYPE_START_GROUP) );

  // End group that we are not currently in.
  assert_does_not_parse( tag(4, UPB_WIRE_TYPE_END_GROUP) );

  // Field number is 0.
  assert_does_not_parse(
      cat( tag(0, UPB_WIRE_TYPE_DELIMITED), varint(0) ));

  // Field number is too large.
  assert_does_not_parse(
      cat( tag(UPB_MAX_FIELDNUMBER + 1, UPB_WIRE_TYPE_DELIMITED),
           varint(0) ));

  // Test exceeding the resource limit of stack depth.
  buffer buf;
  for (int i = 0; i < UPB_MAX_NESTING; i++) {
    buf.assign(submsg(UPB_TYPE(MESSAGE), buf));
  }
  assert_does_not_parse(buf);
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

  // Test implicit startseq/endseq.
  uint32_t repfl_fn = rep_fn(UPB_TYPE(FLOAT));
  uint32_t repdb_fn = rep_fn(UPB_TYPE(DOUBLE));
  assert_successful_parse(
      cat( tag(repfl_fn, UPB_WIRE_TYPE_32BIT), flt(33),
           tag(repdb_fn, UPB_WIRE_TYPE_64BIT), dbl(66) ),
      LINE("<")
      LINE("%u:[")
      LINE("  %u:33")
      LINE("]")
      LINE("%u:[")
      LINE("  %u:66")
      LINE("]")
      LINE(">"), repfl_fn, repfl_fn, repdb_fn, repdb_fn);

  // Submessage tests.
  uint32_t msg_fn = UPB_TYPE(MESSAGE);
  assert_successful_parse(
      submsg(msg_fn, submsg(msg_fn, submsg(msg_fn, buffer()))),
      LINE("<")
      LINE("%u:{")
      LINE("  <")
      LINE("  %u:{")
      LINE("    <")
      LINE("    %u:{")
      LINE("      <")
      LINE("      >")
      LINE("    }")
      LINE("    >")
      LINE("  }")
      LINE("  >")
      LINE("}")
      LINE(">"), msg_fn, msg_fn, msg_fn);

  uint32_t repm_fn = rep_fn(UPB_TYPE(MESSAGE));
  assert_successful_parse(
      submsg(repm_fn, submsg(repm_fn, buffer())),
      LINE("<")
      LINE("%u:[")
      LINE("  %u:{")
      LINE("    <")
      LINE("    %u:[")
      LINE("      %u:{")
      LINE("        <")
      LINE("        >")
      LINE("      }")
      LINE("    ]")
      LINE("    >")
      LINE("  }")
      LINE("]")
      LINE(">"), repm_fn, repm_fn, repm_fn, repm_fn);

  // Staying within the stack limit should work properly.
  buffer buf;
  buffer textbuf;
  int total = UPB_MAX_NESTING - 1;
  for (int i = 0; i < total; i++) {
    buf.assign(submsg(UPB_TYPE(MESSAGE), buf));
    indentbuf(&textbuf, i);
    textbuf.append("<\n");
    indentbuf(&textbuf, i);
    textbuf.appendf("%u:{\n", UPB_TYPE(MESSAGE));
  }
  indentbuf(&textbuf, total);
  textbuf.append("<\n");
  indentbuf(&textbuf, total);
  textbuf.append(">\n");
  for (int i = 0; i < total; i++) {
    indentbuf(&textbuf, total - i - 1);
    textbuf.append("}\n");
    indentbuf(&textbuf, total - i - 1);
    textbuf.append(">\n");
  }
  assert_successful_parse(buf, "%s", textbuf.buf());
}

void run_tests() {
  test_invalid();
  test_valid();
}

extern "C" {

int run_tests(int argc, char *argv[]) {
  if (argc > 1)
    filter_hash = strtol(argv[1], NULL, 16);
  for (int i = 0; i < UPB_MAX_NESTING; i++) {
    closures[i] = i;
  }

  // Create an empty handlers to make sure that the decoder can handle empty
  // messages.
  upb_handlers *h = upb_handlers_new(UPB_TEST_DECODER_EMPTYMESSAGE, &h);
  bool ok = upb_handlers_freeze(&h, 1, NULL);
  ASSERT(ok);
  plan = upb_decoderplan_new(h, true);
  upb_handlers_unref(h, &h);
  upb_decoderplan_unref(plan);

  // Construct decoder plan.
  h = upb_handlers_new(UPB_TEST_DECODER_DECODERTEST, &h);
  reghandlers(h);
  ok = upb_handlers_freeze(&h, 1, NULL);

  // Test without JIT.
  plan = upb_decoderplan_new(h, false);
  ASSERT(!upb_decoderplan_hasjitcode(plan));
  run_tests();
  upb_decoderplan_unref(plan);

#ifdef UPB_USE_JIT_X64
  // Test JIT.
  plan = upb_decoderplan_new(h, true);
  ASSERT(upb_decoderplan_hasjitcode(plan));
  run_tests();
  upb_decoderplan_unref(plan);
#endif

  plan = NULL;
  printf("All tests passed, %d assertions.\n", num_assertions);
  upb_handlers_unref(h, &h);
  return 0;
}

}
