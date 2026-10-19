// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC. All rights reserved.
// Use of this source code is governed by a BSD-style license in the LICENSE file.

#ifndef PYUPB_FREE_THREADING_ACCESS_TEST_UTIL_H_
#define PYUPB_FREE_THREADING_ACCESS_TEST_UTIL_H_
#include "python/free_threading/access.h"
#ifdef __cplusplus
extern "C" {
#endif
PyUpb_Access* PyUpb_Access_NewForTest(void);
void PyUpb_Access_FreeForTest(PyUpb_Access* access);
void PyUpb_Access_SetCountForTest(PyUpb_Access* access, int32_t count);
int32_t PyUpb_Access_GetCountForTest(PyUpb_Access* access);
#ifdef __cplusplus
}
#endif
#endif  // PYUPB_FREE_THREADING_ACCESS_TEST_UTIL_H_
