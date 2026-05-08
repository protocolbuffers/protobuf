// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "python/free_threading/weak_map.h"

#include <assert.h>
#include <stddef.h>

#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

struct PyUpb_WeakMap {
  PyUpb_Mutex mutex;
  upb_inttable table;
  upb_Arena* arena;
};

PyUpb_WeakMap* PyUpb_WeakMap_New(void) {
  upb_Arena* arena = upb_Arena_New();
  PyUpb_WeakMap* map = upb_Arena_Malloc(arena, sizeof(*map));
  map->arena = arena;
  PyUpb_Mutex_Init(&map->mutex);
  upb_inttable_init(&map->table, map->arena);
  return map;
}

void PyUpb_WeakMap_Free(PyUpb_WeakMap* map) {
  PyUpb_Mutex_Destroy(&map->mutex);
  upb_Arena_Free(map->arena);
}

// To give better entropy in the table key, we shift away low bits that are
// always zero.
static const int PyUpb_PtrShift = (sizeof(void*) == 4) ? 2 : 3;

static uintptr_t PyUpb_WeakMap_GetKey(const void* key) {
  uintptr_t n = (uintptr_t)key;
  assert((n & ((1 << PyUpb_PtrShift) - 1)) == 0);
  return n >> PyUpb_PtrShift;
}

static bool PyUpb_WeakMap_TryIncRef(PyUpb_WeakMap* map, uintptr_t key,
                                    upb_value val) {
  PyObject* obj = upb_value_getptr(val);
#ifdef Py_GIL_DISABLED
  if (!PyUnstable_TryIncRef(obj)) {
    upb_inttable_remove(&map->table, key, NULL);
    return false;
  }
#else
  Py_INCREF(obj);
#endif
  return true;
}

static PyObject* PyUpb_WeakMap_GetLocked(PyUpb_WeakMap* map, const void* key) {
  upb_value val;
  if (upb_inttable_lookup(&map->table, PyUpb_WeakMap_GetKey(key), &val) &&
      PyUpb_WeakMap_TryIncRef(map, PyUpb_WeakMap_GetKey(key), val)) {
    return upb_value_getptr(val);
  } else {
    return NULL;
  }
}

PyObject* PyUpb_WeakMap_Get(PyUpb_WeakMap* map, const void* key) {
  PyUpb_Mutex_Lock(&map->mutex);
  PyObject* obj = PyUpb_WeakMap_GetLocked(map, key);
  PyUpb_Mutex_Unlock(&map->mutex);
  return obj;
}

bool PyUpb_WeakMap_Add(PyUpb_WeakMap* map, const void* key, PyObject** obj) {
#ifdef Py_GIL_DISABLED
  PyUnstable_EnableTryIncRef(*obj);
#endif

  PyObject* to_decref = NULL;
  PyObject* existing = NULL;
  PyUpb_Mutex_Lock(&map->mutex);

  bool ok = true;
  if ((existing = PyUpb_WeakMap_GetLocked(map, key)) != NULL) {
    to_decref = *obj;
    *obj = existing;
  } else {
    const uintptr_t k = PyUpb_WeakMap_GetKey(key);
    ok = upb_inttable_insert(&map->table, k, upb_value_ptr(*obj), map->arena);
    if (!ok) {
      PyErr_SetNone(PyExc_MemoryError);
    }
  }

  PyUpb_Mutex_Unlock(&map->mutex);

  if (to_decref) {
    // This can trigger a dealloc which calls back into this WeakMap, so it must
    // be after the unlock.
    Py_DECREF(to_decref);
  }
  return ok;
}

void PyUpb_WeakMap_Erase(PyUpb_WeakMap* map, const void* key, PyObject* obj) {
  bool erased = PyUpb_WeakMap_EraseIfEqual(map, key, obj);
  UPB_ASSERT(erased);
}

bool PyUpb_WeakMap_EraseIfEqual(PyUpb_WeakMap* map, const void* key,
                                PyObject* obj) {
  PyUpb_Mutex_Lock(&map->mutex);
  const uintptr_t k = PyUpb_WeakMap_GetKey(key);
  upb_value val;
  bool ret = false;
  if (upb_inttable_lookup(&map->table, k, &val) &&
      upb_value_getptr(val) == obj) {
    upb_inttable_remove(&map->table, k, NULL);
    ret = true;
  }
  PyUpb_Mutex_Unlock(&map->mutex);
  return ret;
}

bool PyUpb_WeakMap_Next(PyUpb_WeakMap* map, const void** key, PyObject** obj,
                        intptr_t* iter) {
#ifdef Py_GIL_DISABLED
  Py_XDECREF(*obj);
#endif
  if (*iter == PYUPB_WEAKMAP_BEGIN) {
    PyUpb_Mutex_Lock(&map->mutex);
  }
  uintptr_t u_key;
  upb_value val;
  while (upb_inttable_next(&map->table, &u_key, &val, iter)) {
    PyObject* py_obj = upb_value_getptr(val);
#ifdef Py_GIL_DISABLED
    if (!PyUnstable_TryIncRef(py_obj)) {
      // Object is being destroyed, remove it from the map and try again.
      upb_inttable_removeiter(&map->table, iter);
      continue;
    }
#endif
    *key = (void*)(u_key << PyUpb_PtrShift);
    *obj = py_obj;
    return true;
  }
  PyUpb_Mutex_Unlock(&map->mutex);
  return false;
}

void PyUpb_WeakMap_DeleteIter(PyUpb_WeakMap* map, intptr_t* iter) {
  upb_inttable_removeiter(&map->table, iter);
}
