// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/text_format_decode_data.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "google/protobuf/io/coded_stream.h"
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

  bool AddCharacter(char desired, char input);
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

bool DecodeDataBuilder::AddCharacter(char desired, char input) {
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

void TextFormatDecodeData::AddString(int32_t key,
                                     const std::string& input_for_decode,
                                     const std::string& desired_output) {
  for (std::vector<DataEntry>::const_iterator i = entries_.begin();
       i != entries_.end(); ++i) {
    ABSL_CHECK(i->first != key)
        << "error: duplicate key (" << key
        << ") making TextFormat data, input: \"" << input_for_decode
        << "\", desired: \"" << desired_output << "\".";
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
  ABSL_CHECK(!input_for_decode.empty() && !desired_output.empty())
      << "error: got empty string for making TextFormat data, input: \""
      << input_for_decode << "\", desired: \"" << desired_output << "\".";
  ABSL_CHECK(!absl::StrContains(input_for_decode, '\0') &&
             !absl::StrContains(desired_output, '\0'))
      << "error: got a null char in a string for making TextFormat data,"
      << " input: \"" << absl::CEscape(input_for_decode) << "\", desired: \""
      << absl::CEscape(desired_output) << "\".";

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
