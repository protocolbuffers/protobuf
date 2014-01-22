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
 * - test different handlers at every level and whether handlers fire at
 *   the correct field path.
 * - test skips that extend past the end of current buffer (where decoder
 *   returns value greater than the size param).
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS  // For PRIuS, etc.
#endif

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "tests/upb_test.h"
#include "third_party/upb/tests/pb/test_decoder_schema.upb.h"
#include "upb/handlers.h"
#include "upb/pb/decoder.h"
#include "upb/pb/varint.int.h"
#include "upb/upb.h"

#undef PRINT_FAILURE
#define PRINT_FAILURE(expr) \
  fprintf(stderr, "Assertion failed: %s:%d\n", __FILE__, __LINE__); \
  fprintf(stderr, "expr: %s\n", #expr); \
  if (testhash) { \
    fprintf(stderr, "assertion failed running test %x.  " \
                    "Run with the arg %x to run only this test.\n", \
                    testhash, testhash); \
    fprintf(stderr, "Failed at %02.2f%% through tests.\n", \
                    (float)completed * 100 / total); \
  }

uint32_t filter_hash = 0;
double completed;
double total;
double *count;
bool count_only;
upb::BufferHandle global_handle;

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
           const buffer& e = empty,
           const buffer& f = empty) {
  buffer ret;
  ret.append(a);
  ret.append(b);
  ret.append(c);
  ret.append(d);
  ret.append(e);
  ret.append(f);
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

int closures[UPB_DECODER_MAX_NESTING];
buffer output;

void indentbuf(buffer *buf, int depth) {
  for (int i = 0; i < depth; i++)
    buf->append("  ", 2);
}

#define NUMERIC_VALUE_HANDLER(member, ctype, fmt) \
  bool value_ ## member(int* depth, const uint32_t* num, ctype val) {          \
    indentbuf(&output, *depth);                                                \
    output.appendf("%" PRIu32 ":%" fmt "\n", *num, val);                       \
    return true;                                                               \
  }

NUMERIC_VALUE_HANDLER(uint32, uint32_t, PRIu32)
NUMERIC_VALUE_HANDLER(uint64, uint64_t, PRIu64)
NUMERIC_VALUE_HANDLER(int32,  int32_t,  PRId32)
NUMERIC_VALUE_HANDLER(int64,  int64_t,  PRId64)
NUMERIC_VALUE_HANDLER(float,  float,    "g")
NUMERIC_VALUE_HANDLER(double, double,   "g")

bool value_bool(int* depth, const uint32_t* num, bool val) {
  indentbuf(&output, *depth);
  output.appendf("%" PRIu32 ":%s\n", *num, val ? "true" : "false");
  return true;
}

int* startstr(int* depth, const uint32_t* num, size_t size_hint) {
  indentbuf(&output, *depth);
  output.appendf("%" PRIu32 ":(%zu)\"", *num, size_hint);
  return depth + 1;
}

size_t value_string(int* depth, const uint32_t* num, const char* buf,
                    size_t n, const upb::BufferHandle* handle) {
  UPB_UNUSED(num);
  output.append(buf, n);
  ASSERT(handle == &global_handle);
  return n;
}

bool endstr(int* depth, const uint32_t* num) {
  UPB_UNUSED(depth);
  UPB_UNUSED(num);
  output.append("\"\n");
  return true;
}

int* startsubmsg(int* depth, const uint32_t* num) {
  indentbuf(&output, *depth);
  output.appendf("%" PRIu32 ":{\n", *num);
  return depth + 1;
}

bool endsubmsg(int* depth, const uint32_t* num) {
  UPB_UNUSED(num);
  indentbuf(&output, *depth);
  output.append("}\n");
  return true;
}

int* startseq(int* depth, const uint32_t* num) {
  indentbuf(&output, *depth);
  output.appendf("%" PRIu32 ":[\n", *num);
  return depth + 1;
}

