/*
 * Process 2x16 bytes in each iteration.
 * Comments removed for brevity. See range-neon.c for details.
 */
#if defined(__aarch64__) && defined(__ARM_NEON)

#include <stdio.h>
#include <stdint.h>
#include <arm_neon.h>

int utf8_naive(const unsigned char *data, int len);

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
int utf8_range2(const unsigned char *data, int len)
{
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

            const uint8x16_t first_len_a =
                vqtbl1q_u8(first_len_tbl, high_nibbles_a);
            const uint8x16_t first_len_b =
                vqtbl1q_u8(first_len_tbl, high_nibbles_b);

            uint8x16_t range_a = vqtbl1q_u8(first_range_tbl, high_nibbles_a);
            uint8x16_t range_b = vqtbl1q_u8(first_range_tbl, high_nibbles_b);

            range_a =
                vorrq_u8(range_a, vextq_u8(prev_first_len, first_len_a, 15));
            range_b =
                vorrq_u8(range_b, vextq_u8(first_len_a, first_len_b, 15));

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

        if (vmaxvq_u8(error1))
            return -1;

        uint32_t token4;
        vst1q_lane_u32(&token4, vreinterpretq_u32_u8(prev_input), 3);

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
