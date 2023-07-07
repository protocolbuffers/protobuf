// faster-utf8-validator
//
// Copyright (c) 2019 Zach Wegner
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// How this validator works:
//
//   [[[ UTF-8 refresher: UTF-8 encodes text in sequences of "code points",
//   each one from 1-4 bytes. For each code point that is longer than one byte,
//   the code point begins with a unique prefix that specifies how many bytes
//   follow. All bytes in the code point after this first have a continuation
//   marker. All code points in UTF-8 will thus look like one of the following
//   binary sequences, with x meaning "don't care":
//      1 byte:  0xxxxxxx
//      2 bytes: 110xxxxx  10xxxxxx
//      3 bytes: 1110xxxx  10xxxxxx  10xxxxxx
//      4 bytes: 11110xxx  10xxxxxx  10xxxxxx  10xxxxxx
//   ]]]
//
// This validator works in two basic steps: checking continuation bytes, and
// handling special cases. Each step works on one vector's worth of input
// bytes at a time.
//
// The continuation bytes are handled in a fairly straightforward manner in
// the scalar domain. A mask is created from the input byte vector for each
// of the highest four bits of every byte. The first mask allows us to quickly
// skip pure ASCII input vectors, which have no bits set. The first and
// (inverted) second masks together give us every continuation byte (10xxxxxx).
// The other masks are used to find prefixes of multi-byte code points (110,
// 1110, 11110). For these, we keep a "required continuation" mask, by shifting
// these masks 1, 2, and 3 bits respectively forward in the byte stream. That
// is, we take a mask of all bytes that start with 11, and shift it left one
// bit forward to get the mask of all the first continuation bytes, then do the
// same for the second and third continuation bytes. Here's an example input
// sequence along with the corresponding masks:
//
//   bytes:        61 C3 80 62 E0 A0 80 63 F0 90 80 80 00
//   code points:  61|C3 80|62|E0 A0 80|63|F0 90 80 80|00
//   # of bytes:   1 |2  - |1 |3  -  - |1 |4  -  -  - |1
//   cont. mask 1: -  -  1  -  -  1  -  -  -  1  -  -  -
//   cont. mask 2: -  -  -  -  -  -  1  -  -  -  1  -  -
//   cont. mask 3: -  -  -  -  -  -  -  -  -  -  -  1  -
//   cont. mask *: 0  0  1  0  0  1  1  0  0  1  1  1  0
//
// The final required continuation mask is then compared with the mask of
// actual continuation bytes, and must match exactly in valid UTF-8. The only
// complication in this step is that the shifted masks can cross vector
// boundaries, so we need to keep a "carry" mask of the bits that were shifted
// past the boundary in the last loop iteration.
//
// Besides the basic prefix coding of UTF-8, there are several invalid byte
// sequences that need special handling. These are due to three factors:
// code points that could be described in fewer bytes, code points that are
// part of a surrogate pair (which are only valid in UTF-16), and code points
// that are past the highest valid code point U+10FFFF.
//
// All of the invalid sequences can be detected by independently observing
// the first three nibbles of each code point. Since AVX2 can do a 4-bit/16-byte
// lookup in parallel for all 32 bytes in a vector, we can create bit masks
// for all of these error conditions, look up the bit masks for the three
// nibbles for all input bytes, and AND them together to get a final error mask,
// that must be all zero for valid UTF-8. This is somewhat complicated by
// needing to shift the error masks from the first and second nibbles forward in
// the byte stream to line up with the third nibble.
//
// We have these possible values for valid UTF-8 sequences, broken down
// by the first three nibbles:
//
//   1st   2nd   3rd   comment
//   0..7  0..F        ASCII
//   8..B  0..F        continuation bytes
//   C     2..F  8..B  C0 xx and C1 xx can be encoded in 1 byte
//   D     0..F  8..B  D0..DF are valid with a continuation byte
//   E     0     A..B  E0 8x and E0 9x can be encoded with 2 bytes
//         1..C  8..B  E1..EC are valid with continuation bytes
//         D     8..9  ED Ax and ED Bx correspond to surrogate pairs
//         E..F  8..B  EE..EF are valid with continuation bytes
//   F     0     9..B  F0 8x can be encoded with 3 bytes
//         1..3  8..B  F1..F3 are valid with continuation bytes
//         4     8     F4 8F BF BF is the maximum valid code point
//
// That leaves us with these invalid sequences, which would otherwise fit
// into UTF-8's prefix encoding. Each of these invalid sequences needs to
// be detected separately, with their own bits in the error mask.
//
//   1st   2nd   3rd   error bit
//   C     0..1  0..F  0x01
//   E     0     8..9  0x02
//         D     A..B  0x04
//   F     0     0..8  0x08
//         4     9..F  0x10
//         5..F  0..F  0x20
//
// For every possible value of the first, second, and third nibbles, we keep
// a lookup table that contains the bitwise OR of all errors that that nibble
// value can cause. For example, the first nibble has zeroes in every entry
// except for C, E, and F, and the third nibble lookup has the 0x21 bits in
// every entry, since those errors don't depend on the third nibble. After
// doing a parallel lookup of the first/second/third nibble values for all
// bytes, we AND them together. Only when all three have an error bit in common
// do we fail validation.


