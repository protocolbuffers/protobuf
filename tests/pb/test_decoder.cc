/*
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
#include <sstream>

#include "tests/test_util.h"
#include "tests/upb_test.h"
#include "tests/pb/test_decoder.upbdefs.h"

#ifdef AMALGAMATED
#include "upb.h"
#else  // AMALGAMATED
#include "upb/handlers.h"
#include "upb/pb/decoder.h"
#include "upb/upb.h"
#endif  // !AMALGAMATED

#include "upb/port_def.inc"

#undef PRINT_FAILURE
#define PRINT_FAILURE(expr)                                           \
  fprintf(stderr, "Assertion failed: %s:%d\n", __FILE__, __LINE__);   \
  fprintf(stderr, "expr: %s\n", #expr);                               \

#define MAX_NESTING 64

#define LINE(x) x "\n"

uint32_t filter_hash = 0;
double completed;
double total;
double *count;

enum TestMode {
  COUNT_ONLY = 1,
  NO_HANDLERS = 2,
  ALL_HANDLERS = 3
} test_mode;

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

#ifndef USE_GOOGLE
using std::string;
#endif

void vappendf(string* str, const char *format, va_list args) {
  va_list copy;
  va_copy(copy, args);

  int count = vsnprintf(NULL, 0, format, args);
  if (count >= 0)
  {
    UPB_ASSERT(count < 32768);
    char *buffer = new char[count + 1];
    UPB_ASSERT(buffer);
    count = vsnprintf(buffer, count + 1, format, copy);
    UPB_ASSERT(count >= 0);
    str->append(buffer, count);
    delete [] buffer;
  }
  va_end(copy);
}

void appendf(string* str, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vappendf(str, fmt, args);
  va_end(args);
}

void PrintBinary(const string& str) {
  for (size_t i = 0; i < str.size(); i++) {
    if (isprint(str[i])) {
      fprintf(stderr, "%c", str[i]);
    } else {
      fprintf(stderr, "\\x%02x", (int)(uint8_t)str[i]);
    }
  }
}

#define UPB_PB_VARINT_MAX_LEN 10

static size_t upb_vencode64(uint64_t val, char *buf) {
  size_t i;
  if (val == 0) { buf[0] = 0; return 1; }
  i = 0;
  while (val) {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  }
  return i;
}

static uint32_t upb_zzenc_32(int32_t n) {
  return ((uint32_t)n << 1) ^ (n >> 31);
}

static uint64_t upb_zzenc_64(int64_t n) {
  return ((uint64_t)n << 1) ^ (n >> 63);
}

/* Routines for building arbitrary protos *************************************/

const string empty;

string cat(const string& a, const string& b,
           const string& c = empty,
           const string& d = empty,
           const string& e = empty,
           const string& f = empty,
           const string& g = empty,
           const string& h = empty,
           const string& i = empty,
           const string& j = empty,
           const string& k = empty,
           const string& l = empty) {
  string ret;
  ret.reserve(a.size() + b.size() + c.size() + d.size() + e.size() + f.size() +
              g.size() + h.size() + i.size() + j.size() + k.size() + l.size());
  ret.append(a);
  ret.append(b);
  ret.append(c);
  ret.append(d);
  ret.append(e);
  ret.append(f);
  ret.append(g);
  ret.append(h);
  ret.append(i);
  ret.append(j);
  ret.append(k);
  ret.append(l);
  return ret;
}

template <typename T>
string num2string(T num) {
  std::ostringstream ss;
  ss << num;
  return ss.str();
}

string varint(uint64_t x) {
  char buf[UPB_PB_VARINT_MAX_LEN];
  size_t len = upb_vencode64(x, buf);
  return string(buf, len);
}

// TODO: proper byte-swapping for big-endian machines.
string fixed32(void *data) { return string(static_cast<char*>(data), 4); }
string fixed64(void *data) { return string(static_cast<char*>(data), 8); }

string delim(const string& buf) { return cat(varint(buf.size()), buf); }
string uint32(uint32_t u32) { return fixed32(&u32); }
string uint64(uint64_t u64) { return fixed64(&u64); }
string flt(float f) { return fixed32(&f); }
string dbl(double d) { return fixed64(&d); }
string zz32(int32_t x) { return varint(upb_zzenc_32(x)); }
string zz64(int64_t x) { return varint(upb_zzenc_64(x)); }

