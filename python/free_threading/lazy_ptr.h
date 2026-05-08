// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PYUPB_FREE_THREADING_LAZY_PTR_H_
#define PYUPB_FREE_THREADING_LAZY_PTR_H_

// PyUpb_LazyPtr: Lock-free lazy pointer initialization for CPython / upb.
//
// OVERVIEW:
// This module provides lock-free primitives for lazily initialized C pointers
// (`PyUpb_LazyInitPtr`) and CPython reference-counted objects
// (`PyUpb_LazyInitPyObject`).
//
// Under free-threaded Python (`Py_GIL_DISABLED`), lazy pointers expand to
// atomic pointers (`UPB_ATOMIC(T*)`) and use atomic Compare-And-Swap (CAS) with
// acquire/release memory ordering to guarantee safe thread interaction without
// acquiring global locks. On CAS collisions (races where two threads
// concurrently attempt to initialize the same field), the lost instance is
// automatically freed/decref'd and the winning instance is returned to all
// callers.
//
// Under standard GIL Python builds, `PYUPB_LAZYPTR(T)` degrades to standard C
// pointers (`T*`), executing simple null-checks without atomic instruction
// overhead.
//
// USAGE:
// 1. Declare lazy fields using `PYUPB_LAZYPTR(type)`:
//      typedef struct {
//        PYUPB_LAZYPTR(PyObject) cached_options;
//      } MyDescriptor;
//
// 2. Read pointers lock-free via `PyUpb_LazyGetPtr`:
//      PyObject* options = (PyObject*)PyUpb_LazyGetPtr(&desc->cached_options);
//
// 3. Initialize PyObject* fields atomically via `PyUpb_LazyInitPyObject`:
//      PyObject* options = PyUpb_LazyInitPyObject(
//          &desc->cached_options, CreateOptionsCallback, &context_args);

// clang-format off
#include "Python.h"
// clang-format on

#include "upb/port/atomic.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef Py_GIL_DISABLED
#define PYUPB_LAZYPTR(type) UPB_ATOMIC(type*)
#else
#define PYUPB_LAZYPTR(type) type*
#endif

typedef void* (*PyUpb_PtrInitFunc)(void* ctx);
typedef void (*PyUpb_PtrFreeFunc)(void* ptr);

#ifndef Py_GIL_DISABLED
static inline void* PyUpb_LazyGetPtr(const void* lazy_ptr) {
  return *(void* const*)lazy_ptr;
}

static inline void* PyUpb_LazyInitPtr(void* lazy_ptr, PyUpb_PtrInitFunc init_fn,
                                      PyUpb_PtrFreeFunc free_fn, void* ctx) {
  void** ptr = (void**)lazy_ptr;
  if (*ptr) return *ptr;
  *ptr = init_fn(ctx);
  return *ptr;
}
#else
static inline void* PyUpb_LazyGetPtr(const void* lazy_ptr) {
  return upb_Atomic_Load((UPB_ATOMIC(void*)*)lazy_ptr, memory_order_acquire);
}

// Atomically loads or initializes a lazy C pointer. On a CAS collision race,
// calls `free_fn` on the duplicate pointer and returns the winning instance.
void* PyUpb_LazyInitPtr(void* lazy_ptr, PyUpb_PtrInitFunc init_fn,
                        PyUpb_PtrFreeFunc free_fn, void* ctx);
#endif

static inline void PyUpb_PyObject_Decref(void* obj) {
  Py_DECREF((PyObject*)obj);
}

typedef PyObject* (*PyUpb_PyObjectInitFunc)(void* ctx);

// Inline wrapper for PyObject* lazy fields. Automatically INCREFs the returned
// reference to provide a new reference to the caller.
static inline PyObject* PyUpb_LazyInitPyObject(void* lazy_ptr,
                                               PyUpb_PyObjectInitFunc init_fn,
                                               void* ctx) {
  PyObject* obj = PyUpb_LazyInitPtr(lazy_ptr, (PyUpb_PtrInitFunc)init_fn,
                                    PyUpb_PyObject_Decref, ctx);
  Py_XINCREF(obj);
  return obj;
}

#include "upb/port/undef.inc"

#endif  // PYUPB_FREE_THREADING_LAZY_PTR_H_
