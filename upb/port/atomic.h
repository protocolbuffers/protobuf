// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PORT_ATOMIC_H_
#define UPB_PORT_ATOMIC_H_

#include "upb/port/def.inc"

#ifdef UPB_USE_C11_ATOMICS

// IWYU pragma: begin_exports
#include <stdatomic.h>
#include <stdbool.h>
// IWYU pragma: end_exports

#define upb_Atomic_Init(addr, val) atomic_init(addr, val)
#define upb_Atomic_Load(addr, order) atomic_load_explicit(addr, order)
#define upb_Atomic_Store(addr, val, order) \
  atomic_store_explicit(addr, val, order)
#define upb_Atomic_Exchange(addr, val, order) \
  atomic_exchange_explicit(addr, val, order)
#define upb_Atomic_CompareExchangeStrong(addr, expected, desired,      \
                                         success_order, failure_order) \
  atomic_compare_exchange_strong_explicit(addr, expected, desired,     \
                                          success_order, failure_order)
#define upb_Atomic_CompareExchangeWeak(addr, expected, desired, success_order, \
                                       failure_order)                          \
  atomic_compare_exchange_weak_explicit(addr, expected, desired,               \
                                        success_order, failure_order)
#define upb_Atomic_Add(addr, val, order) \
  atomic_fetch_add_explicit(addr, val, order)
#define upb_Atomic_Sub(addr, val, order) \
  atomic_fetch_sub_explicit(addr, val, order)

#elif defined(UPB_USE_MSC_ATOMICS)
#include <intrin.h>
#include <stdbool.h>
#include <stdint.h>

#define upb_Atomic_Init(addr, val) (*(addr) = val)

#pragma intrinsic(_InterlockedExchange)
static int32_t upb_Atomic_LoadMsc32(int32_t volatile* addr) {
  // Compare exchange with an unlikely value reduces the risk of a spurious
  // (but harmless) store
  return _InterlockedCompareExchange(addr, 0xDEADC0DE, 0xDEADC0DE);
}

#pragma intrinsic(_InterlockedCompareExchange)
static bool upb_Atomic_CompareExchangeMscP32(int32_t volatile* addr,
                                             int32_t* expected,
                                             int32_t desired) {
  int32_t expect_val = *expected;
  int32_t actual_val = _InterlockedCompareExchange(addr, desired, expect_val);
  if (expect_val != actual_val) {
    *expected = actual_val;
    return false;
  }
  return true;
}

#if defined(_WIN64)
// MSVC, without C11 atomics, does not have any way in pure C to force
// load-acquire store-release behavior, so we hack it with exchanges.
#pragma intrinsic(_InterlockedCompareExchange64)
static uintptr_t upb_Atomic_LoadMsc64(uint64_t volatile* addr) {
  // Compare exchange with an unlikely value reduces the risk of a spurious
  // (but harmless) store
  return _InterlockedCompareExchange64(addr, 0xDEADC0DEBAADF00D,
                                       0xDEADC0DEBAADF00D);
}

#pragma intrinsic(_InterlockedCompareExchange64)
static bool upb_Atomic_CompareExchangeMscP64(uint64_t volatile* addr,
                                             uint64_t* expected,
                                             uint64_t desired) {
  uint64_t expect_val = *expected;
  uint64_t actual_val =
      _InterlockedCompareExchange64(addr, desired, expect_val);
  if (expect_val != actual_val) {
    *expected = actual_val;
    return false;
  }
  return true;
}

#pragma intrinsic(_InterlockedExchange64)
// If _Generic is available, use it to avoid emitting a "'uintptr_t' differs in
// levels of indirection from 'void *'" or -Wint-conversion compiler warning.
#if __STDC_VERSION__ >= 201112L
#define upb_Atomic_Store(addr, val, order)                            \
  _Generic(addr,                                                      \
      UPB_ATOMIC(uintptr_t)*: (void)_InterlockedExchange64(           \
               (uint64_t volatile*)(addr), (uint64_t)val),            \
      UPB_ATOMIC(int32_t)*: (void)_InterlockedExchange(               \
               (int32_t volatile*)(addr), (int32_t)val),              \
      default: (void)_InterlockedExchange64((uint64_t volatile*)addr, \
                                            (uint64_t)val))

#define upb_Atomic_Load(addr, order)                                         \
  _Generic(addr,                                                             \
      UPB_ATOMIC(uintptr_t)*: upb_Atomic_LoadMsc64(                          \
               (uint64_t volatile*)(addr)),                                  \
      UPB_ATOMIC(int32_t)*: upb_Atomic_LoadMsc32((int32_t volatile*)(addr)), \
      default: (void*)upb_Atomic_LoadMsc64((uint64_t volatile*)(addr)))

