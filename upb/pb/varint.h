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

#include <stdint.h>
#include <string.h>
#include "upb/upb.h"

#ifdef __cplusplus
extern "C" {
#endif

// A list of types as they are encoded on-the-wire.
typedef enum {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5,
} upb_wiretype_t;

// The maximum number of bytes that it takes to encode a 64-bit varint.
// Note that with a better encoding this could be 9 (TODO: write up a
// wiki document about this).
#define UPB_PB_VARINT_MAX_LEN 10

/* Zig-zag encoding/decoding **************************************************/

INLINE int32_t upb_zzdec_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
INLINE int64_t upb_zzdec_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }
INLINE uint32_t upb_zzenc_32(int32_t n) { return (n << 1) ^ (n >> 31); }
INLINE uint64_t upb_zzenc_64(int64_t n) { return (n << 1) ^ (n >> 63); }

/* Decoding *******************************************************************/

// All decoding functions return this struct by value.
typedef struct {
  const char *p;  // NULL if the varint was unterminated.
  uint64_t val;
} upb_decoderet;

// Four functions for decoding a varint of at most eight bytes.  They are all
// functionally identical, but are implemented in different ways and likely have
// different performance profiles.  We keep them around for performance testing.
//
// Note that these functions may not read byte-by-byte, so they must not be used
// unless there are at least eight bytes left in the buffer!
upb_decoderet upb_vdecode_max8_branch32(upb_decoderet r);
upb_decoderet upb_vdecode_max8_branch64(upb_decoderet r);
upb_decoderet upb_vdecode_max8_wright(upb_decoderet r);
upb_decoderet upb_vdecode_max8_massimino(upb_decoderet r);

// Template for a function that checks the first two bytes with branching
// and dispatches 2-10 bytes with a separate function.  Note that this may read
// up to 10 bytes, so it must not be used unless there are at least ten bytes
// left in the buffer!
#define UPB_VARINT_DECODER_CHECK2(name, decode_max8_function)                  \
INLINE upb_decoderet upb_vdecode_check2_ ## name(const char *_p) {             \
  uint8_t *p = (uint8_t*)_p;                                                   \
  if ((*p & 0x80) == 0) { upb_decoderet r = {_p + 1, *p & 0x7fU}; return r; }  \
  upb_decoderet r = {_p + 2, (*p & 0x7fU) | ((*(p + 1) & 0x7fU) << 7)};        \
  if ((*(p + 1) & 0x80) == 0) return r;                                        \
  return decode_max8_function(r);                                              \
}

UPB_VARINT_DECODER_CHECK2(branch32, upb_vdecode_max8_branch32);
UPB_VARINT_DECODER_CHECK2(branch64, upb_vdecode_max8_branch64);
UPB_VARINT_DECODER_CHECK2(wright, upb_vdecode_max8_wright);
UPB_VARINT_DECODER_CHECK2(massimino, upb_vdecode_max8_massimino);
#undef UPB_VARINT_DECODER_CHECK2

// Our canonical functions for decoding varints, based on the currently
// favored best-performing implementations.
INLINE upb_decoderet upb_vdecode_fast(const char *p) {
  if (sizeof(long) == 8)
    return upb_vdecode_check2_massimino(p);
  else
    return upb_vdecode_check2_branch32(p);
}

INLINE upb_decoderet upb_vdecode_max8_fast(upb_decoderet r) {
  return upb_vdecode_max8_massimino(r);
}


/* Encoding *******************************************************************/

INLINE int upb_value_size(uint64_t val) {
#ifdef __GNUC__
  int high_bit = 63 - __builtin_clzll(val);  // 0-based, undef if val == 0.
#else
  int high_bit = 0;
  uint64_t tmp = val;
  while(tmp >>= 1) high_bit++;
#endif
  return val == 0 ? 1 : high_bit / 8 + 1;
}

// Encodes a 64-bit varint into buf (which must be >=UPB_PB_VARINT_MAX_LEN
// bytes long), returning how many bytes were used.
//
// TODO: benchmark and optimize if necessary.
INLINE size_t upb_vencode64(uint64_t val, char *buf) {
  if (val == 0) { buf[0] = 0; return 1; }
  size_t i = 0;
  while (val) {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  }
  return i;
}

// Encodes a 32-bit varint, *not* sign-extended.
INLINE uint64_t upb_vencode32(uint32_t val) {
  char buf[UPB_PB_VARINT_MAX_LEN];
  size_t bytes = upb_vencode64(val, buf);
  uint64_t ret = 0;
  assert(bytes <= 5);
  memcpy(&ret, buf, bytes);
  assert(ret <= 0xffffffffffU);
  return ret;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_VARINT_DECODER_H_ */
