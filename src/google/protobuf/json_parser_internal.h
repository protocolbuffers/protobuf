// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file contains classes used internally by the JSON parser.
//
// They are here so they can be tested by unit tests, independently from the
// rest of the JSON parser.

#ifndef GOOGLE_PROTOBUF_JSON_PARSER_INTERNAL_H__
#define GOOGLE_PROTOBUF_JSON_PARSER_INTERNAL_H__

#include <stack>
#include <vector>

#include "absl/log/absl_check.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream.h"

// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

#define CHK(x) \
  if (!(x)) {  \
    return 0;  \
  }

namespace google {
namespace protobuf {
namespace internal {

// Output Buffering ////////////////////////////////////////////////////////////

// LazyVarintStream is a streaming writer that buffers data when necessary to
// insert varints once a delimited region ends.
//
// For now we just use the naive approach of std::string::insert().
class LazyVarintStream {
 public:
  LazyVarintStream(io::ZeroCopyOutputStream* stream) : stream_(stream) {}

  // Writes the given data to the stream.
  bool Write(absl::string_view data) {
    output_.append(data.data(), data.size());
    return true;
  }

  // Call StartDelimited()/EndDelimited() before and after delimited regions.
  // The stream will insert a length varint at the beginning of each region.
  bool StartDelimited();
  bool EndDelimited();

  bool Flush();

 private:
  io::ZeroCopyOutputStream* stream_;
  std::stack<size_t> stack_;
  std::string output_;
};

// Generic JSON parser /////////////////////////////////////////////////////////

// JsonParser parses the JSON wire format, and doesn't know anything about
// protobuf schemas or semantics.

class JsonParser {
 public:
  struct Position {
    int depth;
    int line;
    uintptr_t column;
    io::ErrorCollector* error_collector;
  };

  JsonParser(io::ZeroCopyInputStream* input,
             io::ErrorCollector* error_collector, int max_depth)
      : stream_(input), depth_(max_depth), error_collector_(error_collector) {}

  JsonParser(io::ZeroCopyInputStream* input, Position position)
      : stream_(input),
        depth_(position.depth),
        error_collector_(position.error_collector),
        line_(position.line),
        line_begin_(-position.column) {}

  // ValueType and Peek() tell us what the next value type is.
  enum ValueType {
    kEnd,
    kError,
    kObject,
    kArray,
    kNumber,
    kString,
    kTrue,
    kFalse,
    kNull
  };

  ValueType Peek();

  // Functions to parse each primitive JSON type.
  [[nodiscard]] bool ParseNumber(double* num);
  [[nodiscard]] bool ParseString(absl::string_view* str);
  [[nodiscard]] bool ParseTrue();
  [[nodiscard]] bool ParseFalse();
  [[nodiscard]] bool ParseNull();

  // Functions to parse an array:
  //   ArrayStart();
  //   while (ArrayNext()) {
  //     // Parse value.
  //   }
  //   ArrayEnd();
  [[nodiscard]] bool ArrayStart();
  [[nodiscard]] bool ArrayNext();
  [[nodiscard]] bool ArrayEnd();

  // Functions to parse an object:
  //   ArrayStart();
  //   while (ArrayNext()) {
  //     // Parse value.
  //   }
  //   ArrayEnd();
  [[nodiscard]] bool ObjectStart();
  [[nodiscard]] bool ObjectNext();
  [[nodiscard]] bool ObjectEnd();

  // Skips a JSON value.
  [[nodiscard]] bool SkipValue();

  // To parse an object key call:
  //    ParseString(&name);
  //    ParseSeparator();
  // Note that name will be invalidated by ParseSeparator(), so use it quick.
  [[nodiscard]] bool ParseEntrySeparator() {
    bool ret = ParseCharSkipWhitespace(':');
    if (!ret) AddError("Expected ':'");
    return ret;
  }

  // If an error occurs in interpreting the JSON, report it here.
  // Errors in JsonParser itself will set this already.
  void AddError(absl::string_view str) {
    error_ = true;
    if (error_collector_) {
      error_collector_->RecordError(
          line_, reinterpret_cast<uintptr_t>(ptr_) - line_begin_, str);
    }
  }
  bool HasError() const { return error_; }

  const char* ptr() { return ptr_; }
  void BackUp() { /*stream_->BackUp(end_ - ptr_);*/ }

  void StartAccumulateAny() {
    any_buf_.clear();
    ABSL_DCHECK(preserve_any_ == nullptr);
    preserve_any_ = ptr_;
  }

  size_t AnyBytesAccumulated() const {
    return any_buf_.size() + (ptr_ - preserve_any_);
  }

  absl::string_view FinishAccumulateAny() {
    ABSL_DCHECK(preserve_any_ != nullptr || eof_);
    if (!eof_) {
      any_buf_.append(preserve_any_, ptr_ - preserve_any_);
    }
    preserve_any_ = nullptr;
    return any_buf_;
  }

  bool IsEof() { return ptr_ == end_ ? IsEofFallback() : false; }

  Position GetPosition() const {
    return Position{depth_, line_,
                    reinterpret_cast<uintptr_t>(ptr_) - line_begin_,
                    error_collector_};
  }

