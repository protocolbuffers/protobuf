// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_SWAP_H_
#define UPB_WIRE_INTERNAL_SWAP_H_

#include <stdint.h>

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE bool _upb_IsLittleEndian(void) {
  int x = 1;
  return *(char*)&x == 1;
}

UPB_INLINE uint32_t _upb_BigEndian_Swap32(uint32_t val) {
  if (_upb_IsLittleEndian()) return val;

  return ((val & 0xff) << 24) | ((val & 0xff00) << 8) |
         ((val & 0xff0000) >> 8) | ((val & 0xff000000) >> 24);
}

UPB_INLINE uint64_t _upb_BigEndian_Swap64(uint64_t val) {
  if (_upb_IsLittleEndian()) return val;

  return ((uint64_t)_upb_BigEndian_Swap32((uint32_t)val) << 32) |
         _upb_BigEndian_Swap32((uint32_t)(val >> 32));
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_INTERNAL_SWAP_H_ */