#if   V_LEN == 64
// AVX512 definitions


#   define z_validate_vec   z_validate_vec_512
#   define z_dvalidate_utf8  u_utf8_d512
#   define z_dvalidate_vec   z_dvalidate_vec_512
#   define z_tvalidate_utf8  u_utf8_t512
#   define z_tvalidate_vec   z_tvalidate_vec_512

// Vector and vector mask types. We use #defines instead of typedefs so this
// header can be included multiple times with different configurations

#   define vec_t            __m512i
#   define vmask_t          uint64_t
#   define vmask2_t         uint128_t

#   define v_load(x)        _mm512_loadu_si512((vec_t *)(x))
#   define v_set1           _mm512_set1_epi8
#   define v_and            _mm512_and_si512
#   define v_or             _mm512_or_si512
#   define v_test_bit7(input) _mm512_movepi8_mask(input)

#   define v_test_bit(input, bit)                                           \
        _mm512_movepi8_mask(_mm512_slli_epi16((input), 7 - (bit)))

// Parallel table lookup for all bytes in a vector. We need to AND with 0x0F
// for the lookup, because vpshufb has the neat "feature" that negative values
// in an index byte will result in a zero.

#   define v_lookup(table, index, mask, shift)                              \
        _mm512_shuffle_epi8((table),                                        \
                v_and(_mm512_srli_epi16((index), (shift)), (mask)))
#   define v_lookup_no_shift(table, index, mask)                            \
        _mm512_shuffle_epi8((table),                                        \
                v_and((index), (mask)))

#   define v_testz(a,b)          (_mm512_test_epi8_mask(a,b) == 0)

// Simple macro to make a vector lookup table for use with vpshufb. Since
// AVX2 is two 16-byte halves, we duplicate the input values.

#   define V_TABLE_16(...)    _mm512_set_epi8(__VA_ARGS__, __VA_ARGS__,__VA_ARGS__, __VA_ARGS__)

// Move all the bytes in "input" to the left by one and fill in the first byte
// with zero. Since AVX2 generally works on two separate 16-byte vectors glued
// together, this needs two steps. The permute2x128 takes the middle 32 bytes
// of the 64-byte concatenation v_zero:input. The align then gives the final
// result in each half:
//      top half: input_L:input_H --> input_L[15]:input_H[0:14]
//   bottom half:  zero_H:input_L -->  zero_H[15]:input_L[0:14]
inline __m512i _mm512_slli_si512(__m512i a, uint8_t byteCount)
{
#ifdef __AVX512_VBMI2__
    return _mm512_maskz_compress_epi8(-1LL << byteCount, a);
#else
    // set up temporary array and set lower half to zero 
    // (this needs to happen outside any critical loop)
    alignas(64) char temp[128];
    _mm512_store_si512(temp, _mm512_setzero_si512());

    // store input into upper half
    _mm512_store_si512(temp + 64, a);

    // load shifted register
    return _mm512_loadu_si512(temp + (64 - byteCount));
#endif
}

#   define v_shift_lanes_left(input)  _mm512_slli_si512(input, 1)


#elif V_LEN == 32

// AVX2 definitions


#   define z_validate_vec   z_validate_vec_256
#   define z_dvalidate_utf8  u_utf8_d256
#   define z_dvalidate_vec   z_dvalidate_vec_256
#   define z_tvalidate_utf8  u_utf8_t256
#   define z_tvalidate_vec   z_tvalidate_vec_256

// Vector and vector mask types. We use #defines instead of typedefs so this
// header can be included multiple times with different configurations

#   define vec_t            __m256i
#   define vmask_t          uint32_t
#   define vmask2_t         uint64_t

#   define v_load(x)        _mm256_loadu_si256((vec_t *)(x))
#   define v_set1           _mm256_set1_epi8
#   define v_and            _mm256_and_si256
#   define v_or             _mm256_or_si256
#   define v_test_bit7(input) _mm256_movemask_epi8(input)

#   define v_test_bit(input, bit)                                     \
        _mm256_movemask_epi8(_mm256_slli_epi16((input), 7 - (bit)))

// Parallel table lookup for all bytes in a vector. We need to AND with 0x0F
// for the lookup, because vpshufb has the neat "feature" that negative values
// in an index byte will result in a zero.