string tag(uint32_t fieldnum, char wire_type) {
  return varint((fieldnum << 3) | wire_type);
}

string submsg(uint32_t fn, const string& buf) {
  return cat( tag(fn, UPB_WIRE_TYPE_DELIMITED), delim(buf) );
}

string group(uint32_t fn, const string& buf) {
  return cat(tag(fn, UPB_WIRE_TYPE_START_GROUP), buf,
             tag(fn, UPB_WIRE_TYPE_END_GROUP));
}

// Like delim()/submsg(), but intentionally encodes an incorrect length.
// These help test when a delimited boundary doesn't land in the right place.
string badlen_delim(int err, const string& buf) {
  return cat(varint(buf.size() + err), buf);
}

string badlen_submsg(int err, uint32_t fn, const string& buf) {
  return cat( tag(fn, UPB_WIRE_TYPE_DELIMITED), badlen_delim(err, buf) );
}


/* A set of handlers that covers all .proto types *****************************/

// The handlers simply append to a string indicating what handlers were called.
// This string is similar to protobuf text format but fields are referred to by
// number instead of name and sequences are explicitly delimited.  We indent
// using the closure depth to test that the stack of closures is properly
// handled.

int closures[MAX_NESTING];
string output;

void indentbuf(string *buf, int depth) {
  buf->append(2 * depth, ' ');
}

#define NUMERIC_VALUE_HANDLER(member, ctype, fmt)                   \
  bool value_##member(int* depth, const uint32_t* num, ctype val) { \
    indentbuf(&output, *depth);                                     \
    appendf(&output, "%" PRIu32 ":%" fmt "\n", *num, val);          \
    return true;                                                    \
  }

NUMERIC_VALUE_HANDLER(uint32, uint32_t, PRIu32)
NUMERIC_VALUE_HANDLER(uint64, uint64_t, PRIu64)
NUMERIC_VALUE_HANDLER(int32,  int32_t,  PRId32)
NUMERIC_VALUE_HANDLER(int64,  int64_t,  PRId64)
NUMERIC_VALUE_HANDLER(float,  float,    "g")
NUMERIC_VALUE_HANDLER(double, double,   "g")

bool value_bool(int* depth, const uint32_t* num, bool val) {
  indentbuf(&output, *depth);
  appendf(&output, "%" PRIu32 ":%s\n", *num, val ? "true" : "false");
  return true;
}

int* startstr(int* depth, const uint32_t* num, size_t size_hint) {
  indentbuf(&output, *depth);
  appendf(&output, "%" PRIu32 ":(%zu)\"", *num, size_hint);
  return depth + 1;
}

size_t value_string(int* depth, const uint32_t* num, const char* buf,
                    size_t n, const upb_bufhandle* handle) {
  UPB_UNUSED(num);
  UPB_UNUSED(depth);
  output.append(buf, n);
  ASSERT(handle == &global_handle);
  return n;
}

bool endstr(int* depth, const uint32_t* num) {
  UPB_UNUSED(num);
  output.append("\n");
  indentbuf(&output, *depth);
  appendf(&output, "%" PRIu32 ":\"\n", *num);
  return true;
}

int* startsubmsg(int* depth, const uint32_t* num) {
  indentbuf(&output, *depth);
  appendf(&output, "%" PRIu32 ":{\n", *num);
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
  appendf(&output, "%" PRIu32 ":[\n", *num);
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
  UPB_UNUSED(status);
  indentbuf(&output, *depth);
  output.append(">\n");
  return true;
}

void free_uint32(void *val) {
  uint32_t *u32 = static_cast<uint32_t*>(val);
  delete u32;
}

