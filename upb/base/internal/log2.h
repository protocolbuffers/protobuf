// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
///
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_BASE_INTERNAL_LOG2_H_
#define UPB_BASE_INTERNAL_LOG2_H_

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE int upb_Log2Ceiling(size_t x) {
  if (x <= 1) return 0;
#if SIZE_MAX == ULL_MAX && UPB_HAS_BUILTIN(__builtin_clzll)
  return (sizeof(size_t) * CHAR_BIT) - __builtin_clzll(x - 1);
#elif SIZE_MAX == ULONG_MAX && UPB_HAS_BUILTIN(__builtin_clzl)
  return (sizeof(size_t) * CHAR_BIT) - __builtin_clzl(x - 1);
#elif SIZE_MAX == UINT_MAX && UPB_HAS_BUILTIN(__builtin_clz)
  return (sizeof(size_t) * CHAR_BIT) - __builtin_clz(x - 1);
#else
  int lg2 = 0;
  while ((1 << lg2) < x) lg2++;
  return lg2;
#endif
}

// Returns the smallest power of two that is greater than or equal to x. Returns
// SIZE_MAX if the computation would overflow.
UPB_INLINE size_t upb_RoundUpToPowerOfTwo(size_t x) {
  int lg2 = upb_Log2Ceiling(x);
  UPB_ASSERT(lg2 >= 0 && lg2 <= (int)sizeof(size_t) * CHAR_BIT);
  if (lg2 == sizeof(size_t) * CHAR_BIT) {
    return SIZE_MAX;
  }
  return ((size_t)1) << lg2;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_BASE_INTERNAL_LOG2_H_ */
