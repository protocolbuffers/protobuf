// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdlib>
#include <string>

#include "google/protobuf/type.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/json_parser_internal.h"
#include "google/protobuf/wire_format_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#define CHK(x)               \
  if (!(x)) {                \
    ABSL_DCHECK(HasError()); \
    return 0;                \
  }

#define CHKRET(x)                 \
  [this](int y) -> int {          \
    ABSL_DCHECK(y || HasError()); \
    return y;                     \
  }(x);

#define CHK_MSG(x, msg) \
  if (!(x)) {           \
    AddError(msg);      \
    return 0;           \
  }

#define CHK_MSGF(x, msg, ...)                     \
  if (!(x)) {                                     \
    AddError(absl::Substitute(msg, __VA_ARGS__)); \
    return 0;                                     \
  }

#define UNREACHABLE() \
  ABSL_DCHECK(false); \
  return false;

using google::protobuf::internal::WireFormatLite;

namespace google {
namespace protobuf {
namespace internal {

// LazyVarintStream ////////////////////////////////////////////////////////////

bool LazyVarintStream::StartDelimited() {
  stack_.push(output_.size());
  return true;
}

bool LazyVarintStream::EndDelimited() {
  size_t pos = stack_.top();
  size_t len = output_.size() - pos;
  uint8_t buf[10];
  uint8_t* end = google::protobuf::io::CodedOutputStream::WriteVarint32ToArray(len, buf);
  output_.insert(pos, reinterpret_cast<char*>(buf), end - buf);
  stack_.pop();
  return true;
}

bool LazyVarintStream::Flush() {
  absl::string_view left(output_);
  void* buf;
  int size = 0;

  ABSL_CHECK(stack_.empty());

  while (!left.empty()) {
    while (size == 0) {
      if (!stream_->Next(&buf, &size)) return false;
    }

    int copy = std::min<int>(left.size(), size);
    std::memcpy(buf, left.data(), copy);
    left.remove_prefix(copy);
    size -= copy;
  }
  stream_->BackUp(size);
  return true;
}

// JsonParser //////////////////////////////////////////////////////////////////

namespace {

bool IsWhitespace(char ch) {
  switch (ch) {
    case ' ':
    case '\n':
    case '\r':
    case '\t':
      return true;
    default:
      return false;
  }
}

bool IsDigit(char ch) { return ch >= '0' && ch <= '9'; }

bool HexDigitToInt(char ch, int* digit) {
  if (ch >= '0' && ch <= '9') {
    *digit = (ch - '0');
  } else if (ch >= 'a' && ch <= 'f') {
    *digit = ((ch - 'a') + 10);
  } else if (ch >= 'A' && ch <= 'F') {
    *digit = ((ch - 'A') + 10);
  } else {
    return false;
  }
  return true;
}

}  // namespace

PROTOBUF_NOINLINE bool JsonParser::IsEofFallback() {
  bool preserve = preserve_ != nullptr;
  bool preserve_any = preserve_any_ != nullptr;
  ABSL_DCHECK(ptr_ == end_);
  if (!stream_ || eof_) return true;
  if (preserve) FinishAccumulate();
  if (preserve_any) FinishAccumulateAny();
  uintptr_t column = reinterpret_cast<uintptr_t>(ptr_) - line_begin_;
  {
    const void* ptr;
    int size = 0;
    while (size == 0) {
      if (!stream_->Next(&ptr, &size)) {
        eof_ = true;
        return true;
      }
    }
    ptr_ = static_cast<const char*>(ptr);
    end_ = ptr_ + size;
  }
  line_begin_ = reinterpret_cast<uintptr_t>(ptr_) - column;
  if (preserve) ResumeAccumulate();
  if (preserve_any) ResumeAccumulateAny();
  return false;
}

PROTOBUF_NOINLINE bool JsonParser::ConsumeCharsFallback(char* data, size_t n) {
  while (n > end_ - ptr_) {
    size_t size = end_ - ptr_;
    memcpy(data, ptr_, size);
    n -= size;
    ptr_ += size;
    data += size;
    if (IsEof()) return false;
  }
  memcpy(data, ptr_, n);
  ptr_ += n;
  return true;
}

bool JsonParser::SkipWhitespace() {
  while (!IsEof()) {
    if (*ptr_ == '\n') {
      line_++;
      line_begin_ = reinterpret_cast<uintptr_t>(ptr_ + 1);
    } else if (!IsWhitespace(*ptr_)) {
      return true;
    }
    ptr_++;
  }
  return false;
}

bool JsonParser::SkipDigits() {
  bool ok = false;  // We must consume at least one digit.

  while (!IsEof()) {
    if (!IsDigit(*ptr_)) goto done;
    ptr_++;
    ok = true;
  }

done:
  if (!ok) AddError("Expected one or more digits");
  return ok;
}

bool JsonParser::ParseCodepoint(uint32_t* cp) {
  int val = 0;
  std::array<char, 4> chars;
  CHK_MSG(ConsumeChars(&chars), "Unexpected EOF parsing unicode escape");
  for (char ch : chars) {
    int digit;
    CHK_MSGF(HexDigitToInt(ch, &digit), "Invalid hex digit '$0'", ch);
    val <<= 4;
    val |= digit;
  }
  *cp = val;
  return true;
}

bool JsonParser::WriteUtf8Codepoint(uint32_t cp) {
  char utf8[4];
  int n;

  if (cp <= 0x7F) {
    utf8[0] = cp;
    n = 1;
  } else if (cp <= 0x07FF) {
    utf8[0] = ((cp >> 6) & 0x1F) | 0xC0;
    utf8[1] = ((cp >> 0) & 0x3F) | 0x80;
    n = 2;
  } else if (cp <= 0xFFFF) {
    utf8[0] = ((cp >> 12) & 0x0F) | 0xE0;
    utf8[1] = ((cp >> 6) & 0x3F) | 0x80;
    utf8[2] = ((cp >> 0) & 0x3F) | 0x80;
    n = 3;
  } else if (cp < 0x10FFFF) {
    utf8[0] = ((cp >> 18) & 0x07) | 0xF0;
    utf8[1] = ((cp >> 12) & 0x3f) | 0x80;
    utf8[2] = ((cp >> 6) & 0x3f) | 0x80;
    utf8[3] = ((cp >> 0) & 0x3f) | 0x80;
    n = 4;
  } else {
    return false;
  }

  tmp_buf_.append(utf8, n);
  return true;
}

PROTOBUF_NOINLINE bool JsonParser::ParseLiteralFallback(absl::string_view lit) {
  absl::string_view orig = lit;
  absl::string_view input;
  size_t size;
  while (lit.size() > end_ - ptr_) {
    size_t size = end_ - ptr_;
    absl::string_view input(ptr_, size);
    if (input != lit.substr(0, size)) goto err;
    lit.remove_prefix(size);
    ptr_ += size;
    if (IsEof()) goto err;
  }
  size = lit.size();
  input = absl::string_view(ptr_, size);
  if (input != lit) goto err;
  ptr_ += size;
  return true;

err:
  CHK_MSGF(false, "Expected: $0", orig);
  return false;
}

bool JsonParser::ParseEscape() {
  switch (ConsumeChar()) {
    case '"':
      tmp_buf_.push_back('\"');
      break;
    case '\\':
      tmp_buf_.push_back('\\');
      break;
    case '/':
      tmp_buf_.push_back('/');
      break;
    case 'b':
      tmp_buf_.push_back('\b');
      break;
    case 'f':
      tmp_buf_.push_back('\f');
      break;
    case 'n':
      tmp_buf_.push_back('\n');
      break;
    case 'r':
      tmp_buf_.push_back('\r');
      break;
    case 't':
      tmp_buf_.push_back('\t');
      break;
    case 'u': {
      uint32_t cp;
      CHK(ParseCodepoint(&cp));
      if (cp >= 0xd800 && cp <= 0xdbff) {
        // Surrogate pair: two 16-bit codepoints become a 32-bit codepoint.
        uint32_t high = cp;
        uint32_t low;
        CHK(ParseLiteral("\\u"));
        CHK(ParseCodepoint(&low));
        CHK(low >= 0xdc00 && low <= 0xdfff);
        cp = (high & 0x3ff) << 10;
        cp |= (low & 0x3ff);
        cp += 0x10000;
      }
      CHK(WriteUtf8Codepoint(cp));
      break;
    }
    default:
      AddError("Invalid escape char");
      return false;
  }

  return true;
}

bool JsonParser::ParseString(absl::string_view* str) {
  CHK(ParseCharSkipWhitespace('"'));
  StartAccumulate();

  while (!IsEof()) {
    // TODO: validate UTF-8.
    switch (*ptr_) {
      case '"':
        if (tmp_buf_.empty()) {
          *str = absl::string_view(preserve_, ptr_ - preserve_);
          preserve_ = nullptr;
        } else {
          *str = FinishAccumulate();
        }
        ptr_++;
        return true;
      case '\\':
        FinishAccumulate();
        ptr_++;
        CHK(ParseEscape());
        ResumeAccumulate();
        break;
      default:
        CHK_MSG((uint8_t)*ptr_ >= 0x20, "Invalid char inside JSON string");
        ptr_++;
        break;
    }
  }

  AddError("EOF inside string");
  return false;
}

bool JsonParser::ParseNumber(double* d) {
  ABSL_DCHECK_EQ(Peek(), kNumber);
  StartAccumulate();

  if (PeekChar() == '-') {
    ptr_++;
  }

  CHK(TryParseChar('0') || SkipDigits());

  if (IsEof()) goto parse;

  if (TryParseChar('.')) {
    CHK(SkipDigits());
  }

  if (IsEof()) goto parse;

  if (*ptr_ == 'e' || *ptr_ == 'E') {
    ptr_++;
    CHK_MSG(!IsEof(), "Unexpected EOF in number");

    if (*ptr_ == '+' || *ptr_ == '-') {
      ptr_++;
    }

    CHK(SkipDigits());
  }

parse:
  errno = 0;
  absl::string_view buf = FinishAccumulate();
  char* end;
  *d = std::strtod(buf.data(), &end);
  ABSL_DCHECK(end == buf.data() + buf.size());

  // Currently the min/max-val conformance tests fail if we check this.  Does
  // this mean the conformance tests are wrong or strtod() is wrong, or
  // something else?  Investigate further.
  // CHK(errno == 0);

  return true;
}

bool JsonParser::ParseTrue() {
  ABSL_DCHECK_EQ(Peek(), kTrue);
  return ParseLiteral("true");
}

bool JsonParser::ParseFalse() {
  ABSL_DCHECK_EQ(Peek(), kFalse);
  return ParseLiteral("false");
}

bool JsonParser::ParseNull() {
  ABSL_DCHECK_EQ(Peek(), kNull);
  return ParseLiteral("null");
}

JsonParser::ValueType JsonParser::Peek() {
  switch (PeekCharSkipWhitespace()) {
    case '{':
      return kObject;
    case '[':
      return kArray;
    case '"':
      return kString;
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
      return kNumber;
    case 't':
      return kTrue;
    case 'f':
      return kFalse;
    case 'n':
      return kNull;
    case 0:
      AddError("Unexpected EOF");
      return kError;
    default:
      AddError(absl::Substitute("Unexpected character '$0'", *ptr_));
      return kError;
  }
}

template <char EndCh>
bool JsonParser::SequenceNext() {
  bool is_first = is_first_;
  is_first_ = false;
  switch (PeekCharSkipWhitespace()) {
    case ',':
      CHK_MSG(!is_first, "Unexpected comma");
      ConsumeChar();
      return true;
    case EndCh:
      return false;
    case 0:
      AddError("Unexpected EOF");
      return false;
    default:
      return true;
  }
}

bool JsonParser::IncAndCheckDepth() {
  is_first_ = true;
  CHK_MSG(--depth_ != 0, "Exceeded maximum nesting depth");
  return true;
}

bool JsonParser::ArrayStart() {
  ABSL_DCHECK_EQ(Peek(), kArray);
  CHK(IncAndCheckDepth());
  return ParseCharSkipWhitespace('[');
}

bool JsonParser::ArrayNext() { return SequenceNext<']'>(); }

bool JsonParser::ArrayEnd() {
  --depth_;
  return ParseCharSkipWhitespace(']');
}

bool JsonParser::ObjectStart() {
  CHK(IncAndCheckDepth());
  CHK_MSG(ParseCharSkipWhitespace('{'), "Expected JSON object");
  return true;
}

bool JsonParser::ObjectNext() {
  if (!SequenceNext<'}'>()) {
    return false;
  }
  CHK_MSGF(
      Peek() == kString, "Object member must start with string, not $0",
      (ptr_ < end_) ? absl::string_view(ptr_, 1) : absl::string_view("<EOF>"));
  return true;
}

bool JsonParser::ObjectEnd() {
  --depth_;
  CHK(ParseCharSkipWhitespace('}'));
  return true;
}
bool JsonParser::SkipValue() {
  absl::string_view str;
  double d;
  switch (Peek()) {
    case kObject:
      CHK(ObjectStart());
      while (ObjectNext()) {
        CHK(ParseString(&str));
        CHK(ParseEntrySeparator());
        CHK(SkipValue());
      }
      return ObjectEnd();
    case kArray:
      CHK(ArrayStart());
      while (ArrayNext()) {
        CHK(SkipValue());
      }
      return CHKRET(ArrayEnd());
    case kTrue:
      return CHKRET(ParseTrue());
    case kFalse:
      return CHKRET(ParseFalse());
    case kNull:
      return CHKRET(ParseNull());
    case kString:
      return CHKRET(ParseString(&str));
    case kNumber:
      return CHKRET(ParseNumber(&d));
    case kEnd:
      AddError("Unexpected EOF");
      return false;
    case kError:
      return CHKRET(false);
  }
}

// Schema-aware JSON -> Protobuf translation ///////////////////////////////////

// This class uses the generic JSON parser above to convert to serialized
// protobuf binary format, according to a given schema.

class JsonConverter {
 public:
  JsonConverter(io::ZeroCopyInputStream* input,
                io::ZeroCopyOutputStream* output,
                io::ErrorCollector* error_collector,
                const JsonParseOptions& options,
                const google::protobuf::DescriptorPool* any_pool)
      : options_(options),
        any_pool_(any_pool),
        parser_(input, error_collector, 64),
        out_(output) {}

