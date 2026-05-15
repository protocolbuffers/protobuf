// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef THIRD_PARTY_PY_GOOGLE_PROTOBUF_PYEXT_LAZY_UNIQUE_PTR_H_
#define THIRD_PARTY_PY_GOOGLE_PROTOBUF_PYEXT_LAZY_UNIQUE_PTR_H_

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <memory>

#ifdef Py_GIL_DISABLED
#include <atomic>
#endif  // Py_GIL_DISABLED

namespace google {
namespace protobuf {
namespace python {

// A unique pointer that is initialized on first access. In a free-threaded
// build, this lazy initialization is thread-safe.
//
// To avoid blocking, this class does not wait if anonther thread is
// initializing the object. When this race condition occurs, both threads will
// create the object, and whichever thread loses the race will delete the object
// it created. For this reason, the constructor and destructor of T must not
// have any meaningful side effects.
template <class T>
class LazyUniquePtr {
 public:
  LazyUniquePtr() = default;
  ~LazyUniquePtr();

  // Returns the pointer to the object if it has been initialized, otherwise
  // returns nullptr.
  //
  // Exercise caution when testing the return value against nullptr. Unless we
  // are being called from a mutating method (eg. msg.Clear()), this is a race
  // condition and the pointer could become non-null at any time.
  T* TryGet();

  // Returns the pointer to the object, initializing it if necessary.
  T* Get();

 private:
#ifdef Py_GIL_DISABLED
  std::atomic<T*> ptr_ = nullptr;
#else
  std::unique_ptr<T> ptr_ = nullptr;
#endif
};

#ifdef Py_GIL_DISABLED

template <class T>
LazyUniquePtr<T>::~LazyUniquePtr() {
  // Relaxed memory order is sufficient because the object is require to be
  // quiescent before destruction.
  delete ptr_.load(std::memory_order_relaxed);
}

template <class T>
T* LazyUniquePtr<T>::TryGet() {
  return ptr_.load(std::memory_order_acquire);
}

// Returns the pointer to the object, initializing it if necessary.
template <class T>
T* LazyUniquePtr<T>::Get() {
  T* instance = ptr_.load(std::memory_order_acquire);
  if (instance != nullptr) {
    return instance;
  }

  std::unique_ptr<T> obj(new T());
  return ptr_.compare_exchange_strong(instance, obj.get(),
                                      std::memory_order_release,
                                      std::memory_order_acquire)
             ? obj.release()
             : instance;
}

#else

template <class T>
LazyUniquePtr<T>::~LazyUniquePtr() = default;

template <class T>
T* LazyUniquePtr<T>::TryGet() {
  return ptr_.get();
}

template <class T>
T* LazyUniquePtr<T>::Get() {
  if (ptr_ == nullptr) {
    ptr_ = std::make_unique<T>();
  }
  return ptr_.get();
}

#endif  // Py_GIL_DISABLED

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // THIRD_PARTY_PY_GOOGLE_PROTOBUF_PYEXT_LAZY_UNIQUE_PTR_H_
