// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "python/free_threading/lazy_ptr.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef Py_GIL_DISABLED
void* PyUpb_LazyPtr_LazyInit(void* lazy_ptr, PyUpb_PtrInitFunc init_fn,
                             PyUpb_PtrFreeFunc free_fn, void* ctx) {
  UPB_ATOMIC(void*)* atomic_ptr = (UPB_ATOMIC(void*)*)lazy_ptr;
  void* ptr = upb_Atomic_Load(atomic_ptr, memory_order_acquire);
  if (ptr) return ptr;

  void* new_ptr = init_fn(ctx);
  if (!new_ptr) return NULL;

  void* expected = NULL;
  if (!upb_Atomic_CompareExchangeStrong(atomic_ptr, &expected, new_ptr,
                                        memory_order_release,
                                        memory_order_acquire)) {
    if (free_fn) free_fn(new_ptr);
    return expected;
  }
  return new_ptr;
}
#endif
