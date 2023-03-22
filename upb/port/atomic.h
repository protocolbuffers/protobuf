/*
 * Copyright (c) 2023, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPB_PORT_ATOMIC_H_
#define UPB_PORT_ATOMIC_H_

#include "upb/port/def.inc"

#ifdef UPB_USE_C11_ATOMICS

#include <stdatomic.h>
#include <stdbool.h>

UPB_INLINE void upb_Atomic_Init(_Atomic uintptr_t* addr, uintptr_t val) {
  atomic_init(addr, val);
}

UPB_INLINE uintptr_t upb_Atomic_LoadAcquire(_Atomic uintptr_t* addr) {
  return atomic_load_explicit(addr, memory_order_acquire);
}

UPB_INLINE void upb_Atomic_StoreRelaxed(_Atomic uintptr_t* addr,
                                        uintptr_t val) {
  atomic_store_explicit(addr, val, memory_order_relaxed);
}

UPB_INLINE void upb_Atomic_AddRelease(_Atomic uintptr_t* addr, uintptr_t val) {
  atomic_fetch_add_explicit(addr, val, memory_order_release);
}

UPB_INLINE void upb_Atomic_SubRelease(_Atomic uintptr_t* addr, uintptr_t val) {
  atomic_fetch_sub_explicit(addr, val, memory_order_release);
}

UPB_INLINE uintptr_t upb_Atomic_ExchangeAcqRel(_Atomic uintptr_t* addr,
                                               uintptr_t val) {
  return atomic_exchange_explicit(addr, val, memory_order_acq_rel);
}

UPB_INLINE bool upb_Atomic_CompareExchangeStrongAcqRel(_Atomic uintptr_t* addr,
                                                       uintptr_t* expected,
                                                       uintptr_t desired) {
  return atomic_compare_exchange_strong_explicit(
      addr, expected, desired, memory_order_release, memory_order_acquire);
}

#else  // !UPB_USE_C11_ATOMICS

UPB_INLINE void upb_Atomic_Init(uintptr_t* addr, uintptr_t val) { *addr = val; }

UPB_INLINE uintptr_t upb_Atomic_LoadAcquire(uintptr_t* addr) { return *addr; }

UPB_INLINE void upb_Atomic_StoreRelaxed(uintptr_t* addr, uintptr_t val) {
  *addr = val;
}

UPB_INLINE void upb_Atomic_AddRelease(uintptr_t* addr, uintptr_t val) {
  *addr += val;
}

UPB_INLINE void upb_Atomic_SubRelease(uintptr_t* addr, uintptr_t val) {
  *addr -= val;
}

UPB_INLINE uintptr_t upb_Atomic_ExchangeAcqRel(uintptr_t* addr, uintptr_t val) {
  uintptr_t ret = *addr;
  *addr = val;
  return ret;
}

UPB_INLINE bool upb_Atomic_CompareExchangeStrongAcqRel(uintptr_t* addr,
                                                       uintptr_t* expected,
                                                       uintptr_t desired) {
  if (*addr == *expected) {
    *addr = desired;
    return true;
  } else {
    *expected = *addr;
    return false;
  }
}

#endif

#include "upb/port/undef.inc"

#endif  // UPB_PORT_ATOMIC_H_
