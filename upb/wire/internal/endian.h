// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_ENDIAN_H_
#define UPB_WIRE_INTERNAL_ENDIAN_H_

#include <stdint.h>

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE bool UPB_PRIVATE(_upb_IsLittleEndian)(void) {
  const int x = 1;
  return *(char*)&x == 1;
}

UPB_INLINE uint32_t UPB_PRIVATE(_upb_BigEndian32)(uint32_t val) {
  if (UPB_PRIVATE(_upb_IsLittleEndian)()) return val;

  return ((val & 0xff) << 24) | ((val & 0xff00) << 8) |
         ((val & 0xff0000) >> 8) | ((val & 0xff000000) >> 24);
}

UPB_INLINE uint64_t UPB_PRIVATE(_upb_BigEndian64)(uint64_t val) {
  if (UPB_PRIVATE(_upb_IsLittleEndian)()) return val;

  return ((uint64_t)UPB_PRIVATE(_upb_BigEndian32)((uint32_t)val) << 32) |
         UPB_PRIVATE(_upb_BigEndian32)((uint32_t)(val >> 32));
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_INTERNAL_ENDIAN_H_ */
