// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PYUPB_FREE_THREADING_MUTEX_H_
#define PYUPB_FREE_THREADING_MUTEX_H_

// PyUpb_Mutex: Zero-cost conditional mutex abstraction for CPython / upb.
//
// OVERVIEW:
// This header provides a lightweight, conditionally compiled mutex type
// (`PyUpb_Mutex`) for protecting shared state across upb Python wrapper
// objects.
//
// Under free-threaded Python (`Py_GIL_DISABLED`), `PyUpb_Mutex` wraps a native
// `pthread_mutex_t` to provide thread safety when the CPython Global
// Interpreter Lock (GIL) is disabled.
//
// Under standard GIL Python builds, the CPython GIL guarantees single-threaded
// execution of C code, so `PyUpb_Mutex` degrades to a zero-byte empty struct
// (`struct {}`), and all mutex operations
// (`Init`, `Lock`, `Unlock`, `Destroy`) compile down to no-ops with zero
// runtime cost.
//
// USAGE:
// 1. Embed `PyUpb_Mutex` into structs needing thread protection:
//      typedef struct {
//        PyUpb_Mutex mutex;
//        // ... shared state ...
//      } PyUpb_WeakMap;
//
// 2. Initialize and destroy:
//      PyUpb_Mutex_Init(&map->mutex);
//      PyUpb_Mutex_Destroy(&map->mutex);
//
// 3. Scope lock/unlock pairs around critical sections:
//      PyUpb_Mutex_Lock(&map->mutex);
//      // ... perform thread-safe operations ...
//      PyUpb_Mutex_Unlock(&map->mutex);

// clang-format off
#include "Python.h"
// clang-format on

#ifdef Py_GIL_DISABLED
#ifdef _POSIX_THREADS
#define PYUPB_ENABLE_MUTEX 1
#include <pthread.h>
#else
#error "GIL is disabled but _POSIX_THREADS isn't available"
#endif  // _POSIX_THREADS
#endif  // Py_GIL_DISABLED

typedef struct {
#ifdef PYUPB_ENABLE_MUTEX
  pthread_mutex_t mutex;
#endif
} PyUpb_Mutex;

static inline void PyUpb_Mutex_Init(PyUpb_Mutex* thread_mutex) {
#ifdef PYUPB_ENABLE_MUTEX
  pthread_mutex_init(&(thread_mutex->mutex), NULL);
#endif
}

static inline void PyUpb_Mutex_Destroy(PyUpb_Mutex* thread_mutex) {
#ifdef PYUPB_ENABLE_MUTEX
  pthread_mutex_destroy(&(thread_mutex->mutex));
#endif
}

static inline void PyUpb_Mutex_Lock(PyUpb_Mutex* thread_mutex) {
#ifdef PYUPB_ENABLE_MUTEX
  pthread_mutex_lock(&(thread_mutex->mutex));
#endif
}

static inline void PyUpb_Mutex_Unlock(PyUpb_Mutex* thread_mutex) {
#ifdef PYUPB_ENABLE_MUTEX
  pthread_mutex_unlock(&(thread_mutex->mutex));
#endif
}

#endif  // PYUPB_FREE_THREADING_MUTEX_H_