  JsonConverter(io::ZeroCopyInputStream* input,
                io::ZeroCopyOutputStream* output,
                const JsonParser::Position position,
                const JsonParseOptions& options,
                const google::protobuf::DescriptorPool* any_pool)
      : options_(options),
        any_pool_(any_pool),
        parser_(input, position),
        out_(output) {}

  bool ParseAndConvert(const google::protobuf::Descriptor* d);
  bool Flush() {
    parser_.BackUp();
    return out_.Flush();
  }

 private:
  // TODO: descriptor.h should have an enum like this built-in.
  enum class WellKnown {
    kUnspecified,
    kAny,
    kFieldMask,
    kDuration,
    kTimestamp,
    kDoubleValue,
    kFloatValue,
    kInt64Value,
    kUInt64Value,
    kInt32Value,
    kUInt32Value,
    kStringValue,
    kBytesValue,
    kBoolValue,
    kValue,
    kListValue,
    kStruct
  };

  void AddError(absl::string_view str) { parser_.AddError(str); }
  bool HasError() const { return parser_.HasError(); }

  // Generic Unitility Routines.
  [[nodiscard]] static int StrToUint64Raw(uint64_t* val,
                                          absl::string_view* str);
  [[nodiscard]] static bool StrToInt64Raw(int64_t* val, absl::string_view* str);
  [[nodiscard]] static bool StrToUint64(absl::string_view str, uint64_t* val);
  [[nodiscard]] static bool StrToInt64(absl::string_view str, int64_t* val);
  [[nodiscard]] static bool CheckUint32Range(uint64_t val, bool limit32);
  [[nodiscard]] static bool CheckInt64Range(int64_t val, bool limit32);
  [[nodiscard]] static int DivideRoundUp(int a, int b);
  [[nodiscard]] static int GetEpochDays(int year, int month, int day);
  [[nodiscard]] static int64_t GetUnixTime(const struct tm* tp);
  [[nodiscard]] static bool IsValue(const google::protobuf::FieldDescriptor* field);
  [[nodiscard]] static bool ConsumeChar(char ch, absl::string_view* str);
  [[nodiscard]] static bool ParseIntDigits(int digits, int* num,
                                           absl::string_view* str);

