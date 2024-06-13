// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/json/internal/writer.h"

#include <cstdint>
#include <initializer_list>
#include <limits>
#include <utility>

#include "absl/algorithm/container.h"
#include "absl/log/absl_check.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace json_internal {

// Tries to write a non-finite double if necessary; returns false if
// nothing was written.
bool JsonWriter::MaybeWriteSpecialFp(double val) {
  if (val == std::numeric_limits<double>::infinity()) {
    Write("\"Infinity\"");
  } else if (val == -std::numeric_limits<double>::infinity()) {
    Write("\"-Infinity\"");
  } else if (std::isnan(val)) {
    Write("\"NaN\"");
  } else {
    return false;
  }
  return true;
}

void JsonWriter::WriteBase64(absl::string_view str) {
  // This is the regular base64, not the "web-safe" version.
  constexpr absl::string_view kBase64 =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const char* ptr = str.data();
  const char* end = ptr + str.size();

  // Reads the `n`th character off of `ptr` while gracefully avoiding
  // sign extension due to implicit conversions
  auto read = [&](size_t n) {
    return static_cast<size_t>(static_cast<uint8_t>(ptr[n]));
  };

  char buf[4];
  absl::string_view view(buf, sizeof(buf));
  Write("\"");

  while (end - ptr >= 3) {
    buf[0] = kBase64[read(0) >> 2];
    buf[1] = kBase64[((read(0) & 0x3) << 4) | (read(1) >> 4)];
    buf[2] = kBase64[((read(1) & 0xf) << 2) | (read(2) >> 6)];
    buf[3] = kBase64[read(2) & 0x3f];
    Write(view);
    ptr += 3;
  }

  switch (end - ptr) {
    case 2:
      buf[0] = kBase64[read(0) >> 2];
      buf[1] = kBase64[((read(0) & 0x3) << 4) | (read(1) >> 4)];
      buf[2] = kBase64[(read(1) & 0xf) << 2];
      buf[3] = '=';
      Write(view);
      break;
    case 1:
      buf[0] = kBase64[read(0) >> 2];
      buf[1] = kBase64[((read(0) & 0x3) << 4)];
      buf[2] = '=';
      buf[3] = '=';
      Write(view);
      break;
  }

  Write("\"");
}

// The minimum value of a unicode high-surrogate code unit in the utf-16
// encoding. A high-surrogate is also known as a leading-surrogate.
// See http://www.unicode.org/glossary/#high_surrogate_code_unit
static constexpr uint16_t kMinHighSurrogate = 0xd800;

// The minimum value of a unicode low-surrogate code unit in the utf-16
// encoding. A low-surrogate is also known as a trailing-surrogate.
// See http://www.unicode.org/glossary/#low_surrogate_code_unit
static constexpr uint16_t kMinLowSurrogate = 0xdc00;

// The maximum value of a unicode low-surrogate code unit in the utf-16
// encoding. A low-surrogate is also known as a trailing surrogate.
// See http://www.unicode.org/glossary/#low_surrogate_code_unit
static constexpr uint16_t kMaxLowSurrogate = 0xdfff;

// The minimum value of a unicode supplementary code point.
// See http://www.unicode.org/glossary/#supplementary_code_point
static constexpr uint32_t kMinSupplementaryCodePoint = 0x010000;

// The maximum value of a unicode code point.
// See http://www.unicode.org/glossary/#code_point
static constexpr uint32_t kMaxCodePoint = 0x10ffff;

// Indicates decoding failure; not a valid Unicode scalar.
static constexpr uint32_t kErrorSentinel = 0xaaaaaaaa;

// A Unicode Scalar encoded two ways.
struct Utf8Scalar {
  // The Unicode scalar value as a 32-bit integer. If decoding failed, this
  // is equal to kErrorSentinel.
  uint32_t u32;
  // The Unicode scalar value encoded as UTF-8 bytes. May not reflect the
  // contents of `u32` if it is kErrorSentinel.
  absl::string_view utf8;
};

