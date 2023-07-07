#pragma once


#ifdef _MSC_VER
#define INLINE  __forceinline
#elif defined (__GNUC__)
#define INLINE __attribute__((always_inline)) inline
#else
#define INLINE inline
#endif

static INLINE const uint8_t *utf8MidBoundary(const uint8_t *start, const uint8_t *end, uint32_t ratio);

static INLINE const uint8_t *utf8MidBoundary(const uint8_t *start, const uint8_t *end, uint32_t ratio) {
    const uint8_t *mid = start + ((ratio - 1) * (end - start) + ratio - 1) / ratio;
    // search for a utf8 boundary in the middle of the buffer
    // make sure the second part of the buffer is proportionally the smallest
    while (mid != end) {
        uint32_t byte2 = *mid;
        if ((byte2 & 0x80) == 0x00) break;
        if ((byte2 & 0xc0) == 0xc0) break;
        mid++;
    }
    return mid;
}

#undef INLINE