  [[nodiscard]] static uint32_t ZigZagEncode32(int32_t n) {
    return (n << 1) ^ (n >> 31);
  }
  [[nodiscard]] static uint64_t ZigZagEncode64(int64_t n) {
    return (n << 1) ^ (n >> 63);
  }

  // The Read*() functions read a value and check its validity but do not write
  // anything.  They will set an error message if necessary.
  [[nodiscard]] bool ReadSignedInteger(bool limit32, int64_t* i64);
  [[nodiscard]] bool ReadUnsignedInteger(bool limit32, uint64_t* u64);
  [[nodiscard]] bool ReadDouble(double* d);
  [[nodiscard]] bool ReadBool(const FieldDescriptor* f, bool* b);
  [[nodiscard]] bool ReadNanos(int32_t* nanos, absl::string_view* str);

  // Base64.
  [[nodiscard]] bool ConvertBase64(absl::string_view str);
  [[nodiscard]] bool ConvertNonBase64Chars(absl::string_view str);
  [[nodiscard]] bool ConvertBase64Padding(absl::string_view str);
  [[nodiscard]] bool ConvertPartialBase64(absl::string_view str);

  // Well-known types.
  [[nodiscard]] bool ConvertWellKnownListValue();
  [[nodiscard]] bool ConvertWellKnownStructEntry();
  [[nodiscard]] bool ConvertWellKnownStruct();
  [[nodiscard]] bool ConvertWellKnownValue();
  [[nodiscard]] bool ConvertTimestamp();
  [[nodiscard]] bool ConvertDuration();
  [[nodiscard]] bool ConvertFieldMaskField(absl::string_view str);
  [[nodiscard]] bool ConvertFieldMask();
  [[nodiscard]] bool ConvertAnyField(const google::protobuf::Descriptor* m);
  [[nodiscard]] const google::protobuf::Descriptor* ConvertAnyTypeUrl();
  [[nodiscard]] bool ConvertAny();
  [[nodiscard]] bool ConvertWellKnown(const google::protobuf::Descriptor* m);

  // Fundamental JSON types.
  [[nodiscard]] bool ConvertJsonArray(const google::protobuf::FieldDescriptor* field);
  [[nodiscard]] bool ConvertJsonMap(const google::protobuf::FieldDescriptor* field);
  [[nodiscard]] bool ConvertJsonValue(const google::protobuf::FieldDescriptor* field);
  [[nodiscard]] bool ConvertJsonField(const google::protobuf::Descriptor* d);
  [[nodiscard]] bool ConvertJsonObject(const google::protobuf::Descriptor* d);

  // Output routines.

  [[nodiscard]] bool StartDelimited() { return out_.StartDelimited(); }
  [[nodiscard]] bool EndDelimited() { return out_.EndDelimited(); }
  [[nodiscard]] bool WriteBuf(const void* data, size_t size) {
    absl::string_view view(static_cast<const char*>(data), size);
    CHK_MSG(out_.Write(view), "Failed to write to output");
    return true;
  }

  [[nodiscard]] bool WriteVarint(uint64_t val) {
    uint8_t buf[10];
    uint8_t* end =
        google::protobuf::io::CodedOutputStream::WriteVarint64ToArray(val, buf);
    return CHKRET(WriteBuf(buf, end - buf));
  }

  template <class T>
  [[nodiscard]] bool WriteFixed(T val) {
    return CHKRET(WriteBuf(&val, sizeof(val)));
  }

  [[nodiscard]] bool WriteChar(char ch) { return WriteFixed(ch); }

  [[nodiscard]] bool WriteTag(uint8_t wire_type, uint32_t fieldnum) {
    return CHKRET(WriteVarint(wire_type | (fieldnum << 3)));
  }

  [[nodiscard]] bool WriteTag(const google::protobuf::FieldDescriptor* field) {
    int wire_type = kWireTypeForType[field->type()];
    return CHKRET(WriteTag(wire_type, field->number()));
  }

