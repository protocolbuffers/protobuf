// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

/* This is a wrapper for the Google range-sse.cc algorithm which checks whether a
 * sequence of bytes is a valid UTF-8 sequence and finds the longest valid prefix of
 * the UTF-8 sequence.
 *
 * The key difference is that it checks for as much ASCII symbols as possible
 * and then falls back to the range-sse.cc algorithm. The changes to the
 * algorithm are cosmetic, mostly to trick the clang compiler to produce optimal
 * code.
 *
 * For API see the utf8_validity.h header.
 */

#include "utf8_validity.h"

#include <cstddef>

#include "absl/strings/string_view.h"
#include "utf8_range.h"

namespace utf8_range {

bool IsStructurallyValid(absl::string_view str) {
  return utf8_range_IsValid(str.data(), str.size());
}

size_t SpanStructurallyValid(absl::string_view str) {
  return utf8_range_ValidPrefix(str.data(), str.size());
}

}  // namespace utf8_range
