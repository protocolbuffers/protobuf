
/*
 * http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94
 *
 * Table 3-7. Well-Formed UTF-8 Byte Sequences
 *
 * +--------------------+------------+-------------+------------+-------------+
 * | Code Points        | First Byte | Second Byte | Third Byte | Fourth Byte |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+0000..U+007F     | 00..7F     |             |            |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+0080..U+07FF     | C2..DF     | 80..BF      |            |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+0800..U+0FFF     | E0         | A0..BF      | 80..BF     |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+1000..U+CFFF     | E1..EC     | 80..BF      | 80..BF     |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+D000..U+D7FF     | ED         | 80..9F      | 80..BF     |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+E000..U+FFFF     | EE..EF     | 80..BF      | 80..BF     |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+10000..U+3FFFF   | F0         | 90..BF      | 80..BF     | 80..BF      |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+40000..U+FFFFF   | F1..F3     | 80..BF      | 80..BF     | 80..BF      |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+100000..U+10FFFF | F4         | 80..8F      | 80..BF     | 80..BF      |
 * +--------------------+------------+-------------+------------+-------------+
 */

/* Return 0 - success,  >0 - index(1 based) of first error char */
int utf8_naive(const unsigned char* data, int len) {
  int err_pos = 1;

  while (len) {
    int bytes;
    const unsigned char byte1 = data[0];

    /* 00..7F */
    if (byte1 <= 0x7F) {
      bytes = 1;
      /* C2..DF, 80..BF */
    } else if (len >= 2 && byte1 >= 0xC2 && byte1 <= 0xDF &&
               (signed char)data[1] <= (signed char)0xBF) {
      bytes = 2;
    } else if (len >= 3) {
      const unsigned char byte2 = data[1];

      /* Is byte2, byte3 between 0x80 ~ 0xBF */
      const int byte2_ok = (signed char)byte2 <= (signed char)0xBF;
      const int byte3_ok = (signed char)data[2] <= (signed char)0xBF;

      if (byte2_ok && byte3_ok &&
          /* E0, A0..BF, 80..BF */
          ((byte1 == 0xE0 && byte2 >= 0xA0) ||
           /* E1..EC, 80..BF, 80..BF */
           (byte1 >= 0xE1 && byte1 <= 0xEC) ||
           /* ED, 80..9F, 80..BF */
           (byte1 == 0xED && byte2 <= 0x9F) ||
           /* EE..EF, 80..BF, 80..BF */
           (byte1 >= 0xEE && byte1 <= 0xEF))) {
        bytes = 3;
      } else if (len >= 4) {
        /* Is byte4 between 0x80 ~ 0xBF */
        const int byte4_ok = (signed char)data[3] <= (signed char)0xBF;

        if (byte2_ok && byte3_ok && byte4_ok &&
            /* F0, 90..BF, 80..BF, 80..BF */
            ((byte1 == 0xF0 && byte2 >= 0x90) ||
             /* F1..F3, 80..BF, 80..BF, 80..BF */
             (byte1 >= 0xF1 && byte1 <= 0xF3) ||
             /* F4, 80..8F, 80..BF, 80..BF */
             (byte1 == 0xF4 && byte2 <= 0x8F))) {
          bytes = 4;
        } else {
          return err_pos;
        }
      } else {
        return err_pos;
      }
    } else {
      return err_pos;
    }

    len -= bytes;
    err_pos += bytes;
    data += bytes;
  }

  return 0;
}

#ifdef __SSE4_1__

#include <stdint.h>
#include <stdio.h>
#include <x86intrin.h>

int utf8_naive(const unsigned char* data, int len);

static const int8_t _first_len_tbl[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3,
};

static const int8_t _first_range_tbl[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8,
};

static const int8_t _range_min_tbl[] = {
    0x00, 0x80, 0x80, 0x80, 0xA0, 0x80, 0x90, 0x80,
    0xC2, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
};
static const int8_t _range_max_tbl[] = {
    0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x9F, 0xBF, 0x8F,
    0xF4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
};