#define upb_Atomic_Exchange(addr, val, order)                               \
  _Generic(addr,                                                            \
      UPB_ATOMIC(uintptr_t)*: _InterlockedExchange64(                       \
               (uint64_t volatile*)(addr), (uint64_t)val),                  \
      UPB_ATOMIC(int32_t)*: _InterlockedExchange((int32_t volatile*)(addr), \
                                                 (int32_t)val),             \
      default: (void*)_InterlockedExchange64((uint64_t volatile*)addr,      \
                                             (uint64_t)val))

#define upb_Atomic_CompareExchangeStrong(addr, expected, desired,           \
                                         success_order, failure_order)      \
  _Generic(addr,                                                            \
      UPB_ATOMIC(int32_t)*: upb_Atomic_CompareExchangeMscP32(               \
               (int32_t volatile*)(addr), (int32_t*)expected,               \
               (int32_t)desired),                                           \
      default: upb_Atomic_CompareExchangeMscP64((uint64_t volatile*)(addr), \
                                                (uint64_t*)expected,        \
                                                (uint64_t)desired))

#define upb_Atomic_CompareExchangeWeak(addr, expected, desired, success_order, \
                                       failure_order)                          \
  _Generic(addr,                                                               \
      UPB_ATOMIC(int32_t)*: upb_Atomic_CompareExchangeMscP32(                  \
               (int32_t volatile*)(addr), (int32_t*)expected,                  \
               (int32_t)desired),                                              \
      default: upb_Atomic_CompareExchangeMscP64((uint64_t volatile*)(addr),    \
                                                (uint64_t*)expected,           \
                                                (uint64_t)desired))

#else

UPB_INLINE void _upb_Atomic_StoreP(void volatile* addr, uint64_t val,
                                   size_t size) {
  if (size == sizeof(int32_t)) {
    (void)_InterlockedExchange((int32_t volatile*)addr, (int32_t)val);
  } else {
    (void)_InterlockedExchange64((uint64_t volatile*)addr, val);
  }
}

#define upb_Atomic_Store(addr, val, order) \
  _upb_Atomic_StoreP(addr, val, sizeof(*addr))

UPB_INLINE int64_t _upb_Atomic_LoadP(void volatile* addr, size_t size) {
  if (size == sizeof(int32_t)) {
    return (int64_t)upb_Atomic_LoadMsc32((int32_t volatile*)addr);
  } else {
    return upb_Atomic_LoadMsc64((uint64_t volatile*)addr);
  }
}

#define upb_Atomic_Load(addr, order) \
  (void*)_upb_Atomic_LoadP((void volatile*)addr, sizeof(*addr))

UPB_INLINE int64_t _upb_Atomic_ExchangeP(void volatile* addr, uint64_t val,
                                         size_t size) {
  if (size == sizeof(int32_t)) {
    return (int64_t)_InterlockedExchange((int32_t volatile*)addr, (int32_t)val);
  } else {
    return (int64_t)_InterlockedExchange64((uint64_t volatile*)addr, val);
  }
}

#define upb_Atomic_Exchange(addr, val, order)                       \
  (void*)_upb_Atomic_ExchangeP((void volatile*)addr, (uint64_t)val, \
                               sizeof(*addr))

UPB_INLINE bool _upb_Atomic_CompareExchangeMscP(void volatile* addr,
                                                void* expected,
                                                uint64_t desired, size_t size) {
  if (size == sizeof(int32_t)) {
    return upb_Atomic_CompareExchangeMscP32(
        (int32_t volatile*)addr, (int32_t*)expected, (int32_t)desired);
  } else {
    return upb_Atomic_CompareExchangeMscP64((uint64_t volatile*)addr,
                                            (uint64_t*)expected, desired);
  }
}

#define upb_Atomic_CompareExchangeStrong(addr, expected, desired,      \
                                         success_order, failure_order) \
  _upb_Atomic_CompareExchangeMscP(addr, expected, (uint64_t)desired,   \
                                  sizeof(*addr))

#define upb_Atomic_CompareExchangeWeak(addr, expected, desired, success_order, \
                                       failure_order)                          \
  _upb_Atomic_CompareExchangeMscP(addr, expected, (uint64_t)desired,           \
                                  sizeof(*addr))

#endif

#else  // 32 bit pointers
#pragma intrinsic(_InterlockedExchange)
#define upb_Atomic_Store(addr, val, order) \
  (void)_InterlockedExchange((uint32_t volatile*)addr, (uint32_t)val)

// If _Generic is available, use it to avoid emitting 'uintptr_t' differs in
// levels of indirection from 'void *'
#if __STDC_VERSION__ >= 201112L
#define upb_Atomic_Load(addr, order)                                         \
  _Generic(addr,                                                             \
      UPB_ATOMIC(uintptr_t)*: (uintptr_t)upb_Atomic_LoadMsc32(               \
               (uint32_t volatile*)(addr)),                                  \
      UPB_ATOMIC(int32_t)*: upb_Atomic_LoadMsc32((int32_t volatile*)(addr)), \
      default: (void*)upb_Atomic_LoadMsc32((uint32_t volatile*)(addr)))

