// Adapted from https://github.com/lemire/fastvalidate-utf-8

#ifdef __AVX2__

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>

/*
 * legal utf-8 byte sequence
 * http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94
 *
 *  Code Points        1st       2s       3s       4s
 * U+0000..U+007F     00..7F
 * U+0080..U+07FF     C2..DF   80..BF
 * U+0800..U+0FFF     E0       A0..BF   80..BF
 * U+1000..U+CFFF     E1..EC   80..BF   80..BF
 * U+D000..U+D7FF     ED       80..9F   80..BF
 * U+E000..U+FFFF     EE..EF   80..BF   80..BF
 * U+10000..U+3FFFF   F0       90..BF   80..BF   80..BF
 * U+40000..U+FFFFF   F1..F3   80..BF   80..BF   80..BF
 * U+100000..U+10FFFF F4       80..8F   80..BF   80..BF
 *
 */

#if 0
static void print256(const char *s, const __m256i v256)
{
  const unsigned char *v8 = (const unsigned char *)&v256;
  if (s)
    printf("%s:\t", s);
  for (int i = 0; i < 32; i++)
    printf("%02x ", v8[i]);
  printf("\n");
}
#endif

static inline __m256i push_last_byte_of_a_to_b(__m256i a, __m256i b) {
  return _mm256_alignr_epi8(b, _mm256_permute2x128_si256(a, b, 0x21), 15);
}

static inline __m256i push_last_2bytes_of_a_to_b(__m256i a, __m256i b) {
  return _mm256_alignr_epi8(b, _mm256_permute2x128_si256(a, b, 0x21), 14);
}

// all byte values must be no larger than 0xF4
static inline void avxcheckSmallerThan0xF4(__m256i current_bytes,
                                           __m256i *has_error) {
  // unsigned, saturates to 0 below max
  *has_error = _mm256_or_si256(
      *has_error, _mm256_subs_epu8(current_bytes, _mm256_set1_epi8(0xF4)));
}

static inline __m256i avxcontinuationLengths(__m256i high_nibbles) {
  return _mm256_shuffle_epi8(
      _mm256_setr_epi8(1, 1, 1, 1, 1, 1, 1, 1, // 0xxx (ASCII)
                       0, 0, 0, 0,             // 10xx (continuation)
                       2, 2,                   // 110x
                       3,                      // 1110
                       4, // 1111, next should be 0 (not checked here)
                       1, 1, 1, 1, 1, 1, 1, 1, // 0xxx (ASCII)
                       0, 0, 0, 0,             // 10xx (continuation)
                       2, 2,                   // 110x
                       3,                      // 1110
                       4 // 1111, next should be 0 (not checked here)
                       ),
      high_nibbles);
}

static inline __m256i avxcarryContinuations(__m256i initial_lengths,
                                            __m256i previous_carries) {

  __m256i right1 = _mm256_subs_epu8(
      push_last_byte_of_a_to_b(previous_carries, initial_lengths),
      _mm256_set1_epi8(1));
  __m256i sum = _mm256_add_epi8(initial_lengths, right1);

  __m256i right2 = _mm256_subs_epu8(
      push_last_2bytes_of_a_to_b(previous_carries, sum), _mm256_set1_epi8(2));
  return _mm256_add_epi8(sum, right2);
}

static inline void avxcheckContinuations(__m256i initial_lengths,
                                         __m256i carries, __m256i *has_error) {

  // overlap || underlap
  // carry > length && length > 0 || !(carry > length) && !(length > 0)
  // (carries > length) == (lengths > 0)
  __m256i overunder = _mm256_cmpeq_epi8(
      _mm256_cmpgt_epi8(carries, initial_lengths),
      _mm256_cmpgt_epi8(initial_lengths, _mm256_setzero_si256()));

  *has_error = _mm256_or_si256(*has_error, overunder);
}

// when 0xED is found, next byte must be no larger than 0x9F
// when 0xF4 is found, next byte must be no larger than 0x8F
// next byte must be continuation, ie sign bit is set, so signed < is ok
static inline void avxcheckFirstContinuationMax(__m256i current_bytes,
                                                __m256i off1_current_bytes,
                                                __m256i *has_error) {
  __m256i maskED =
      _mm256_cmpeq_epi8(off1_current_bytes, _mm256_set1_epi8(0xED));
  __m256i maskF4 =
      _mm256_cmpeq_epi8(off1_current_bytes, _mm256_set1_epi8(0xF4));

  __m256i badfollowED = _mm256_and_si256(
      _mm256_cmpgt_epi8(current_bytes, _mm256_set1_epi8(0x9F)), maskED);
  __m256i badfollowF4 = _mm256_and_si256(
      _mm256_cmpgt_epi8(current_bytes, _mm256_set1_epi8(0x8F)), maskF4);

  *has_error =
      _mm256_or_si256(*has_error, _mm256_or_si256(badfollowED, badfollowF4));
}