#   define v_lookup(table, index, mask, shift)                        \
                _mm256_shuffle_epi8((table),                          \
                v_and(_mm256_srli_epi16((index), (shift)), (mask)))
#   define v_lookup_no_shift(table, index, mask)                      \
                _mm256_shuffle_epi8((table),                          \
                v_and((index), (mask)))

#   define v_testz(a,b)          (_mm256_testz_si256(a,b) == 1)

// Simple macro to make a vector lookup table for use with vpshufb. Since
// AVX2 is two 16-byte halves, we duplicate the input values.

#   define V_TABLE_16(...)    _mm256_set_epi8(__VA_ARGS__, __VA_ARGS__)

// Move all the bytes in "input" to the left by one and fill in the first byte
// with zero. Since AVX2 generally works on two separate 16-byte vectors glued
// together, this needs two steps. The permute2x128 takes the middle 32 bytes
// of the 64-byte concatenation v_zero:input. The align then gives the final
// result in each half:
//      top half: input_L:input_H --> input_L[15]:input_H[0:14]
//   bottom half:  zero_H:input_L -->  zero_H[15]:input_L[0:14]
#   define v_shift_lanes_left(input)  _mm256_alignr_epi8((input), _mm256_permute2x128_si256((input), (input), 0x08), 15)

#elif V_LEN == 16

// SSE definitions. We require at least SSE4.1 for _mm_test_all_zeros()


#   define z_validate_vec   z_validate_vec_128
#   define z_dvalidate_utf8  u_utf8_d128
#   define z_dvalidate_vec   z_dvalidate_vec_028
#   define z_tvalidate_utf8  u_utf8_t128
#   define z_tvalidate_vec   z_tvalidate_vec_128

#   define vec_t            __m128i
#   define vmask_t          uint16_t
#   define vmask2_t         uint32_t

#   define v_load(x)        _mm_lddqu_si128((vec_t *)(x))
#   define v_set1           _mm_set1_epi8
#   define v_and            _mm_and_si128
#   define v_or             _mm_or_si128
#   define v_test_bit7(input) _mm_movemask_epi8(input)

#   define v_test_bit(input, bit)                                           \
        _mm_movemask_epi8(_mm_slli_epi16((input), (uint8_t)(7 - (bit))))

#   define v_lookup(table, index, mask, shift)                              \
                _mm_shuffle_epi8((table),                                   \
                v_and(_mm_srli_epi16((index), (shift)), (mask)))
#   define v_lookup_no_shift(table, index, mask)                            \
                _mm_shuffle_epi8((table),                                   \
                v_and((index), (mask)))

#   define v_testz(a,b)          (_mm_testz_si128(a,b) == 1)

#   define V_TABLE_16(...)  _mm_set_epi8(__VA_ARGS__)

#   define v_shift_lanes_left v_shift_lanes_left_sse4

static inline vec_t v_shift_lanes_left(vec_t top)
{
    return _mm_alignr_epi8(top, v_set1(0), 15);
}

#elif V_LEN == 8

// SSE definitions. We require at least SSE4.1 for _mm_test_all_zeros()


#   define z_validate_vec   z_validate_vec_64
#   define z_dvalidate_utf8  u_utf8_d128
#   define z_dvalidate_vec   z_dvalidate_vec_64
#   define z_tvalidate_utf8  u_utf8_t64
#   define z_tvalidate_vec   z_tvalidate_vec_64

#   define vec_t            uint64_t
#   define vmask_t          uint8_t
#   define vmask2_t         uint16_t

#   define v_load(x)        *((vec_t *)(x))
#   define v_set1(x)        (x | (x << 8) | (x << 16) | (x << 24) | (x << 32) | (x << 40) | (x << 48) | (x << 56)) 
#   define v_and(x, y)      (x & y)
#   define v_or(x, y)      (x | y)
#   define v_test_bit7(input) _pext_u32((input), 0x8080808080808080ull)

#   define v_test_bit(input, bit)                                           \
        _pext_u32((input) << (7 - (bit)), 0x8080808080808080ull))

#   define v_lookup(table, index, mask, shift)                              \
        _mm_shuffle_epi8((table),                                           \
                v_and(_mm_srli_epi16((index), (shift)), (mask)))
#   define v_lookup_no_shift(table, index, mask)                                    \
        _mm_shuffle_epi8((table),                                           \
                v_and((index), (mask)))

#   define v_testz(a,b)          (((a) & (b)) == 0)

#   define V_TABLE_16(...)  _mm_set_epi8(__VA_ARGS__)

#   define v_shift_lanes_left v_shift_lanes_left_64

static inline vec_t v_shift_lanes_left(vec_t top)
{
    return top << 8;
}

#else

#   error "No valid configuration: must define one of the vector length V_LEN, and AVX512, AVX2 or SSE4.2"

