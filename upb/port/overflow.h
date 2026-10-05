// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PORT_OVERFLOW_H_
#define UPB_PORT_OVERFLOW_H_

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

// Must be last
#include "upb/port/def.inc"

#if ((UPB_HAS_BUILTIN(__builtin_add_overflow) &&  \
      UPB_HAS_BUILTIN(__builtin_mul_overflow)) || \
     (defined(__GNUC__) && __GNUC__ >= 5)) &&     \
    !defined(UPB_DISABLE_BUILTIN_OVERFLOW)
#define UPB_USE_BUILTIN_OVERFLOW
#endif

UPB_NODISCARD UPB_INLINE bool upb_AddOverflow_size_t_size_t(size_t a, size_t b,
                                                            size_t* out) {
#ifdef UPB_USE_BUILTIN_OVERFLOW
  return __builtin_add_overflow(a, b, out);
#else
  if (b > SIZE_MAX - a) return true;
  *out = a + b;
  return false;
#endif
}

UPB_NODISCARD UPB_INLINE bool upb_AddOverflow_size_t_uint32_t(size_t a,
                                                              uint32_t b,
                                                              size_t* out) {
#ifdef UPB_USE_BUILTIN_OVERFLOW
  return __builtin_add_overflow(a, b, out);
#else
  return upb_AddOverflow_size_t_size_t(a, (size_t)b, out);
#endif
}

UPB_NODISCARD UPB_INLINE bool upb_AddOverflow_uint32_t_size_t(uint32_t a,
                                                              size_t b,
                                                              size_t* out) {
#ifdef UPB_USE_BUILTIN_OVERFLOW
  return __builtin_add_overflow(a, b, out);
#else
  return upb_AddOverflow_size_t_size_t((size_t)a, b, out);
#endif
}

UPB_NODISCARD UPB_INLINE bool upb_MulOverflow_size_t_size_t(size_t a, size_t b,
                                                            size_t* out) {
#ifdef UPB_USE_BUILTIN_OVERFLOW
  return __builtin_mul_overflow(a, b, out);
#else
  if (b != 0 && a > SIZE_MAX / b) return true;
  *out = a * b;
  return false;
#endif
}

UPB_NODISCARD UPB_INLINE bool upb_MulOverflow_size_t_uint32_t(size_t a,
                                                              uint32_t b,
                                                              size_t* out) {
#ifdef UPB_USE_BUILTIN_OVERFLOW
  return __builtin_mul_overflow(a, b, out);
#else
  return upb_MulOverflow_size_t_size_t(a, (size_t)b, out);
#endif
}

UPB_NODISCARD UPB_INLINE bool upb_MulOverflow_uint32_t_size_t(uint32_t a,
                                                              size_t b,
                                                              size_t* out) {
#ifdef UPB_USE_BUILTIN_OVERFLOW
  return __builtin_mul_overflow(a, b, out);
#else
  return upb_MulOverflow_size_t_size_t((size_t)a, b, out);
#endif
}

#if __STDC_VERSION__ >= 201112L && !defined(__cplusplus)
#if SIZE_MAX > 0xffffffffULL
#define upb_AddOverflow(a, b, out)                               \
  _Generic((a),                                                  \
      size_t: _Generic((b),                                      \
          size_t: upb_AddOverflow_size_t_size_t(a, b, out),      \
          uint32_t: upb_AddOverflow_size_t_uint32_t(a, b, out)), \
      uint32_t: _Generic((b),                                    \
          size_t: upb_AddOverflow_uint32_t_size_t(a, b, out)))

#define upb_MulOverflow(a, b, out)                               \
  _Generic((a),                                                  \
      size_t: _Generic((b),                                      \
          size_t: upb_MulOverflow_size_t_size_t(a, b, out),      \
          uint32_t: upb_MulOverflow_size_t_uint32_t(a, b, out)), \
      uint32_t: _Generic((b),                                    \
          size_t: upb_MulOverflow_uint32_t_size_t(a, b, out)))
#else
// On 32-bit platforms, size_t and uint32_t might be the same type.
// We fallback to direct calls without _Generic to avoid duplicate association
// errors.
#define upb_AddOverflow(a, b, out) upb_AddOverflow_size_t_size_t(a, b, out)
#define upb_MulOverflow(a, b, out) upb_MulOverflow_size_t_size_t(a, b, out)
#endif
#else
// Fallback for C++ or older C.
#define upb_AddOverflow(a, b, out) upb_AddOverflow_size_t_size_t(a, b, out)
#define upb_MulOverflow(a, b, out) upb_MulOverflow_size_t_size_t(a, b, out)
#endif

#include "upb/port/undef.inc"

#endif  // UPB_PORT_OVERFLOW_H_
