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
// SimpleDtoa()
// SimpleFtoa()
//    Description: converts a double or float to a string which, if
//    passed to NoLocaleStrtod(), will produce the exact same original double
//    (except in case of NaN; all NaNs are considered the same value).
//    We try to keep the string short but it's not guaranteed to be as
//    short as possible.
//
//    Return value: string
// ----------------------------------------------------------------------
PROTOBUF_EXPORT std::string SimpleDtoa(double value);
PROTOBUF_EXPORT std::string SimpleFtoa(float value);

namespace strings {

using Hex = absl::Hex;

}  // namespace strings

// ----------------------------------------------------------------------
// ToHex()
//    Return a lower-case hex string representation of the given integer.
// ----------------------------------------------------------------------
PROTOBUF_EXPORT std::string ToHex(uint64_t num);

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

namespace internal {

// A locale-independent version of the standard strtod(), which always
// uses a dot as the decimal separator.
double NoLocaleStrtod(const char* str, char** endptr);

}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_STUBS_STRUTIL_H__