static const int8_t _df_ee_tbl[] = {
    0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,
};
static const int8_t _ef_fe_tbl[] = {
    0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* Return 0 on success, -1 on error */
int utf8_range2(const unsigned char* data, int len) {
  if (len >= 32) {
    __m128i prev_input = _mm_set1_epi8(0);
    __m128i prev_first_len = _mm_set1_epi8(0);

    const __m128i first_len_tbl =
        _mm_loadu_si128((const __m128i*)_first_len_tbl);
    const __m128i first_range_tbl =
        _mm_loadu_si128((const __m128i*)_first_range_tbl);
    const __m128i range_min_tbl =
        _mm_loadu_si128((const __m128i*)_range_min_tbl);
    const __m128i range_max_tbl =
        _mm_loadu_si128((const __m128i*)_range_max_tbl);
    const __m128i df_ee_tbl = _mm_loadu_si128((const __m128i*)_df_ee_tbl);
    const __m128i ef_fe_tbl = _mm_loadu_si128((const __m128i*)_ef_fe_tbl);

    __m128i error = _mm_set1_epi8(0);

    while (len >= 32) {
      /***************************** block 1 ****************************/
      const __m128i input_a = _mm_loadu_si128((const __m128i*)data);

      __m128i high_nibbles =
          _mm_and_si128(_mm_srli_epi16(input_a, 4), _mm_set1_epi8(0x0F));

      __m128i first_len_a = _mm_shuffle_epi8(first_len_tbl, high_nibbles);

      __m128i range_a = _mm_shuffle_epi8(first_range_tbl, high_nibbles);

      range_a = _mm_or_si128(range_a,
                             _mm_alignr_epi8(first_len_a, prev_first_len, 15));

      __m128i tmp;
      tmp = _mm_alignr_epi8(first_len_a, prev_first_len, 14);
      tmp = _mm_subs_epu8(tmp, _mm_set1_epi8(1));
      range_a = _mm_or_si128(range_a, tmp);

      tmp = _mm_alignr_epi8(first_len_a, prev_first_len, 13);
      tmp = _mm_subs_epu8(tmp, _mm_set1_epi8(2));
      range_a = _mm_or_si128(range_a, tmp);

      __m128i shift1, pos, range2;
      shift1 = _mm_alignr_epi8(input_a, prev_input, 15);
      pos = _mm_sub_epi8(shift1, _mm_set1_epi8(0xEF));
      tmp = _mm_subs_epu8(pos, _mm_set1_epi8(0xF0));
      range2 = _mm_shuffle_epi8(df_ee_tbl, tmp);
      tmp = _mm_adds_epu8(pos, _mm_set1_epi8(0x70));
      range2 = _mm_add_epi8(range2, _mm_shuffle_epi8(ef_fe_tbl, tmp));

      range_a = _mm_add_epi8(range_a, range2);

      __m128i minv = _mm_shuffle_epi8(range_min_tbl, range_a);
      __m128i maxv = _mm_shuffle_epi8(range_max_tbl, range_a);

      tmp = _mm_or_si128(_mm_cmplt_epi8(input_a, minv),
                         _mm_cmpgt_epi8(input_a, maxv));
      error = _mm_or_si128(error, tmp);

      /***************************** block 2 ****************************/
      const __m128i input_b = _mm_loadu_si128((const __m128i*)(data + 16));

      high_nibbles =
          _mm_and_si128(_mm_srli_epi16(input_b, 4), _mm_set1_epi8(0x0F));

      __m128i first_len_b = _mm_shuffle_epi8(first_len_tbl, high_nibbles);

      __m128i range_b = _mm_shuffle_epi8(first_range_tbl, high_nibbles);

      range_b =
          _mm_or_si128(range_b, _mm_alignr_epi8(first_len_b, first_len_a, 15));

      tmp = _mm_alignr_epi8(first_len_b, first_len_a, 14);
      tmp = _mm_subs_epu8(tmp, _mm_set1_epi8(1));
      range_b = _mm_or_si128(range_b, tmp);

      tmp = _mm_alignr_epi8(first_len_b, first_len_a, 13);
      tmp = _mm_subs_epu8(tmp, _mm_set1_epi8(2));
      range_b = _mm_or_si128(range_b, tmp);

      shift1 = _mm_alignr_epi8(input_b, input_a, 15);
      pos = _mm_sub_epi8(shift1, _mm_set1_epi8(0xEF));
      tmp = _mm_subs_epu8(pos, _mm_set1_epi8(0xF0));
      range2 = _mm_shuffle_epi8(df_ee_tbl, tmp);
      tmp = _mm_adds_epu8(pos, _mm_set1_epi8(0x70));
      range2 = _mm_add_epi8(range2, _mm_shuffle_epi8(ef_fe_tbl, tmp));

      range_b = _mm_add_epi8(range_b, range2);

      minv = _mm_shuffle_epi8(range_min_tbl, range_b);
      maxv = _mm_shuffle_epi8(range_max_tbl, range_b);

      tmp = _mm_or_si128(_mm_cmplt_epi8(input_b, minv),
                         _mm_cmpgt_epi8(input_b, maxv));
      error = _mm_or_si128(error, tmp);

      /************************ next iteration **************************/
      prev_input = input_b;
      prev_first_len = first_len_b;

      data += 32;
      len -= 32;
    }

    if (!_mm_testz_si128(error, error)) return -1;

    int32_t token4 = _mm_extract_epi32(prev_input, 3);
    const int8_t* token = (const int8_t*)&token4;
    int lookahead = 0;
    if (token[3] > (int8_t)0xBF)
      lookahead = 1;
    else if (token[2] > (int8_t)0xBF)
      lookahead = 2;
    else if (token[1] > (int8_t)0xBF)
      lookahead = 3;

    data -= lookahead;
    len += lookahead;
  }

  return utf8_naive(data, len);
}

#endif

#ifdef __ARM_NEON

#include <arm_neon.h>
#include <stdint.h>
#include <stdio.h>

int utf8_naive(const unsigned char* data, int len);

static const uint8_t _first_len_tbl[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3,
};

static const uint8_t _first_range_tbl[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8,
};

static const uint8_t _range_min_tbl[] = {
    0x00, 0x80, 0x80, 0x80, 0xA0, 0x80, 0x90, 0x80,
    0xC2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
static const uint8_t _range_max_tbl[] = {
    0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x9F, 0xBF, 0x8F,
    0xF4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t _range_adjust_tbl[] = {
    2, 3, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0,
};

/* Return 0 on success, -1 on error */
int utf8_range2(const unsigned char* data, int len) {
  if (len >= 32) {
    uint8x16_t prev_input = vdupq_n_u8(0);
    uint8x16_t prev_first_len = vdupq_n_u8(0);

    const uint8x16_t first_len_tbl = vld1q_u8(_first_len_tbl);
    const uint8x16_t first_range_tbl = vld1q_u8(_first_range_tbl);
    const uint8x16_t range_min_tbl = vld1q_u8(_range_min_tbl);
    const uint8x16_t range_max_tbl = vld1q_u8(_range_max_tbl);
    const uint8x16x2_t range_adjust_tbl = vld2q_u8(_range_adjust_tbl);

    const uint8x16_t const_1 = vdupq_n_u8(1);
    const uint8x16_t const_2 = vdupq_n_u8(2);
    const uint8x16_t const_e0 = vdupq_n_u8(0xE0);

    uint8x16_t error1 = vdupq_n_u8(0);
    uint8x16_t error2 = vdupq_n_u8(0);
    uint8x16_t error3 = vdupq_n_u8(0);
    uint8x16_t error4 = vdupq_n_u8(0);

    while (len >= 32) {
      /******************* two blocks interleaved **********************/

#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 8)
      /* gcc doesn't support vldq1_u8_x2 until version 8 */
      const uint8x16_t input_a = vld1q_u8(data);
      const uint8x16_t input_b = vld1q_u8(data + 16);
#else
      /* Forces a double load on Clang */
      const uint8x16x2_t input_pair = vld1q_u8_x2(data);
      const uint8x16_t input_a = input_pair.val[0];
      const uint8x16_t input_b = input_pair.val[1];
#endif

      const uint8x16_t high_nibbles_a = vshrq_n_u8(input_a, 4);
      const uint8x16_t high_nibbles_b = vshrq_n_u8(input_b, 4);

      const uint8x16_t first_len_a = vqtbl1q_u8(first_len_tbl, high_nibbles_a);
      const uint8x16_t first_len_b = vqtbl1q_u8(first_len_tbl, high_nibbles_b);

      uint8x16_t range_a = vqtbl1q_u8(first_range_tbl, high_nibbles_a);
      uint8x16_t range_b = vqtbl1q_u8(first_range_tbl, high_nibbles_b);

      range_a = vorrq_u8(range_a, vextq_u8(prev_first_len, first_len_a, 15));
      range_b = vorrq_u8(range_b, vextq_u8(first_len_a, first_len_b, 15));

      uint8x16_t tmp1_a, tmp2_a, tmp1_b, tmp2_b;
      tmp1_a = vextq_u8(prev_first_len, first_len_a, 14);
      tmp1_a = vqsubq_u8(tmp1_a, const_1);
      range_a = vorrq_u8(range_a, tmp1_a);

      tmp1_b = vextq_u8(first_len_a, first_len_b, 14);
      tmp1_b = vqsubq_u8(tmp1_b, const_1);
      range_b = vorrq_u8(range_b, tmp1_b);

      tmp2_a = vextq_u8(prev_first_len, first_len_a, 13);
      tmp2_a = vqsubq_u8(tmp2_a, const_2);
      range_a = vorrq_u8(range_a, tmp2_a);

      tmp2_b = vextq_u8(first_len_a, first_len_b, 13);
      tmp2_b = vqsubq_u8(tmp2_b, const_2);
      range_b = vorrq_u8(range_b, tmp2_b);

      uint8x16_t shift1_a = vextq_u8(prev_input, input_a, 15);
      uint8x16_t pos_a = vsubq_u8(shift1_a, const_e0);
      range_a = vaddq_u8(range_a, vqtbl2q_u8(range_adjust_tbl, pos_a));

      uint8x16_t shift1_b = vextq_u8(input_a, input_b, 15);
      uint8x16_t pos_b = vsubq_u8(shift1_b, const_e0);
      range_b = vaddq_u8(range_b, vqtbl2q_u8(range_adjust_tbl, pos_b));

      uint8x16_t minv_a = vqtbl1q_u8(range_min_tbl, range_a);
      uint8x16_t maxv_a = vqtbl1q_u8(range_max_tbl, range_a);

      uint8x16_t minv_b = vqtbl1q_u8(range_min_tbl, range_b);
      uint8x16_t maxv_b = vqtbl1q_u8(range_max_tbl, range_b);

      error1 = vorrq_u8(error1, vcltq_u8(input_a, minv_a));
      error2 = vorrq_u8(error2, vcgtq_u8(input_a, maxv_a));

      error3 = vorrq_u8(error3, vcltq_u8(input_b, minv_b));
      error4 = vorrq_u8(error4, vcgtq_u8(input_b, maxv_b));

      /************************ next iteration *************************/
      prev_input = input_b;
      prev_first_len = first_len_b;

      data += 32;
      len -= 32;
    }
    error1 = vorrq_u8(error1, error2);
    error1 = vorrq_u8(error1, error3);
    error1 = vorrq_u8(error1, error4);

    if (vmaxvq_u8(error1)) return -1;

    uint32_t token4;
    vst1q_lane_u32(&token4, vreinterpretq_u32_u8(prev_input), 3);

    const int8_t* token = (const int8_t*)&token4;
    int lookahead = 0;
    if (token[3] > (int8_t)0xBF)
      lookahead = 1;
    else if (token[2] > (int8_t)0xBF)
      lookahead = 2;
    else if (token[1] > (int8_t)0xBF)
      lookahead = 3;

    data -= lookahead;
    len += lookahead;
  }

  return utf8_naive(data, len);
}

#endif