#endif


// force inlining
#ifdef _MSC_VER
#define INLINE  __forceinline
#elif defined (__GNUC__)
#define INLINE __attribute__((always_inline)) inline
#else
#define INLINE inline
#endif


static INLINE bool z_validate_vec(vec_t bytes, vec_t shifted_bytes, vmask_t *last_cont);

// Validate one vector's worth of input bytes
static INLINE bool z_validate_vec(vec_t bytes, vec_t shifted_bytes, vmask_t *last_cont)
{
    // Error lookup tables for the first, second, and third nibbles
    const vec_t error_1 = V_TABLE_16(
        0x38, 0x06, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    );
    const vec_t error_2 = V_TABLE_16(
        0x20, 0x20, 0x24, 0x20,
        0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x10,
        0x00, 0x00, 0x01, 0x0B
    );
    const vec_t error_3 = V_TABLE_16(
        0x31, 0x31, 0x31, 0x31,
        0x35, 0x35, 0x33, 0x2B,
        0x29, 0x29, 0x29, 0x29,
        0x29, 0x29, 0x29, 0x29
    );

    // Quick skip for ascii-only input. If there are no bytes with the high bit
    // set, we don't need to do any more work. We return either valid or
    // invalid based on whether we expected any continuation bytes here.
    vmask_t high = v_test_bit7(bytes);
    if (!high)
        return *last_cont == 0;

    bool pass = true;
    // Which bytes are required to be continuation bytes
    vmask2_t req = *last_cont;
    // A bitmask of the actual continuation bytes in the input
    vmask_t cont;

    // Compute the continuation byte mask by finding bytes that start with
    // 11x, 111x, and 1111. For each of these prefixes, we get a bitmask
    // and shift it forward by 1, 2, or 3. This loop should be unrolled by
    // the compiler, and the (n == 1) branch inside eliminated.
    vmask_t set = high;
    set &= v_test_bit(bytes, 6);
    // Mark continuation bytes: those that have the high bit set but
    // not the next one
    cont = high ^ set;

    // We add the shifted mask here instead of ORing it, which would
    // be the more natural operation, so that this line can be done
    // with one lea. While adding could give a different result due
    // to carries, this will only happen for invalid UTF-8 sequences,
    // and in a way that won't cause it to pass validation. Reasoning:
    // Any bits for required continuation bytes come after the bits
    // for their leader bytes, and are all contiguous. For a carry to
    // happen, two of these bit sequences would have to overlap. If
    // this is the case, there is a leader byte before the second set
    // of required continuation bytes (and thus before the bit that
    // will be cleared by a carry). This leader byte will not be
    // in the continuation mask, despite being required. QEDish.
    req += (vmask2_t)set << 1;
    set &= v_test_bit(bytes, 5);
    req += (vmask2_t)set << 2;
    set &= v_test_bit(bytes, 4);
    req += (vmask2_t)set << 3;

    // Check that continuation bytes match. We must cast req from vmask2_t
    // (which holds the carry mask in the upper half) to vmask_t, which
    // zeroes out the upper bits
    pass &= cont == (vmask_t)req;

    // Look up error masks for three consecutive nibbles.
    vec_t mask = v_set1(0x0F);
    vec_t e_1 = v_lookup(error_1, shifted_bytes, mask, 4);
    vec_t e_2 = v_lookup_no_shift(error_2, shifted_bytes, mask);
    vec_t e_3 = v_lookup(error_3, bytes, mask, 4);

    // Check if any bits are set in all three error masks
    pass &= v_testz(v_and(e_1, e_2), e_3) != 0;

    // Save continuation bits and input bytes for the next round
    *last_cont = req >> V_LEN;

    return pass;
}

static INLINE bool z_dvalidate_vec(vec_t bytes0, vec_t shifted_bytes0, vmask_t *last_cont0, vec_t bytes1, vec_t shifted_bytes1, vmask_t *last_cont1);

