// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef THIRD_PARTY_UTF8_RANGE_UTF8_VALIDITY_H_
#define THIRD_PARTY_UTF8_RANGE_UTF8_VALIDITY_H_

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "absl/strings/string_view.h"
#include "utf8_range.h"

namespace utf8_range {

namespace internal {

inline uint64_t Load64(const char* p) {
  uint64_t t;
  memcpy(&t, p, sizeof t);
  return t;
}
inline uint32_t Load32(const char* p) {
  uint32_t t;
  memcpy(&t, p, sizeof t);
  return t;
}

constexpr size_t kInlineAsciiLimit = 32;

// Returns true if every one of the `len` bytes at `data` is ASCII, for
// `len` <= kInlineAsciiLimit. An all-ASCII buffer is by definition valid UTF-8.
inline bool AllAsciiShort(const char* data, size_t len) {
  constexpr uint64_t kHighBits = 0x8080808080808080ull;
  if (len >= 8) {
    // Test the leading window on its own first. Text that is not ASCII is
    // usually not ASCII from its very first codepoint, so this returns after a
    // single load for CJK,.
    if ((Load64(data) & kHighBits) != 0) {
      return false;
    }
    // Test the last three overlapping 8-byte windows.
    const size_t last = len - 8;
    const size_t o1 = last < 8 ? last : 8;
    const size_t o2 = last < 16 ? last : 16;
    const uint64_t u =
        Load64(data + o1) | Load64(data + o2) | Load64(data + last);
    return (u & kHighBits) == 0;
  }
  if (len >= 4) {
    const uint32_t u = Load32(data) | Load32(data + len - 4);
    return (u & 0x80808080u) == 0;
  }
  uint32_t u = 0;
  for (size_t i = 0; i < len; ++i) {
    u |= static_cast<unsigned char>(data[i]);
  }
  return (u & 0x80u) == 0;
}
}  // namespace internal

// Returns true if the sequence of characters is a valid UTF-8 sequence.
inline bool IsStructurallyValid(absl::string_view str) {
  // Fast path for short ASCII strings (<= 32 bytes).
  if (str.size() <= internal::kInlineAsciiLimit &&
      internal::AllAsciiShort(str.data(), str.size())) {
    return true;
  }
  return utf8_range_IsValid(str.data(), str.size());
}

// Returns the length in bytes of the prefix of str that is all
// structurally valid UTF-8.
inline size_t SpanStructurallyValid(absl::string_view str) {
  return utf8_range_ValidPrefix(str.data(), str.size());
}

}  // namespace utf8_range

#endif  // THIRD_PARTY_UTF8_RANGE_UTF8_VALIDITY_H_
