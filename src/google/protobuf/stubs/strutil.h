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

// from google3/strings/strutil.h

#ifndef GOOGLE_PROTOBUF_STUBS_STRUTIL_H__
#define GOOGLE_PROTOBUF_STUBS_STRUTIL_H__

#include <stdlib.h>

#include <cstring>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/stubs/common.h"

// Must be last.
#include "google/protobuf/port_def.inc"  // NOLINT

namespace google {
namespace protobuf {

#if defined(_MSC_VER) && _MSC_VER < 1800
#define strtoll  _strtoi64
#define strtoull _strtoui64
#elif defined(__DECCXX) && defined(__osf__)
// HP C++ on Tru64 does not have strtoll, but strtol is already 64-bit.
#define strtoll strtol
#define strtoull strtoul
#endif

inline int hex_digit_to_int(char c) {
  /* Assume ASCII. */
  int x = static_cast<unsigned char>(c);
  if (x > '9') {
    x += 9;
  }
  return x & 0xf;
}

// ----------------------------------------------------------------------
// HasPrefixString()
//    Check if a string begins with a given prefix.
// StripPrefixString()
//    Given a string and a putative prefix, returns the string minus the
//    prefix string if the prefix matches, otherwise the original
//    string.
// ----------------------------------------------------------------------
inline bool HasPrefixString(absl::string_view str, absl::string_view prefix) {
  return str.size() >= prefix.size() &&
         memcmp(str.data(), prefix.data(), prefix.size()) == 0;
}

inline std::string StripPrefixString(const std::string& str,
                                     const std::string& prefix) {
  if (HasPrefixString(str, prefix)) {
    return str.substr(prefix.size());
  } else {
    return str;
  }
}

// ----------------------------------------------------------------------
// HasSuffixString()
//    Return true if str ends in suffix.
// StripSuffixString()
//    Given a string and a putative suffix, returns the string minus the
//    suffix string if the suffix matches, otherwise the original
//    string.
// ----------------------------------------------------------------------
inline bool HasSuffixString(absl::string_view str, absl::string_view suffix) {
  return str.size() >= suffix.size() &&
         memcmp(str.data() + str.size() - suffix.size(), suffix.data(),
                suffix.size()) == 0;
}

inline std::string StripSuffixString(const std::string& str,
                                     const std::string& suffix) {
  if (HasSuffixString(str, suffix)) {
    return str.substr(0, str.size() - suffix.size());
  } else {
    return str;
  }
}

// ----------------------------------------------------------------------
// ReplaceCharacters
//    Replaces any occurrence of the character 'remove' (or the characters
//    in 'remove') with the character 'replacewith'.
//    Good for keeping html characters or protocol characters (\t) out
//    of places where they might cause a problem.
// StripWhitespace
//    Removes whitespaces from both ends of the given string.
// ----------------------------------------------------------------------
PROTOBUF_EXPORT void ReplaceCharacters(std::string* s, const char* remove,
                                       char replacewith);

PROTOBUF_EXPORT void StripWhitespace(std::string* s);

// ----------------------------------------------------------------------
// LowerString()
// UpperString()
// ToUpper()
//    Convert the characters in "s" to lowercase or uppercase.  ASCII-only:
//    these functions intentionally ignore locale because they are applied to
//    identifiers used in the Protocol Buffer language, not to natural-language
//    strings.
// ----------------------------------------------------------------------

inline void LowerString(std::string* s) {
  std::string::iterator end = s->end();
  for (std::string::iterator i = s->begin(); i != end; ++i) {
    // tolower() changes based on locale.  We don't want this!
    if ('A' <= *i && *i <= 'Z') *i += 'a' - 'A';
  }
}

inline void UpperString(std::string* s) {
  std::string::iterator end = s->end();
  for (std::string::iterator i = s->begin(); i != end; ++i) {
    // toupper() changes based on locale.  We don't want this!
    if ('a' <= *i && *i <= 'z') *i += 'A' - 'a';
  }
}

inline void ToUpper(std::string* s) { UpperString(s); }

inline std::string ToUpper(const std::string& s) {
  std::string out = s;
  UpperString(&out);
  return out;
}

// ----------------------------------------------------------------------
// StringReplace()
//    Give me a string and two patterns "old" and "new", and I replace
//    the first instance of "old" in the string with "new", if it
//    exists.  RETURN a new string, regardless of whether the replacement
//    happened or not.
// ----------------------------------------------------------------------

PROTOBUF_EXPORT std::string StringReplace(const std::string& s,
                                          const std::string& oldsub,
                                          const std::string& newsub,
                                          bool replace_all);

// ----------------------------------------------------------------------
// strto32()
// strtou32()
// strto64()
// strtou64()
//    Architecture-neutral plug compatible replacements for strtol() and
//    strtoul().  Long's have different lengths on ILP-32 and LP-64
//    platforms, so using these is safer, from the point of view of
//    overflow behavior, than using the standard libc functions.
// ----------------------------------------------------------------------
PROTOBUF_EXPORT int32_t strto32_adaptor(const char* nptr, char** endptr,
                                        int base);
PROTOBUF_EXPORT uint32_t strtou32_adaptor(const char* nptr, char** endptr,
                                          int base);

inline int32_t strto32(const char *nptr, char **endptr, int base) {
  if (sizeof(int32_t) == sizeof(long))
    return strtol(nptr, endptr, base);
  else
    return strto32_adaptor(nptr, endptr, base);
}

inline uint32_t strtou32(const char *nptr, char **endptr, int base) {
  if (sizeof(uint32_t) == sizeof(unsigned long))
    return strtoul(nptr, endptr, base);
  else
    return strtou32_adaptor(nptr, endptr, base);
}

// For now, long long is 64-bit on all the platforms we care about, so these
// functions can simply pass the call to strto[u]ll.
inline int64_t strto64(const char *nptr, char **endptr, int base) {
  static_assert(sizeof(int64_t) == sizeof(long long),
                "sizeof int64_t is not sizeof long long");
  return strtoll(nptr, endptr, base);
}

inline uint64_t strtou64(const char *nptr, char **endptr, int base) {
  static_assert(sizeof(uint64_t) == sizeof(unsigned long long),
                "sizeof uint64_t is not sizeof unsigned long long");
  return strtoull(nptr, endptr, base);
}

// ----------------------------------------------------------------------
// safe_strtob()
// safe_strto32()
// safe_strtou32()
// safe_strto64()
// safe_strtou64()
// safe_strtof()
// safe_strtod()
// ----------------------------------------------------------------------
PROTOBUF_EXPORT bool safe_strtob(absl::string_view str, bool* value);

PROTOBUF_EXPORT bool safe_strto32(const std::string& str, int32_t* value);
PROTOBUF_EXPORT bool safe_strtou32(const std::string& str, uint32_t* value);
inline bool safe_strto32(const char* str, int32_t* value) {
  return safe_strto32(std::string(str), value);
}
inline bool safe_strto32(absl::string_view str, int32_t* value) {
  return safe_strto32(std::string(str), value);
}
inline bool safe_strtou32(const char* str, uint32_t* value) {
  return safe_strtou32(std::string(str), value);
}
inline bool safe_strtou32(absl::string_view str, uint32_t* value) {
  return safe_strtou32(std::string(str), value);
}

PROTOBUF_EXPORT bool safe_strto64(const std::string& str, int64_t* value);
PROTOBUF_EXPORT bool safe_strtou64(const std::string& str, uint64_t* value);
inline bool safe_strto64(const char* str, int64_t* value) {
  return safe_strto64(std::string(str), value);
}
inline bool safe_strto64(absl::string_view str, int64_t* value) {
  return safe_strto64(std::string(str), value);
}
inline bool safe_strtou64(const char* str, uint64_t* value) {
  return safe_strtou64(std::string(str), value);
}
inline bool safe_strtou64(absl::string_view str, uint64_t* value) {
  return safe_strtou64(std::string(str), value);
}

PROTOBUF_EXPORT bool safe_strtof(const char* str, float* value);
PROTOBUF_EXPORT bool safe_strtod(const char* str, double* value);
inline bool safe_strtof(const std::string& str, float* value) {
  return safe_strtof(str.c_str(), value);
}
inline bool safe_strtod(const std::string& str, double* value) {
  return safe_strtod(str.c_str(), value);
}
inline bool safe_strtof(absl::string_view str, float* value) {
  return safe_strtof(std::string(str), value);
}
inline bool safe_strtod(absl::string_view str, double* value) {
  return safe_strtod(std::string(str), value);
}

// ----------------------------------------------------------------------
// FastIntToBuffer()
// FastHexToBuffer()
// FastHex64ToBuffer()
// FastHex32ToBuffer()
// FastTimeToBuffer()
//    These are intended for speed.  FastIntToBuffer() assumes the
//    integer is non-negative.  FastHexToBuffer() puts output in
//    hex rather than decimal.  FastTimeToBuffer() puts the output
//    into RFC822 format.
//
//    FastHex64ToBuffer() puts a 64-bit unsigned value in hex-format,
//    padded to exactly 16 bytes (plus one byte for '\0')
//
//    FastHex32ToBuffer() puts a 32-bit unsigned value in hex-format,
//    padded to exactly 8 bytes (plus one byte for '\0')
//
//       All functions take the output buffer as an arg.
//    They all return a pointer to the beginning of the output,
//    which may not be the beginning of the input buffer.
// ----------------------------------------------------------------------

// Suggested buffer size for FastToBuffer functions.  Also works with
// DoubleToBuffer() and FloatToBuffer().
static const int kFastToBufferSize = 32;

PROTOBUF_EXPORT char* FastInt32ToBuffer(int32_t i, char* buffer);
PROTOBUF_EXPORT char* FastInt64ToBuffer(int64_t i, char* buffer);
char* FastUInt32ToBuffer(uint32_t i, char* buffer);  // inline below
char* FastUInt64ToBuffer(uint64_t i, char* buffer);  // inline below
PROTOBUF_EXPORT char* FastHexToBuffer(int i, char* buffer);
PROTOBUF_EXPORT char* FastHex64ToBuffer(uint64_t i, char* buffer);
PROTOBUF_EXPORT char* FastHex32ToBuffer(uint32_t i, char* buffer);

// at least 22 bytes long
inline char* FastIntToBuffer(int i, char* buffer) {
  return (sizeof(i) == 4 ?
          FastInt32ToBuffer(i, buffer) : FastInt64ToBuffer(i, buffer));
}
inline char* FastUIntToBuffer(unsigned int i, char* buffer) {
  return (sizeof(i) == 4 ?
          FastUInt32ToBuffer(i, buffer) : FastUInt64ToBuffer(i, buffer));
}
inline char* FastLongToBuffer(long i, char* buffer) {
  return (sizeof(i) == 4 ?
          FastInt32ToBuffer(i, buffer) : FastInt64ToBuffer(i, buffer));
}
inline char* FastULongToBuffer(unsigned long i, char* buffer) {
  return (sizeof(i) == 4 ?
          FastUInt32ToBuffer(i, buffer) : FastUInt64ToBuffer(i, buffer));
}

// ----------------------------------------------------------------------
// FastInt32ToBufferLeft()
// FastUInt32ToBufferLeft()
// FastInt64ToBufferLeft()
// FastUInt64ToBufferLeft()
//
// Like the Fast*ToBuffer() functions above, these are intended for speed.
// Unlike the Fast*ToBuffer() functions, however, these functions write
// their output to the beginning of the buffer (hence the name, as the
// output is left-aligned).  The caller is responsible for ensuring that
// the buffer has enough space to hold the output.
//
// Returns a pointer to the end of the string (i.e. the null character
// terminating the string).
// ----------------------------------------------------------------------

PROTOBUF_EXPORT char* FastInt32ToBufferLeft(int32_t i, char* buffer);
PROTOBUF_EXPORT char* FastUInt32ToBufferLeft(uint32_t i, char* buffer);
PROTOBUF_EXPORT char* FastInt64ToBufferLeft(int64_t i, char* buffer);
PROTOBUF_EXPORT char* FastUInt64ToBufferLeft(uint64_t i, char* buffer);

// Just define these in terms of the above.
inline char* FastUInt32ToBuffer(uint32_t i, char* buffer) {
  FastUInt32ToBufferLeft(i, buffer);
  return buffer;
}
inline char* FastUInt64ToBuffer(uint64_t i, char* buffer) {
  FastUInt64ToBufferLeft(i, buffer);
  return buffer;
}

inline std::string SimpleBtoa(bool value) { return value ? "true" : "false"; }

// ----------------------------------------------------------------------
// SimpleItoa()
//    Description: converts an integer to a string.
//
//    Return value: string
// ----------------------------------------------------------------------
PROTOBUF_EXPORT std::string SimpleItoa(int i);
PROTOBUF_EXPORT std::string SimpleItoa(unsigned int i);
PROTOBUF_EXPORT std::string SimpleItoa(long i);
PROTOBUF_EXPORT std::string SimpleItoa(unsigned long i);
PROTOBUF_EXPORT std::string SimpleItoa(long long i);
PROTOBUF_EXPORT std::string SimpleItoa(unsigned long long i);

// ----------------------------------------------------------------------
// SimpleDtoa()
// SimpleFtoa()
// DoubleToBuffer()
// FloatToBuffer()
//    Description: converts a double or float to a string which, if
//    passed to NoLocaleStrtod(), will produce the exact same original double
//    (except in case of NaN; all NaNs are considered the same value).
//    We try to keep the string short but it's not guaranteed to be as
//    short as possible.
//
//    DoubleToBuffer() and FloatToBuffer() write the text to the given
//    buffer and return it.  The buffer must be at least
//    kDoubleToBufferSize bytes for doubles and kFloatToBufferSize
//    bytes for floats.  kFastToBufferSize is also guaranteed to be large
//    enough to hold either.
//
//    Return value: string
// ----------------------------------------------------------------------
PROTOBUF_EXPORT std::string SimpleDtoa(double value);
PROTOBUF_EXPORT std::string SimpleFtoa(float value);

PROTOBUF_EXPORT char* DoubleToBuffer(double i, char* buffer);
PROTOBUF_EXPORT char* FloatToBuffer(float i, char* buffer);

// In practice, doubles should never need more than 24 bytes and floats
// should never need more than 14 (including null terminators), but we
// overestimate to be safe.
static const int kDoubleToBufferSize = 32;
static const int kFloatToBufferSize = 24;

namespace strings {

using Hex = absl::Hex;

}  // namespace strings

// ----------------------------------------------------------------------
// ToHex()
//    Return a lower-case hex string representation of the given integer.
// ----------------------------------------------------------------------
PROTOBUF_EXPORT std::string ToHex(uint64_t num);

// ----------------------------------------------------------------------
// GlobalReplaceSubstring()
//    Replaces all instances of a substring in a string.  Does nothing
//    if 'substring' is empty.  Returns the number of replacements.
//
//    NOTE: The string pieces must not overlap s.
// ----------------------------------------------------------------------
PROTOBUF_EXPORT int GlobalReplaceSubstring(const std::string& substring,
                                           const std::string& replacement,
                                           std::string* s);

namespace strings {
// Encode src into dest web-safely with padding.
PROTOBUF_EXPORT void WebSafeBase64EscapeWithPadding(absl::string_view src,
                                                    std::string* dest);
PROTOBUF_EXPORT void LegacyBase64EscapeWithoutPadding(absl::string_view src,
                                                      std::string* dest);
}  // namespace strings

inline bool IsValidCodePoint(uint32_t code_point) {
  return code_point < 0xD800 ||
         (code_point >= 0xE000 && code_point <= 0x10FFFF);
}

static const int UTFmax = 4;
// ----------------------------------------------------------------------
// EncodeAsUTF8Char()
//  Helper to append a Unicode code point to a string as UTF8, without bringing
//  in any external dependencies. The output buffer must be as least 4 bytes
//  large.
// ----------------------------------------------------------------------
PROTOBUF_EXPORT int EncodeAsUTF8Char(uint32_t code_point, char* output);

// ----------------------------------------------------------------------
// UTF8FirstLetterNumBytes()
//   Length of the first UTF-8 character.
// ----------------------------------------------------------------------
PROTOBUF_EXPORT int UTF8FirstLetterNumBytes(const char* src, int len);

namespace strings {
inline bool EndsWith(absl::string_view text, absl::string_view suffix) {
  return suffix.empty() ||
      (text.size() >= suffix.size() &&
       memcmp(text.data() + (text.size() - suffix.size()), suffix.data(),
              suffix.size()) == 0);
}
}  // namespace strings

namespace internal {

// A locale-independent version of the standard strtod(), which always
// uses a dot as the decimal separator.
double NoLocaleStrtod(const char* str, char** endptr);

}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_STUBS_STRUTIL_H__