template<class T, bool F(int*, const uint32_t*, T)>
void doreg(upb::HandlersPtr h, uint32_t num) {
  upb::FieldDefPtr f = h.message_def().FindFieldByNumber(num);
  ASSERT(f);
  ASSERT(h.SetValueHandler<T>(f, UpbBind(F, new uint32_t(num))));
  if (f.IsSequence()) {
    ASSERT(h.SetStartSequenceHandler(f, UpbBind(startseq, new uint32_t(num))));
    ASSERT(h.SetEndSequenceHandler(f, UpbBind(endseq, new uint32_t(num))));
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
void reg(upb::HandlersPtr h, upb_descriptortype_t type) {
  // We register both a repeated and a non-repeated field for every type.
  // For the non-repeated field we make the field number the same as the
  // type.  For the repeated field we make it a function of the type.
  doreg<T, F>(h, type);
  doreg<T, F>(h, rep_fn(type));
}

void regseq(upb::HandlersPtr h, upb::FieldDefPtr f, uint32_t num) {
  ASSERT(h.SetStartSequenceHandler(f, UpbBind(startseq, new uint32_t(num))));
  ASSERT(h.SetEndSequenceHandler(f, UpbBind(endseq, new uint32_t(num))));
}

void reg_subm(upb::HandlersPtr h, uint32_t num) {
  upb::FieldDefPtr f = h.message_def().FindFieldByNumber(num);
  ASSERT(f);
  if (f.IsSequence()) regseq(h, f, num);
  ASSERT(
      h.SetStartSubMessageHandler(f, UpbBind(startsubmsg, new uint32_t(num))));
  ASSERT(h.SetEndSubMessageHandler(f, UpbBind(endsubmsg, new uint32_t(num))));
}

void reg_str(upb::HandlersPtr h, uint32_t num) {
  upb::FieldDefPtr f = h.message_def().FindFieldByNumber(num);
  ASSERT(f);
  if (f.IsSequence()) regseq(h, f, num);
  ASSERT(h.SetStartStringHandler(f, UpbBind(startstr, new uint32_t(num))));
  ASSERT(h.SetEndStringHandler(f, UpbBind(endstr, new uint32_t(num))));
  ASSERT(h.SetStringHandler(f, UpbBind(value_string, new uint32_t(num))));
}

struct HandlerRegisterData {
  TestMode mode;
};

void callback(const void *closure, upb::Handlers* h_ptr) {
  upb::HandlersPtr h(h_ptr);
  const HandlerRegisterData* data =
      static_cast<const HandlerRegisterData*>(closure);
  if (data->mode == ALL_HANDLERS) {
    h.SetStartMessageHandler(UpbMakeHandler(startmsg));
    h.SetEndMessageHandler(UpbMakeHandler(endmsg));

    // Register handlers for each type.
    reg<double,   value_double>(h, UPB_DESCRIPTOR_TYPE_DOUBLE);
    reg<float,    value_float> (h, UPB_DESCRIPTOR_TYPE_FLOAT);
    reg<int64_t,  value_int64> (h, UPB_DESCRIPTOR_TYPE_INT64);
    reg<uint64_t, value_uint64>(h, UPB_DESCRIPTOR_TYPE_UINT64);
    reg<int32_t,  value_int32> (h, UPB_DESCRIPTOR_TYPE_INT32);
    reg<uint64_t, value_uint64>(h, UPB_DESCRIPTOR_TYPE_FIXED64);
    reg<uint32_t, value_uint32>(h, UPB_DESCRIPTOR_TYPE_FIXED32);
    reg<bool,     value_bool>  (h, UPB_DESCRIPTOR_TYPE_BOOL);
    reg<uint32_t, value_uint32>(h, UPB_DESCRIPTOR_TYPE_UINT32);
    reg<int32_t,  value_int32> (h, UPB_DESCRIPTOR_TYPE_ENUM);
    reg<int32_t,  value_int32> (h, UPB_DESCRIPTOR_TYPE_SFIXED32);
    reg<int64_t,  value_int64> (h, UPB_DESCRIPTOR_TYPE_SFIXED64);
    reg<int32_t,  value_int32> (h, UPB_DESCRIPTOR_TYPE_SINT32);
    reg<int64_t,  value_int64> (h, UPB_DESCRIPTOR_TYPE_SINT64);

    reg_str(h, UPB_DESCRIPTOR_TYPE_STRING);
    reg_str(h, UPB_DESCRIPTOR_TYPE_BYTES);
    reg_str(h, rep_fn(UPB_DESCRIPTOR_TYPE_STRING));
    reg_str(h, rep_fn(UPB_DESCRIPTOR_TYPE_BYTES));

    // Register submessage/group handlers that are self-recursive
    // to this type, eg: message M { optional M m = 1; }
    reg_subm(h, UPB_DESCRIPTOR_TYPE_MESSAGE);
    reg_subm(h, rep_fn(UPB_DESCRIPTOR_TYPE_MESSAGE));

    if (h.message_def().full_name() == std::string("DecoderTest")) {
      reg_subm(h, UPB_DESCRIPTOR_TYPE_GROUP);
      reg_subm(h, rep_fn(UPB_DESCRIPTOR_TYPE_GROUP));
    }

    // For NOP_FIELD we register no handlers, so we can pad a proto freely without
    // changing the output.
  }
}

/* Running of test cases ******************************************************/

const upb::Handlers *global_handlers;
upb::pb::DecoderMethodPtr global_method;

upb::pb::DecoderPtr CreateDecoder(upb::Arena* arena,
                                  upb::pb::DecoderMethodPtr method,
                                  upb::Sink sink, upb::Status* status) {
  upb::pb::DecoderPtr ret =
      upb::pb::DecoderPtr::Create(arena, method, sink, status);
  ret.set_max_nesting(MAX_NESTING);
  return ret;
}

void CheckBytesParsed(upb::pb::DecoderPtr decoder, size_t ofs) {
  // We can't have parsed more data than the decoder callback is telling us it
  // parsed.
  ASSERT(decoder.BytesParsed() <= ofs);

  // The difference between what we've decoded and what the decoder has accepted
  // represents the internally buffered amount.  This amount should not exceed
  // this value which comes from decoder.int.h.
  ASSERT(ofs <= (decoder.BytesParsed() + UPB_DECODER_MAX_RESIDUAL_BYTES));
}

static bool parse(VerboseParserEnvironment* env,
                  upb::pb::DecoderPtr decoder, int bytes) {
  CheckBytesParsed(decoder, env->ofs());
  bool ret = env->ParseBuffer(bytes);
  if (ret) {
    CheckBytesParsed(decoder, env->ofs());
  }

  return ret;
}

void do_run_decoder(VerboseParserEnvironment* env, upb::pb::DecoderPtr decoder,
                    const string& proto, const string* expected_output,
                    size_t i, size_t j, bool may_skip) {
  env->Reset(proto.c_str(), proto.size(), may_skip, expected_output == NULL);
  decoder.Reset();

  if (test_mode != COUNT_ONLY) {
    output.clear();

    if (filter_hash) {
      fprintf(stderr, "RUNNING TEST CASE\n");
      fprintf(stderr, "Input (len=%u): ", (unsigned)proto.size());
      PrintBinary(proto);
      fprintf(stderr, "\n");
      if (expected_output) {
        if (test_mode == ALL_HANDLERS) {
          fprintf(stderr, "Expected output: %s\n", expected_output->c_str());
        } else if (test_mode == NO_HANDLERS) {
          fprintf(stderr,
                  "No handlers are registered, BUT if they were "
                  "the expected output would be: %s\n",
                  expected_output->c_str());
        }
      } else {
        fprintf(stderr, "Expected to FAIL\n");
      }
    }

    bool ok = env->Start() &&
              parse(env, decoder, (int)i) &&
              parse(env, decoder, (int)(j - i)) &&
              parse(env, decoder, -1) &&
              env->End();

    ASSERT(env->CheckConsistency());

    if (test_mode == ALL_HANDLERS) {
      if (expected_output) {
        if (output != *expected_output) {
          fprintf(stderr, "Text mismatch: '%s' vs '%s'\n",
                  output.c_str(), expected_output->c_str());
        }
        ASSERT(ok);
        ASSERT(output == *expected_output);
      } else {
        if (ok) {
          fprintf(stderr, "Didn't expect ok result, but got output: '%s'\n",
                  output.c_str());
        }
        ASSERT(!ok);
      }
    }
  }
  (*count)++;
}

void run_decoder(const string& proto, const string* expected_output) {
  VerboseParserEnvironment env(filter_hash != 0);
  upb::Sink sink(global_handlers, &closures[0]);
  upb::pb::DecoderPtr decoder = CreateDecoder(env.arena(), global_method, sink, env.status());
  env.ResetBytesSink(decoder.input());
  for (size_t i = 0; i < proto.size(); i++) {
    for (size_t j = i; j < UPB_MIN(proto.size(), i + 5); j++) {
      do_run_decoder(&env, decoder, proto, expected_output, i, j, true);
      if (env.SkippedWithNull()) {
        do_run_decoder(&env, decoder, proto, expected_output, i, j, false);
      }
    }
  }
}

const static string thirty_byte_nop = cat(
    tag(NOP_FIELD, UPB_WIRE_TYPE_DELIMITED), delim(string(30, 'X')) );

// Indents and wraps text as if it were a submessage with this field number
string wrap_text(int32_t fn, const string& text) {
  string wrapped_text = text;
  size_t pos = 0;
  string replace_with = "\n  ";
  while ((pos = wrapped_text.find("\n", pos)) != string::npos &&
         pos != wrapped_text.size() - 1) {
    wrapped_text.replace(pos, 1, replace_with);
    pos += replace_with.size();
  }
  wrapped_text = cat(
      LINE("<"),
      num2string(fn), LINE(":{")
      "  ", wrapped_text,
      LINE("  }")
      LINE(">"));
  return wrapped_text;
}

void assert_successful_parse(const string& proto,
                             const char *expected_fmt, ...) {
  string expected_text;
  va_list args;
  va_start(args, expected_fmt);
  vappendf(&expected_text, expected_fmt, args);
  va_end(args);
  // To test both middle-of-buffer and end-of-buffer code paths,
  // repeat once with no-op padding data at the end of buffer.
  run_decoder(proto, &expected_text);
  run_decoder(cat( proto, thirty_byte_nop ), &expected_text);

  // Test that this also works when wrapped in a submessage or group.
  // Indent the expected text one level and wrap it.
  string wrapped_text1 = wrap_text(UPB_DESCRIPTOR_TYPE_MESSAGE, expected_text);
  string wrapped_text2 = wrap_text(UPB_DESCRIPTOR_TYPE_GROUP, expected_text);

  run_decoder(submsg(UPB_DESCRIPTOR_TYPE_MESSAGE, proto), &wrapped_text1);
  run_decoder(group(UPB_DESCRIPTOR_TYPE_GROUP, proto), &wrapped_text2);
}

void assert_does_not_parse_at_eof(const string& proto) {
  run_decoder(proto, NULL);

  // Also test that we fail to parse at end-of-submessage, not just
  // end-of-message.  But skip this if we have no handlers, because in that
  // case we won't descend into the submessage.
  if (test_mode != NO_HANDLERS) {
    run_decoder(submsg(UPB_DESCRIPTOR_TYPE_MESSAGE, proto), NULL);
    run_decoder(cat(submsg(UPB_DESCRIPTOR_TYPE_MESSAGE, proto),
                    thirty_byte_nop), NULL);
  }
}

void assert_does_not_parse(const string& proto) {
  // Test that the error is caught both at end-of-buffer and middle-of-buffer.
  assert_does_not_parse_at_eof(proto);
  assert_does_not_parse_at_eof(cat( proto, thirty_byte_nop ));
}


/* The actual tests ***********************************************************/

void test_premature_eof_for_type(upb_descriptortype_t type) {
  // Incomplete values for each wire type.
  static const string incompletes[6] = {
    string("\x80"),     // UPB_WIRE_TYPE_VARINT
    string("abcdefg"),  // UPB_WIRE_TYPE_64BIT
    string("\x80"),     // UPB_WIRE_TYPE_DELIMITED (partial length)
    string(),           // UPB_WIRE_TYPE_START_GROUP (no value required)
    string(),           // UPB_WIRE_TYPE_END_GROUP (no value required)
    string("abc")       // UPB_WIRE_TYPE_32BIT
  };

  uint32_t fieldnum = type;
  uint32_t rep_fieldnum = rep_fn(type);
  int wire_type = upb_decoder_types[type].native_wire_type;
  const string& incomplete = incompletes[wire_type];

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
      string incomplete_submsg =
          cat ( tag(UPB_DESCRIPTOR_TYPE_INT32, UPB_WIRE_TYPE_VARINT),
                incompletes[UPB_WIRE_TYPE_VARINT] );
      assert_does_not_parse(
          cat( tag(fieldnum, UPB_WIRE_TYPE_DELIMITED),
               varint(incomplete_submsg.size()),
               incomplete_submsg ));
    }
  } else {
    // Packed region ends in the middle of a value.
    assert_does_not_parse(
        cat( tag(rep_fieldnum, UPB_WIRE_TYPE_DELIMITED),
             varint(incomplete.size()),
             incomplete ));

    // EOF in the middle of packed region.
    assert_does_not_parse_at_eof(
        cat( tag(rep_fieldnum, UPB_WIRE_TYPE_DELIMITED), varint(1) ));
  }
}

