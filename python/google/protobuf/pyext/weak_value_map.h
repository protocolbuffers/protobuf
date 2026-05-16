// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef THIRD_PARTY_PY_GOOGLE_PROTOBUF_PYEXT_WEAK_VALUE_MAP_H_
#define THIRD_PARTY_PY_GOOGLE_PROTOBUF_PYEXT_WEAK_VALUE_MAP_H_

#include <atomic>
#include <memory>
#include <utility>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "google/protobuf/descriptor.pb.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/synchronization/mutex.h"

// A map of void* to PyObject*.
//
// The values in the map are only held by weak reference, meaning that the map
// does not own a reference to the objects. It is up to the user to ensure that
// objects are removed from the map when they are deallocated.
class PyWeakValueMap {
 public:
  // Sets the value in the cache if the key is not already present.
  //
  // Returns true if the value was set, false if the key was already present.
  // When false is returned, `value` is decref'd and replaced with the existing
  // value, which the caller will own a ref on.
  bool TrySet(const void* key, PyObject*& value);

  // Sets the value in the cache. The key must not be already present.
  // This must only be called from logically mutating methods. For nominally
  // read-only methods, this is inherently racy.
  void Set(const void* key, PyObject* value) { ABSL_CHECK(TrySet(key, value)); }

  // Returns a new reference to the cached value. If the key is not found,
  // invokes the given function to create the value, and caches it.
  template <class Func>
  PyObject* GetOrInsert(const void* key, const PyTypeObject* type, Func&& func);

  // Returns a new reference to the cached value, or nullptr if not found.
  PyObject* Get(const void* key, const PyTypeObject* type);

  // Removes the entry from the map. Returns true if the entry was found.
  bool Erase(const void* key);

  // Removes the entry from the cache, but only if it matches the given value.
  void EraseIfEqual(const void* key, PyObject* value);

  // Returns true if the map is empty.
  bool IsEmpty() const;

  // Removes all entries from the map.
  void Clear();

  // Calls the given function for each entry in the map.
  //
  // Ownership: The callback `func` receives a reference to the PyObject that is
  // guaranteed to be alive for the duration of the call.
  //
  // In free-threaded builds, this is a temporary strong reference. In
  // GIL-enabled builds, this is a borrowed reference protected by the GIL.
  //
  // If the callback needs to keep the object alive after it returns, it MUST
  // explicitly call `Py_INCREF`.
  template <typename Func>
  void ForEach(Func&& func);

 private:
#ifdef Py_GIL_DISABLED
  mutable absl::Mutex mutex_;
  absl::flat_hash_map<const void*, PyObject*> cache_ ABSL_GUARDED_BY(mutex_);
#else
  absl::flat_hash_map<const void*, PyObject*> cache_;
#endif
};

#ifdef Py_GIL_DISABLED

template <class Func>
PyObject* PyWeakValueMap::GetOrInsert(const void* key, const PyTypeObject* type,
                                      Func&& func) {
  if (auto obj = Get(key, type); obj != nullptr) {
    return obj;
  }

  PyObject* obj = func();

  if (obj == nullptr) {
    return nullptr;
  }

  TrySet(key, obj);
  return obj;
}

template <typename Func>
void PyWeakValueMap::ForEach(Func&& func) {
  absl::MutexLock lock(&mutex_);
  for (auto it = cache_.begin(); it != cache_.end();) {
    if (PyUnstable_TryIncRef(it->second)) {
      func(it->first, it->second);
      Py_DECREF(it->second);
      ++it;
    } else {
      cache_.erase(it++);
    }
  }
}

#else  // !Py_GIL_DISABLED

template <class Func>
PyObject* PyWeakValueMap::GetOrInsert(const void* key, const PyTypeObject* type,
                                      Func&& func) {
  auto [it, inserted] = cache_.insert(std::make_pair(key, nullptr));
  if (inserted) {
    it->second = func();
  } else {
    Py_INCREF(it->second);
  }
  return it->second;
}

template <typename Func>
void PyWeakValueMap::ForEach(Func&& func) {
  for (auto const& [key, val] : cache_) {
    func(key, val);
  }
}

#endif

#endif  // THIRD_PARTY_PY_GOOGLE_PROTOBUF_PYEXT_WEAK_VALUE_MAP_H_
