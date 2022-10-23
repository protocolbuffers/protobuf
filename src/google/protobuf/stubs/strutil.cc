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

// from google3/strings/strutil.cc

#include "google/protobuf/stubs/strutil.h"

#include <errno.h>
#include <float.h>  // FLT_DIG and DBL_DIG
#include <limits.h>
#include <stdio.h>

#include <cmath>
#include <iterator>
#include <limits>

#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/stubs/logging.h"

#ifdef _WIN32
// MSVC has only _snprintf, not snprintf.
//
// MinGW has both snprintf and _snprintf, but they appear to be different
// functions.  The former is buggy.  When invoked like so:
//   char buffer[32];
//   snprintf(buffer, 32, "%.*g\n", FLT_DIG, 1.23e10f);
// it prints "1.23000e+10".  This is plainly wrong:  %g should never print
// trailing zeros after the decimal point.  For some reason this bug only
// occurs with some input values, not all.  In any case, _snprintf does the
// right thing, so we use it.
#define snprintf _snprintf
#endif

namespace google {
namespace protobuf {

namespace {
void StringReplace(const std::string &s, const std::string &oldsub,
                   const std::string &newsub, bool replace_all,
                   std::string *res) {
  if (oldsub.empty()) {
    res->append(s);  // if empty, append the given string.
    return;
  }

  std::string::size_type start_pos = 0;
  std::string::size_type pos;
  do {
    pos = s.find(oldsub, start_pos);
    if (pos == std::string::npos) {
      break;
    }
    res->append(s, start_pos, pos - start_pos);
    res->append(newsub);
    start_pos = pos + oldsub.size();  // start searching again after the "old"
  } while (replace_all);
  res->append(s, start_pos, s.length() - start_pos);
}
}  // namespace

// ----------------------------------------------------------------------
// StringReplace()
//    Give me a string and two patterns "old" and "new", and I replace
//    the first instance of "old" in the string with "new", if it
//    exists.  If "global" is true; call this repeatedly until it
//    fails.  RETURN a new string, regardless of whether the replacement
//    happened or not.
// ----------------------------------------------------------------------

std::string StringReplace(const std::string &s, const std::string &oldsub,
                          const std::string &newsub, bool replace_all) {
  std::string ret;
  StringReplace(s, oldsub, newsub, replace_all, &ret);
  return ret;
}

// ----------------------------------------------------------------------
// strto32_adaptor()
// strtou32_adaptor()
//    Implementation of strto[u]l replacements that have identical
//    overflow and underflow characteristics for both ILP-32 and LP-64
//    platforms, including errno preservation in error-free calls.
// ----------------------------------------------------------------------

int32_t strto32_adaptor(const char *nptr, char **endptr, int base) {
  const int saved_errno = errno;
  errno = 0;
  const long result = strtol(nptr, endptr, base);
  if (errno == ERANGE && result == LONG_MIN) {
    return std::numeric_limits<int32_t>::min();
  } else if (errno == ERANGE && result == LONG_MAX) {
    return std::numeric_limits<int32_t>::max();
  } else if (errno == 0 && result < std::numeric_limits<int32_t>::min()) {
    errno = ERANGE;
    return std::numeric_limits<int32_t>::min();
  } else if (errno == 0 && result > std::numeric_limits<int32_t>::max()) {
    errno = ERANGE;
    return std::numeric_limits<int32_t>::max();
  }
  if (errno == 0)
    errno = saved_errno;
  return static_cast<int32_t>(result);
}

uint32_t strtou32_adaptor(const char *nptr, char **endptr, int base) {
  const int saved_errno = errno;
  errno = 0;
  const unsigned long result = strtoul(nptr, endptr, base);
  if (errno == ERANGE && result == ULONG_MAX) {
    return std::numeric_limits<uint32_t>::max();
  } else if (errno == 0 && result > std::numeric_limits<uint32_t>::max()) {
    errno = ERANGE;
    return std::numeric_limits<uint32_t>::max();
  }
  if (errno == 0)
    errno = saved_errno;
  return static_cast<uint32_t>(result);
}

inline bool safe_parse_sign(std::string *text /*inout*/,
                            bool *negative_ptr /*output*/) {
  const char* start = text->data();
  const char* end = start + text->size();

  // Consume whitespace.
  while (start < end && (start[0] == ' ')) {
    ++start;
  }
  while (start < end && (end[-1] == ' ')) {
    --end;
  }
  if (start >= end) {
    return false;
  }

  // Consume sign.
  *negative_ptr = (start[0] == '-');
  if (*negative_ptr || start[0] == '+') {
    ++start;
    if (start >= end) {
      return false;
    }
  }
  *text = text->substr(start - text->data(), end - start);
  return true;
}

template <typename IntType>
bool safe_parse_positive_int(std::string text, IntType *value_p) {
  int base = 10;
  IntType value = 0;
  const IntType vmax = std::numeric_limits<IntType>::max();
  assert(vmax > 0);
  assert(vmax >= base);
  const IntType vmax_over_base = vmax / base;
  const char* start = text.data();
  const char* end = start + text.size();
  // loop over digits
  for (; start < end; ++start) {
    unsigned char c = static_cast<unsigned char>(start[0]);
    int digit = c - '0';
    if (digit >= base || digit < 0) {
      *value_p = value;
      return false;
    }
    if (value > vmax_over_base) {
      *value_p = vmax;
      return false;
    }
    value *= base;
    if (value > vmax - digit) {
      *value_p = vmax;
      return false;
    }
    value += digit;
  }
  *value_p = value;
  return true;
}

template <typename IntType>
bool safe_parse_negative_int(const std::string &text, IntType *value_p) {
  int base = 10;
  IntType value = 0;
  const IntType vmin = std::numeric_limits<IntType>::min();
  assert(vmin < 0);
  assert(vmin <= 0 - base);
  IntType vmin_over_base = vmin / base;
  // 2003 c++ standard [expr.mul]
  // "... the sign of the remainder is implementation-defined."
  // Although (vmin/base)*base + vmin%base is always vmin.
  // 2011 c++ standard tightens the spec but we cannot rely on it.
  if (vmin % base > 0) {
    vmin_over_base += 1;
  }
  const char* start = text.data();
  const char* end = start + text.size();
  // loop over digits
  for (; start < end; ++start) {
    unsigned char c = static_cast<unsigned char>(start[0]);
    int digit = c - '0';
    if (digit >= base || digit < 0) {
      *value_p = value;
      return false;
    }
    if (value < vmin_over_base) {
      *value_p = vmin;
      return false;
    }
    value *= base;
    if (value < vmin + digit) {
      *value_p = vmin;
      return false;
    }
    value -= digit;
  }
  *value_p = value;
  return true;
}

template <typename IntType>
bool safe_int_internal(std::string text, IntType *value_p) {
  *value_p = 0;
  bool negative;
  if (!safe_parse_sign(&text, &negative)) {
    return false;
  }
  if (!negative) {
    return safe_parse_positive_int(text, value_p);
  } else {
    return safe_parse_negative_int(text, value_p);
  }
}

template <typename IntType>
bool safe_uint_internal(std::string text, IntType *value_p) {
  *value_p = 0;
  bool negative;
  if (!safe_parse_sign(&text, &negative) || negative) {
    return false;
  }
  return safe_parse_positive_int(text, value_p);
}

// ----------------------------------------------------------------------
// SimpleDtoa()
// SimpleFtoa()
//    We want to print the value without losing precision, but we also do
//    not want to print more digits than necessary.  This turns out to be
//    trickier than it sounds.  Numbers like 0.2 cannot be represented
//    exactly in binary.  If we print 0.2 with a very large precision,
//    e.g. "%.50g", we get "0.2000000000000000111022302462515654042363167".
//    On the other hand, if we set the precision too low, we lose
//    significant digits when printing numbers that actually need them.
//    It turns out there is no precision value that does the right thing
//    for all numbers.
//
//    Our strategy is to first try printing with a precision that is never
//    over-precise, then parse the result with strtod() to see if it
//    matches.  If not, we print again with a precision that will always
//    give a precise result, but may use more digits than necessary.
//
//    An arguably better strategy would be to use the algorithm described
//    in "How to Print Floating-Point Numbers Accurately" by Steele &
//    White, e.g. as implemented by David M. Gay's dtoa().  It turns out,
//    however, that the following implementation is about as fast as
//    DMG's code.  Furthermore, DMG's code locks mutexes, which means it
//    will not scale well on multi-core machines.  DMG's code is slightly
//    more accurate (in that it will never use more digits than
//    necessary), but this is probably irrelevant for most users.
//
//    Rob Pike and Ken Thompson also have an implementation of dtoa() in
//    third_party/fmt/fltfmt.cc.  Their implementation is similar to this
//    one in that it makes guesses and then uses strtod() to check them.
//    Their implementation is faster because they use their own code to
//    generate the digits in the first place rather than use snprintf(),
//    thus avoiding format string parsing overhead.  However, this makes
//    it considerably more complicated than the following implementation,
//    and it is embedded in a larger library.  If speed turns out to be
//    an issue, we could re-implement this in terms of their
//    implementation.
// ----------------------------------------------------------------------

namespace {
// In practice, doubles should never need more than 24 bytes and floats
// should never need more than 14 (including null terminators), but we
// overestimate to be safe.
constexpr int kDoubleToBufferSize = 32;
constexpr int kFloatToBufferSize = 24;

static inline bool IsValidFloatChar(char c) {
  return ('0' <= c && c <= '9') || c == 'e' || c == 'E' || c == '+' || c == '-';
}

void DelocalizeRadix(char *buffer) {
  // Fast check:  if the buffer has a normal decimal point, assume no
  // translation is needed.
  if (strchr(buffer, '.') != nullptr) return;

  // Find the first unknown character.
  while (IsValidFloatChar(*buffer)) ++buffer;

  if (*buffer == '\0') {
    // No radix character found.
    return;
  }

  // We are now pointing at the locale-specific radix character.  Replace it
  // with '.'.
  *buffer = '.';
  ++buffer;

  if (!IsValidFloatChar(*buffer) && *buffer != '\0') {
    // It appears the radix was a multi-byte character.  We need to remove the
    // extra bytes.
    char *target = buffer;
    do {
      ++buffer;
    } while (!IsValidFloatChar(*buffer) && *buffer != '\0');
    memmove(target, buffer, strlen(buffer) + 1);
  }
}

char *FloatToBuffer(float value, char *buffer) {
  // FLT_DIG is 6 for IEEE-754 floats, which are used on almost all
  // platforms these days.  Just in case some system exists where FLT_DIG
  // is significantly larger -- and risks overflowing our buffer -- we have
  // this assert.
  static_assert(FLT_DIG < 10, "FLT_DIG_is_too_big");

  if (value == std::numeric_limits<double>::infinity()) {
    strcpy(buffer, "inf");
    return buffer;
  } else if (value == -std::numeric_limits<double>::infinity()) {
    strcpy(buffer, "-inf");
    return buffer;
  } else if (std::isnan(value)) {
    strcpy(buffer, "nan");
    return buffer;
  }

  int snprintf_result =
      snprintf(buffer, kFloatToBufferSize, "%.*g", FLT_DIG, value);

  // The snprintf should never overflow because the buffer is significantly
  // larger than the precision we asked for.
  GOOGLE_DCHECK(snprintf_result > 0 && snprintf_result < kFloatToBufferSize);

  float parsed_value;
  if (!safe_strtof(buffer, &parsed_value) || parsed_value != value) {
    snprintf_result =
        snprintf(buffer, kFloatToBufferSize, "%.*g", FLT_DIG + 3, value);

    // Should never overflow; see above.
    GOOGLE_DCHECK(snprintf_result > 0 && snprintf_result < kFloatToBufferSize);
  }

  DelocalizeRadix(buffer);
  return buffer;
}

char* DoubleToBuffer(double value, char* buffer) {
  // DBL_DIG is 15 for IEEE-754 doubles, which are used on almost all
  // platforms these days.  Just in case some system exists where DBL_DIG
  // is significantly larger -- and risks overflowing our buffer -- we have
  // this assert.
  static_assert(DBL_DIG < 20, "DBL_DIG_is_too_big");

  if (value == std::numeric_limits<double>::infinity()) {
    strcpy(buffer, "inf");
    return buffer;
  } else if (value == -std::numeric_limits<double>::infinity()) {
    strcpy(buffer, "-inf");
    return buffer;
  } else if (std::isnan(value)) {
    strcpy(buffer, "nan");
    return buffer;
  }

  int snprintf_result =
    snprintf(buffer, kDoubleToBufferSize, "%.*g", DBL_DIG, value);

  // The snprintf should never overflow because the buffer is significantly
  // larger than the precision we asked for.
  GOOGLE_DCHECK(snprintf_result > 0 && snprintf_result < kDoubleToBufferSize);

  // We need to make parsed_value volatile in order to force the compiler to
  // write it out to the stack.  Otherwise, it may keep the value in a
  // register, and if it does that, it may keep it as a long double instead
  // of a double.  This long double may have extra bits that make it compare
  // unequal to "value" even though it would be exactly equal if it were
  // truncated to a double.
  volatile double parsed_value = internal::NoLocaleStrtod(buffer, nullptr);
  if (parsed_value != value) {
    snprintf_result =
        snprintf(buffer, kDoubleToBufferSize, "%.*g", DBL_DIG + 2, value);

    // Should never overflow; see above.
    GOOGLE_DCHECK(snprintf_result > 0 && snprintf_result < kDoubleToBufferSize);
  }

  DelocalizeRadix(buffer);
  return buffer;
}
}  // namespace

std::string SimpleDtoa(double value) {
  char buffer[kDoubleToBufferSize];
  return DoubleToBuffer(value, buffer);
}

std::string SimpleFtoa(float value) {
  char buffer[kFloatToBufferSize];
  return FloatToBuffer(value, buffer);
}

static int memcasecmp(const char *s1, const char *s2, size_t len) {
  const unsigned char *us1 = reinterpret_cast<const unsigned char *>(s1);
  const unsigned char *us2 = reinterpret_cast<const unsigned char *>(s2);

  for (size_t i = 0; i < len; i++) {
    const int diff =
        static_cast<int>(
            static_cast<unsigned char>(absl::ascii_tolower(us1[i]))) -
        static_cast<int>(
            static_cast<unsigned char>(absl::ascii_tolower(us2[i])));
    if (diff != 0) return diff;
  }
  return 0;
}

inline bool CaseEqual(absl::string_view s1, absl::string_view s2) {
  if (s1.size() != s2.size()) return false;
  return memcasecmp(s1.data(), s2.data(), s1.size()) == 0;
}

bool safe_strtob(absl::string_view str, bool *value) {
  GOOGLE_CHECK(value != nullptr) << "nullptr output boolean given.";
  if (CaseEqual(str, "true") || CaseEqual(str, "t") ||
      CaseEqual(str, "yes") || CaseEqual(str, "y") ||
      CaseEqual(str, "1")) {
    *value = true;
    return true;
  }
  if (CaseEqual(str, "false") || CaseEqual(str, "f") ||
      CaseEqual(str, "no") || CaseEqual(str, "n") ||
      CaseEqual(str, "0")) {
    *value = false;
    return true;
  }
  return false;
}

bool safe_strtof(const char* str, float* value) {
  char* endptr;
  errno = 0;  // errno only gets set on errors
#if defined(_WIN32) || defined (__hpux)  // has no strtof()
  *value = internal::NoLocaleStrtod(str, &endptr);
#else
  *value = strtof(str, &endptr);
#endif
  return *str != 0 && *endptr == 0 && errno == 0;
}

bool safe_strtod(const char* str, double* value) {
  char* endptr;
  *value = internal::NoLocaleStrtod(str, &endptr);
  if (endptr != str) {
    while (absl::ascii_isspace(*endptr)) ++endptr;
  }
  // Ignore range errors from strtod.  The values it
  // returns on underflow and overflow are the right
  // fallback in a robust setting.
  return *str != '\0' && *endptr == '\0';
}

bool safe_strto32(const std::string &str, int32_t *value) {
  return safe_int_internal(str, value);
}

bool safe_strtou32(const std::string &str, uint32_t *value) {
  return safe_uint_internal(str, value);
}

bool safe_strto64(const std::string &str, int64_t *value) {
  return safe_int_internal(str, value);
}

bool safe_strtou64(const std::string &str, uint64_t *value) {
  return safe_uint_internal(str, value);
}

namespace {
int CalculateBase64EscapedLen(int input_len, bool do_padding) {
  // Base64 encodes three bytes of input at a time. If the input is not
  // divisible by three, we pad as appropriate.
  //
  // (from http://tools.ietf.org/html/rfc3548)
  // Special processing is performed if fewer than 24 bits are available
  // at the end of the data being encoded.  A full encoding quantum is
  // always completed at the end of a quantity.  When fewer than 24 input
  // bits are available in an input group, zero bits are added (on the
  // right) to form an integral number of 6-bit groups.  Padding at the
  // end of the data is performed using the '=' character.  Since all base
  // 64 input is an integral number of octets, only the following cases
  // can arise:

  // Base64 encodes each three bytes of input into four bytes of output.
  int len = (input_len / 3) * 4;

  if (input_len % 3 == 0) {
    // (from http://tools.ietf.org/html/rfc3548)
    // (1) the final quantum of encoding input is an integral multiple of 24
    // bits; here, the final unit of encoded output will be an integral
    // multiple of 4 characters with no "=" padding,
  } else if (input_len % 3 == 1) {
    // (from http://tools.ietf.org/html/rfc3548)
    // (2) the final quantum of encoding input is exactly 8 bits; here, the
    // final unit of encoded output will be two characters followed by two
    // "=" padding characters, or
    len += 2;
    if (do_padding) {
      len += 2;
    }
  } else {  // (input_len % 3 == 2)
    // (from http://tools.ietf.org/html/rfc3548)
    // (3) the final quantum of encoding input is exactly 16 bits; here, the
    // final unit of encoded output will be three characters followed by one
    // "=" padding character.
    len += 3;
    if (do_padding) {
      len += 1;
    }
  }

  assert(len >= input_len);  // make sure we didn't overflow
  return len;
}

int Base64EscapeInternal(const unsigned char *src, int szsrc, char *dest,
                         int szdest, const absl::string_view base64,
                         bool do_padding) {
  static const char kPad64 = '=';

  if (szsrc <= 0) return 0;

  if (szsrc * 4 > szdest * 3) return 0;

  char *cur_dest = dest;
  const unsigned char *cur_src = src;

  char *limit_dest = dest + szdest;
  const unsigned char *limit_src = src + szsrc;

  // Three bytes of data encodes to four characters of ciphertext.
  // So we can pump through three-byte chunks atomically.
  while (cur_src < limit_src - 3) {  // keep going as long as we have >= 32 bits
    uint32_t in = BigEndian::Load32(cur_src) >> 8;

    cur_dest[0] = base64[in >> 18];
    in &= 0x3FFFF;
    cur_dest[1] = base64[in >> 12];
    in &= 0xFFF;
    cur_dest[2] = base64[in >> 6];
    in &= 0x3F;
    cur_dest[3] = base64[in];

    cur_dest += 4;
    cur_src += 3;
  }
  // To save time, we didn't update szdest or szsrc in the loop.  So do it now.
  szdest = limit_dest - cur_dest;
  szsrc = limit_src - cur_src;

  /* now deal with the tail (<=3 bytes) */
  switch (szsrc) {
    case 0:
      // Nothing left; nothing more to do.
      break;
    case 1: {
      // One byte left: this encodes to two characters, and (optionally)
      // two pad characters to round out the four-character cipherblock.
      if ((szdest -= 2) < 0) return 0;
      uint32_t in = cur_src[0];
      cur_dest[0] = base64[in >> 2];
      in &= 0x3;
      cur_dest[1] = base64[in << 4];
      cur_dest += 2;
      if (do_padding) {
        if ((szdest -= 2) < 0) return 0;
        cur_dest[0] = kPad64;
        cur_dest[1] = kPad64;
        cur_dest += 2;
      }
      break;
    }
    case 2: {
      // Two bytes left: this encodes to three characters, and (optionally)
      // one pad character to round out the four-character cipherblock.
      if ((szdest -= 3) < 0) return 0;
      uint32_t in = BigEndian::Load16(cur_src);
      cur_dest[0] = base64[in >> 10];
      in &= 0x3FF;
      cur_dest[1] = base64[in >> 4];
      in &= 0x00F;
      cur_dest[2] = base64[in << 2];
      cur_dest += 3;
      if (do_padding) {
        if ((szdest -= 1) < 0) return 0;
        cur_dest[0] = kPad64;
        cur_dest += 1;
      }
      break;
    }
    case 3: {
      // Three bytes left: same as in the big loop above.  We can't do this in
      // the loop because the loop above always reads 4 bytes, and the fourth
      // byte is past the end of the input.
      if ((szdest -= 4) < 0) return 0;
      uint32_t in = (cur_src[0] << 16) + BigEndian::Load16(cur_src + 1);
      cur_dest[0] = base64[in >> 18];
      in &= 0x3FFFF;
      cur_dest[1] = base64[in >> 12];
      in &= 0xFFF;
      cur_dest[2] = base64[in >> 6];
      in &= 0x3F;
      cur_dest[3] = base64[in];
      cur_dest += 4;
      break;
    }
    default:
      // Should not be reached: blocks of 4 bytes are handled
      // in the while loop before this switch statement.
      GOOGLE_LOG(FATAL) << "Logic problem? szsrc = " << szsrc;
      break;
  }
  return (cur_dest - dest);
}

void Base64EscapeInternal(const unsigned char *src, int szsrc,
                          std::string *dest, bool do_padding,
                          const absl::string_view base64_chars) {
  const int calc_escaped_size = CalculateBase64EscapedLen(szsrc, do_padding);
  dest->resize(calc_escaped_size);
  const int escaped_len = Base64EscapeInternal(
      src, szsrc, &(*dest)[0], dest->size(), base64_chars, do_padding);
  GOOGLE_DCHECK_EQ(calc_escaped_size, escaped_len);
  dest->erase(escaped_len);
}

static constexpr absl::string_view kBase64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static constexpr absl::string_view kWebSafeBase64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

}  // namespace

namespace strings {

void LegacyBase64EscapeWithoutPadding(absl::string_view src,
                                      std::string *dest) {
  Base64EscapeInternal(reinterpret_cast<const unsigned char *>(src.data()),
                       src.size(), dest, /*do_padding=*/false, kBase64Chars);
}

void WebSafeBase64EscapeWithPadding(absl::string_view src, std::string *dest) {
  Base64EscapeInternal(reinterpret_cast<const unsigned char *>(src.data()),
                       src.size(), dest,
                       /*do_padding=*/true, kWebSafeBase64Chars);
}

}  // namespace strings

// Helper to append a Unicode code point to a string as UTF8, without bringing
// in any external dependencies.
int EncodeAsUTF8Char(uint32_t code_point, char* output) {
  uint32_t tmp = 0;
  int len = 0;
  if (code_point <= 0x7f) {
    tmp = code_point;
    len = 1;
  } else if (code_point <= 0x07ff) {
    tmp = 0x0000c080 |
        ((code_point & 0x07c0) << 2) |
        (code_point & 0x003f);
    len = 2;
  } else if (code_point <= 0xffff) {
    tmp = 0x00e08080 |
        ((code_point & 0xf000) << 4) |
        ((code_point & 0x0fc0) << 2) |
        (code_point & 0x003f);
    len = 3;
  } else {
    // UTF-16 is only defined for code points up to 0x10FFFF, and UTF-8 is
    // normally only defined up to there as well.
    tmp = 0xf0808080 |
        ((code_point & 0x1c0000) << 6) |
        ((code_point & 0x03f000) << 4) |
        ((code_point & 0x000fc0) << 2) |
        (code_point & 0x003f);
    len = 4;
  }
  tmp = ghtonl(tmp);
  memcpy(output, reinterpret_cast<const char*>(&tmp) + sizeof(tmp) - len, len);
  return len;
}

// Table of UTF-8 character lengths, based on first byte
static const unsigned char kUTF8LenTbl[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

// Return length of a single UTF-8 source character
int UTF8FirstLetterNumBytes(const char* src, int len) {
  if (len == 0) {
    return 0;
  }
  return kUTF8LenTbl[*reinterpret_cast<const uint8_t*>(src)];
}

namespace internal {

// ----------------------------------------------------------------------
// NoLocaleStrtod()
//   This code will make you cry.
// ----------------------------------------------------------------------

namespace {

// Returns a string identical to *input except that the character pointed to
// by radix_pos (which should be '.') is replaced with the locale-specific
// radix character.
std::string LocalizeRadix(const char *input, const char *radix_pos) {
  // Determine the locale-specific radix character by calling sprintf() to
  // print the number 1.5, then stripping off the digits.  As far as I can
  // tell, this is the only portable, thread-safe way to get the C library
  // to divuldge the locale's radix character.  No, localeconv() is NOT
  // thread-safe.
  char temp[16];
  int size = snprintf(temp, sizeof(temp), "%.1f", 1.5);
  GOOGLE_CHECK_EQ(temp[0], '1');
  GOOGLE_CHECK_EQ(temp[size - 1], '5');
  GOOGLE_CHECK_LE(size, 6);

  // Now replace the '.' in the input with it.
  std::string result;
  result.reserve(strlen(input) + size - 3);
  result.append(input, radix_pos);
  result.append(temp + 1, size - 2);
  result.append(radix_pos + 1);
  return result;
}

}  // namespace

double NoLocaleStrtod(const char *str, char **endptr) {
  // We cannot simply set the locale to "C" temporarily with setlocale()
  // as this is not thread-safe.  Instead, we try to parse in the current
  // locale first.  If parsing stops at a '.' character, then this is a
  // pretty good hint that we're actually in some other locale in which
  // '.' is not the radix character.

  char *temp_endptr;
  double result = strtod(str, &temp_endptr);
  if (endptr != NULL) *endptr = temp_endptr;
  if (*temp_endptr != '.') return result;

  // Parsing halted on a '.'.  Perhaps we're in a different locale?  Let's
  // try to replace the '.' with a locale-specific radix character and
  // try again.
  std::string localized = LocalizeRadix(str, temp_endptr);
  const char *localized_cstr = localized.c_str();
  char *localized_endptr;
  result = strtod(localized_cstr, &localized_endptr);
  if ((localized_endptr - localized_cstr) > (temp_endptr - str)) {
    // This attempt got further, so replacing the decimal must have helped.
    // Update endptr to point at the right location.
    if (endptr != NULL) {
      // size_diff is non-zero if the localized radix has multiple bytes.
      int size_diff = localized.size() - strlen(str);
      // const_cast is necessary to match the strtod() interface.
      *endptr = const_cast<char *>(
          str + (localized_endptr - localized_cstr - size_diff));
    }
  }

  return result;
}

}  // namespace internal

}  // namespace protobuf
}  // namespace google