// "33" and "66" are just two random values that all numeric types can
// represent.
void test_valid_data_for_type(upb_descriptortype_t type,
                              const string& enc33, const string& enc66) {
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
                                     const string& enc33, const string& enc66) {
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
  assert_does_not_parse_at_eof( string("\x80") );

  // EOF inside a known group.
  // TODO(haberman): add group to decoder test schema.
  //assert_does_not_parse_at_eof( tag(4, UPB_WIRE_TYPE_START_GROUP) );

  // EOF inside an unknown group.
  assert_does_not_parse_at_eof( tag(UNKNOWN_FIELD, UPB_WIRE_TYPE_START_GROUP) );

  // End group that we are not currently in.
  assert_does_not_parse( tag(4, UPB_WIRE_TYPE_END_GROUP) );

  // Field number is 0.
  assert_does_not_parse(
      cat( tag(0, UPB_WIRE_TYPE_DELIMITED), varint(0) ));
  // The previous test alone did not catch this particular pattern which could
  // corrupt the internal state.
  assert_does_not_parse(
      cat( tag(0, UPB_WIRE_TYPE_64BIT), uint64(0) ));

  // Field number is too large.
  assert_does_not_parse(
      cat( tag(UPB_MAX_FIELDNUMBER + 1, UPB_WIRE_TYPE_DELIMITED),
           varint(0) ));

  // Known group inside a submessage has ENDGROUP tag AFTER submessage end.
  assert_does_not_parse(
      cat ( submsg(UPB_DESCRIPTOR_TYPE_MESSAGE,
                   tag(UPB_DESCRIPTOR_TYPE_GROUP, UPB_WIRE_TYPE_START_GROUP)),
            tag(UPB_DESCRIPTOR_TYPE_GROUP, UPB_WIRE_TYPE_END_GROUP)));

  // Unknown string extends past enclosing submessage.
  assert_does_not_parse(
      cat (badlen_submsg(-1, UPB_DESCRIPTOR_TYPE_MESSAGE,
                         submsg(12345, string("   "))),
           submsg(UPB_DESCRIPTOR_TYPE_MESSAGE, string("     "))));

  // Unknown fixed-length field extends past enclosing submessage.
  assert_does_not_parse(
      cat (badlen_submsg(-1, UPB_DESCRIPTOR_TYPE_MESSAGE,
                         cat( tag(12345, UPB_WIRE_TYPE_64BIT), uint64(0))),
           submsg(UPB_DESCRIPTOR_TYPE_MESSAGE, string("     "))));

  // Test exceeding the resource limit of stack depth.
  if (test_mode != NO_HANDLERS) {
    string buf;
    for (int i = 0; i <= MAX_NESTING; i++) {
      buf.assign(submsg(UPB_DESCRIPTOR_TYPE_MESSAGE, buf));
    }
    assert_does_not_parse(buf);
  }
}