#define upb_Atomic_Exchange(addr, val, order)                                  \
  _Generic(addr,                                                               \
      UPB_ATOMIC(uintptr_t)*: _InterlockedExchange((uint32_t volatile*)(addr), \
                                                   (uint32_t)val),             \
      default: (void*)_InterlockedExchange((uint32_t volatile*)addr,           \
                                           (uint32_t)val))
#else
#define upb_Atomic_Load(addr, order) \
  (void*)upb_Atomic_LoadMsc32((uint32_t volatile*)(addr))

#define upb_Atomic_Exchange(addr, val, order) \
  (void*)_InterlockedExchange((uint32_t volatile*)addr, (uint32_t)val)
#endif

#define upb_Atomic_CompareExchangeStrong(addr, expected, desired,      \
                                         success_order, failure_order) \
  upb_Atomic_CompareExchangeMscP32((uint32_t volatile*)addr,           \
                                   (uint32_t*)expected, (uint32_t)desired)

#define upb_Atomic_CompareExchangeWeak(addr, expected, desired, success_order, \
                                       failure_order)                          \
  upb_Atomic_CompareExchangeMscP32((uint32_t volatile*)addr,                   \
                                   (uint32_t*)expected, (uint32_t)desired)
#endif

#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedExchangeAdd64)

// If _Generic is available, use it to switch between 32 and 64 bit types.
#if __STDC_VERSION__ >= 201112L
#define upb_Atomic_Add(addr, val, order)                                   \
  _Generic(addr,                                                           \
      UPB_ATOMIC(int64_t)*: _InterlockedExchangeAdd64(addr, (int64_t)val), \
      UPB_ATOMIC(int32_t)*: _InterlockedExchangeAdd(addr, (int32_t)val))
#define upb_Atomic_Sub(addr, val, order)                                    \
  _Generic(addr,                                                            \
      UPB_ATOMIC(int64_t)*: _InterlockedExchangeAdd64(addr, -(int64_t)val), \
      UPB_ATOMIC(int32_t)*: _InterlockedExchangeAdd(addr, -(int32_t)val))
#else
#define upb_Atomic_Add(addr, val, order)                                \
  sizeof(*addr) == sizeof(int32_t)                                      \
      ? _InterlockedExchangeAdd((uint32_t volatile*)addr, (int32_t)val) \
      : _InterlockedExchangeAdd64((uint64_t volatile*)addr, (int64_t)val)
#define upb_Atomic_Sub(addr, val, order)                                 \
  sizeof(*addr) == sizeof(int32_t)                                       \
      ? _InterlockedExchangeAdd((uint32_t volatile*)addr, -(int32_t)val) \
      : _InterlockedExchangeAdd64((uint64_t volatile*)addr, -(int64_t)val)
#endif

#else  // No atomics

#if !defined(UPB_SUPPRESS_MISSING_ATOMICS)
// NOLINTNEXTLINE
#error Your compiler does not support atomic instructions, which UPB uses. If you do not use UPB on multiple threads, you can suppress this error by defining UPB_SUPPRESS_MISSING_ATOMICS.
#endif

#include <string.h>

#define upb_Atomic_Init(addr, val) (*addr = val)
#define upb_Atomic_Load(addr, order) (*addr)
#define upb_Atomic_Store(addr, val, order) (*(addr) = val)

UPB_INLINE void* _upb_NonAtomic_Exchange(void* addr, void* value) {
  void* old;
  memcpy(&old, addr, sizeof(value));
  memcpy(addr, &value, sizeof(value));
  return old;
}

#define upb_Atomic_Exchange(addr, val, order) _upb_NonAtomic_Exchange(addr, val)

// `addr` and `expected` are logically double pointers.
UPB_INLINE bool _upb_NonAtomic_CompareExchangeStrongP(void* addr,
                                                      void* expected,
                                                      void* desired) {
  if (memcmp(addr, expected, sizeof(desired)) == 0) {
    memcpy(addr, &desired, sizeof(desired));
    return true;
  } else {
    memcpy(expected, addr, sizeof(desired));
    return false;
  }
}

#define upb_Atomic_CompareExchangeStrong(addr, expected, desired,      \
                                         success_order, failure_order) \
  _upb_NonAtomic_CompareExchangeStrongP((void*)addr, (void*)expected,  \
                                        (void*)desired)
#define upb_Atomic_CompareExchangeWeak(addr, expected, desired, success_order, \
                                       failure_order)                          \
  upb_Atomic_CompareExchangeStrong(addr, expected, desired, 0, 0)

#define upb_Atomic_Add(addr, val, order) (*addr += val)
#define upb_Atomic_Sub(addr, val, order) (*addr -= val)

#endif

#include "upb/port/undef.inc"

#endif  // UPB_PORT_ATOMIC_H_
