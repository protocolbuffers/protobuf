// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/io/strtod.h"

#include <float.h>  // FLT_DIG and DBL_DIG

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <system_error>  // NOLINT(build/c++11)

#include "absl/log/absl_check.h"
#include "absl/strings/charconv.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"

namespace google {
namespace protobuf {
namespace io {

// This approximately 0x1.ffffffp127, but we don't use 0x1.ffffffp127 because
// it won't compile in MSVC.
constexpr double MAX_FLOAT_AS_DOUBLE_ROUNDED = 3.4028235677973366e+38;

float SafeDoubleToFloat(double value) {
  // static_cast<float> on a number larger than float can result in illegal
  // instruction error, so we need to manually convert it to infinity or max.
  if (value > std::numeric_limits<float>::max()) {
    // Max float value is about 3.4028234664E38 when represented as a double.
    // However, when printing float as text, it will be rounded as
    // 3.4028235e+38. If we parse the value of 3.4028235e+38 from text and
    // compare it to 3.4028234664E38, we may think that it is larger, but
    // actually, any number between these two numbers could only be represented
    // as the same max float number in float, so we should treat them the same
    // as max float.
    if (value <= MAX_FLOAT_AS_DOUBLE_ROUNDED) {
      return std::numeric_limits<float>::max();
    }
    return std::numeric_limits<float>::infinity();
  } else if (value < -std::numeric_limits<float>::max()) {
    if (value >= -MAX_FLOAT_AS_DOUBLE_ROUNDED) {
      return -std::numeric_limits<float>::max();
    }
    return -std::numeric_limits<float>::infinity();
  } else {
    return static_cast<float>(value);
  }
}

double NoLocaleStrtod(const char *str, char **endptr) {
  double ret = 0.0;
  // This isn't ideal, but the existing function interface does not provide any
  // bounds.
  const char *end = strchr(str, 0);
  auto result = absl::from_chars(str, end, ret);
  // from_chars() with DR 3081's current wording will return max() on
  // overflow.  SimpleAtod returns infinity instead.
  if (result.ec == std::errc::result_out_of_range) {
    if (ret > 1.0) {
      ret = std::numeric_limits<double>::infinity();
    } else if (ret < -1.0) {
      ret = -std::numeric_limits<double>::infinity();
    }
  }
  if (endptr) {
    *endptr = const_cast<char *>(result.ptr);
  }
  return ret;
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

inline bool IsValidFloatChar(char c) {
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

bool safe_strtof(const char *str, float *value) {
  char *endptr;
  errno = 0;  // errno only gets set on errors
  *value = strtof(str, &endptr);
  return *str != 0 && *endptr == 0 && errno == 0;
}

char *FloatToBuffer(float value, char *buffer) {
  // FLT_DIG is 6 for IEEE-754 floats, which are used on almost all
  // platforms these days.  Just in case some system exists where FLT_DIG
  // is significantly larger -- and risks overflowing our buffer -- we have
  // this assert.
  static_assert(FLT_DIG < 10, "FLT_DIG_is_too_big");

  if (value == std::numeric_limits<double>::infinity()) {
    absl::SNPrintF(buffer, kFloatToBufferSize, "inf");
    return buffer;
  } else if (value == -std::numeric_limits<double>::infinity()) {
    absl::SNPrintF(buffer, kFloatToBufferSize, "-inf");
    return buffer;
  } else if (std::isnan(value)) {
    absl::SNPrintF(buffer, kFloatToBufferSize, "nan");
    return buffer;
  }

  int snprintf_result =
      absl::SNPrintF(buffer, kFloatToBufferSize, "%.*g", FLT_DIG, value);

  // The snprintf should never overflow because the buffer is significantly
  // larger than the precision we asked for.
  ABSL_DCHECK(snprintf_result > 0 && snprintf_result < kFloatToBufferSize);

  float parsed_value;
  if (!safe_strtof(buffer, &parsed_value) || parsed_value != value) {
    snprintf_result =
        absl::SNPrintF(buffer, kFloatToBufferSize, "%.*g", FLT_DIG + 3, value);

    // Should never overflow; see above.
    ABSL_DCHECK(snprintf_result > 0 && snprintf_result < kFloatToBufferSize);
  }

  DelocalizeRadix(buffer);
  return buffer;
}

char *DoubleToBuffer(double value, char *buffer) {
  // DBL_DIG is 15 for IEEE-754 doubles, which are used on almost all
  // platforms these days.  Just in case some system exists where DBL_DIG
  // is significantly larger -- and risks overflowing our buffer -- we have
  // this assert.
  static_assert(DBL_DIG < 20, "DBL_DIG_is_too_big");

  if (value == std::numeric_limits<double>::infinity()) {
    absl::SNPrintF(buffer, kDoubleToBufferSize, "inf");
    return buffer;
  } else if (value == -std::numeric_limits<double>::infinity()) {
    absl::SNPrintF(buffer, kDoubleToBufferSize, "-inf");
    return buffer;
  } else if (std::isnan(value)) {
    absl::SNPrintF(buffer, kDoubleToBufferSize, "nan");
    return buffer;
  }

  int snprintf_result =
      absl::SNPrintF(buffer, kDoubleToBufferSize, "%.*g", DBL_DIG, value);

  // The snprintf should never overflow because the buffer is significantly
  // larger than the precision we asked for.
  ABSL_DCHECK(snprintf_result > 0 && snprintf_result < kDoubleToBufferSize);

  // We need to make parsed_value volatile in order to force the compiler to
  // write it out to the stack.  Otherwise, it may keep the value in a
  // register, and if it does that, it may keep it as a long double instead
  // of a double.  This long double may have extra bits that make it compare
  // unequal to "value" even though it would be exactly equal if it were
  // truncated to a double.
  volatile double parsed_value = NoLocaleStrtod(buffer, nullptr);
  if (parsed_value != value) {
    snprintf_result =
        absl::SNPrintF(buffer, kDoubleToBufferSize, "%.*g", DBL_DIG + 2, value);

    // Should never overflow; see above.
    ABSL_DCHECK(snprintf_result > 0 && snprintf_result < kDoubleToBufferSize);
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

}  // namespace io
}  // namespace protobuf
}  // namespace google