void test_valid() {
  // Empty protobuf.
  assert_successful_parse(string(""), "<\n>\n");

  // Empty protobuf where we never call PutString between
  // StartString/EndString.

  upb::Status status;
  upb::Arena arena;
  upb::Sink sink(global_handlers, &closures[0]);
  upb::pb::DecoderPtr decoder =
      CreateDecoder(&arena, global_method, sink, &status);
  output.clear();
  bool ok = upb::PutBuffer(std::string(), decoder.input());
  ASSERT(ok);
  ASSERT(status.ok());
  if (test_mode == ALL_HANDLERS) {
    ASSERT(output == string("<\n>\n"));
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
      submsg(12345, string("                ")),
      "<\n>\n");

  // Unknown field inside a known submessage.
  assert_successful_parse(
      submsg(UPB_DESCRIPTOR_TYPE_MESSAGE, submsg(12345, string("   "))),
      LINE("<")
      LINE("%u:{")
      LINE("  <")
      LINE("  >")
      LINE("  }")
      LINE(">"), UPB_DESCRIPTOR_TYPE_MESSAGE);

  assert_successful_parse(
      cat (submsg(UPB_DESCRIPTOR_TYPE_MESSAGE, submsg(12345, string("   "))),
           tag(UPB_DESCRIPTOR_TYPE_INT32, UPB_WIRE_TYPE_VARINT),
           varint(5)),
      LINE("<")
      LINE("%u:{")
      LINE("  <")
      LINE("  >")
      LINE("  }")
      LINE("%u:5")
      LINE(">"), UPB_DESCRIPTOR_TYPE_MESSAGE, UPB_DESCRIPTOR_TYPE_INT32);

  // This triggered a previous bug in the decoder.
  assert_successful_parse(
      cat( tag(UPB_DESCRIPTOR_TYPE_SFIXED32, UPB_WIRE_TYPE_VARINT),
           varint(0) ),
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
      LINE("    }")
      LINE("  >")
      LINE("  }")
      LINE("%u:22222")
      LINE(">"), msg_type, msg_type, int32_type, int32_type);

  assert_successful_parse(
      cat( tag(UPB_DESCRIPTOR_TYPE_INT32, UPB_WIRE_TYPE_VARINT), varint(1),
           tag(12345, UPB_WIRE_TYPE_VARINT), varint(2345678) ),
      LINE("<")
      LINE("%u:1")
      LINE(">"), UPB_DESCRIPTOR_TYPE_INT32);

  // String inside submsg.
  uint32_t msg_fn = UPB_DESCRIPTOR_TYPE_MESSAGE;
  assert_successful_parse(
      submsg(msg_fn,
             cat ( tag(UPB_DESCRIPTOR_TYPE_STRING, UPB_WIRE_TYPE_DELIMITED),
                   delim(string("abcde"))
                 )
             ),
      LINE("<")
      LINE("%u:{")
      LINE("  <")
      LINE("  %u:(5)\"abcde")
      LINE("    %u:\"")
      LINE("  >")
      LINE("  }")
      LINE(">"), msg_fn, UPB_DESCRIPTOR_TYPE_STRING,
                 UPB_DESCRIPTOR_TYPE_STRING);

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
  assert_successful_parse(
      submsg(msg_fn, submsg(msg_fn, submsg(msg_fn, string()))),
      LINE("<")
      LINE("%u:{")
      LINE("  <")
      LINE("  %u:{")
      LINE("    <")
      LINE("    %u:{")
      LINE("      <")
      LINE("      >")
      LINE("      }")
      LINE("    >")
      LINE("    }")
      LINE("  >")
      LINE("  }")
      LINE(">"), msg_fn, msg_fn, msg_fn);

  uint32_t repm_fn = rep_fn(UPB_DESCRIPTOR_TYPE_MESSAGE);
  assert_successful_parse(
      submsg(repm_fn, submsg(repm_fn, string())),
      LINE("<")
      LINE("%u:[")
      LINE("  %u:{")
      LINE("    <")
      LINE("    %u:[")
      LINE("      %u:{")
      LINE("        <")
      LINE("        >")
      LINE("        }")
      LINE("    ]")
      LINE("    >")
      LINE("    }")
      LINE("]")
      LINE(">"), repm_fn, repm_fn, repm_fn, repm_fn);

  // Test unknown group.
  uint32_t unknown_group_fn = 12321;
  assert_successful_parse(
      cat( tag(unknown_group_fn, UPB_WIRE_TYPE_START_GROUP),
           tag(unknown_group_fn, UPB_WIRE_TYPE_END_GROUP) ),
      LINE("<")
      LINE(">")
  );

  // Test some unknown fields inside an unknown group.
  const string unknown_group_with_data =
      cat(
          tag(unknown_group_fn, UPB_WIRE_TYPE_START_GROUP),
          tag(12345, UPB_WIRE_TYPE_VARINT), varint(2345678),
          tag(123456789, UPB_WIRE_TYPE_32BIT), uint32(2345678),
          tag(123477, UPB_WIRE_TYPE_64BIT), uint64(2345678),
          tag(123, UPB_WIRE_TYPE_DELIMITED), varint(0),
          tag(unknown_group_fn, UPB_WIRE_TYPE_END_GROUP)
         );

  // Nested unknown group with data.
  assert_successful_parse(
      cat(
           tag(unknown_group_fn, UPB_WIRE_TYPE_START_GROUP),
           unknown_group_with_data,
           tag(unknown_group_fn, UPB_WIRE_TYPE_END_GROUP),
           tag(UPB_DESCRIPTOR_TYPE_INT32, UPB_WIRE_TYPE_VARINT), varint(1)
         ),
      LINE("<")
      LINE("%u:1")
      LINE(">"),
      UPB_DESCRIPTOR_TYPE_INT32
  );

  assert_successful_parse(
      cat( tag(unknown_group_fn, UPB_WIRE_TYPE_START_GROUP),
           tag(unknown_group_fn + 1, UPB_WIRE_TYPE_START_GROUP),
           tag(unknown_group_fn + 1, UPB_WIRE_TYPE_END_GROUP),
           tag(unknown_group_fn, UPB_WIRE_TYPE_END_GROUP) ),
      LINE("<")
      LINE(">")
  );

  // Staying within the stack limit should work properly.
  string buf;
  string textbuf;
  int total = MAX_NESTING - 1;
  for (int i = 0; i < total; i++) {
    buf.assign(submsg(UPB_DESCRIPTOR_TYPE_MESSAGE, buf));
    indentbuf(&textbuf, i);
    textbuf.append("<\n");
    indentbuf(&textbuf, i);
    appendf(&textbuf, "%u:{\n", UPB_DESCRIPTOR_TYPE_MESSAGE);
  }
  indentbuf(&textbuf, total);
  textbuf.append("<\n");
  indentbuf(&textbuf, total);
  textbuf.append(">\n");
  for (int i = 0; i < total; i++) {
    indentbuf(&textbuf, total - i - 1);
    textbuf.append("  }\n");
    indentbuf(&textbuf, total - i - 1);
    textbuf.append(">\n");
  }
  // Have to use run_decoder directly, because we are at max nesting and can't
  // afford the extra nesting that assert_successful_parse() will do.
  run_decoder(buf, &textbuf);
}