// Validate two vector's worth of input bytes
static INLINE bool z_dvalidate_vec(vec_t bytes0, vec_t shifted_bytes0, vmask_t *last_cont0, vec_t bytes1, vec_t shifted_bytes1, vmask_t *last_cont1)
{
    // Error lookup tables for the first, second, and third nibbles
    const vec_t error_1 = V_TABLE_16(
        0x38, 0x06, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    );
    const vec_t error_2 = V_TABLE_16(
        0x20, 0x20, 0x24, 0x20,
        0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x10,
        0x00, 0x00, 0x01, 0x0B
    );
    const vec_t error_3 = V_TABLE_16(
        0x31, 0x31, 0x31, 0x31,
        0x35, 0x35, 0x33, 0x2B,
        0x29, 0x29, 0x29, 0x29,
        0x29, 0x29, 0x29, 0x29
    );

    // Which bytes are required to be continuation bytes
    vmask2_t req0 = *last_cont0;
    vmask2_t req1 = *last_cont1;
    // A bitmask of the actual continuation bytes in the input
    vmask_t cont0, cont1;

    // Compute the continuation byte mask by finding bytes that start with
    // 11x, 111x, and 1111. For each of these prefixes, we get a bitmask
    // and shift it forward by 1, 2, or 3. This loop should be unrolled by
    // the compiler, and the (n == 1) branch inside eliminated.
    vmask_t high0 = v_test_bit7(bytes0);
    vmask_t high1 = v_test_bit7(bytes1);
    vmask_t set0 = high0 & v_test_bit(bytes0, 6);
    vmask_t set1 = high1 & v_test_bit(bytes1, 6);
    cont0 = high0 ^ set0;
    cont1 = high1 ^ set1;
    req0 += (vmask2_t)set0 << 1;
    req1 += (vmask2_t)set1 << 1;
    set0 &= v_test_bit(bytes0, 5);
    set1 &= v_test_bit(bytes1, 5);
    req0 += (vmask2_t)set0 << 2;
    req1 += (vmask2_t)set1 << 2;
    set0 &= v_test_bit(bytes0, 4);
    set1 &= v_test_bit(bytes1, 4);
    req0 += (vmask2_t)set0 << 3;
    req1 += (vmask2_t)set1 << 3;

    // Check that continuation bytes match. We must cast req from vmask2_t
    // (which holds the carry mask in the upper half) to vmask_t, which
    // zeroes out the upper bits
    bool pass = (cont0 == (vmask_t)req0);
    pass &= (cont1 == (vmask_t)req1);

    // Look up error masks for three consecutive nibbles.
    vec_t mask0f = v_set1(0x0f);
    vec_t e_10 = v_lookup(error_1, shifted_bytes0, mask0f, 4);
    vec_t e_11 = v_lookup(error_1, shifted_bytes1, mask0f, 4);
    vec_t e_20 = v_lookup_no_shift(error_2, shifted_bytes0, mask0f);
    vec_t e_21 = v_lookup_no_shift(error_2, shifted_bytes1, mask0f);
    vec_t e_30 = v_lookup(error_3, bytes0, mask0f, 4);
    vec_t e_31 = v_lookup(error_3, bytes1, mask0f, 4);

    // Check if any bits are set in all three error masks
    pass &= v_testz(v_and(e_10, e_20), e_30) != 0;
    pass &= v_testz(v_and(e_11, e_21), e_31) != 0;

    // Save continuation bits and input bytes for the next round
    *last_cont0 = req0 >> V_LEN;
    *last_cont1 = req1 >> V_LEN;
    return pass;
}

static INLINE bool z_tvalidate_vec(vec_t bytes0, vec_t shifted_bytes0, vmask_t *last_cont0,
    vec_t bytes1, vec_t shifted_bytes1, vmask_t *last_cont1,
    vec_t bytes2, vec_t shifted_bytes2, vmask_t *last_cont2);

