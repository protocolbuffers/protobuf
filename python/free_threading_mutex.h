// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_PYTHON_FREE_THREADING_MUTEX_H__
#define GOOGLE_UPB_PYTHON_FREE_THREADING_MUTEX_H__

#ifdef Py_GIL_DISABLED
#include <pthread.h>
#endif

// Zero-cost mutex wrapper that compiles away to nothing in GIL-enabled builds.
// NOTE: Protobuf Free-threading support is still experimental.
typedef struct {
#ifdef Py_GIL_DISABLED
  pthread_mutex_t mutex;
#endif
} FreeThreadingMutex;

void FreeThreadingLock(FreeThreadingMutex* thread_mutex) {
#ifdef Py_GIL_DISABLED
  pthread_mutex_lock(&(thread_mutex->mutex));
#endif
}

void FreeThreadingUnlock(FreeThreadingMutex* thread_mutex) {
#ifdef Py_GIL_DISABLED
  pthread_mutex_unlock(&(thread_mutex->mutex));
#endif
}

#endif  // GOOGLE_UPB_PYTHON_FREE_THREADING_MUTEX_H__