// map off1_hibits => error condition
// hibits     off1    cur
// C       => < C2 && true
// E       => < E1 && < A0
// F       => < F1 && < 90
// else      false && false
static inline void avxcheckOverlong(__m256i current_bytes,
                                    __m256i off1_current_bytes, __m256i hibits,
                                    __m256i previous_hibits,
                                    __m256i *has_error) {
  __m256i off1_hibits = push_last_byte_of_a_to_b(previous_hibits, hibits);
  __m256i initial_mins = _mm256_shuffle_epi8(
      _mm256_setr_epi8(-128, -128, -128, -128, -128, -128, -128, -128, -128,
                       -128, -128, -128, // 10xx => false
                       0xC2, -128,       // 110x
                       0xE1,             // 1110
                       0xF1, -128, -128, -128, -128, -128, -128, -128, -128,
                       -128, -128, -128, -128, // 10xx => false
                       0xC2, -128,             // 110x
                       0xE1,                   // 1110
                       0xF1),
      off1_hibits);

  __m256i initial_under = _mm256_cmpgt_epi8(initial_mins, off1_current_bytes);

  __m256i second_mins = _mm256_shuffle_epi8(
      _mm256_setr_epi8(-128, -128, -128, -128, -128, -128, -128, -128, -128,
                       -128, -128, -128, // 10xx => false
                       127, 127,         // 110x => true
                       0xA0,             // 1110
                       0x90, -128, -128, -128, -128, -128, -128, -128, -128,
                       -128, -128, -128, -128, // 10xx => false
                       127, 127,               // 110x => true
                       0xA0,                   // 1110
                       0x90),
      off1_hibits);
  __m256i second_under = _mm256_cmpgt_epi8(second_mins, current_bytes);
  *has_error = _mm256_or_si256(*has_error,
                               _mm256_and_si256(initial_under, second_under));
}

struct avx_processed_utf_bytes {
  __m256i rawbytes;
  __m256i high_nibbles;
  __m256i carried_continuations;
};

static inline void avx_count_nibbles(__m256i bytes,
                                     struct avx_processed_utf_bytes *answer) {
  answer->rawbytes = bytes;
  answer->high_nibbles =
      _mm256_and_si256(_mm256_srli_epi16(bytes, 4), _mm256_set1_epi8(0x0F));
}

// check whether the current bytes are valid UTF-8
// at the end of the function, previous gets updated
static struct avx_processed_utf_bytes
avxcheckUTF8Bytes(__m256i current_bytes,
                  struct avx_processed_utf_bytes *previous,
                  __m256i *has_error) {
  struct avx_processed_utf_bytes pb;
  avx_count_nibbles(current_bytes, &pb);

  avxcheckSmallerThan0xF4(current_bytes, has_error);

  __m256i initial_lengths = avxcontinuationLengths(pb.high_nibbles);

  pb.carried_continuations =
      avxcarryContinuations(initial_lengths, previous->carried_continuations);

  avxcheckContinuations(initial_lengths, pb.carried_continuations, has_error);

  __m256i off1_current_bytes =
      push_last_byte_of_a_to_b(previous->rawbytes, pb.rawbytes);
  avxcheckFirstContinuationMax(current_bytes, off1_current_bytes, has_error);

  avxcheckOverlong(current_bytes, off1_current_bytes, pb.high_nibbles,
                   previous->high_nibbles, has_error);
  return pb;
}

/* Return 0 on success, -1 on error */
int utf8_lemire_avx2(const unsigned char *src, int len) {
  size_t i = 0;
  __m256i has_error = _mm256_setzero_si256();
  struct avx_processed_utf_bytes previous = {
      .rawbytes = _mm256_setzero_si256(),
      .high_nibbles = _mm256_setzero_si256(),
      .carried_continuations = _mm256_setzero_si256()};
  if (len >= 32) {
    for (; i <= len - 32; i += 32) {
      __m256i current_bytes = _mm256_loadu_si256((const __m256i *)(src + i));
      previous = avxcheckUTF8Bytes(current_bytes, &previous, &has_error);
    }
  }

  // last part
  if (i < len) {
    char buffer[32];
    memset(buffer, 0, 32);
    memcpy(buffer, src + i, len - i);
    __m256i current_bytes = _mm256_loadu_si256((const __m256i *)(buffer));
    previous = avxcheckUTF8Bytes(current_bytes, &previous, &has_error);
  } else {
    has_error = _mm256_or_si256(
        _mm256_cmpgt_epi8(previous.carried_continuations,
                          _mm256_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                           9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                           9, 9, 9, 9, 9, 9, 9, 1)),
        has_error);
  }

  return _mm256_testz_si256(has_error, has_error) ? 0 : -1;
}

#endif