void empty_callback(const void* /* closure */, upb::Handlers* /* h_ptr */) {}

void test_emptyhandlers(upb::SymbolTable* symtab) {
  // Create an empty handlers to make sure that the decoder can handle empty
  // messages.
  HandlerRegisterData handlerdata;
  handlerdata.mode = test_mode;

  upb::HandlerCache handler_cache(empty_callback, &handlerdata);
  upb::pb::CodeCache pb_code_cache(&handler_cache);

  upb::MessageDefPtr md = upb::MessageDefPtr(Empty_getmsgdef(symtab->ptr()));
  global_handlers = handler_cache.Get(md);
  global_method = pb_code_cache.Get(md);

  // TODO: also test the case where a message has fields, but the fields are
  // submessage fields and have no handlers. This also results in a decoder
  // method with no field-handling code.

  // Ensure that the method can run with empty and non-empty input.
  string test_unknown_field_msg =
    cat(tag(1, UPB_WIRE_TYPE_VARINT), varint(42),
        tag(2, UPB_WIRE_TYPE_DELIMITED), delim("My test data"));
  const struct {
    const char* data;
    size_t length;
  } testdata[] = {
    { "", 0 },
    { test_unknown_field_msg.data(), test_unknown_field_msg.size() },
    { NULL, 0 },
  };
  for (int i = 0; testdata[i].data; i++) {
    VerboseParserEnvironment env(filter_hash != 0);
    upb::Sink sink(global_method.dest_handlers(), &closures[0]);
    upb::pb::DecoderPtr decoder =
        CreateDecoder(env.arena(), global_method, sink, env.status());
    env.ResetBytesSink(decoder.input());
    env.Reset(testdata[i].data, testdata[i].length, true, false);
    ASSERT(env.Start());
    ASSERT(env.ParseBuffer(-1));
    ASSERT(env.End());
    ASSERT(env.CheckConsistency());
  }
}

void run_tests() {
  HandlerRegisterData handlerdata;
  handlerdata.mode = test_mode;

  upb::SymbolTable symtab;
  upb::HandlerCache handler_cache(callback, &handlerdata);
  upb::pb::CodeCache pb_code_cache(&handler_cache);

  upb::MessageDefPtr md(DecoderTest_getmsgdef(symtab.ptr()));
  global_handlers = handler_cache.Get(md);
  global_method = pb_code_cache.Get(md);
  completed = 0;

  test_invalid();
  test_valid();

  test_emptyhandlers(&symtab);
}

extern "C" {

int run_tests(int argc, char *argv[]) {
  if (argc > 1)
    filter_hash = (uint32_t)strtol(argv[1], NULL, 16);
  for (int i = 0; i < MAX_NESTING; i++) {
    closures[i] = i;
  }

  // Count tests.
  count = &total;
  total = 0;
  test_mode = COUNT_ONLY;
  run_tests();
  count = &completed;

  total *= 2;  // NO_HANDLERS, ALL_HANDLERS.

  test_mode = NO_HANDLERS;
  run_tests();

  test_mode = ALL_HANDLERS;
  run_tests();

  printf("All tests passed, %d assertions.\n", num_assertions);
  return 0;
}

}
