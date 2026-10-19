// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC. All rights reserved.
// Use of this source code is governed by a BSD-style license in the LICENSE file.

#include "python/free_threading/access_test_util.h"

#include <stdlib.h>

PyUpb_Access* PyUpb_Access_NewForTest(void) {
  PyUpb_Access* access = malloc(sizeof(*access));
  if (access) PyUpb_Access_Init(access);
  return access;
}

void PyUpb_Access_FreeForTest(PyUpb_Access* access) { free(access); }

void PyUpb_Access_SetCountForTest(PyUpb_Access* access, int32_t count) {
  upb_Atomic_Store(&access->state, count, memory_order_relaxed);
}

int32_t PyUpb_Access_GetCountForTest(PyUpb_Access* access) {
  return upb_Atomic_Load(&access->state, memory_order_relaxed);
}
