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

/* The SSE4.1 code is used directly when the compiler targets SSE4.1. Other x86
 * builds (such as the default -march=x86-64, or MSVC x64) compile it anyway
 * and pick it at run time with CPUID, so portable binaries can use it too.
 */
#if defined(__SSE4_1__) || defined(__AVX__)
#define UTF8_RANGE_SSE4_1 1
#elif (defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || \
       defined(_M_IX86)) &&                                           \
    !defined(_M_ARM64EC) &&                                           \
    (defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER))
#define UTF8_RANGE_SSE4_1 1
#define UTF8_RANGE_SSE4_1_DISPATCH 1
#endif

#if defined(UTF8_RANGE_SSE4_1_DISPATCH) && \
    (defined(__GNUC__) || defined(__clang__))
#define UTF8_RANGE_SSE4_1_TARGET __attribute__((target("ssse3,sse4.1")))
#define UTF8_RANGE_NOINLINE __attribute__((noinline))
#else
#define UTF8_RANGE_SSE4_1_TARGET
#define UTF8_RANGE_NOINLINE
#endif

#if defined(__GNUC__)
#define FORCE_INLINE_ATTR __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define FORCE_INLINE_ATTR __forceinline
#else
#define FORCE_INLINE_ATTR inline
#endif

static FORCE_INLINE_ATTR uint64_t utf8_range_UnalignedLoad64(
    const void* p) {
  uint64_t t;
  memcpy(&t, p, sizeof t);
  return t;
}

static FORCE_INLINE_ATTR int utf8_range_AsciiIsAscii(unsigned char c) {
  return c < 128;
}

static FORCE_INLINE_ATTR int utf8_range_IsTrailByteOk(const char c) {
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

#if defined(UTF8_RANGE_SSE4_1) || \
    (defined(__ARM_NEON) && defined(__ARM_64BIT_STATE))
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

#if defined(UTF8_RANGE_SSE4_1)
#include "utf8_range_sse.inc"
#elif defined(__ARM_NEON) && defined(__ARM_64BIT_STATE)
#include "utf8_range_neon.inc"
#endif

#if defined(UTF8_RANGE_SSE4_1_DISPATCH)
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif

/* 0: not checked yet, 1: no SSE4.1, 2: SSE4.1. Threads that race on the first
 * call all store the same value.
 */
static int utf8_range_sse4_1_state;

/* Not inlined: CPUID overwrites EBX, and inlining it would make every call
 * save and restore RBX, ASCII-only calls included.
 */
static UTF8_RANGE_NOINLINE int utf8_range_DetectSse4_1(void) {
  unsigned int ecx;
#if defined(_MSC_VER)
  int regs[4];
  __cpuid(regs, 1);
  ecx = (unsigned int)regs[2];
#else
  unsigned int eax, ebx, edx;
  if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) return 1;
#endif
  /* SSSE3 is bit 9 and SSE4.1 is bit 19 of ECX. */
  return (ecx & (1u << 9)) && (ecx & (1u << 19)) ? 2 : 1;
}

static inline int utf8_range_HasSse4_1(void) {
#if defined(__GNUC__) || defined(__clang__)
  int state = __atomic_load_n(&utf8_range_sse4_1_state, __ATOMIC_RELAXED);
  if (state == 0) {
    state = utf8_range_DetectSse4_1();
    __atomic_store_n(&utf8_range_sse4_1_state, state, __ATOMIC_RELAXED);
  }
#else
  int state = *(volatile int*)&utf8_range_sse4_1_state;
  if (state == 0) {
    state = utf8_range_DetectSse4_1();
    *(volatile int*)&utf8_range_sse4_1_state = state;
  }
#endif
  return state == 2;
}

/* Built for SSE4.1 even when its callers are not, so it can't be inlined into
 * them. Each branch inlines the SIMD loop with a constant return_position.
 */
static UTF8_RANGE_SSE4_1_TARGET UTF8_RANGE_NOINLINE size_t
utf8_range_ValidateUTF8SimdOutOfLine(const char* data_original,
                                     const char* data, const char* end,
                                     int return_position) {
  return return_position
             ? utf8_range_ValidateUTF8Simd(data_original, data, end, 1)
             : utf8_range_ValidateUTF8Simd(data_original, data, end, 0);
}
#endif  // UTF8_RANGE_SSE4_1_DISPATCH

static FORCE_INLINE_ATTR size_t utf8_range_Validate(
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
#if defined(UTF8_RANGE_SSE4_1_DISPATCH)
  if (utf8_range_HasSse4_1()) {
    return utf8_range_ValidateUTF8SimdOutOfLine(data_original, data, end,
                                                return_position);
  }
#elif defined(UTF8_RANGE_SSE4_1) || \
    (defined(__ARM_NEON) && defined(__ARM_64BIT_STATE))
  return utf8_range_ValidateUTF8Simd(
      data_original, data, end, return_position);
#endif
  return (return_position ? (data - data_original) : 0) +
         utf8_range_ValidateUTF8Naive(data, end, return_position);
}

bool utf8_range_IsValid(const char* data, size_t len) {
  return utf8_range_Validate(data, len, /*return_position=*/0) != 0;
}

size_t utf8_range_ValidPrefix(const char* data, size_t len) {
  return utf8_range_Validate(data, len, /*return_position=*/1);
}
