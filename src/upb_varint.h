/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A number of routines for varint manipulation (we keep them all around to
 * have multiple approaches available for benchmarking).
 */

#ifndef UPB_VARINT_DECODER_H_
#define UPB_VARINT_DECODER_H_

#include "upb.h"
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Decoding *******************************************************************/

// All decoding functions return this struct by value.
typedef struct {
  const char *p;  // NULL if the varint was unterminated.
  uint64_t val;
} upb_decoderet;

// A basic branch-based decoder, uses 32-bit values to get good performance
// on 32-bit architectures (but performs well on 64-bits also).
INLINE upb_decoderet upb_vdecode_branch32(const char *p) {
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
INLINE upb_decoderet upb_vdecode_branch64(const char *p) {
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

// Decodes a varint of at most 8 bytes without branching (except for error).
upb_decoderet upb_vdecode_max8_wright(upb_decoderet r);

// Another implementation of the previous.
upb_decoderet upb_vdecode_max8_massimino(upb_decoderet r);

// Template for a function that checks the first two bytes with branching
// and dispatches 2-10 bytes with a separate function.
#define UPB_VARINT_DECODER_CHECK2(name, decode_max8_function)                \
INLINE upb_decoderet upb_vdecode_check2_ ## name(const char *_p) {           \
  uint8_t *p = (uint8_t*)_p;                                                 \
  if ((*p & 0x80) == 0) { upb_decoderet r = {_p + 1, *p & 0x7f}; return r; } \
  upb_decoderet r = {_p + 2, (*p & 0x7f) | ((*(p + 1) & 0x7f) << 7)};        \
  if ((*(p + 1) & 0x80) == 0) return r;                                      \
  return decode_max8_function(r);                                            \
}

UPB_VARINT_DECODER_CHECK2(wright, upb_vdecode_max8_wright);
UPB_VARINT_DECODER_CHECK2(massimino, upb_vdecode_max8_massimino);
#undef UPB_VARINT_DECODER_CHECK2

// Our canonical functions for decoding varints, based on the currently
// favored best-performing implementations.
INLINE upb_decoderet upb_vdecode_fast(const char *p) {
  // Use nobranch2 on 64-bit, branch32 on 32-bit.
  if (sizeof(long) == 8)
    return upb_vdecode_check2_massimino(p);
  else
    return upb_vdecode_branch32(p);
}

INLINE upb_decoderet upb_vdecode_max8_fast(upb_decoderet r) {
  return upb_vdecode_max8_massimino(r);
}


/* Encoding *******************************************************************/

INLINE size_t upb_value_size(uint64_t val) {
#ifdef __GNUC__
  int high_bit = 63 - __builtin_clzll(val);  // 0-based, undef if val == 0.
#else
  int high_bit = 0;
  uint64_t tmp = val;
  while(tmp >>= 1) high_bit++;
#endif
  return val == 0 ? 1 : high_bit / 8 + 1;
}

// Encodes a 32-bit varint, *not* sign-extended.
INLINE uint64_t upb_vencode32(uint32_t val) {
  uint64_t ret = 0;
  for (int bitpos = 0; val; bitpos+=8, val >>=7) {
    if (bitpos > 0) ret |= (1 << (bitpos-1));
    ret |= (val & 0x7f) << bitpos;
  }
  return ret;
}


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_VARINT_DECODER_H_ */
