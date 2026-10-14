// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: tibell@google.com (Johan Tibell)

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_SCOPED_PYOBJECT_PTR_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_SCOPED_PYOBJECT_PTR_H__

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstddef>

namespace google {
namespace protobuf {
namespace python {

// Owns a python object and decrements the reference count on destruction.
// This class is not threadsafe.
template <typename PyObjectStruct>
class ScopedPythonPtr {
 public:
  constexpr ScopedPythonPtr(std::nullptr_t = nullptr) noexcept
      : ptr_(nullptr) {}

  // Takes the ownership of the specified object to ScopedPythonPtr.
  // The reference count of the specified py_object is not incremented.
  explicit ScopedPythonPtr(PyObjectStruct* py_object) : ptr_(py_object) {}

  ScopedPythonPtr(const ScopedPythonPtr& other) : ptr_(other.ptr_) {
    Py_XINCREF(ptr_);
  }

  ScopedPythonPtr& operator=(const ScopedPythonPtr& other) {
    Py_XINCREF(other.ptr_);
    Py_XDECREF(ptr_);
    ptr_ = other.ptr_;
    return *this;
  }

  ScopedPythonPtr(ScopedPythonPtr&& other) noexcept : ptr_(other.ptr_) {
    other.ptr_ = nullptr;
  }

  ScopedPythonPtr& operator=(ScopedPythonPtr&& other) noexcept {
    if (this != &other) {
      Py_XDECREF(ptr_);
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
    }
    return *this;
  }

  // If a PyObject is owned, decrement its reference count.
  ~ScopedPythonPtr() { Py_XDECREF(ptr_); }

  // Deletes the current owned object, if any.
  // Then takes ownership of a new object without incrementing the reference
  // count.
  // This function must be called with a reference that you own.
  //   this->reset(this->get()) is wrong!
  //   this->reset(this->release()) is OK.
  PyObjectStruct* reset(PyObjectStruct* p = nullptr) {
    Py_XDECREF(ptr_);
    ptr_ = p;
    return ptr_;
  }

  // Releases ownership of the object without decrementing the reference count.
  // The caller now owns the returned reference.
  PyObjectStruct* release() {
    PyObject* p = ptr_;
    ptr_ = nullptr;
    return p;
  }

  PyObjectStruct* get() const { return ptr_; }

  PyObject* as_pyobject() const { return reinterpret_cast<PyObject*>(ptr_); }

  // Increments the reference count of the current object.
  // Should not be called when no object is held.
  void inc() const { Py_INCREF(ptr_); }

  // True when a ScopedPyObjectPtr and a raw pointer refer to the same object.
  // Comparison operators are non reflexive.
  bool operator==(const PyObjectStruct* p) const { return ptr_ == p; }
  bool operator!=(const PyObjectStruct* p) const { return ptr_ != p; }

 private:
  PyObjectStruct* ptr_;
};

typedef ScopedPythonPtr<PyObject> ScopedPyObjectPtr;

// Destroys a C++ object embedded in a PyObject and frees its Python memory.
// Suitable for use in or as a tp_dealloc slot.
template <typename T>
void DeallocPyObject(PyObject* pself) {
  auto* py_type = Py_TYPE(pself);
  if (PyType_HasFeature(py_type, Py_TPFLAGS_HAVE_GC)) {
    PyObject_GC_UnTrack(pself);
  }
  PyObject ob_base = *pself;
  reinterpret_cast<T*>(pself)->~T();
  *pself = ob_base;
  py_type->tp_free(pself);
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_SCOPED_PYOBJECT_PTR_H__
