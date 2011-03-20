/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A number of routines for varint decoding (we keep them all around to have
 * multiple approaches available for benchmarking).  All of these functions
 * require the buffer to have at least 10 bytes available; if we don't know
 * for sure that there are 10 bytes, then there is only one viable option
 * (branching on every byte).
 */

#ifndef UPB_VARINT_DECODER_H_
#define UPB_VARINT_DECODER_H_

#include "upb.h"
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// All decoding functions return this struct by value.
typedef struct {
  const char *p;  // NULL if the varint was unterminated.
  uint64_t val;
} upb_decoderet;

// A basic branch-based decoder, uses 32-bit values to get good performance
// on 32-bit architectures (but performs well on 64-bits also).
INLINE upb_decoderet upb_decode_varint_branch32(const char *p) {
  upb_decoderet r = {NULL, 0};
  uint32_t low, high = 0;
  uint32_t b;
  b = *(p++); low   = (b & 0x7f)      ; if(!(b & 0x80)) goto done;
  b = *(p++); low  |= (b & 0x7f) <<  7; if(!(b & 0x80)) goto done;
  b = *(p++); low  |= (b & 0x7f) << 14; if(!(b & 0x80)) goto done;
  b = *(p++); low  |= (b & 0x7f) << 21; if(!(b & 0x80)) goto done;
  b = *(p++); low  |= (b & 0x7f) << 28;
              high  = (b & 0x7f) >>  4; if(!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7f) <<  3; if(!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7f) << 10; if(!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7f) << 17; if(!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7f) << 24; if(!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7f) << 31; if(!(b & 0x80)) goto done;
  return r;

done:
  r.val = ((uint64_t)high << 32) | low;
  r.p = p;
  return r;
}

// Like the previous, but uses 64-bit values.
INLINE upb_decoderet upb_decode_varint_branch64(const char *p) {
  uint64_t val;
  uint64_t b;
  upb_decoderet r = {(void*)0, 0};
  b = *(p++); val  = (b & 0x7f)      ; if(!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7f) <<  7; if(!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7f) << 14; if(!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7f) << 21; if(!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7f) << 28; if(!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7f) << 35; if(!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7f) << 42; if(!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7f) << 49; if(!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7f) << 56; if(!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7f) << 63; if(!(b & 0x80)) goto done;
  return r;

done:
  r.val = val;
  r.p = p;
  return r;
}

#ifdef __SSE__

#include <xmmintrin.h>

// Avoids branches (this can very likely be improved).  Requires SSE.
INLINE upb_decoderet upb_decode_varint_nobranch(const char *p) {
  upb_decoderet r = {(void*)0, 0};
  __m128i val128 = _mm_loadu_si128((void*)p);
  unsigned int continuation_bits = _mm_movemask_epi8(val128);
  unsigned int bsr_val = ~continuation_bits;
  int varint_length = __builtin_ffs(bsr_val);
  if (varint_length > 10) return r;

  uint16_t twob;
  memcpy(&twob, p, 2);
  twob &= 0x7f7f;
  twob = ((twob & 0xff00) >> 1) | (twob & 0xff);

  uint64_t eightb;
  memcpy(&eightb, p + 2, 8);
  eightb &= 0x7f7f7f7f7f7f7f7f;
  eightb = ((eightb & 0xff00ff00ff00ff00) >> 1) | (eightb & 0x00ff00ff00ff00ff);
  eightb = ((eightb & 0xffff0000ffff0000) >> 2) | (eightb & 0x0000ffff0000ffff);
  eightb = ((eightb & 0xffffffff00000000) >> 4) | (eightb & 0x00000000ffffffff);

  uint64_t all_bits = twob | (eightb << 14);
  int varint_bits = varint_length * 7;
  uint64_t mask = varint_bits == 70 ? (uint64_t)-1 : (1ULL << (varint_bits)) - 1;
  r.val = all_bits & mask;
  r.p = p + varint_length;
  return r;
}

#endif

// For now, always use the branch32 decoder.
#define upb_decode_varint_fast upb_decode_varint_branch32

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_VARINT_DECODER_H_ */