// Parses a single UTF-8-encoded Unicode scalar from `str`. Returns a pair of
// the scalar and the UTF-8-encoded content corresponding to it from `str`.
//
// Returns U+FFFD on failure, and consumes an unspecified number of bytes in
// doing so.
static Utf8Scalar ConsumeUtf8Scalar(absl::string_view& str) {
  ABSL_DCHECK(!str.empty());
  uint32_t scalar = static_cast<uint8_t>(str[0]);
  const char* start = str.data();
  size_t len = 1;

  str = str.substr(1);

  // Verify this is valid UTF-8. UTF-8 is a varint encoding satisfying
  // one of the following (big-endian) patterns:
  //
  // 0b0xxxxxxx
  // 0b110xxxxx'10xxxxxx
  // 0b1110xxxx'10xxxxxx'10xxxxxx
  // 0b11110xxx'10xxxxxx'10xxxxxx'10xxxxxx
  //
  // We don't need to decode it; just validate it.
  int lookahead = 0;
  switch (absl::countl_one(static_cast<uint8_t>(scalar))) {
    case 0:
      break;
    case 2:
      lookahead = 1;
      scalar &= (1 << 5) - 1;
      break;
    case 3:
      lookahead = 2;
      scalar &= (1 << 4) - 1;
      break;
    case 4:
      lookahead = 3;
      scalar &= (1 << 3) - 1;
      break;
    default:
      scalar = kErrorSentinel;
      break;
  }

  for (int i = 0; i < lookahead; ++i) {
    if (str.empty()) {
      scalar = kErrorSentinel;
      break;
    }

    uint8_t next = str[0];
    str = str.substr(1);
    ++len;

    // Looking for top 2 bits are 0b10.
    if (next >> 6 != 2) {
      scalar = kErrorSentinel;
      break;
    }
    next &= (1 << 6) - 1;
    scalar <<= 6;
    scalar |= next;
  }

  if (scalar > kMaxCodePoint) {
    scalar = kErrorSentinel;
  }

  return {scalar, absl::string_view(start, len)};
}

// Decides whether we must escape `scalar`.
//
// If the given Unicode scalar would not use a \u escape, `custom_escape` will
// be set to a non-empty string.
static bool MustEscape(uint32_t scalar, absl::string_view& custom_escape) {
  switch (scalar) {
    // These escapes are defined by the JSON spec. We do not escape /.
    case '\n':
      custom_escape = R"(\n)";
      return true;
    case '\r':
      custom_escape = R"(\r)";
      return true;
    case '\t':
      custom_escape = R"(\t)";
      return true;
    case '\"':
      custom_escape = R"(\")";
      return true;
    case '\f':
      custom_escape = R"(\f)";
      return true;
    case '\b':
      custom_escape = R"(\b)";
      return true;
    case '\\':
      custom_escape = R"(\\)";
      return true;

    case kErrorSentinel:
      // Decoding failure turns into spaces, *not* replacement characters. We
      // handle this separately from "normal" spaces so that it follows the
      // escaping code-path.
      //
      // Note that literal replacement characters in the input string DO NOT
      // get turned into spaces; this is only for decoding failures!
      custom_escape = " ";
      return true;

    // These are not required by the JSON spec, but help
    // to prevent security bugs in JavaScript.
    //
    // These were originally present in the ESF parser, so they are kept for
    // legacy compatibility (and because escaping most of these is in good
    // taste, regardless).
    case '<':
    case '>':
    case 0xfeff:      // Zero width no-break space.
    case 0xfff9:      // Interlinear annotation anchor.
    case 0xfffa:      // Interlinear annotation separator.
    case 0xfffb:      // Interlinear annotation terminator.
    case 0x00ad:      // Soft-hyphen.
    case 0x06dd:      // Arabic end of ayah.
    case 0x070f:      // Syriac abbreviation mark.
    case 0x17b4:      // Khmer vowel inherent Aq.
    case 0x17b5:      // Khmer vowel inherent Aa.
    case 0x000e0001:  // Language tag.
      return true;
    default:
      static constexpr std::pair<uint32_t, uint32_t> kEscapedRanges[] = {
          {0x0000, 0x001f},          // ASCII control.
          {0x007f, 0x009f},          // High ASCII bytes.
          {0x0600, 0x0603},          // Arabic signs.
          {0x200b, 0x200f},          // Zero width etc.
          {0x2028, 0x202e},          // Separators etc.
          {0x2060, 0x2064},          // Invisible etc.
          {0x206a, 0x206f},          // Shaping etc.
          {0x0001d173, 0x0001d17a},  // Music formatting.
          {0x000e0020, 0x000e007f},  // TAG symbols.
      };

      return absl::c_any_of(kEscapedRanges, [scalar](auto range) {
        return range.first <= scalar && scalar <= range.second;
      });
  }
}

void JsonWriter::WriteEscapedUtf8(absl::string_view str) {
  while (!str.empty()) {
    auto scalar = ConsumeUtf8Scalar(str);
    absl::string_view custom_escape;

    if (!MustEscape(scalar.u32, custom_escape)) {
      Write(scalar.utf8);
      continue;
    }

    if (!custom_escape.empty()) {
      Write(custom_escape);
      continue;
    }

    if (scalar.u32 < 0x10000) {
      WriteUEscape(scalar.u32);
      continue;
    }

    uint16_t lo =
        (scalar.u32 & (kMaxLowSurrogate - kMinLowSurrogate)) + kMinLowSurrogate;
    uint16_t hi = (scalar.u32 >> 10) +
                  (kMinHighSurrogate - (kMinSupplementaryCodePoint >> 10));
    WriteUEscape(hi);
    WriteUEscape(lo);
  }
}

void JsonWriter::WriteUEscape(uint16_t val) {
  char hex[7];
  int len = absl::SNPrintF(hex, sizeof(hex), R"(\u%04x)", val);
  Write(absl::string_view(hex, static_cast<size_t>(len)));
}
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google
