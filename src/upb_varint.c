/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb_varint.h"

// Given an encoded varint v, returns an integer with a single bit set that
// indicates the end of the varint.  Subtracting one from this value will
// yield a mask that leaves only bits that are part of the varint.  Returns
// 0 if the varint is unterminated.
INLINE uint64_t upb_get_vstopbit(uint64_t v) {
  uint64_t cbits = v | 0x7f7f7f7f7f7f7f7fULL;
  return ~cbits & (cbits+1);
}
INLINE uint64_t upb_get_vmask(uint64_t v) { return upb_get_vstopbit(v) - 1; }

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

upb_decoderet upb_vdecode_max8_wright(upb_decoderet r) {
  uint64_t b;
  memcpy(&b, r.p, sizeof(b));
  uint64_t stop_bit = upb_get_vstopbit(b);
  b &= (stop_bit - 1);
  b = ((b & 0x7f007f007f007f00) >> 1) | (b & 0x007f007f007f007f);
  b = ((b & 0xffff0000ffff0000) >> 2) | (b & 0x0000ffff0000ffff);
  b = ((b & 0xffffffff00000000) >> 4) | (b & 0x00000000ffffffff);
  if (stop_bit == 0) {
    // Error: unterminated varint.
    upb_decoderet err_r = {(void*)0, 0};
    return err_r;
  }
  upb_decoderet my_r = {r.p + ((__builtin_ctzll(stop_bit) + 1) / 8),
                        r.val | (b << 14)};
  return my_r;
}
