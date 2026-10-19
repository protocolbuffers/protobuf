// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file or at https://developers.google.com/open-source/licenses/bsd

#ifndef PYUPB_FREE_THREADING_ACCESS_H_
#define PYUPB_FREE_THREADING_ACCESS_H_

#include <stdbool.h>
#include <stdint.h>

// Non-waiting admission for one mutable arena domain, not a blocking mutex.
// Readers may coexist; a writer requires exclusive access. A failed admission
// must be surfaced before accessing native storage, never bypassed or retried
// while waiting for the current owner. Read admission is lock-free (not
// wait-free): CAS retries only when another reader makes progress.
//
// This low-level primitive is deliberately non-recursive. Binding scopes must
// track nesting, retain the domain lifetime, and distinguish safe internal
// calls from callbacks. Read-to-write upgrades are NOT supported. All aliases,
// borrowed native pointers, source trees and arena allocations must be covered
// before this primitive provides a Python API memory-safety guarantee.
#ifdef __cplusplus
// Keep C11 atomic representation out of C++ headers. The binding embeds this
// state in C; C++ tests operate through an opaque pointer allocated by C.
typedef struct PyUpb_Access PyUpb_Access;
#else
#include "upb/port/atomic.h"
#include "upb/port/def.inc"
#if !defined(UPB_USE_C11_ATOMICS) && !defined(UPB_USE_MSC_ATOMICS)
#error "Arena access admission requires a real atomic implementation"
#endif
typedef struct PyUpb_Access {
  UPB_ATOMIC(int32_t) state;  // -1: writer, >= 0: number of readers.
} PyUpb_Access;
#include "upb/port/undef.inc"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void PyUpb_Access_Init(PyUpb_Access* access);
bool PyUpb_Access_TryRead(PyUpb_Access* access);
bool PyUpb_Access_TryWrite(PyUpb_Access* access);
void PyUpb_Access_EndRead(PyUpb_Access* access);
void PyUpb_Access_EndWrite(PyUpb_Access* access);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // PYUPB_FREE_THREADING_ACCESS_H_