bool endseq(int* depth, const uint32_t* num) {
  UPB_UNUSED(num);
  indentbuf(&output, *depth);
  output.append("]\n");
  return true;
}

bool startmsg(int* depth) {
  indentbuf(&output, *depth);
  output.append("<\n");
  return true;
}

bool endmsg(int* depth, upb_status* status) {
  indentbuf(&output, *depth);
  output.append(">\n");
  return true;
}

void free_uint32(void *val) {
  uint32_t *u32 = static_cast<uint32_t*>(val);
  delete u32;
}

template<class T, bool F(int*, const uint32_t*, T)>
void doreg(upb_handlers *h, uint32_t num) {
  const upb_fielddef *f = upb_msgdef_itof(upb_handlers_msgdef(h), num);
  ASSERT(f);
  ASSERT(h->SetValueHandler<T>(f, UpbBindT(F, new uint32_t(num))));
  if (f->IsSequence()) {
    ASSERT(h->SetStartSequenceHandler(f, UpbBind(startseq, new uint32_t(num))));
    ASSERT(h->SetEndSequenceHandler(f, UpbBind(endseq, new uint32_t(num))));
  }
}

// The repeated field number to correspond to the given non-repeated field
// number.
uint32_t rep_fn(uint32_t fn) {
  return (UPB_MAX_FIELDNUMBER - 1000) + fn;
}

#define NOP_FIELD 40
#define UNKNOWN_FIELD 666

template <class T, bool F(int*, const uint32_t*, T)>
void reg(upb_handlers *h, upb_descriptortype_t type) {
  // We register both a repeated and a non-repeated field for every type.
  // For the non-repeated field we make the field number the same as the
  // type.  For the repeated field we make it a function of the type.
  doreg<T, F>(h, type);
  doreg<T, F>(h, rep_fn(type));
}

void regseq(upb::Handlers* h, const upb::FieldDef* f, uint32_t num) {
  ASSERT(h->SetStartSequenceHandler(f, UpbBind(startseq, new uint32_t(num))));
  ASSERT(h->SetEndSequenceHandler(f, UpbBind(endseq, new uint32_t(num))));
}

void reg_subm(upb_handlers *h, uint32_t num) {
  const upb_fielddef *f = upb_msgdef_itof(upb_handlers_msgdef(h), num);
  ASSERT(f);
  if (f->IsSequence()) regseq(h, f, num);
  ASSERT(
      h->SetStartSubMessageHandler(f, UpbBind(startsubmsg, new uint32_t(num))));
  ASSERT(h->SetEndSubMessageHandler(f, UpbBind(endsubmsg, new uint32_t(num))));
  ASSERT(upb_handlers_setsubhandlers(h, f, h));
}

void reg_str(upb_handlers *h, uint32_t num) {
  const upb_fielddef *f = upb_msgdef_itof(upb_handlers_msgdef(h), num);
  ASSERT(f);
  if (f->IsSequence()) regseq(h, f, num);
  ASSERT(h->SetStartStringHandler(f, UpbBind(startstr, new uint32_t(num))));
  ASSERT(h->SetEndStringHandler(f, UpbBind(endstr, new uint32_t(num))));
  ASSERT(h->SetStringHandler(f, UpbBind(value_string, new uint32_t(num))));
}

