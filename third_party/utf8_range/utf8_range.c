// Copyright 2023 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

/* This is a wrapper for the Google range-sse.cc algorithm which checks whether
 * a sequence of bytes is a valid UTF-8 sequence and finds the longest valid
 * prefix of the UTF-8 sequence.
 *
 * The key difference is that it checks for as much ASCII symbols as possible
 * and then falls back to the range-sse.cc algorithm. The changes to the
 * algorithm are cosmetic, mostly to trick the clang compiler to produce optimal
 * code.
 *
 * For API see the utf8_validity.h header.
 */
#include "utf8_range.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(__GNUC__)
#define FORCE_INLINE_ATTR __attribute__((always_inline))
#elif defined(_MSC_VER)
#define FORCE_INLINE_ATTR __forceinline
#else
#define FORCE_INLINE_ATTR
#endif

static FORCE_INLINE_ATTR inline uint64_t utf8_range_UnalignedLoad64(
    const void* p) {
  uint64_t t;
  memcpy(&t, p, sizeof t);
  return t;
}

static FORCE_INLINE_ATTR inline int utf8_range_AsciiIsAscii(unsigned char c) {
  return c < 128;
}

static FORCE_INLINE_ATTR inline int utf8_range_IsTrailByteOk(const char c) {
  return (int8_t)(c) <= (int8_t)(0xBF);
}

/* If return_position is false then it returns 1 if |data| is a valid utf8
 * sequence, otherwise returns 0.
 * If return_position is set to true, returns the length in bytes of the prefix
   of |data| that is all structurally valid UTF-8.
 */
static size_t utf8_range_ValidateUTF8Naive(const char* data, const char* end,
                                           int return_position) {
  /* We return err_pos in the loop which is always 0 if !return_position */
  size_t err_pos = 0;
  size_t codepoint_bytes = 0;
  /* The early check is done because of early continue's on codepoints of all
   * sizes, i.e. we first check for ascii and if it is, we call continue, then
   * for 2 byte codepoints, etc. This is done in order to reduce indentation and
   * improve readability of the codepoint validity check.
   */
  while (data + codepoint_bytes < end) {
    if (return_position) {
      err_pos += codepoint_bytes;
    }
    data += codepoint_bytes;
    const size_t len = end - data;
    const unsigned char byte1 = data[0];

    /* We do not skip many ascii bytes at the same time as this function is
       used for tail checking (< 16 bytes) and for non x86 platforms. We also
       don't think that cases where non-ASCII codepoints are followed by ascii
       happen often. For small strings it also introduces some penalty. For
       purely ascii UTF8 strings (which is the overwhelming case) we call
       SkipAscii function which is multiplatform and extremely fast.
     */
    /* [00..7F] ASCII -> 1 byte */
    if (utf8_range_AsciiIsAscii(byte1)) {
      codepoint_bytes = 1;
      continue;
    }
    /* [C2..DF], [80..BF] -> 2 bytes */
    if (len >= 2 && byte1 >= 0xC2 && byte1 <= 0xDF &&
        utf8_range_IsTrailByteOk(data[1])) {
      codepoint_bytes = 2;
      continue;
    }
    if (len >= 3) {
      const unsigned char byte2 = data[1];
      const unsigned char byte3 = data[2];

      /* Is byte2, byte3 between [0x80, 0xBF]
       * Check for 0x80 was done above.
       */
      if (!utf8_range_IsTrailByteOk(byte2) ||
          !utf8_range_IsTrailByteOk(byte3)) {
        return err_pos;
      }

      if (/* E0, A0..BF, 80..BF */
          ((byte1 == 0xE0 && byte2 >= 0xA0) ||
           /* E1..EC, 80..BF, 80..BF */
           (byte1 >= 0xE1 && byte1 <= 0xEC) ||
           /* ED, 80..9F, 80..BF */
           (byte1 == 0xED && byte2 <= 0x9F) ||
           /* EE..EF, 80..BF, 80..BF */
           (byte1 >= 0xEE && byte1 <= 0xEF))) {
        codepoint_bytes = 3;
        continue;
      }
      if (len >= 4) {
        const unsigned char byte4 = data[3];
        /* Is byte4 between 0x80 ~ 0xBF */
        if (!utf8_range_IsTrailByteOk(byte4)) {
          return err_pos;
        }

        if (/* F0, 90..BF, 80..BF, 80..BF */
            ((byte1 == 0xF0 && byte2 >= 0x90) ||
             /* F1..F3, 80..BF, 80..BF, 80..BF */
             (byte1 >= 0xF1 && byte1 <= 0xF3) ||
             /* F4, 80..8F, 80..BF, 80..BF */
             (byte1 == 0xF4 && byte2 <= 0x8F))) {
          codepoint_bytes = 4;
          continue;
        }
      }
    }
    return err_pos;
  }
  if (return_position) {
    err_pos += codepoint_bytes;
  }
  /* if return_position is false, this returns 1.
   * if return_position is true, this returns err_pos.
   */
  return err_pos + (1 - return_position);
}

#if defined(__SSE4_1__) || (defined(__ARM_NEON) && defined(__ARM_64BIT_STATE))
/* Returns the number of bytes needed to skip backwards to get to the first
   byte of codepoint.
 */
static inline int utf8_range_CodepointSkipBackwards(int32_t codepoint_word) {
  const int8_t* const codepoint = (const int8_t*)(&codepoint_word);
  if (!utf8_range_IsTrailByteOk(codepoint[3])) {
    return 1;
  } else if (!utf8_range_IsTrailByteOk(codepoint[2])) {
    return 2;
  } else if (!utf8_range_IsTrailByteOk(codepoint[1])) {
    return 3;
  }
  return 0;
}
#endif  // __SSE4_1__

/* Skipping over ASCII as much as possible, per 8 bytes. It is intentional
   as most strings to check for validity consist only of 1 byte codepoints.
 */
static inline const char* utf8_range_SkipAscii(const char* data,
                                               const char* end) {
  while (8 <= end - data &&
         (utf8_range_UnalignedLoad64(data) & 0x8080808080808080) == 0) {
    data += 8;
  }
  while (data < end && utf8_range_AsciiIsAscii(*data)) {
    ++data;
  }
  return data;
}

#if defined(__SSE4_1__)
#include "utf8_range_sse.inc"
#elif defined(__ARM_NEON) && defined(__ARM_64BIT_STATE)
#include "utf8_range_neon.inc"
#endif

static FORCE_INLINE_ATTR inline size_t utf8_range_Validate(
    const char* data, size_t len, int return_position) {
  if (len == 0) return 1 - return_position;
  // Save buffer start address for later use
  const char* const data_original = data;
  const char* const end = data + len;
  data = utf8_range_SkipAscii(data, end);
  /* SIMD algorithm always outperforms the naive version for any data of
     length >=16.
   */
  if (end - data < 16) {
    return (return_position ? (data - data_original) : 0) +
           utf8_range_ValidateUTF8Naive(data, end, return_position);
  }
#if defined(__SSE4_1__) || (defined(__ARM_NEON) && defined(__ARM_64BIT_STATE))
  return utf8_range_ValidateUTF8Simd(
      data_original, data, end, return_position);
#else
  return (return_position ? (data - data_original) : 0) +
         utf8_range_ValidateUTF8Naive(data, end, return_position);
#endif
}

int utf8_range_IsValid(const char* data, size_t len) {
  return utf8_range_Validate(data, len, /*return_position=*/0) != 0;
}

size_t utf8_range_ValidPrefix(const char* data, size_t len) {
  return utf8_range_Validate(data, len, /*return_position=*/1);
}