  [[nodiscard]] bool WriteDelimitedString(absl::string_view str) {
    CHK(WriteVarint(str.size()));
    return CHKRET(WriteBuf(str.data(), str.size()));
  }

  [[nodiscard]] bool WriteStringField(uint32_t fieldnum,
                                      absl::string_view str) {
    CHK(WriteTag(WireFormatLite::WIRETYPE_LENGTH_DELIMITED, fieldnum));
    return CHKRET(WriteDelimitedString(str));
  }

  [[nodiscard]] bool WriteVarintField(uint32_t fieldnum, uint64_t val) {
    CHK(WriteTag(WireFormatLite::WIRETYPE_VARINT, fieldnum));
    return CHKRET(WriteVarint(val));
  }

  static const uint8_t kWireTypeForType[19];
  static const signed char kBase64[256];

  const JsonParseOptions options_;
  const google::protobuf::DescriptorPool* any_pool_;
  JsonParser parser_;
  LazyVarintStream out_;

  // The field currently being parsed.  If non-nullptr, the field name will be
  // appended to any error message.  We do *not* use this for any parsing logic.
  // For actual parsing we prefer the explicitness of passing field info
  // explicitly.
  const google::protobuf::FieldDescriptor* debug_field_ = nullptr;
};

const uint8_t JsonConverter::kWireTypeForType[19] = {
    WireFormatLite::WIRETYPE_END_GROUP,         // ENDGROUP */
    WireFormatLite::WIRETYPE_FIXED64,           // DOUBLE */
    WireFormatLite::WIRETYPE_FIXED32,           // FLOAT */
    WireFormatLite::WIRETYPE_VARINT,            // INT64 */
    WireFormatLite::WIRETYPE_VARINT,            // UINT64 */
    WireFormatLite::WIRETYPE_VARINT,            // INT32 */
    WireFormatLite::WIRETYPE_FIXED64,           // FIXED64 */
    WireFormatLite::WIRETYPE_FIXED32,           // FIXED32 */
    WireFormatLite::WIRETYPE_VARINT,            // BOOL */
    WireFormatLite::WIRETYPE_LENGTH_DELIMITED,  // STRING */
    WireFormatLite::WIRETYPE_START_GROUP,       // GROUP */
    WireFormatLite::WIRETYPE_LENGTH_DELIMITED,  // MESSAGE */
    WireFormatLite::WIRETYPE_LENGTH_DELIMITED,  // BYTES */
    WireFormatLite::WIRETYPE_VARINT,            // UINT32 */
    WireFormatLite::WIRETYPE_VARINT,            // ENUM */
    WireFormatLite::WIRETYPE_FIXED32,           // SFIXED32 */
    WireFormatLite::WIRETYPE_FIXED64,           // SFIXED64 */
    WireFormatLite::WIRETYPE_VARINT,            // SINT32 */
    WireFormatLite::WIRETYPE_VARINT,            // SINT64 */
};

// Table includes the normal base64 chars plus the URL-safe variant.
const signed char JsonConverter::kBase64[256] = {
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

bool JsonConverter::ConvertBase64Padding(absl::string_view str) {
  if (str.size() == 4) {
    if (str[3] == '=') {
      if (str[2] == '=') {
        return ConvertPartialBase64(str.substr(0, 2));  // "XX=="
      } else {
        return ConvertPartialBase64(str.substr(0, 3));  // "XXX="
      }
    }
  }

  parser_.AddError(absl::Substitute("Incorrect base64 padding '$0'", str));
  return false;
}

bool JsonConverter::ConvertPartialBase64(absl::string_view str) {
  int32_t val;
  int outbytes;
  char buf[2];

  // Sign-extend to 32 bits to elide multiple error checks into one.
  auto table = [](unsigned char ch) -> int { return kBase64[ch]; };

  switch (str.size()) {
    case 2:
      val = table(str[0]) << 18 | table(str[1]) << 12;
      buf[0] = val >> 16;
      outbytes = 1;
      break;
    case 3:
      val = table(str[0]) << 18 | table(str[1]) << 12 | table(str[2]) << 6;
      buf[0] = val >> 16;
      buf[1] = (val >> 8) & 0xff;
      outbytes = 2;
      break;
    default:
      return false;
  }

  if (val >= 0) {
    // No non-base64 chars (or padding) encountered.
    return WriteBuf(buf, outbytes);
  } else {
    parser_.AddError(absl::Substitute("Incorrect base64 padding $0", str));
    return false;
  }
}

bool JsonConverter::ConvertNonBase64Chars(absl::string_view str) {
  auto nonbase64 = [](unsigned char ch) -> bool {
    return kBase64[ch] == -1 && ch != '=';
  };

  if (nonbase64(str[0]) || nonbase64(str[1]) || nonbase64(str[2]) ||
      nonbase64(str[3])) {
    parser_.AddError("Non-base64 characters encountered");
    return false;
  }

  return ConvertBase64Padding(str);
}

bool JsonConverter::ConvertBase64(absl::string_view str) {
  CHK(StartDelimited());

  // Sign-extend to 32 bits to elide multiple error checks into one.
  auto table = [](unsigned char ch) -> int { return kBase64[ch]; };

  for (; str.size() >= 4; str.remove_prefix(4)) {
    int val = table(str[0]) << 18 | table(str[1]) << 12 | table(str[2]) << 6 |
              table(str[3]);

    if (val < 0) {
      CHK(ConvertNonBase64Chars(str));
      goto done;
    }

    char out[3];
    out[0] = val >> 16;
    out[1] = (val >> 8) & 0xff;
    out[2] = val & 0xff;
    CHK(WriteBuf(out, 3));
  }

  // Permissively allow non-padded ending.
  if (!str.empty()) {
    CHK(ConvertPartialBase64(str));
  }

done:
  return EndDelimited();
}

bool JsonConverter::CheckUint32Range(uint64_t val, bool limit32) {
  return !limit32 || (val <= UINT32_MAX);
}

bool JsonConverter::CheckInt64Range(int64_t val, bool limit32) {
  return !limit32 || (val <= INT32_MAX && val >= INT32_MIN);
}

int JsonConverter::StrToUint64Raw(uint64_t* val, absl::string_view* str) {
  uint64_t u64 = 0;
  int i;
  for (i = 0; i < str->size(); i++) {
    unsigned ch = (*str)[i] - '0';
    if (ch >= 10) break;
    if (u64 > ULONG_MAX / 10 || u64 * 10 > ULONG_MAX - ch) {
      return -1;  // Overflow
    }
    u64 *= 10;
    u64 += ch;
  }

  *val = u64;
  str->remove_prefix(i);
  return i;
}

bool JsonConverter::StrToInt64Raw(int64_t* val, absl::string_view* str) {
  bool neg = false;
  uint64_t u64;

  if (ConsumeChar('-', str)) {
    neg = true;
  }

  if (!StrToUint64Raw(&u64, str)) return false;
  if (u64 > static_cast<uint64_t>(INT64_MAX) + neg) return false;  // Overflow

  *val = neg ? -u64 : u64;
  return true;
}

bool JsonConverter::StrToUint64(absl::string_view str, uint64_t* val) {
  return StrToUint64Raw(val, &str) > 0 && str.empty();
}

bool JsonConverter::StrToInt64(absl::string_view str, int64_t* val) {
  return StrToInt64Raw(val, &str) && str.empty();
}

bool JsonConverter::ReadSignedInteger(bool limit32, int64_t* i64) {
  switch (parser_.Peek()) {
    case JsonParser::kNumber: {
      double d;
      CHK(parser_.ParseNumber(&d));
      CHK_MSGF(d <= 9223372036854774784.0 && d >= -9223372036854775808.0,
               "JSON number $0 is out of range", d);
      *i64 = d;
      CHK_MSGF(*i64 == d && CheckInt64Range(*i64, limit32),
               "JSON number $0 is out of range or not an integer", d);
      return true;
    }
    case JsonParser::kString: {
      absl::string_view str;
      CHK(parser_.ParseString(&str));
      CHK_MSGF(StrToInt64(str, i64) && CheckInt64Range(*i64, limit32),
               "Malformed or out-of-range number '$0'", str);
      return true;
    }
    default:
      AddError("Expected number or string");
      return false;
  }
}

bool JsonConverter::ReadUnsignedInteger(bool limit32, uint64_t* u64) {
  switch (parser_.Peek()) {
    case JsonParser::kNumber: {
      double d;
      CHK(parser_.ParseNumber(&d));
      CHK_MSGF(d <= 18446744073709549568.0 && d >= 0,
               "JSON number $0 is out of range", d);
      *u64 = d;
      CHK_MSGF(*u64 == d && CheckUint32Range(*u64, limit32),
               "JSON number $0 is out of range or not an integer", d)
      return true;
    }
    case JsonParser::kString: {
      absl::string_view str;
      CHK(parser_.ParseString(&str));
      CHK_MSGF(StrToUint64(str, u64) && CheckUint32Range(*u64, limit32),
               "Malformed or out-of-range number '$0'", str);
      return true;
    }
    default:
      AddError("Expected number or string");
      return false;
  }
}

bool JsonConverter::ReadDouble(double* d) {
  switch (parser_.Peek()) {
    case JsonParser::kNumber:
      return parser_.ParseNumber(d);
    case JsonParser::kString: {
      absl::string_view str;
      CHK(parser_.ParseString(&str));
      if (str == "NaN") {
        *d = NAN;
      } else if (str == "Infinity") {
        *d = std::numeric_limits<double>::infinity();
      } else if (str == "-Infinity") {
        *d = -std::numeric_limits<double>::infinity();
      } else {
        char* end;
        errno = 0;
        *d = strtod(str.data(), &end);
        CHK_MSGF(errno == 0, "Number out of range: $0", str);
        CHK_MSGF(end == str.data() + str.size(),
                 "Stray characters converting quoted string to number: $0",
                 str);
      }
      return true;
    }
    default:
      parser_.AddError(absl::Substitute("Expected number or string"));
      return false;
  }
}

bool JsonConverter::ReadBool(const google::protobuf::FieldDescriptor* field, bool* b) {
  bool is_map_key =
      field->number() == 1 && field->containing_type()->options().map_entry();
  if (is_map_key) {
    // In map keys, bools are quoted strings.
    absl::string_view str;
    CHK(parser_.ParseString(&str));
    if (str == "false") {
      *b = false;
      return true;
    } else if (str == "true") {
      *b = true;
      return true;
    } else {
      parser_.AddError(absl::Substitute("Invalid boolean map key: $0", str));
      return false;
    }
  } else {
    switch (parser_.Peek()) {
      case JsonParser::kFalse:
        CHK(parser_.ParseFalse());
        *b = false;
        return true;
      case JsonParser::kTrue:
        CHK(parser_.ParseTrue());
        *b = true;
        return true;
      default:
        // Should we accept 0/nonzero as true/false?
        parser_.AddError(absl::Substitute("Expected true or false"));
        return false;
    }
  }
}

bool JsonConverter::ConvertWellKnownListValue() {
  CHK(parser_.ArrayStart());

  while (parser_.ArrayNext()) {
    // repeated Value values = 1;
    CHK(WriteTag(WireFormatLite::WIRETYPE_LENGTH_DELIMITED, 1));
    CHK(StartDelimited());
    CHK(ConvertWellKnownValue());
    CHK(EndDelimited());
  }

  return parser_.ArrayEnd();
}

bool JsonConverter::ConvertWellKnownStructEntry() {
  absl::string_view str;

  // map<string, Value> fields = 1;
  CHK(parser_.ParseString(&str));
  CHK(WriteStringField(1, str));
  CHK(parser_.ParseEntrySeparator());

  CHK(WriteTag(WireFormatLite::WIRETYPE_LENGTH_DELIMITED, 2));
  CHK(StartDelimited());
  CHK(ConvertWellKnownValue());
  CHK(EndDelimited());

  return true;
}

bool JsonConverter::ConvertWellKnownStruct() {
  CHK(parser_.ObjectStart());

  while (parser_.ObjectNext()) {
    // map<string, Value> fields = 1;
    CHK(WriteTag(WireFormatLite::WIRETYPE_LENGTH_DELIMITED, 1));
    CHK(StartDelimited());
    CHK(ConvertWellKnownStructEntry());
    CHK(EndDelimited());
  }

  return parser_.ObjectEnd();
}

bool JsonConverter::ConvertWellKnownValue() {
  switch (parser_.Peek()) {
    case JsonParser::kNull:
      // NullValue null_value = 1;
      CHK(parser_.ParseNull());
      return CHKRET(WriteVarintField(1, 0));
    case JsonParser::kNumber: {
      // double number_value = 2;
      double d;
      CHK(parser_.ParseNumber(&d));
      CHK(WriteTag(WireFormatLite::WIRETYPE_FIXED64, 2));
      return CHKRET(WriteFixed(d));
    }
    case JsonParser::kString: {
      // string string_value = 3;
      absl::string_view str;
      CHK(parser_.ParseString(&str));
      return CHKRET(WriteStringField(3, str));
    }
    case JsonParser::kFalse:
      // bool bool_value = 4;
      CHK(parser_.ParseFalse());
      return CHKRET(WriteVarintField(4, 0));
    case JsonParser::kTrue:
      // bool bool_value = 4;
      CHK(parser_.ParseTrue());
      return CHKRET(WriteVarintField(4, 1));
    case JsonParser::kObject: {
      // Struct struct_value = 5;
      CHK(WriteTag(WireFormatLite::WIRETYPE_LENGTH_DELIMITED, 5));
      CHK(StartDelimited());
      CHK(ConvertWellKnownStruct());
      return CHKRET(EndDelimited());
    }
    case JsonParser::kArray: {
      // ListValue list_value = 6;
      CHK(WriteTag(WireFormatLite::WIRETYPE_LENGTH_DELIMITED, 6));
      CHK(StartDelimited());
      CHK(ConvertWellKnownListValue());
      return CHKRET(EndDelimited());
    }
    case JsonParser::kEnd:
    case JsonParser::kError:
      return false;
  }
}

int JsonConverter::DivideRoundUp(int a, int b) {
  ABSL_DCHECK(a >= 0 && b > 0);
  return (a + (b - 1)) / b;
}

// GetEpochDays(1970, 1, 1) == 1970-01-01 == 0.
int JsonConverter::GetEpochDays(int year, int month, int day) {
  static const uint16_t month_yday[12] = {0,   31,  59,  90,  120, 151,
                                          181, 212, 243, 273, 304, 334};
  int febs_since_0 = month > 2 ? year + 1 : year;
  int leap_days_since_0 = DivideRoundUp(febs_since_0, 4) -
                          DivideRoundUp(febs_since_0, 100) +
                          DivideRoundUp(febs_since_0, 400);
  int days_since_0 =
      365 * year + month_yday[month - 1] + (day - 1) + leap_days_since_0;

  // Convert from 0-epoch (0001-01-01 BC) to Unix Epoch (1970-01-01 AD).
  // Since the "BC" system does not have a year zero, 1 BC == year zero.
  return days_since_0 - 719528;
}

int64_t JsonConverter::GetUnixTime(const struct tm* tp) {
  int64_t ret = GetEpochDays(tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday);
  ret = (ret * 24) + tp->tm_hour;
  ret = (ret * 60) + tp->tm_min;
  ret = (ret * 60) + tp->tm_sec;
  return ret;
}

bool JsonConverter::ParseIntDigits(int digits, int* num,
                                   absl::string_view* str) {
  uint64_t u64 = 0;
  ABSL_DCHECK_LE(digits, 9);  // int can't overflow.
  if (!StrToUint64(str->substr(0, digits), &u64)) {
    return false;
  }
  str->remove_prefix(digits);
  *num = u64;
  return true;
}

bool JsonConverter::ConsumeChar(char ch, absl::string_view* str) {
  if (str->empty() || (*str)[0] != ch) {
    return false;
  }
  str->remove_prefix(1);
  return true;
}

bool JsonConverter::ReadNanos(int32_t* nanos, absl::string_view* str) {
  if (ConsumeChar('.', str)) {
    uint64_t u64;
    int digits = StrToUint64Raw(&u64, str);
    CHK_MSG(digits >= 0 && digits <= 9, "Too many digits for partial seconds");
    int exp_lg10 = 9 - digits;
    while (exp_lg10-- > 0) {
      u64 *= 10;
    }
    *nanos = u64;
  }
  return true;
}

bool JsonConverter::ConvertTimestamp() {
  int64_t seconds;
  int32_t nanos = 0;
  absl::string_view str;

  CHK(parser_.ParseString(&str));
  CHK_MSG(str.size() >= 20, "Malformed timestamp");

  {
    struct tm time;

    // 1972-01-01T01:00:00
    CHK_MSG(
        ParseIntDigits(4, &time.tm_year, &str) && ConsumeChar('-', &str) &&
            ParseIntDigits(2, &time.tm_mon, &str) && ConsumeChar('-', &str) &&
            ParseIntDigits(2, &time.tm_mday, &str) && ConsumeChar('T', &str) &&
            ParseIntDigits(2, &time.tm_hour, &str) && ConsumeChar(':', &str) &&
            ParseIntDigits(2, &time.tm_min, &str) && ConsumeChar(':', &str) &&
            ParseIntDigits(2, &time.tm_sec, &str),
        "Malformed timestamp");

    // Weird "struct tm" conventions.
    time.tm_year -= 1900;
    time.tm_mon--;

    CHK_MSG(time.tm_mon < 12, "Bad month in timestamp");

    seconds = GetUnixTime(&time);
  }

  CHK(ReadNanos(&nanos, &str));

  {
    // [+-]08:00 or Z
    int offset = 0;
    bool neg = false;

    CHK(!str.empty());
    char ch = str[0];
    str.remove_prefix(1);
    switch (ch) {
      case '-':
        neg = true;
        ABSL_FALLTHROUGH_INTENDED;
      case '+':
        CHK_MSG(str.size() == 5, "Malformed time zone offset");
        CHK(ParseIntDigits(2, &offset, &str));
        CHK_MSG(str == ":00", "Bad minute offset");
        offset *= 60 * 60;
        seconds += (neg ? offset : -offset);
        break;
      case 'Z':
        CHK_MSG(str.empty(), "Malformed time zone offset");
        break;
      default:
        AddError("Malformed time zone offset");
        return false;
    }
  }

  CHK_MSG(seconds >= -62135596800,
          "error parsing timestamp: minimum acceptable value is "
          "0001-01-01T00:00:00Z");

  // int64_t seconds = 1;
  // int32_t nanos = 2;
  CHK(WriteVarintField(1, seconds));
  CHK(WriteVarintField(2, nanos));
  return true;
}

bool JsonConverter::ConvertDuration() {
  int64_t seconds;
  int32_t nanos = 0;
  absl::string_view str;

  // "3.000000001s", "3s", etc.
  CHK(parser_.ParseString(&str));
  bool minus = !str.empty() && (str.front() == '-');
  CHK_MSG(StrToInt64Raw(&seconds, &str),
          "Failed to convert seconds in Duration");
  CHK(ReadNanos(&nanos, &str));
  CHK(ConsumeChar('s', &str));
  CHK(str.empty());

  CHK_MSG(seconds >= -315576000000LL && seconds <= 315576000000LL,
          "Duration out of range.");

  if (seconds < 0 || minus) {
    nanos = -nanos;
  }

  // int64_t seconds = 1;
  // int32_t nanos = 2;
  CHK(WriteVarintField(1, seconds));
  CHK(WriteVarintField(2, nanos));
  return true;
}

bool JsonConverter::ConvertFieldMaskField(absl::string_view str) {
  // repeated string paths = 1;
  CHK(WriteTag(WireFormatLite::WIRETYPE_LENGTH_DELIMITED, 1));
  CHK(StartDelimited());

  // fooBarBaz -> foo_bar_baz
  for (char ch : str) {
    if (ch >= 'A' && ch <= 'Z') {
      CHK(WriteChar('_'));
      CHK(WriteChar(ch + 32));
    } else {
      CHK(WriteChar(ch));
    }
  }

  return EndDelimited();
}

bool JsonConverter::ConvertFieldMask() {
  absl::string_view str;
  CHK(parser_.ParseString(&str));

  if (str.empty()) {
    return true;
  }

  // repeated string paths = 1;
  size_t pos;
  while ((pos = str.find_first_of(',')) != absl::string_view::npos) {
    CHK(ConvertFieldMaskField(str.substr(0, pos)));
    str.remove_prefix(pos + 1);
  }
  CHK(ConvertFieldMaskField(str));

  return true;
}

bool JsonConverter::ConvertAnyField(const google::protobuf::Descriptor* d) {
  CHK_MSG(d, "Any message did not contain @type field");
  if (d->well_known_type() == google::protobuf::Descriptor::WELLKNOWNTYPE_UNSPECIFIED) {
    // For regular types: {"@type": "[user type]", "f1": <V1>, "f2": <V2>}
    // where f1, f2, etc. are the normal fields of this type.
    return ConvertJsonField(d);
  } else {
    // For well-known types: {"@type": "[well-known type]", "value": <X>}
    // where <X> is whatever encoding the WKT normally uses.
    absl::string_view str;
    CHK(parser_.ParseString(&str));
    CHK(str == "value");
    CHK(parser_.ParseEntrySeparator());
    return ConvertWellKnown(d);
  }
}

const google::protobuf::Descriptor* JsonConverter::ConvertAnyTypeUrl() {
  absl::string_view type_url;
  CHK(parser_.ParseString(&type_url));

  // string type_url = 1;
  CHK(WriteStringField(1, type_url));

  // type.googleapis.com/google.protobuf.Duration (we just strip to '/').
  size_t pos = type_url.find_first_of('/');
  CHK(pos != absl::string_view::npos);
  type_url.remove_prefix(pos + 1);
  CHK(!type_url.empty());

  const google::protobuf::Descriptor* m =
      any_pool_->FindMessageTypeByName(std::string(type_url));
  CHK_MSGF(m, "No such type for Any: $0", type_url);
  return m;
}

bool JsonConverter::ConvertAny() {
  const google::protobuf::Descriptor* m = nullptr;
  CHK(parser_.ObjectStart());
  JsonParser::Position position;
  std::string pre_type_data;

  {
    position = parser_.GetPosition();
    parser_.StartAccumulateAny();
    size_t typeurl_location = 0;

    // A completely empty object is valid as an Any with
    // no type_url or value.
    bool is_empty_object = true;

    // Scan looking for "@type", which is not necessarily first.
    while (!m && parser_.ObjectNext()) {
      is_empty_object = false;
      absl::string_view name;
      CHK(parser_.ParseString(&name));
      bool is_type = name == "@type";
      CHK(parser_.ParseEntrySeparator());
      if (is_type) {
        m = ConvertAnyTypeUrl();
      } else {
        CHK(parser_.SkipValue());
        typeurl_location = parser_.AnyBytesAccumulated();
      }
    }

    absl::string_view data = parser_.FinishAccumulateAny();
    ABSL_DCHECK(is_empty_object || typeurl_location < data.size());
    pre_type_data.assign(data.data(), typeurl_location);
  }

  // string type_url = 1;
  // bytes value = 2;
  CHK(WriteTag(WireFormatLite::WIRETYPE_LENGTH_DELIMITED, 2));
  CHK(StartDelimited());

  if (!pre_type_data.empty()) {
    // Pick up fields before "@type".  Surround in {} so it parses like a normal
    // JSON object.
    pre_type_data = absl::StrCat("{", pre_type_data, "}");
    position.column--;  // Compensate for prepended '{'.
    std::string output;
    google::protobuf::io::ArrayInputStream in_stream(pre_type_data.data(),
                                           pre_type_data.size());
    google::protobuf::io::StringOutputStream out_stream(&output);
    JsonConverter any_converter(&in_stream, &out_stream, position, options_,
                                any_pool_);
    CHK(any_converter.parser_.ObjectStart());
    while (any_converter.parser_.ObjectNext()) {
      CHK(any_converter.ConvertAnyField(m));
    }
    CHK(any_converter.parser_.ObjectEnd());
    CHK(any_converter.Flush());
    out_.Write(output);
  }

  // Parse fields after "@type"
  while (parser_.ObjectNext()) {
    CHK(ConvertAnyField(m));
  }

  CHK(parser_.ObjectEnd());
  return EndDelimited();
}

bool JsonConverter::ConvertWellKnown(const google::protobuf::Descriptor* m) {
  switch (m->well_known_type()) {
    case Descriptor::WELLKNOWNTYPE_STRINGVALUE:
    case Descriptor::WELLKNOWNTYPE_BYTESVALUE:
    case Descriptor::WELLKNOWNTYPE_DOUBLEVALUE:
    case Descriptor::WELLKNOWNTYPE_FLOATVALUE:
    case Descriptor::WELLKNOWNTYPE_INT64VALUE:
    case Descriptor::WELLKNOWNTYPE_UINT64VALUE:
    case Descriptor::WELLKNOWNTYPE_UINT32VALUE:
    case Descriptor::WELLKNOWNTYPE_INT32VALUE:
    case Descriptor::WELLKNOWNTYPE_BOOLVALUE:
      return CHKRET(ConvertJsonValue(m->FindFieldByNumber(1)));
    case Descriptor::WELLKNOWNTYPE_FIELDMASK:
      return CHKRET(ConvertFieldMask());
    case Descriptor::WELLKNOWNTYPE_DURATION:
      return CHKRET(ConvertDuration());
    case Descriptor::WELLKNOWNTYPE_TIMESTAMP:
      return CHKRET(ConvertTimestamp());
    case Descriptor::WELLKNOWNTYPE_ANY:
      return CHKRET(ConvertAny());
    case Descriptor::WELLKNOWNTYPE_VALUE:
      return CHKRET(ConvertWellKnownValue());
    case Descriptor::WELLKNOWNTYPE_LISTVALUE:
      return CHKRET(ConvertWellKnownListValue());
    case Descriptor::WELLKNOWNTYPE_STRUCT:
      return CHKRET(ConvertWellKnownStruct());
    case Descriptor::WELLKNOWNTYPE_UNSPECIFIED:
    // Since this is part of the protobuf implementation itself and must be
    // updated when a new well-known type is added, we use this internal-only
    // enumerator.
    case Descriptor::__WELLKNOWNTYPE__DO_NOT_USE__ADD_DEFAULT_INSTEAD__:
      ABSL_DCHECK(false);
      return false;
  }
}

bool JsonConverter::ConvertJsonArray(const google::protobuf::FieldDescriptor* field) {
  switch (parser_.Peek()) {
    case JsonParser::kNull:
      return parser_.ParseNull();
    case JsonParser::kArray:
      break;
    default:
      parser_.AddError("Expected array");
      return false;
  }

  CHK(parser_.ArrayStart());

  while (parser_.ArrayNext()) {
    CHK(ConvertJsonValue(field));
  }

  CHK(parser_.ArrayEnd());
  return true;
}

bool JsonConverter::ConvertJsonMap(const google::protobuf::FieldDescriptor* field) {
  const google::protobuf::Descriptor* entry = field->message_type();
  const google::protobuf::FieldDescriptor* key = entry->FindFieldByNumber(1);
  const google::protobuf::FieldDescriptor* value = entry->FindFieldByNumber(2);

  ABSL_DCHECK(key && value);

  CHK(parser_.ObjectStart());
  while (parser_.ObjectNext()) {
    CHK(WriteTag(field));
    CHK(StartDelimited());
    CHK(ConvertJsonValue(key));
    CHK_MSG(parser_.ParseEntrySeparator(), "Expected ':'");
    CHK(ConvertJsonValue(value));
    CHK(EndDelimited());
  }
  CHK(parser_.ObjectEnd());
  return true;
}

bool JsonConverter::ConvertJsonValue(const google::protobuf::FieldDescriptor* field) {
  CHK(WriteTag(field));
  switch (field->cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
      bool b;
      CHK(ReadBool(field, &b));
      return CHKRET(WriteChar(b));
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
      double d;
      CHK(ReadDouble(&d));
      return CHKRET(WriteFixed<float>(d));
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
      double d;
      CHK(ReadDouble(&d));
      return CHKRET(WriteFixed(d));
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
      uint64_t u64;
      CHK(ReadUnsignedInteger(true, &u64));
      switch (field->type()) {
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
          return CHKRET(WriteFixed<uint32_t>(u64));
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
          return CHKRET(WriteVarint(u64));
        default:
          UNREACHABLE();
      }
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
      uint64_t u64;
      CHK(ReadUnsignedInteger(false, &u64));
      switch (field->type()) {
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
          return CHKRET(WriteFixed(u64));
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
          return CHKRET(WriteVarint(u64));
        default:
          UNREACHABLE();
      }
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
      int64_t i64;
    int32_val:
      CHK(ReadSignedInteger(true, &i64));
      switch (field->type()) {
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
          return CHKRET(WriteFixed<int32_t>(i64));
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_ENUM:
          return CHKRET(WriteVarint(i64));
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
          return CHKRET(WriteVarint(ZigZagEncode32(i64)));
        default:
          UNREACHABLE();
      }
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
      int64_t i64;
      CHK(ReadSignedInteger(false, &i64));
      switch (field->type()) {
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
          return CHKRET(WriteFixed(i64));
        case google::protobuf::FieldDescriptor::TYPE_INT64:
          return CHKRET(WriteVarint(i64));
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
          return CHKRET(WriteVarint(ZigZagEncode64(i64)));
        default:
          UNREACHABLE();
      }
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
      absl::string_view str;
      CHK_MSGF(parser_.Peek() == JsonParser::kString,
               "Expected string for string field $0", field->name());
      CHK(parser_.ParseString(&str));
      if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES) {
        return CHKRET(ConvertBase64(str));
      } else {
        return CHKRET(WriteDelimitedString(str));
      }
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
      if (parser_.Peek() == JsonParser::kString) {
        absl::string_view str;
        CHK(parser_.ParseString(&str));
        const google::protobuf::EnumValueDescriptor* val =
            field->enum_type()->FindValueByName(std::string(str));
        CHK_MSGF(val, "Unknown enumerator $0", str);
        return CHKRET(WriteVarint(val->number()));
      }
      goto int32_val;
    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
      const google::protobuf::Descriptor* m = field->message_type();
      if (field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
        CHK(StartDelimited());
        if (m->well_known_type() == Descriptor::WELLKNOWNTYPE_UNSPECIFIED) {
          CHK(ConvertJsonObject(m));
        } else {
          CHK(ConvertWellKnown(m));
        }
        return CHKRET(EndDelimited());
      } else {
        // proto3 groups, does anyone use this?
        CHK(ConvertJsonObject(m));
        CHK(WriteTag(WireFormatLite::WIRETYPE_END_GROUP, field->number()));
        return true;
      }
    }
  }