// Validate two vector's worth of input bytes
static INLINE bool z_tvalidate_vec(vec_t bytes0, vec_t shifted_bytes0, vmask_t *last_cont0,
    vec_t bytes1, vec_t shifted_bytes1, vmask_t *last_cont1,
    vec_t bytes2, vec_t shifted_bytes2, vmask_t *last_cont2)
{
    // Error lookup tables for the first, second, and third nibbles
    const vec_t error_1 = V_TABLE_16(
        0x38, 0x06, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    );
    const vec_t error_2 = V_TABLE_16(
        0x20, 0x20, 0x24, 0x20,
        0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x10,
        0x00, 0x00, 0x01, 0x0B
    );
    const vec_t error_3 = V_TABLE_16(
        0x31, 0x31, 0x31, 0x31,
        0x35, 0x35, 0x33, 0x2B,
        0x29, 0x29, 0x29, 0x29,
        0x29, 0x29, 0x29, 0x29
    );

    // Which bytes are required to be continuation bytes
    vmask2_t req0 = *last_cont0;
    vmask2_t req1 = *last_cont1;
    vmask2_t req2 = *last_cont2;
    // A bitmask of the actual continuation bytes in the input
    vmask_t cont0, cont1, cont2;

    // Compute the continuation byte mask by finding bytes that start with
    // 11x, 111x, and 1111. For each of these prefixes, we get a bitmask
    // and shift it forward by 1, 2, or 3. This loop should be unrolled by
    // the compiler, and the (n == 1) branch inside eliminated.
    vmask_t high0 = v_test_bit7(bytes0);
    vmask_t high1 = v_test_bit7(bytes1);
    vmask_t high2 = v_test_bit7(bytes2);
    vmask_t set0 = high0 & v_test_bit(bytes0, 6);
    vmask_t set1 = high1 & v_test_bit(bytes1, 6);
    vmask_t set2 = high2 & v_test_bit(bytes2, 6);
    cont0 = high0 ^ set0;
    cont1 = high1 ^ set1;
    cont2 = high2 ^ set2;
    req0 += (vmask2_t)set0 << 1;
    req1 += (vmask2_t)set1 << 1;
    req2 += (vmask2_t)set2 << 1;
    set0 &= v_test_bit(bytes0, 5);
    set1 &= v_test_bit(bytes1, 5);
    set2 &= v_test_bit(bytes2, 5);
    req0 += (vmask2_t)set0 << 2;
    req1 += (vmask2_t)set1 << 2;
    req2 += (vmask2_t)set2 << 2;
    set0 &= v_test_bit(bytes0, 4);
    set1 &= v_test_bit(bytes1, 4);
    set2 &= v_test_bit(bytes2, 4);
    req0 += (vmask2_t)set0 << 3;
    req1 += (vmask2_t)set1 << 3;
    req2 += (vmask2_t)set2 << 3;

    // Check that continuation bytes match. We must cast req from vmask2_t
    // (which holds the carry mask in the upper half) to vmask_t, which
    // zeroes out the upper bits
    bool pass = (cont0 == (vmask_t)req0);
    pass &= (cont1 == (vmask_t)req1);
    pass &= (cont2 == (vmask_t)req2);

    // Look up error masks for three consecutive nibbles.
    vec_t mask0f = v_set1(0x0f);
    vec_t e_10 = v_lookup(error_1, shifted_bytes0, mask0f, 4);
    vec_t e_11 = v_lookup(error_1, shifted_bytes1, mask0f, 4);
    vec_t e_12 = v_lookup(error_1, shifted_bytes2, mask0f, 4);
    vec_t e_20 = v_lookup_no_shift(error_2, shifted_bytes0, mask0f);
    vec_t e_21 = v_lookup_no_shift(error_2, shifted_bytes1, mask0f);
    vec_t e_22 = v_lookup_no_shift(error_2, shifted_bytes2, mask0f);
    vec_t e_30 = v_lookup(error_3, bytes0, mask0f, 4);
    vec_t e_31 = v_lookup(error_3, bytes1, mask0f, 4);
    vec_t e_32 = v_lookup(error_3, bytes2, mask0f, 4);

    // Check if any bits are set in all three error masks
    pass &= v_testz(v_and(e_10, e_20), e_30) != 0;
    pass &= v_testz(v_and(e_11, e_21), e_31) != 0;
    pass &= v_testz(v_and(e_12, e_22), e_32) != 0;

    // Save continuation bits and input bytes for the next round
    *last_cont0 = req0 >> V_LEN;
    *last_cont1 = req1 >> V_LEN;
    *last_cont2 = req2 >> V_LEN;
    return pass;
}


