// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PYUPB_FREE_THREADING_MUTEX_H_
#define PYUPB_FREE_THREADING_MUTEX_H_

// PyUpb_Mutex: Compatibility shim for PyMutex that supports Python < 3.13.

#include "Python.h"

#ifdef Py_GIL_DISABLED
#define PYUPB_ENABLE_MUTEX 1
#endif  // Py_GIL_DISABLED

typedef struct {
#ifdef Py_GIL_DISABLED
  PyMutex mutex;
#else
  char unused;
#endif
} PyUpb_Mutex;

static inline void PyUpb_Mutex_Init(PyUpb_Mutex* thread_mutex) {
#ifdef PYUPB_ENABLE_MUTEX
  thread_mutex->mutex = (PyMutex){0};
#endif
}

static inline void PyUpb_Mutex_Destroy(PyUpb_Mutex* thread_mutex) {}

static inline void PyUpb_Mutex_Lock(PyUpb_Mutex* thread_mutex) {
#ifdef PYUPB_ENABLE_MUTEX
  PyMutex_Lock(&(thread_mutex->mutex));
#endif
}

static inline void PyUpb_Mutex_Unlock(PyUpb_Mutex* thread_mutex) {
#ifdef PYUPB_ENABLE_MUTEX
  PyMutex_Unlock(&(thread_mutex->mutex));
#endif
}

static inline bool PyUpb_Mutex_IsLocked(PyUpb_Mutex* thread_mutex) {
#ifdef PYUPB_ENABLE_MUTEX
  return PyMutex_IsLocked(&(thread_mutex->mutex));
#else
  return true;
#endif
}

#endif  // PYUPB_FREE_THREADING_MUTEX_H_
