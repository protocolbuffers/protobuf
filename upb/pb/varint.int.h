/*
** A number of routines for varint manipulation (we keep them all around to
** have multiple approaches available for benchmarking).
*/

#ifndef UPB_VARINT_DECODER_H_
#define UPB_VARINT_DECODER_H_

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "upb/upb.h"

#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

#define UPB_MAX_WIRE_TYPE 5

/* The maximum number of bytes that it takes to encode a 64-bit varint. */
#define UPB_PB_VARINT_MAX_LEN 10

/* Array of the "native" (ie. non-packed-repeated) wire type for the given a
 * descriptor type (upb_descriptortype_t). */
extern const uint8_t upb_pb_native_wire_types[];

UPB_INLINE uint64_t byteswap64(uint64_t val)
{
  return ((((val) & 0xff00000000000000ull) >> 56)
    | (((val) & 0x00ff000000000000ull) >> 40)
    | (((val) & 0x0000ff0000000000ull) >> 24)
    | (((val) & 0x000000ff00000000ull) >> 8)
    | (((val) & 0x00000000ff000000ull) << 8)
    | (((val) & 0x0000000000ff0000ull) << 24)
    | (((val) & 0x000000000000ff00ull) << 40)
    | (((val) & 0x00000000000000ffull) << 56));
}

/* Zig-zag encoding/decoding **************************************************/

UPB_INLINE int32_t upb_zzdec_32(uint32_t n) {
  return (n >> 1) ^ -(int32_t)(n & 1);
}
UPB_INLINE int64_t upb_zzdec_64(uint64_t n) {
  return (n >> 1) ^ -(int64_t)(n & 1);
}
UPB_INLINE uint32_t upb_zzenc_32(int32_t n) { return (n << 1) ^ (n >> 31); }
UPB_INLINE uint64_t upb_zzenc_64(int64_t n) { return (n << 1) ^ (n >> 63); }

/* Decoding *******************************************************************/

/* All decoding functions return this struct by value. */
typedef struct {
  const char *p;  /* NULL if the varint was unterminated. */
  uint64_t val;
} upb_decoderet;

UPB_INLINE upb_decoderet upb_decoderet_make(const char *p, uint64_t val) {
  upb_decoderet ret;
  ret.p = p;
  ret.val = val;
  return ret;
}

upb_decoderet upb_vdecode_max8_branch32(upb_decoderet r);
upb_decoderet upb_vdecode_max8_branch64(upb_decoderet r);

/* Template for a function that checks the first two bytes with branching
 * and dispatches 2-10 bytes with a separate function.  Note that this may read
 * up to 10 bytes, so it must not be used unless there are at least ten bytes
 * left in the buffer! */
#define UPB_VARINT_DECODER_CHECK2(name, decode_max8_function)                  \
UPB_INLINE upb_decoderet upb_vdecode_check2_ ## name(const char *_p) {         \
  uint8_t *p = (uint8_t*)_p;                                                   \
  upb_decoderet r;                                                             \
  if ((*p & 0x80) == 0) {                                                      \
  /* Common case: one-byte varint. */                                          \
    return upb_decoderet_make(_p + 1, *p & 0x7fU);                             \
  }                                                                            \
  r = upb_decoderet_make(_p + 2, (*p & 0x7fU) | ((*(p + 1) & 0x7fU) << 7));    \
  if ((*(p + 1) & 0x80) == 0) {                                                \
    /* Two-byte varint. */                                                     \
    return r;                                                                  \
  }                                                                            \
  /* Longer varint, fallback to out-of-line function. */                       \
  return decode_max8_function(r);                                              \
}

UPB_VARINT_DECODER_CHECK2(branch32, upb_vdecode_max8_branch32)
UPB_VARINT_DECODER_CHECK2(branch64, upb_vdecode_max8_branch64)
#undef UPB_VARINT_DECODER_CHECK2

/* Our canonical functions for decoding varints, based on the currently
 * favored best-performing implementations. */
UPB_INLINE upb_decoderet upb_vdecode_fast(const char *p) {
  if (sizeof(long) == 8)
    return upb_vdecode_check2_branch64(p);
  else
    return upb_vdecode_check2_branch32(p);
}


/* Encoding *******************************************************************/

UPB_INLINE int upb_value_size(uint64_t val) {
#ifdef __GNUC__
  int high_bit = 63 - __builtin_clzll(val);  /* 0-based, undef if val == 0. */
#else
  int high_bit = 0;
  uint64_t tmp = val;
  while(tmp >>= 1) high_bit++;
#endif
  return val == 0 ? 1 : high_bit / 8 + 1;
}

/* Encodes a 64-bit varint into buf (which must be >=UPB_PB_VARINT_MAX_LEN
 * bytes long), returning how many bytes were used.
 *
 * TODO: benchmark and optimize if necessary. */
UPB_INLINE size_t upb_vencode64(uint64_t val, char *buf) {
  size_t i;
  if (val == 0) { buf[0] = 0; return 1; }
  i = 0;
  while (val) {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  }
  return i;
}

UPB_INLINE size_t upb_varint_size(uint64_t val) {
  char buf[UPB_PB_VARINT_MAX_LEN];
  return upb_vencode64(val, buf);
}

/* Encodes a 32-bit varint, *not* sign-extended. */
UPB_INLINE uint64_t upb_vencode32(uint32_t val) {
  char buf[UPB_PB_VARINT_MAX_LEN];
  size_t bytes = upb_vencode64(val, buf);
  uint64_t ret = 0;
  UPB_ASSERT(bytes <= 5);
  memcpy(&ret, buf, bytes);
#ifdef UPB_BIG_ENDIAN
  ret = byteswap64(ret);
#endif
  UPB_ASSERT(ret <= 0xffffffffffU);
  return ret;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif  /* UPB_VARINT_DECODER_H_ */