upb::reffed_ptr<const upb::Handlers> NewHandlers() {
  upb::reffed_ptr<upb::Handlers> h(
      upb::Handlers::New(UPB_TEST_DECODER_DECODERTEST));

  h->SetStartMessageHandler(UpbMakeHandler(startmsg));
  h->SetEndMessageHandler(UpbMakeHandler(endmsg));

  // Register handlers for each type.
  reg<double,   value_double>(h.get(), UPB_DESCRIPTOR_TYPE_DOUBLE);
  reg<float,    value_float> (h.get(), UPB_DESCRIPTOR_TYPE_FLOAT);
  reg<int64_t,  value_int64> (h.get(), UPB_DESCRIPTOR_TYPE_INT64);
  reg<uint64_t, value_uint64>(h.get(), UPB_DESCRIPTOR_TYPE_UINT64);
  reg<int32_t,  value_int32> (h.get(), UPB_DESCRIPTOR_TYPE_INT32);
  reg<uint64_t, value_uint64>(h.get(), UPB_DESCRIPTOR_TYPE_FIXED64);
  reg<uint32_t, value_uint32>(h.get(), UPB_DESCRIPTOR_TYPE_FIXED32);
  reg<bool,     value_bool>  (h.get(), UPB_DESCRIPTOR_TYPE_BOOL);
  reg<uint32_t, value_uint32>(h.get(), UPB_DESCRIPTOR_TYPE_UINT32);
  reg<int32_t,  value_int32> (h.get(), UPB_DESCRIPTOR_TYPE_ENUM);
  reg<int32_t,  value_int32> (h.get(), UPB_DESCRIPTOR_TYPE_SFIXED32);
  reg<int64_t,  value_int64> (h.get(), UPB_DESCRIPTOR_TYPE_SFIXED64);
  reg<int32_t,  value_int32> (h.get(), UPB_DESCRIPTOR_TYPE_SINT32);
  reg<int64_t,  value_int64> (h.get(), UPB_DESCRIPTOR_TYPE_SINT64);

  reg_str(h.get(), UPB_DESCRIPTOR_TYPE_STRING);
  reg_str(h.get(), UPB_DESCRIPTOR_TYPE_BYTES);
  reg_str(h.get(), rep_fn(UPB_DESCRIPTOR_TYPE_STRING));
  reg_str(h.get(), rep_fn(UPB_DESCRIPTOR_TYPE_BYTES));

  // Register submessage/group handlers that are self-recursive
  // to this type, eg: message M { optional M m = 1; }
  reg_subm(h.get(), UPB_DESCRIPTOR_TYPE_MESSAGE);
  reg_subm(h.get(), rep_fn(UPB_DESCRIPTOR_TYPE_MESSAGE));

  // For NOP_FIELD we register no handlers, so we can pad a proto freely without
  // changing the output.

  bool ok = h->Freeze(NULL);
  ASSERT(ok);

  return h;
}


/* Running of test cases ******************************************************/

const upb::Handlers *global_handlers;
const upb::pb::DecoderMethod *global_method;

uint32_t Hash(const buffer& proto, const buffer* expected_output, size_t seam1,
              size_t seam2) {
  uint32_t hash = MurmurHash2(proto.buf(), proto.len(), 0);
  if (expected_output)
    hash = MurmurHash2(expected_output->buf(), expected_output->len(), hash);
  hash = MurmurHash2(&seam1, sizeof(seam1), hash);
  hash = MurmurHash2(&seam2, sizeof(seam2), hash);
  return hash;
}

bool parse(upb::BytesSink* s, void* subc, const char* buf, size_t start,
           size_t end, size_t* ofs, upb::Status* status) {
  start = UPB_MAX(start, *ofs);
  if (start <= end) {
    size_t len = end - start;
    size_t parsed = s->PutBuffer(subc, buf + start, len, &global_handle);
    if (status->ok() != (parsed >= len)) {
      fprintf(stderr, "Status: %s, parsed=%zu, len=%zu\n",
              status->error_message(), parsed, len);
      ASSERT(false);
    }
    if (!status->ok())
      return false;
    *ofs += parsed;
  }
  return true;
}

