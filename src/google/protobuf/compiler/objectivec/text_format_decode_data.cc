// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "google/protobuf/compiler/objectivec/text_format_decode_data.h"

#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

// NOTE: src/google/protobuf/compiler/plugin.cc makes use of cerr for some
// error cases, so it seems to be ok to use as a back door for errors.

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

// Helper to build up the decode data for a string.
class DecodeDataBuilder {
 public:
  DecodeDataBuilder() { Reset(); }

  bool AddCharacter(const char desired, const char input);
  void AddUnderscore() {
    Push();
    need_underscore_ = true;
  }
  std::string Finish() {
    Push();
    return decode_data_;
  }

 private:
  static constexpr uint8_t kAddUnderscore = 0x80;

  static constexpr uint8_t kOpAsIs = 0x00;
  static constexpr uint8_t kOpFirstUpper = 0x40;
  static constexpr uint8_t kOpFirstLower = 0x20;
  static constexpr uint8_t kOpAllUpper = 0x60;

  static constexpr int kMaxSegmentLen = 0x1f;

  void AddChar(const char desired) {
    ++segment_len_;
    is_all_upper_ &= absl::ascii_isupper(desired);
  }

  void Push() {
    uint8_t op = (op_ | segment_len_);
    if (need_underscore_) op |= kAddUnderscore;
    if (op != 0) {
      decode_data_ += (char)op;
    }
    Reset();
  }

  bool AddFirst(const char desired, const char input) {
    if (desired == input) {
      op_ = kOpAsIs;
    } else if (desired == absl::ascii_toupper(input)) {
      op_ = kOpFirstUpper;
    } else if (desired == absl::ascii_tolower(input)) {
      op_ = kOpFirstLower;
    } else {
      // Can't be transformed to match.
      return false;
    }
    AddChar(desired);
    return true;
  }

  void Reset() {
    need_underscore_ = false;
    op_ = 0;
    segment_len_ = 0;
    is_all_upper_ = true;
  }

  bool need_underscore_;
  bool is_all_upper_;
  uint8_t op_;
  int segment_len_;

  std::string decode_data_;
};

bool DecodeDataBuilder::AddCharacter(const char desired, const char input) {
  // If we've hit the max size, push to start a new segment.
  if (segment_len_ == kMaxSegmentLen) {
    Push();
  }
  if (segment_len_ == 0) {
    return AddFirst(desired, input);
  }

  // Desired and input match...
  if (desired == input) {
    // If we aren't transforming it, or we're upper casing it and it is
    // supposed to be uppercase; just add it to the segment.
    if ((op_ != kOpAllUpper) || absl::ascii_isupper(desired)) {
      AddChar(desired);
      return true;
    }

    // Add the current segment, and start the next one.
    Push();
    return AddFirst(desired, input);
  }

  // If we need to uppercase, and everything so far has been uppercase,
  // promote op to AllUpper.
  if ((desired == absl::ascii_toupper(input)) && is_all_upper_) {
    op_ = kOpAllUpper;
    AddChar(desired);
    return true;
  }

  // Give up, push and start a new segment.
  Push();
  return AddFirst(desired, input);
}

// If decode data can't be generated, a directive for the raw string
// is used instead.
std::string DirectDecodeString(const std::string& str) {
  std::string result;
  result += (char)'\0';  // Marker for full string.
  result += str;
  result += (char)'\0';  // End of string.
  return result;
}

}  // namespace

TextFormatDecodeData::TextFormatDecodeData() {}

TextFormatDecodeData::~TextFormatDecodeData() {}

void TextFormatDecodeData::AddString(int32_t key,
                                     const std::string& input_for_decode,
                                     const std::string& desired_output) {
  for (std::vector<DataEntry>::const_iterator i = entries_.begin();
       i != entries_.end(); ++i) {
    if (i->first == key) {
      std::cerr << "error: duplicate key (" << key
                << ") making TextFormat data, input: \"" << input_for_decode
                << "\", desired: \"" << desired_output << "\"." << std::endl;
      std::cerr.flush();
      abort();
    }
  }

  const std::string& data = TextFormatDecodeData::DecodeDataForString(
      input_for_decode, desired_output);
  entries_.push_back(DataEntry(key, data));
}

std::string TextFormatDecodeData::Data() const {
  std::ostringstream data_stringstream;

  if (num_entries() > 0) {
    io::OstreamOutputStream data_outputstream(&data_stringstream);
    io::CodedOutputStream output_stream(&data_outputstream);

    output_stream.WriteVarint32(num_entries());
    for (std::vector<DataEntry>::const_iterator i = entries_.begin();
         i != entries_.end(); ++i) {
      output_stream.WriteVarint32(i->first);
      output_stream.WriteString(i->second);
    }
  }

  data_stringstream.flush();
  return data_stringstream.str();
}

// static
std::string TextFormatDecodeData::DecodeDataForString(
    const std::string& input_for_decode, const std::string& desired_output) {
  if (input_for_decode.empty() || desired_output.empty()) {
    std::cerr << "error: got empty string for making TextFormat data, input: \""
              << input_for_decode << "\", desired: \"" << desired_output
              << "\"." << std::endl;
    std::cerr.flush();
    abort();
  }
  if ((input_for_decode.find('\0') != std::string::npos) ||
      (desired_output.find('\0') != std::string::npos)) {
    std::cerr
        << "error: got a null char in a string for making TextFormat data,"
        << " input: \"" << absl::CEscape(input_for_decode) << "\", desired: \""
        << absl::CEscape(desired_output) << "\"." << std::endl;
    std::cerr.flush();
    abort();
  }

  DecodeDataBuilder builder;

  // Walk the output building it from the input.
  int x = 0;
  for (int y = 0; y < desired_output.size(); y++) {
    const char d = desired_output[y];
    if (d == '_') {
      builder.AddUnderscore();
      continue;
    }

    if (x >= input_for_decode.size()) {
      // Out of input, no way to encode it, just return a full decode.
      return DirectDecodeString(desired_output);
    }
    if (builder.AddCharacter(d, input_for_decode[x])) {
      ++x;  // Consumed one input
    } else {
      // Couldn't transform for the next character, just return a full decode.
      return DirectDecodeString(desired_output);
    }
  }

  if (x != input_for_decode.size()) {
    // Extra input (suffix from name sanitizing?), just return a full decode.
    return DirectDecodeString(desired_output);
  }

  // Add the end marker.
  return builder.Finish() + (char)'\0';
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
