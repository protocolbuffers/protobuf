#ifndef THIRD_PARTY_UPB_UPB_HASH_INTERNAL_HASH_H_
#define THIRD_PARTY_UPB_UPB_HASH_INTERNAL_HASH_H_

#include <stdint.h>
#include <string.h>

// Must be last.
#include "upb/port/def.inc"

/* Adapted from ABSL's wyhash. */

UPB_INLINE uint64_t _upb_UnalignedLoad64(const void* p) {
  uint64_t val;
  memcpy(&val, p, 8);
  return val;
}

UPB_INLINE uint32_t _upb_UnalignedLoad32(const void* p) {
  uint32_t val;
  memcpy(&val, p, 4);
  return val;
}

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif

/* Computes a * b, returning the low 64 bits of the result and storing the high
 * 64 bits in |*high|. */
UPB_INLINE uint64_t _upb_umul128(uint64_t v0, uint64_t v1, uint64_t* out_high) {
#ifdef __SIZEOF_INT128__
  __uint128_t p = v0;
  p *= v1;
  *out_high = (uint64_t)(p >> 64);
  return (uint64_t)p;
#elif defined(_MSC_VER) && defined(_M_X64)
  return _umul128(v0, v1, out_high);
#else
  uint64_t a32 = v0 >> 32;
  uint64_t a00 = v0 & 0xffffffff;
  uint64_t b32 = v1 >> 32;
  uint64_t b00 = v1 & 0xffffffff;
  uint64_t high = a32 * b32;
  uint64_t low = a00 * b00;
  uint64_t mid1 = a32 * b00;
  uint64_t mid2 = a00 * b32;
  low += (mid1 << 32) + (mid2 << 32);
  // Omit carry bit, for mixing we do not care about exact numerical precision.
  high += (mid1 >> 32) + (mid2 >> 32);
  *out_high = high;
  return low;
#endif
}

UPB_INLINE uint64_t _upb_WyhashMix(uint64_t v0, uint64_t v1) {
  uint64_t high;
  uint64_t low = _upb_umul128(v0, v1, &high);
  return low ^ high;
}

UPB_INLINE uint64_t _upb_Wyhash(const void* data, size_t len, uint64_t seed,
                                const uint64_t salt[]) {
  const uint8_t* ptr = (const uint8_t*)data;
  uint64_t starting_length = (uint64_t)len;
  uint64_t current_state = seed ^ salt[0];

  if (len > 64) {
    // If we have more than 64 bytes, we're going to handle chunks of 64
    // bytes at a time. We're going to build up two separate hash states
    // which we will then hash together.
    uint64_t duplicated_state = current_state;

    do {
      uint64_t a = _upb_UnalignedLoad64(ptr);
      uint64_t b = _upb_UnalignedLoad64(ptr + 8);
      uint64_t c = _upb_UnalignedLoad64(ptr + 16);
      uint64_t d = _upb_UnalignedLoad64(ptr + 24);
      uint64_t e = _upb_UnalignedLoad64(ptr + 32);
      uint64_t f = _upb_UnalignedLoad64(ptr + 40);
      uint64_t g = _upb_UnalignedLoad64(ptr + 48);
      uint64_t h = _upb_UnalignedLoad64(ptr + 56);

      uint64_t cs0 = _upb_WyhashMix(a ^ salt[1], b ^ current_state);
      uint64_t cs1 = _upb_WyhashMix(c ^ salt[2], d ^ current_state);
      current_state = (cs0 ^ cs1);

      uint64_t ds0 = _upb_WyhashMix(e ^ salt[3], f ^ duplicated_state);
      uint64_t ds1 = _upb_WyhashMix(g ^ salt[4], h ^ duplicated_state);
      duplicated_state = (ds0 ^ ds1);

      ptr += 64;
      len -= 64;
    } while (len > 64);

    current_state = current_state ^ duplicated_state;
  }

  // We now have a data `ptr` with at most 64 bytes and the current state
  // of the hashing state machine stored in current_state.
  while (len > 16) {
    uint64_t a = _upb_UnalignedLoad64(ptr);
    uint64_t b = _upb_UnalignedLoad64(ptr + 8);

    current_state = _upb_WyhashMix(a ^ salt[1], b ^ current_state);

    ptr += 16;
    len -= 16;
  }

  // We now have a data `ptr` with at most 16 bytes.
  uint64_t a = 0;
  uint64_t b = 0;
  if (len > 8) {
    // When we have at least 9 and at most 16 bytes, set A to the first 64
    // bits of the input and B to the last 64 bits of the input. Yes, they will
    // overlap in the middle if we are working with less than the full 16
    // bytes.
    a = _upb_UnalignedLoad64(ptr);
    b = _upb_UnalignedLoad64(ptr + len - 8);
  } else if (len > 3) {
    // If we have at least 4 and at most 8 bytes, set A to the first 32
    // bits and B to the last 32 bits.
    a = _upb_UnalignedLoad32(ptr);
    b = _upb_UnalignedLoad32(ptr + len - 4);
  } else if (len > 0) {
    // If we have at least 1 and at most 3 bytes, read all of the provided
    // bits into A, with some adjustments.
    a = ((ptr[0] << 16) | (ptr[len >> 1] << 8) | ptr[len - 1]);
    b = 0;
  } else {
    a = 0;
    b = 0;
  }

  uint64_t w = _upb_WyhashMix(a ^ salt[1], b ^ current_state);
  uint64_t z = salt[1] ^ starting_length;
  return _upb_WyhashMix(w, z);
}

static const uint64_t kWyhashSalt[5] = {
    0x243F6A8885A308D3ULL, 0x13198A2E03707344ULL, 0xA4093822299F31D0ULL,
    0x082EFA98EC4E6C89ULL, 0x452821E638D01377ULL,
};

#endif  // THIRD_PARTY_UPB_UPB_HASH_INTERNAL_HASH_H_