bool z_dvalidate_utf8(const uint8_t *data, uint32_t len)
{
    vec_t bytes0, shifted_bytes0;
    vec_t bytes1, shifted_bytes1;

    if (V_LEN <= len)
    {
        // quickly filter out ascii7 (trimm both sides of the input buffer)
        while (2 * V_LEN <= len)
        {
            bytes0 = v_load(data);
            bytes1 = v_load(data + len - V_LEN);
            if (v_test_bit7(v_or(bytes0, bytes1))) break;  // not ascii7
            data += V_LEN;
            len -= 2 * V_LEN;
        }
        while (V_LEN <= len)
        {
            bytes0 = v_load(data);
            if (v_test_bit7(bytes0)) break;  // not ascii7
            data += V_LEN;
            len -= V_LEN;
        }
    }

    const uint8_t *end1 = data + len;
    const uint8_t *data1 = utf8MidBoundary(data, end1, 2);
    const uint8_t *end0 = data1;
    const uint8_t *data0 = data;
    uint32_t len0 = (uint32_t)(end0 - data0);
    uint32_t len1 = (uint32_t)(end1 - data1);
    vmask_t last_cont0 = 0;
    vmask_t last_cont1 = 0;
    len = len0 < len1 ? len0 : len1;

    // Deal with the input up until the last section of bytes
    if (len >= V_LEN)
    {
        // Keep continuation bits from the previous iteration that carry over to
        // each input chunk vector

        // We need a vector of the input byte stream shifted forward one byte.
        // Since we don't want to read the memory before the data pointer
        // (which might not even be mapped), for the first chunk of input just
        // use vector instructions.
        bytes0 = v_load(data0);
        bytes1 = v_load(data1);
        shifted_bytes0 = v_shift_lanes_left(bytes0);
        shifted_bytes1 = v_shift_lanes_left(bytes1);

        // Loop over input in V_LEN-byte chunks, as long as we can safely read
        // that far into memory
        while (len >= 2 * V_LEN)
        {
            if (!z_dvalidate_vec(bytes0, shifted_bytes0, &last_cont0, bytes1, shifted_bytes1, &last_cont1))
                return false;
            len -= V_LEN;
            shifted_bytes0 = v_load(data0 + V_LEN - 1);
            shifted_bytes1 = v_load(data1 + V_LEN - 1);
            data0 += V_LEN;
            data1 += V_LEN;
            bytes0 = v_load(data0);
            bytes1 = v_load(data1);
        }
        if (data0 + V_LEN <= end0)
        {
            if (!z_validate_vec(bytes0, shifted_bytes0, &last_cont0))
                return false;
            data0 += V_LEN;
        }
        if (data1 + V_LEN <= end1)
        {
            if (!z_validate_vec(bytes1, shifted_bytes1, &last_cont1))
                return false;
            data1 += V_LEN;
        }
    }
    while (data0 + V_LEN <= end0)
    {
        bytes0 = v_load(data0);
        shifted_bytes0 = v_shift_lanes_left(bytes0);
        if (!z_validate_vec(bytes0, shifted_bytes0, &last_cont0))
            return false;
        data0 += V_LEN;
    }
    while (data1 + V_LEN <= end1)
    {
        bytes1 = v_load(data1);
        shifted_bytes1 = v_shift_lanes_left(bytes1);
        if (!z_validate_vec(bytes1, shifted_bytes1, &last_cont1))
            return false;
        data1 += V_LEN;
    }

    // Deal with any bytes remaining. Rather than making a separate scalar path,
    // just fill in a buffer, reading bytes only up to len, and load from that.
    if (data0 < end0)
    {
        char buffer[V_LEN + 1] = { 0 };
        unsigned i = 1;
        if (data0 - data)
            buffer[0] = data0[-1];
        while (data0 < end0)
            buffer[i++] = *data0++;

        bytes0 = v_load(buffer + 1);
        shifted_bytes0 = v_load(buffer);
        if (!z_validate_vec(bytes0, shifted_bytes0, &last_cont0))
            return false;
    }
    if (data1 < end1)
    {
        char buffer[V_LEN + 1] = { 0 };
        unsigned i = 1;
        if (data1 - data)
            buffer[0] = data1[-1];
        while (data1 < end1)
            buffer[i++] = *data1++;

        bytes1 = v_load(buffer + 1);
        shifted_bytes1 = v_load(buffer);
        if (!z_validate_vec(bytes1, shifted_bytes1, &last_cont1))
            return false;
    }

    // The input is valid if we don't have any more expected continuation bytes
    return last_cont0 == 0 && last_cont1 == 0;
}