  bool SkipWhitespace();

 private:
  // If this returns false, then the caller can assume that input_ is not empty.
  bool IsEofFallback();

  template <size_t N>
  bool ConsumeChars(std::array<char, N>* out) {
    if (end_ - ptr_ >= N) {
      memcpy(out->data(), ptr_, N);
      ptr_ += N;
      return true;
    } else {
      return ConsumeCharsFallback(out->data(), N);
    }
  }
  bool ConsumeCharsFallback(char* data, size_t N);

  bool TryParseChar(char ch) {
    if (IsEof() || *ptr_ != ch) {
      return false;
    }
    ptr_++;
    return true;
  }

  bool ParseCharSkipWhitespace(char want_ch) {
    char ch = ConsumeCharSkipWhitespace();
    return ch == want_ch;
  }

  bool ParseLiteral(absl::string_view lit) {
    if (end_ - ptr_ >= lit.size() &&
        lit == absl::string_view(ptr_, lit.size())) {
      ptr_ += lit.size();
      return true;
    } else {
      return ParseLiteralFallback(lit);
    }
  }
  bool ParseLiteralFallback(absl::string_view lit);

  // NULL is not allowed in JSON text, so we use 0 as failure in the proceeding
  // functions that return char.

  char PeekCharSkipWhitespace() {
    CHK(SkipWhitespace());
    return *ptr_;
  }

  char PeekChar() {
    CHK(!IsEof());
    return *ptr_;
  }

  char ConsumeCharSkipWhitespace() {
    char ch = PeekCharSkipWhitespace();
    CHK(ch);
    ptr_++;
    return ch;
  }

  char ConsumeChar() {
    CHK(!IsEof());
    return *(ptr_++);
  }

  void StartAccumulate() {
    tmp_buf_.clear();
    ResumeAccumulate();
  }

  void ResumeAccumulate() {
    ABSL_DCHECK(preserve_ == nullptr);
    preserve_ = ptr_;
  }

  absl::string_view FinishAccumulate() {
    ABSL_DCHECK(preserve_ != nullptr || eof_);
    if (!eof_) {
      tmp_buf_.append(preserve_, ptr_ - preserve_);
    }
    preserve_ = nullptr;
    return tmp_buf_;
  }

  void ResumeAccumulateAny() {
    ABSL_DCHECK(preserve_any_ == nullptr);
    preserve_any_ = ptr_;
  }

  bool SkipDigits();
  bool ParseCodepoint(uint32_t* cp);
  bool WriteUtf8Codepoint(uint32_t cp);
  bool ParseEscape();

  template <char EndCh>
  bool SequenceNext();
  bool IncAndCheckDepth();

  io::ZeroCopyInputStream* stream_;  // Input remaining to be parsed.

  // Current parse position and end of buffer.
  const char* ptr_ = nullptr;
  const char* end_ = nullptr;

  // tmp_buf_ accumulates string data while we are parsing a string or number,
  // so the entire string value is contiguous.  This is necessary when there are
  // escape sequences or buffer seams in the string.
  //
  // When preserve_ is non-nullptr, it must point into the current buffer.
  // If a buffer seam is encountered, all data starting at preserve_str_ will be
  // appended to tmp_buf_ and preserve_str_ will be reset to point to the
  // beginning of the next buffer.
  std::string tmp_buf_;
  const char* preserve_ = nullptr;

  // any_buf_: like tmp_buf_, but this is specifically for accumulating data for
  // Any fields before we see @type.
  std::string any_buf_;
  const char* preserve_any_ = nullptr;

  // We track depth to prevent stack overflow.  depth_ starts at the recursion
  // limit and counts down to 0, so depth_ > 0 checks against overflow.
  int depth_;

  // For error reporting.
  io::ErrorCollector* error_collector_;
  int line_ = 0;
  uintptr_t line_begin_ = 0;

  bool eof_ = false;
  bool error_ = false;
  // is_first_ should always be left false, except between xxxStart and xxxNext.
  bool is_first_ = false;
};

// Copied from json_util.h.  That is in the crust component, so we can't depend
// on it from here.
struct JsonParseOptions {
  // Whether to ignore unknown JSON fields during parsing
  bool ignore_unknown_fields;

  // If true, when a lowercase enum value fails to parse, try convert it to
  // UPPER_CASE and see if it matches a valid enum.
  // WARNING: This option exists only to preserve legacy behavior. Avoid using
  // this option. If your enum needs to support different casing, consider using
  // allow_alias instead.
  bool case_insensitive_enum_parsing;

  JsonParseOptions()
      : ignore_unknown_fields(false), case_insensitive_enum_parsing(false) {}
};

bool JsonToBinaryStream(const Descriptor* descriptor,
                        io::ZeroCopyInputStream* json_input,
                        io::ZeroCopyOutputStream* binary_output,
                        io::ErrorCollector* error_collector,
                        const JsonParseOptions& options,
                        const DescriptorPool* any_pool);

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#undef CHK

#endif  // GOOGLE_PROTOBUF_JSON_PARSER_INTERNAL_H__
