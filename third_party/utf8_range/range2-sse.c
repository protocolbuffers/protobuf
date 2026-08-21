/*
 * Process 2x16 bytes in each iteration.
 * Comments removed for brevity. See range-sse.c for details.
 */
#ifdef __SSE4_1__

#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>

int utf8_naive(const unsigned char *data, int len);

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
int utf8_range2(const unsigned char *data, int len)
{
    if (len >= 32) {
        __m128i prev_input = _mm_set1_epi8(0);
        __m128i prev_first_len = _mm_set1_epi8(0);

        const __m128i first_len_tbl =
            _mm_loadu_si128((const __m128i *)_first_len_tbl);
        const __m128i first_range_tbl =
            _mm_loadu_si128((const __m128i *)_first_range_tbl);
        const __m128i range_min_tbl =
            _mm_loadu_si128((const __m128i *)_range_min_tbl);
        const __m128i range_max_tbl =
            _mm_loadu_si128((const __m128i *)_range_max_tbl);
        const __m128i df_ee_tbl =
            _mm_loadu_si128((const __m128i *)_df_ee_tbl);
        const __m128i ef_fe_tbl =
            _mm_loadu_si128((const __m128i *)_ef_fe_tbl);

        __m128i error = _mm_set1_epi8(0);

        while (len >= 32) {
            /***************************** block 1 ****************************/
            const __m128i input_a = _mm_loadu_si128((const __m128i *)data);

            __m128i high_nibbles =
                _mm_and_si128(_mm_srli_epi16(input_a, 4), _mm_set1_epi8(0x0F));

            __m128i first_len_a = _mm_shuffle_epi8(first_len_tbl, high_nibbles);

            __m128i range_a = _mm_shuffle_epi8(first_range_tbl, high_nibbles);

            range_a = _mm_or_si128(
                    range_a, _mm_alignr_epi8(first_len_a, prev_first_len, 15));

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

            tmp = _mm_or_si128(
                      _mm_cmplt_epi8(input_a, minv),
                      _mm_cmpgt_epi8(input_a, maxv)
                  );
            error = _mm_or_si128(error, tmp);

            /***************************** block 2 ****************************/
            const __m128i input_b = _mm_loadu_si128((const __m128i *)(data+16));

            high_nibbles =
                _mm_and_si128(_mm_srli_epi16(input_b, 4), _mm_set1_epi8(0x0F));

            __m128i first_len_b = _mm_shuffle_epi8(first_len_tbl, high_nibbles);

            __m128i range_b = _mm_shuffle_epi8(first_range_tbl, high_nibbles);

            range_b = _mm_or_si128(
                    range_b, _mm_alignr_epi8(first_len_b, first_len_a, 15));


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


            tmp = _mm_or_si128(
                      _mm_cmplt_epi8(input_b, minv),
                      _mm_cmpgt_epi8(input_b, maxv)
                  );
            error = _mm_or_si128(error, tmp);

            /************************ next iteration **************************/
            prev_input = input_b;
            prev_first_len = first_len_b;

            data += 32;
            len -= 32;
        }

        if (!_mm_testz_si128(error, error))
            return -1;

        int32_t token4 = _mm_extract_epi32(prev_input, 3);
        const int8_t *token = (const int8_t *)&token4;
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
