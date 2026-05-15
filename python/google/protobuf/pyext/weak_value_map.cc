#include "google/protobuf/pyext/weak_value_map.h"

#include <utility>

#include "absl/log/absl_check.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#ifdef Py_GIL_DISABLED

// In a free threaded build, we have to always be prepared for the possibility
// that an object is being deallocated, but has not yet been removed from the
// map. We accomplish this by using the unstable PyUnstable_TryIncRef() function
// which returns false if the object is dying.

PyObject* PyWeakValueMap::Get(const void* key, const PyTypeObject* type) {
  absl::MutexLock lock(&mutex_);
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    ABSL_DCHECK(type == nullptr || Py_TYPE(it->second) == type);
    if (PyUnstable_TryIncRef(it->second)) {
      return it->second;
    }
    // Object is deallocating, remove it from the map.
    cache_.erase(it);
  }
  return nullptr;
}

bool PyWeakValueMap::TrySet(const void* key, PyObject*& value) {
  PyTypeObject* type = Py_TYPE(value);
  PyObject* decref;
  PyUnstable_EnableTryIncRef(value);

  {
    absl::MutexLock lock(&mutex_);
    auto [it, inserted] = cache_.insert(std::make_pair(key, value));
    if (inserted) return true;

    // The object is already in the map. Try to use the existing object.
    ABSL_DCHECK(type == nullptr || Py_TYPE(it->second) == type);

    if (PyUnstable_TryIncRef(it->second)) {
      // The existing object is valid, so we can deallocate our copy, but we
      // should drop the lock first.
      decref = value;
      value = it->second;
      // Fall through to the end of the function.
    } else {
      // The existing object is dying, replace it.
      it->second = value;
      return true;
    }
  }

  Py_DECREF(decref);
  return false;
}

bool PyWeakValueMap::Erase(const void* key) {
  absl::MutexLock lock(&mutex_);
  return cache_.erase(key) != 0;
}

void PyWeakValueMap::EraseIfEqual(const void* key, PyObject* value) {
  absl::MutexLock lock(&mutex_);
  auto it = cache_.find(key);
  if (it != cache_.end() && it->second == value) {
    cache_.erase(it);
  }
}

bool PyWeakValueMap::IsEmpty() const {
  absl::MutexLock lock(&mutex_);
  return cache_.empty();
}

void PyWeakValueMap::Clear() {
  absl::MutexLock lock(&mutex_);
  cache_.clear();
}

#else  // !Py_GIL_DISABLED

bool PyWeakValueMap::TrySet(const void* key, PyObject*& value) {
  auto [it, inserted] = cache_.insert(std::make_pair(key, value));
  if (inserted) return true;
  Py_DECREF(value);
  Py_INCREF(it->second);
  value = it->second;
  return false;
}

PyObject* PyWeakValueMap::Get(const void* key, const PyTypeObject* type) {
  auto it = cache_.find(key);
  if (it == cache_.end()) return nullptr;
  Py_INCREF(it->second);
  return it->second;
}

bool PyWeakValueMap::Erase(const void* key) { return cache_.erase(key) != 0; }

void PyWeakValueMap::EraseIfEqual(const void* key, PyObject* value) {
  auto it = cache_.find(key);
  ABSL_CHECK(it != cache_.end() && it->second == value);
  cache_.erase(it);
}

bool PyWeakValueMap::IsEmpty() const { return cache_.empty(); }

void PyWeakValueMap::Clear() { cache_.clear(); }

#endif
