// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PYUPB_FREE_THREADING_WEAK_MAP_H_
#define PYUPB_FREE_THREADING_WEAK_MAP_H_

// clang-format off
#include "Python.h"
// clang-format on

#include <stdbool.h>
#include <stdint.h>

#include "python/free_threading/mutex.h"
#include "upb/hash/int_table.h"

// A WeakMap maps C pointers to the corresponding Python wrapper object. We
// want a consistent Python wrapper object for each C object, both to save
// memory and to provide object stability (ie. x is x).
//
// Each wrapped object should add itself to the map when it is constructed and
// remove itself from the map when it is destroyed. The map is weak so it does
// not take references to the cached objects.

struct PyUpb_WeakMap;
typedef struct PyUpb_WeakMap PyUpb_WeakMap;

PyUpb_WeakMap* PyUpb_WeakMap_New(void);
void PyUpb_WeakMap_Free(PyUpb_WeakMap* map);

// Adds the given object to the map, indexed by the given key.
// If key is already present in the map, py_obj is DECREF'd and replaced with
// the existing object.
bool PyUpb_WeakMap_Add(PyUpb_WeakMap* map, const void* key, PyObject** obj);

// Removes the given key from the cache. It must exist in the cache currently.
void PyUpb_WeakMap_Erase(PyUpb_WeakMap* map, const void* key, PyObject* obj);

// Removes the entry from the cache, but only if it matches the given value.
// Returns true if the entry was found and removed.
bool PyUpb_WeakMap_EraseIfEqual(PyUpb_WeakMap* map, const void* key,
                                PyObject* obj);

// Returns a new reference to an object if it exists, otherwise returns NULL.
PyObject* PyUpb_WeakMap_Get(PyUpb_WeakMap* map, const void* key);

#define PYUPB_WEAKMAP_BEGIN UPB_INTTABLE_BEGIN

// Iteration over the weak map, eg.
//
// intptr_t it = PYUPB_WEAKMAP_BEGIN;
// PyObject* obj = NULL;
// while (PyUpb_WeakMap_Next(map, &key, &obj, &it)) {
//   // ...
// }
//
// Note that under Py_GIL_DISABLED, `Next` manages references on `obj` across
// steps (incref on return, decref on next call/end).
bool PyUpb_WeakMap_Next(PyUpb_WeakMap* map, const void** key, PyObject** obj,
                        intptr_t* iter);
void PyUpb_WeakMap_DeleteIter(PyUpb_WeakMap* map, intptr_t* iter);

#endif  // PYUPB_FREE_THREADING_WEAK_MAP_H_