  UNREACHABLE();
}

bool JsonConverter::IsValue(const google::protobuf::FieldDescriptor* field) {
  return field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE &&
         field->message_type()->well_known_type() ==
             Descriptor::WELLKNOWNTYPE_VALUE;
}

bool JsonConverter::ConvertJsonField(const google::protobuf::Descriptor* d) {
  absl::string_view name;

  CHK_MSGF(parser_.Peek() == JsonParser::kString,
           "Expected string at beginning of field, not '$0'", parser_.Peek());
  CHK(parser_.ParseString(&name));
  std::string str_name(name);  // Concession to FindFieldByCamelcaseName().
  const google::protobuf::FieldDescriptor* f = d->FindFieldByCamelcaseName(str_name);

  if (!f) {
    f = d->FindFieldByName(str_name);
  }

  if (!f) {
    CHK_MSGF(options_.ignore_unknown_fields,
             "Unknown field '$0' when parsing message $1", name,
             d->full_name());
    CHK(parser_.ParseEntrySeparator());
    return CHKRET(parser_.SkipValue());
  } else {
    CHK(parser_.ParseEntrySeparator());
  }

  if (parser_.Peek() == JsonParser::kNull && !IsValue(f)) {
    // JSON "null" indicates a default value, so no need to encode anything.
    return CHKRET(parser_.ParseNull());
  }

  bool ret;
  const google::protobuf::FieldDescriptor* preserved_field = debug_field_;
  debug_field_ = f;

  if (f->is_map()) {
    ret = ConvertJsonMap(f);
  } else if (f->is_repeated()) {
    ret = ConvertJsonArray(f);
  } else {
    ret = ConvertJsonValue(f);
  }

  debug_field_ = preserved_field;
  return CHKRET(ret);
}

bool JsonConverter::ConvertJsonObject(const google::protobuf::Descriptor* d) {
  CHK(parser_.ObjectStart());

  while (parser_.ObjectNext()) {
    CHK(ConvertJsonField(d));
  }

  CHK(parser_.ObjectEnd());
  return true;
}

bool JsonConverter::ParseAndConvert(const google::protobuf::Descriptor* d) {
  // TODO: should we support various well-known types at the top-level, or
  // does the top-level need to be a regular message?
  return CHKRET(ConvertJsonObject(d));
}

bool JsonToBinaryStream(const Descriptor* descriptor,
                        io::ZeroCopyInputStream* json_input,
                        io::ZeroCopyOutputStream* binary_output,
                        io::ErrorCollector* error_collector,
                        const JsonParseOptions& options,
                        const DescriptorPool* any_pool) {
  JsonConverter converter(json_input, binary_output, error_collector, options,
                          any_pool);
  return converter.ParseAndConvert(descriptor) && converter.Flush();
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
