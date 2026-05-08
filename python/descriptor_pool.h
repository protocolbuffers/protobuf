// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PYUPB_DESCRIPTOR_POOL_H__
#define PYUPB_DESCRIPTOR_POOL_H__

// clang-format off
#include "Python.h"
// clang-format on
#include <stdbool.h>

#include "protobuf.h"


// Given a Python DescriptorPool, returns the underlying symtab.
upb_DefPool* PyUpb_DescriptorPool_GetSymtab(PyObject* pool);

// Returns the default DescriptorPool (a global singleton).
PyObject* PyUpb_DescriptorPool_GetDefaultPool(void);

bool PyUpb_DescriptorPool_CacheAdd(PyObject* pool, const void* key,
                                   PyObject** py_obj);
PyObject* PyUpb_DescriptorPool_CacheGet(PyObject* pool, const void* key);
bool PyUpb_DescriptorPool_CacheEraseIfEqual(PyObject* pool, const void* key,
                                            PyObject* obj);

// Module-level init.
bool PyUpb_InitDescriptorPool(PyObject* m);

#endif  // PYUPB_DESCRIPTOR_POOL_H__