#define LINE(x) x "\n"
void run_decoder(const buffer& proto, const buffer* expected_output) {
  upb::Status status;
  upb::pb::Decoder decoder(global_method, &status);
  upb::Sink sink(global_handlers, &closures[0]);
  decoder.ResetOutput(&sink);
  for (size_t i = 0; i < proto.len(); i++) {
    for (size_t j = i; j < UPB_MIN(proto.len(), i + 5); j++) {
      testhash = Hash(proto, expected_output, i, j);
      if (filter_hash && testhash != filter_hash) continue;
      if (!count_only) {
        decoder.Reset();
        output.clear();
        status.Clear();
        size_t ofs = 0;
        upb::BytesSink* input = decoder.input();
        void *sub;
        bool ok =
            input->Start(proto.len(), &sub) &&
            parse(input, sub, proto.buf(), 0, i, &ofs, &status) &&
            parse(input, sub, proto.buf(), i, j, &ofs, &status) &&
            parse(input, sub, proto.buf(), j, proto.len(), &ofs, &status) &&
            ofs == proto.len() &&
            input->End();
        if (expected_output) {
          if (!output.eql(*expected_output)) {
            fprintf(stderr, "Text mismatch: '%s' vs '%s'\n",
                    output.buf(), expected_output->buf());
          }
          if (!ok) {
            fprintf(stderr, "Failed: %s\n", status.error_message());
          }
          ASSERT(ok);
          ASSERT(output.eql(*expected_output));
        } else {
          if (ok) {
            fprintf(stderr, "Didn't expect ok result, but got output: '%s'\n",
                    output.buf());
          }
          ASSERT(!ok);
        }
      }
      (*count)++;
    }
  }
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
  // To test both middle-of-buffer and end-of-buffer code paths,
  // repeat once with no-op padding data at the end of buffer.
  run_decoder(proto, &expected_text);
  run_decoder(cat( proto, thirty_byte_nop ), &expected_text);
}

void assert_does_not_parse_at_eof(const buffer& proto) {
  run_decoder(proto, NULL);
}

void assert_does_not_parse(const buffer& proto) {
  // Test that the error is caught both at end-of-buffer and middle-of-buffer.
  assert_does_not_parse_at_eof(proto);
  assert_does_not_parse_at_eof(cat( proto, thirty_byte_nop ));
}


/* The actual tests ***********************************************************/

void test_premature_eof_for_type(upb_descriptortype_t type) {
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

    if (type == UPB_DESCRIPTOR_TYPE_MESSAGE) {
      // Submessage ends in the middle of a value.
      buffer incomplete_submsg =
          cat ( tag(UPB_DESCRIPTOR_TYPE_INT32, UPB_WIRE_TYPE_VARINT),
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
void test_valid_data_for_type(upb_descriptortype_t type,
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

void test_valid_data_for_signed_type(upb_descriptortype_t type,
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
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_DOUBLE);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_FLOAT);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_INT64);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_UINT64);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_INT32);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_FIXED64);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_FIXED32);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_BOOL);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_STRING);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_BYTES);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_UINT32);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_ENUM);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_SFIXED32);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_SFIXED64);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_SINT32);
  test_premature_eof_for_type(UPB_DESCRIPTOR_TYPE_SINT64);

  // EOF inside a tag's varint.
  assert_does_not_parse_at_eof( buffer("\x80") );

  // EOF inside a known group.
  // TODO(haberman): add group to decoder test schema.
  //assert_does_not_parse_at_eof( tag(4, UPB_WIRE_TYPE_START_GROUP) );

  // EOF inside an unknown group.
  // TODO(haberman): unknown groups not supported yet.
  //assert_does_not_parse_at_eof( tag(UNKNOWN_FIELD, UPB_WIRE_TYPE_START_GROUP) );

  // End group that we are not currently in.
  assert_does_not_parse( tag(4, UPB_WIRE_TYPE_END_GROUP) );

  // Field number is 0.
  assert_does_not_parse(
      cat( tag(0, UPB_WIRE_TYPE_DELIMITED), varint(0) ));

  // Field number is too large.
  assert_does_not_parse(
      cat( tag(UPB_MAX_FIELDNUMBER + 1, UPB_WIRE_TYPE_DELIMITED),
           varint(0) ));

  // Known group inside a submessage has ENDGROUP tag AFTER submessage end.
  assert_does_not_parse(
      cat ( submsg(UPB_DESCRIPTOR_TYPE_MESSAGE,
                   tag(UPB_DESCRIPTOR_TYPE_GROUP, UPB_WIRE_TYPE_START_GROUP)),
            tag(UPB_DESCRIPTOR_TYPE_GROUP, UPB_WIRE_TYPE_END_GROUP)));

  // Test exceeding the resource limit of stack depth.
  buffer buf;
  for (int i = 0; i <= UPB_DECODER_MAX_NESTING; i++) {
    buf.assign(submsg(UPB_DESCRIPTOR_TYPE_MESSAGE, buf));
  }
  assert_does_not_parse(buf);
}

