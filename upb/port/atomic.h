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
#define upb_Atomic_LoadAcquire(addr) \
  atomic_load_explicit(addr, memory_order_acquire)
#define upb_Atomic_LoadRelaxed(addr) \
  atomic_load_explicit(addr, memory_order_relaxed)
#define upb_Atomic_StoreRelaxed(addr, val) \
  atomic_store_explicit(addr, val, memory_order_relaxed)
#define upb_Atomic_StoreRelease(addr, val) \
  atomic_store_explicit(addr, val, memory_order_release)
#define upb_Atomic_Add(addr, val, order) \
  atomic_fetch_add_explicit(addr, val, order)
#define upb_Atomic_Sub(addr, val, order) \
  atomic_fetch_sub_explicit(addr, val, order)
#define upb_Atomic_ExchangeRelaxed(addr, val) \
  atomic_exchange_explicit(addr, val, memory_order_relaxed)
#define upb_Atomic_CompareExchangeStrongRelaxedRelaxed(addr, expected, \
                                                       desired)        \
  atomic_compare_exchange_strong_explicit(                             \
      addr, expected, desired, memory_order_relaxed, memory_order_relaxed)
#define upb_Atomic_CompareExchangeStrongReleaseAcquire(addr, expected, \
                                                       desired)        \
  atomic_compare_exchange_strong_explicit(                             \
      addr, expected, desired, memory_order_release, memory_order_acquire)
#define upb_Atomic_CompareExchangeWeakReleaseAcquire(addr, expected, desired) \
  atomic_compare_exchange_weak_explicit(                                      \
      addr, expected, desired, memory_order_release, memory_order_acquire)

#else  // !UPB_USE_C11_ATOMICS

#include <string.h>

#define upb_Atomic_Init(addr, val) (*addr = val)
#define upb_Atomic_LoadAcquire(addr) (*addr)
#define upb_Atomic_LoadRelaxed(addr) (*addr)
#define upb_Atomic_StoreRelaxed(addr, val) (*(addr) = val)
#define upb_Atomic_StoreRelease(addr, val) (*(addr) = val)
#define upb_Atomic_Add(addr, val, order) (*(addr) += val)
#define upb_Atomic_Sub(addr, val, order) (*(addr) -= val)

UPB_INLINE void* _upb_NonAtomic_Exchange(void* addr, void* value) {
  void* old;
  memcpy(&old, addr, sizeof(value));
  memcpy(addr, &value, sizeof(value));
  return old;
}

#define upb_Atomic_ExchangeRelaxed(addr, val) _upb_NonAtomic_Exchange(addr, val)

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

#define upb_Atomic_CompareExchangeStrongRelaxedRelaxed(addr, expected, \
                                                       desired)        \
  _upb_NonAtomic_CompareExchangeStrongP((void*)addr, (void*)expected,  \
                                        (void*)desired)
#define upb_Atomic_CompareExchangeStrongReleaseAcquire(addr, expected, \
                                                       desired)        \
  _upb_NonAtomic_CompareExchangeStrongP((void*)addr, (void*)expected,  \
                                        (void*)desired)
#define upb_Atomic_CompareExchangeWeakReleaseAcquire(addr, expected, desired) \
  upb_Atomic_CompareExchangeStrongReleaseAcquire(addr, expected, desired)

#endif

#include "upb/port/undef.inc"

#endif  // UPB_PORT_ATOMIC_H_
