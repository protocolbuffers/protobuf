/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/pb/varint.h"

// A basic branch-based decoder, uses 32-bit values to get good performance
// on 32-bit architectures (but performs well on 64-bits also).
// This scheme comes from the original Google Protobuf implementation (proto2).
upb_decoderet upb_vdecode_max8_branch32(upb_decoderet r) {
  upb_decoderet err = {NULL, 0};
  const char *p = r.p;
  uint32_t low = (uint32_t)r.val;
  uint32_t high = 0;
  uint32_t b;
  b = *(p++); low  |= (b & 0x7fU) << 14; if (!(b & 0x80)) goto done;
  b = *(p++); low  |= (b & 0x7fU) << 21; if (!(b & 0x80)) goto done;
  b = *(p++); low  |= (b & 0x7fU) << 28;
              high  = (b & 0x7fU) >>  4; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) <<  3; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 10; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 17; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 24; if (!(b & 0x80)) goto done;
  b = *(p++); high |= (b & 0x7fU) << 31; if (!(b & 0x80)) goto done;
  return err;

done:
  r.val = ((uint64_t)high << 32) | low;
  r.p = p;
  return r;
}

// Like the previous, but uses 64-bit values.
upb_decoderet upb_vdecode_max8_branch64(upb_decoderet r) {
  const char *p = r.p;
  uint64_t val = r.val;
  uint64_t b;
  upb_decoderet err = {NULL, 0};
  b = *(p++); val |= (b & 0x7fU) << 14; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 21; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 28; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 35; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 42; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 49; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 56; if (!(b & 0x80)) goto done;
  b = *(p++); val |= (b & 0x7fU) << 63; if (!(b & 0x80)) goto done;
  return err;

done:
  r.val = val;
  r.p = p;
  return r;
}

// Given an encoded varint v, returns an integer with a single bit set that
// indicates the end of the varint.  Subtracting one from this value will
// yield a mask that leaves only bits that are part of the varint.  Returns
// 0 if the varint is unterminated.
static uint64_t upb_get_vstopbit(uint64_t v) {
  uint64_t cbits = v | 0x7f7f7f7f7f7f7f7fULL;
  return ~cbits & (cbits+1);
}

// A branchless decoder.  Credit to Pascal Massimino for the bit-twiddling.
upb_decoderet upb_vdecode_max8_massimino(upb_decoderet r) {
  uint64_t b;
  memcpy(&b, r.p, sizeof(b));
  uint64_t stop_bit = upb_get_vstopbit(b);
  b =  (b & 0x7f7f7f7f7f7f7f7fULL) & (stop_bit - 1);
  b +=       b & 0x007f007f007f007fULL;
  b +=  3 * (b & 0x0000ffff0000ffffULL);
  b += 15 * (b & 0x00000000ffffffffULL);
  if (stop_bit == 0) {
    // Error: unterminated varint.
    upb_decoderet err_r = {(void*)0, 0};
    return err_r;
  }
  upb_decoderet my_r = {r.p + ((__builtin_ctzll(stop_bit) + 1) / 8),
                        r.val | (b << 7)};
  return my_r;
}

// A branchless decoder.  Credit to Daniel Wright for the bit-twiddling.
upb_decoderet upb_vdecode_max8_wright(upb_decoderet r) {
  uint64_t b;
  memcpy(&b, r.p, sizeof(b));
  uint64_t stop_bit = upb_get_vstopbit(b);
  b &= (stop_bit - 1);
  b = ((b & 0x7f007f007f007f00ULL) >> 1) | (b & 0x007f007f007f007fULL);
  b = ((b & 0xffff0000ffff0000ULL) >> 2) | (b & 0x0000ffff0000ffffULL);
  b = ((b & 0xffffffff00000000ULL) >> 4) | (b & 0x00000000ffffffffULL);
  if (stop_bit == 0) {
    // Error: unterminated varint.
    upb_decoderet err_r = {(void*)0, 0};
    return err_r;
  }
  upb_decoderet my_r = {r.p + ((__builtin_ctzll(stop_bit) + 1) / 8),
                        r.val | (b << 14)};
  return my_r;
}