void test_valid() {
  // Empty protobuf.
  assert_successful_parse(buffer(""), "<\n>\n");

  // Empty protobuf where we never call PutString between
  // StartString/EndString.
  {
    upb::Status status;
    upb::pb::Decoder decoder(global_method, &status);
    upb::Sink sink(global_handlers, &closures[0]);
    decoder.ResetOutput(&sink);
    output.clear();
    bool ok = upb::BufferSource::PutBuffer("", 0, decoder.input());
    ASSERT(ok);
    ASSERT(status.ok());
    ASSERT(output.eql(buffer("<\n>\n")));
  }

  test_valid_data_for_signed_type(UPB_DESCRIPTOR_TYPE_DOUBLE,
                                  dbl(33),
                                  dbl(-66));
  test_valid_data_for_signed_type(UPB_DESCRIPTOR_TYPE_FLOAT, flt(33), flt(-66));
  test_valid_data_for_signed_type(UPB_DESCRIPTOR_TYPE_INT64,
                                  varint(33),
                                  varint(-66));
  test_valid_data_for_signed_type(UPB_DESCRIPTOR_TYPE_INT32,
                                  varint(33),
                                  varint(-66));
  test_valid_data_for_signed_type(UPB_DESCRIPTOR_TYPE_ENUM,
                                  varint(33),
                                  varint(-66));
  test_valid_data_for_signed_type(UPB_DESCRIPTOR_TYPE_SFIXED32,
                                  uint32(33),
                                  uint32(-66));
  test_valid_data_for_signed_type(UPB_DESCRIPTOR_TYPE_SFIXED64,
                                  uint64(33),
                                  uint64(-66));
  test_valid_data_for_signed_type(UPB_DESCRIPTOR_TYPE_SINT32,
                                  zz32(33),
                                  zz32(-66));
  test_valid_data_for_signed_type(UPB_DESCRIPTOR_TYPE_SINT64,
                                  zz64(33),
                                  zz64(-66));

  test_valid_data_for_type(UPB_DESCRIPTOR_TYPE_UINT64, varint(33), varint(66));
  test_valid_data_for_type(UPB_DESCRIPTOR_TYPE_UINT32, varint(33), varint(66));
  test_valid_data_for_type(UPB_DESCRIPTOR_TYPE_FIXED64, uint64(33), uint64(66));
  test_valid_data_for_type(UPB_DESCRIPTOR_TYPE_FIXED32, uint32(33), uint32(66));

  // Unknown fields.
  int int32_type = UPB_DESCRIPTOR_TYPE_INT32;
  int msg_type = UPB_DESCRIPTOR_TYPE_MESSAGE;
  assert_successful_parse(
      cat( tag(12345, UPB_WIRE_TYPE_VARINT), varint(2345678) ),
      "<\n>\n");
  assert_successful_parse(
      cat( tag(12345, UPB_WIRE_TYPE_32BIT), uint32(2345678) ),
      "<\n>\n");
  assert_successful_parse(
      cat( tag(12345, UPB_WIRE_TYPE_64BIT), uint64(2345678) ),
      "<\n>\n");
  assert_successful_parse(
      submsg(12345, buffer("                ")),
      "<\n>\n");

  assert_successful_parse(
      cat(
        submsg(UPB_DESCRIPTOR_TYPE_MESSAGE,
          submsg(UPB_DESCRIPTOR_TYPE_MESSAGE,
            cat( tag(int32_type, UPB_WIRE_TYPE_VARINT), varint(2345678),
                 tag(12345, UPB_WIRE_TYPE_VARINT), varint(2345678) ))),
        tag(int32_type, UPB_WIRE_TYPE_VARINT), varint(22222)),
      LINE("<")
      LINE("%u:{")
      LINE("  <")
      LINE("  %u:{")
      LINE("    <")
      LINE("    %u:2345678")
      LINE("    >")
      LINE("  }")
      LINE("  >")
      LINE("}")
      LINE("%u:22222")
      LINE(">"), msg_type, msg_type, int32_type, int32_type);

  assert_successful_parse(
      cat( tag(UPB_DESCRIPTOR_TYPE_INT32, UPB_WIRE_TYPE_VARINT), varint(1),
           tag(12345, UPB_WIRE_TYPE_VARINT), varint(2345678) ),
      LINE("<")
      LINE("%u:1")
      LINE(">"), UPB_DESCRIPTOR_TYPE_INT32);

  // Test implicit startseq/endseq.
  uint32_t repfl_fn = rep_fn(UPB_DESCRIPTOR_TYPE_FLOAT);
  uint32_t repdb_fn = rep_fn(UPB_DESCRIPTOR_TYPE_DOUBLE);
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
  uint32_t msg_fn = UPB_DESCRIPTOR_TYPE_MESSAGE;
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

  uint32_t repm_fn = rep_fn(UPB_DESCRIPTOR_TYPE_MESSAGE);
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
  int total = UPB_DECODER_MAX_NESTING - 1;
  for (int i = 0; i < total; i++) {
    buf.assign(submsg(UPB_DESCRIPTOR_TYPE_MESSAGE, buf));
    indentbuf(&textbuf, i);
    textbuf.append("<\n");
    indentbuf(&textbuf, i);
    textbuf.appendf("%u:{\n", UPB_DESCRIPTOR_TYPE_MESSAGE);
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

upb::reffed_ptr<const upb::pb::DecoderMethod> NewMethod(
    const upb::Handlers* dest_handlers, bool allow_jit) {
  upb::pb::CodeCache cache;
  cache.set_allow_jit(allow_jit);
  return cache.GetDecoderMethodForDestHandlers(dest_handlers);
}

void test_emptyhandlers(bool allowjit) {
  // Create an empty handlers to make sure that the decoder can handle empty
  // messages.
  upb::reffed_ptr<upb::Handlers> h(
      upb::Handlers::New(UPB_TEST_DECODER_EMPTYMESSAGE));
  bool ok = h->Freeze(NULL);
  ASSERT(ok);
  NewMethod(h.get(), allowjit);
}

extern "C" {

int run_tests(int argc, char *argv[]) {
  if (argc > 1)
    filter_hash = strtol(argv[1], NULL, 16);
  for (int i = 0; i < UPB_DECODER_MAX_NESTING; i++) {
    closures[i] = i;
  }

  upb::reffed_ptr<const upb::pb::DecoderMethod> method;
  upb::reffed_ptr<const upb::Handlers> handlers;

  // Construct decoder plan.
  handlers = NewHandlers();
  global_handlers = handlers.get();

  // Count tests.
  method = NewMethod(handlers.get(), false);
  global_method = method.get();
  count_only = true;
  count = &total;
  total = 0;
  run_tests();
  count_only = false;
  count = &completed;

  // Test without JIT.
  method = NewMethod(handlers.get(), false);
  global_method = method.get();
  ASSERT(!global_method->is_native());
  completed = 0;
  run_tests();

  test_emptyhandlers(false);

#ifdef UPB_USE_JIT_X64
  // Test JIT.
  method = NewMethod(handlers.get(), true);
  global_method = method.get();
  ASSERT(global_method->is_native());
  completed = 0;
  run_tests();

  test_emptyhandlers(true);
#endif

  printf("All tests passed, %d assertions.\n", num_assertions);
  return 0;
}

}