bool z_tvalidate_utf8(const uint8_t *data, uint32_t len)
{
    vec_t bytes0, shifted_bytes0;
    vec_t bytes1, shifted_bytes1;
    vec_t bytes2, shifted_bytes2;

    if (V_LEN <= len)
    {
        // quickly filter out ascii7 (trimm both sides of the input buffer)
        while (2 * V_LEN <= len)
        {
            bytes0 = v_load(data);
            bytes1 = v_load(data + len - V_LEN);
            if (v_test_bit7(v_or(bytes0, bytes1))) break;  // not ascii7
            data += V_LEN;
            len -= 2 * V_LEN;
        }
        while (V_LEN <= len)
        {
            bytes0 = v_load(data);
            if (v_test_bit7(bytes0)) break;  // not ascii7
            data += V_LEN;
            len -= V_LEN;
        }
    }


    const uint8_t *end2 = data + len;
    const uint8_t *data2 = utf8MidBoundary(data, end2, 3);
    const uint8_t *end1 = data2;
    const uint8_t *data1 = utf8MidBoundary(data, end1, 2);
    const uint8_t *end0 = data1;
    const uint8_t *data0 = data;

    // find the smallest buffer
    uint32_t len0 = (uint32_t)(end0 - data0);
    uint32_t len1 = (uint32_t)(end1 - data1);
    uint32_t len2 = (uint32_t)(end2 - data2);
    len = len0 < len1 ? len0 : len1;
    len = len < len2 ? len : len2;

    vmask_t last_cont0 = 0;
    vmask_t last_cont1 = 0;
    vmask_t last_cont2 = 0;

    // Deal with the input up until the last section of bytes
    if (len >= V_LEN)
    {
        // Keep continuation bits from the previous iteration that carry over to
        // each input chunk vector

        // We need a vector of the input byte stream shifted forward one byte.
        // Since we don't want to read the memory before the data pointer
        // (which might not even be mapped), for the first chunk of input just
        // use vector instructions.
        bytes0 = v_load(data0);
        bytes1 = v_load(data1);
        bytes2 = v_load(data2);
        shifted_bytes0 = v_shift_lanes_left(bytes0);
        shifted_bytes1 = v_shift_lanes_left(bytes1);
        shifted_bytes2 = v_shift_lanes_left(bytes2);

        // Loop over input in V_LEN-byte chunks, as long as we can safely read
        // that far into memory
        while (len >= 2 * V_LEN)
        {
            if (!z_tvalidate_vec(bytes0, shifted_bytes0, &last_cont0, bytes1, shifted_bytes1, &last_cont1, bytes2, shifted_bytes2, &last_cont2))
                return false;
            len -= V_LEN;
            shifted_bytes0 = v_load(data0 + V_LEN - 1);
            shifted_bytes1 = v_load(data1 + V_LEN - 1);
            shifted_bytes2 = v_load(data2 + V_LEN - 1);
            data0 += V_LEN;
            data1 += V_LEN;
            data2 += V_LEN;
            bytes0 = v_load(data0);
            bytes1 = v_load(data1);
            bytes2 = v_load(data2);
        }
        if (data0 + V_LEN <= end0)
        {
            if (!z_validate_vec(bytes0, shifted_bytes0, &last_cont0))
                return false;
            data0 += V_LEN;
        }
        if (data1 + V_LEN <= end1)
        {
            if (!z_validate_vec(bytes1, shifted_bytes1, &last_cont1))
                return false;
            data1 += V_LEN;
        }
        if (data2 + V_LEN <= end2)
        {
            if (!z_validate_vec(bytes2, shifted_bytes2, &last_cont2))
                return false;
            data2 += V_LEN;
        }
    }
    while (data0 + V_LEN <= end0)
    {
        bytes0 = v_load(data0);
        shifted_bytes0 = v_shift_lanes_left(bytes0);
        if (!z_validate_vec(bytes0, shifted_bytes0, &last_cont0))
            return false;
        data0 += V_LEN;
    }
    while (data1 + V_LEN <= end1)
    {
        bytes1 = v_load(data1);
        shifted_bytes1 = v_shift_lanes_left(bytes1);
        if (!z_validate_vec(bytes1, shifted_bytes1, &last_cont1))
            return false;
        data1 += V_LEN;
    }
    while (data2 + V_LEN <= end2)
    {
        bytes2 = v_load(data2);
        shifted_bytes2 = v_shift_lanes_left(bytes2);
        if (!z_validate_vec(bytes2, shifted_bytes2, &last_cont2))
            return false;
        data2 += V_LEN;
    }

    // Deal with any bytes remaining. Rather than making a separate scalar path,
    // just fill in a buffer, reading bytes only up to len, and load from that.
    if (data0 < end0)
    {
        char buffer[V_LEN + 1] = { 0 };
        unsigned i = 1;
        if (data0 - data)
            buffer[0] = data0[-1];
        while (data0 < end0)
            buffer[i++] = *data0++;

        bytes0 = v_load(buffer + 1);
        shifted_bytes0 = v_load(buffer);
        if (!z_validate_vec(bytes0, shifted_bytes0, &last_cont0))
            return false;
    }
    if (data1 < end1)
    {
        char buffer[V_LEN + 1] = { 0 };
        unsigned i = 1;
        if (data1 - data)
            buffer[0] = data1[-1];
        while (data1 < end1)
            buffer[i++] = *data1++;

        bytes1 = v_load(buffer + 1);
        shifted_bytes1 = v_load(buffer);
        if (!z_validate_vec(bytes1, shifted_bytes1, &last_cont1))
            return false;
    }
    if (data2 < end2)
    {
        char buffer[V_LEN + 1] = { 0 };
        unsigned i = 1;
        if (data2 - data)
            buffer[0] = data2[-1];
        while (data2 < end2)
            buffer[i++] = *data2++;

        bytes2 = v_load(buffer + 1);
        shifted_bytes2 = v_load(buffer);
        if (!z_validate_vec(bytes2, shifted_bytes2, &last_cont2))
            return false;
    }

    // The input is valid if we don't have any more expected continuation bytes
    return last_cont0 == 0 && last_cont1 == 0 && last_cont2 == 0;
}

// Undefine all macros

#undef INLINE
#undef z_validate_vec
#undef z_dvalidate_utf8
#undef z_dvalidate_vec
#undef z_tvalidate_utf8
#undef z_tvalidate_vec
#undef V_LEN
#undef vec_t
#undef vmask_t
#undef vmask2_t
#undef v_load
#undef v_set1
#undef v_and
#undef v_or
#undef v_test_bit
#undef v_test_bit7
#undef v_testz
#undef v_lookup
#undef v_lookup_no_shift
#undef V_TABLE_16
#undef v_shift_lanes_left

