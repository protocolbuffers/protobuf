// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef THIRD_PARTY_PY_GOOGLE_PROTOBUF_PYEXT_WEAK_VALUE_MAP_H_
#define THIRD_PARTY_PY_GOOGLE_PROTOBUF_PYEXT_WEAK_VALUE_MAP_H_

#include <utility>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "google/protobuf/descriptor.pb.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"

class PyWeakValueMap {
 public:
  // Returns a new reference to the cached value. If the key is not found,
  // invokes the given function to create the value, and caches it.
  //
  // The value is not referenced from the cache, so it may be deallocated while
  // the cache still references it.
  template <class Func>
  PyObject* Get(const void* key, const PyTypeObject* type, Func&& func);

  // The Dealloc() function should call method to remove the entry from the
  // cache.
  void Delete(const void* key, PyObject* value);

 private:
#ifdef Py_GIL_DISABLED
  absl::Mutex mutex_;
  absl::flat_hash_map<const void*, PyObject*> cache_ ABSL_GUARDED_BY(mutex_);
#else
  absl::flat_hash_map<const void*, PyObject*> cache_;
#endif
};

#ifdef Py_GIL_DISABLED

template <class Func>
PyObject* PyWeakValueMap::Get(const void* key, const PyTypeObject* type,
                              Func&& func) {
  {
    absl::MutexLock lock(mutex_);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
      ABSL_DCHECK(Py_TYPE(it->second) == type);
      if (PyUnstable_TryIncRef(it->second)) {
        return it->second;
      }
      // Object is deallocating, remove it from the map.
      cache_.erase(it);
    }
  }

  PyObject* decref;
  PyObject* obj = func();
  if (obj == nullptr) {
    return nullptr;
  }

  PyUnstable_EnableTryIncRef(obj);

  // Cache the fully initialized object.
  // Check again if another thread cached it while we were initializing.
  {
    absl::MutexLock lock(mutex_);
    auto [it, inserted] = cache_.insert(std::make_pair(key, obj));
    if (inserted) return obj;

    // Another thread beat us to it. Use the existing object.
    ABSL_DCHECK(Py_TYPE(it->second) == type);
    if (PyUnstable_TryIncRef(it->second)) {
      // The existing object is valid, so we can deallocate our copy, but we
      // should drop the lock first.
      decref = obj;
      obj = it->second;
      // Fall through to the end of the function.
    } else {
      // The existing object is dying, replace it.
      it->second = obj;
      return obj;
    }
  }

  Py_DECREF(decref);
  return obj;
}

inline void PyWeakValueMap::Delete(const void* key, PyObject* value) {
  absl::MutexLock lock(mutex_);
  auto it = cache_.find(key);
  if (it != cache_.end() && it->second == value) {
    cache_.erase(it);
  }
}

#else  // !Py_GIL_DISABLED

template <class Func>
PyObject* PyWeakValueMap::Get(const void* key, const PyTypeObject* type,
                              Func&& func) {
  auto [it, inserted] = cache_.insert(std::make_pair(key, nullptr));
  if (inserted) {
    it->second = func();
  } else {
    Py_INCREF(it->second);
  }
  return it->second;
}

inline void PyWeakValueMap::Delete(const void* key, PyObject* value) {
  auto it = cache_.find(key);
  ABSL_CHECK(it != cache_.end() && it->second == value);
  cache_.erase(it);
}

#endif

#endif  // THIRD_PARTY_PY_GOOGLE_PROTOBUF_PYEXT_WEAK_VALUE_MAP_H_
