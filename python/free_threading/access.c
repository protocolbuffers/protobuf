// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file or at https://developers.google.com/open-source/licenses/bsd

#include "python/free_threading/access.h"

#include <assert.h>
#include <limits.h>

#include "upb/port/def.inc"

void PyUpb_Access_Init(PyUpb_Access* access) {
  upb_Atomic_Init(&access->state, 0);
}

bool PyUpb_Access_TryRead(PyUpb_Access* access) {
  int32_t state = upb_Atomic_Load(&access->state, memory_order_relaxed);
  for (;;) {
    if (state < 0 || state == INT32_MAX) return false;
    if (upb_Atomic_CompareExchangeWeak(&access->state, &state, state + 1,
                                     memory_order_acquire,
                                     memory_order_relaxed)) {
      return true;
    }
  }
}

bool PyUpb_Access_TryWrite(PyUpb_Access* access) {
  int32_t state = 0;
  return upb_Atomic_CompareExchangeStrong(&access->state, &state, -1,
                                         memory_order_acquire,
                                         memory_order_relaxed);
}

void PyUpb_Access_EndRead(PyUpb_Access* access) {
  int32_t previous = upb_Atomic_Sub(&access->state, 1, memory_order_release);
  assert(previous > 0);
  (void)previous;
}

void PyUpb_Access_EndWrite(PyUpb_Access* access) {
  int32_t previous = upb_Atomic_Exchange(&access->state, 0, memory_order_release);
  assert(previous == -1);
  (void)previous;
}

#include "upb/port/undef.inc"
