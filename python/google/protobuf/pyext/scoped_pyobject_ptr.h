// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: tibell@google.com (Johan Tibell)

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_SCOPED_PYOBJECT_PTR_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_SCOPED_PYOBJECT_PTR_H__

#include <google/protobuf/stubs/common.h>

#include <Python.h>
namespace google {
namespace protobuf {
namespace python {

// Owns a python object and decrements the reference count on destruction.
// This class is not threadsafe.
template <typename PyObjectStruct>
class ScopedPythonPtr {
 public:
  // Takes the ownership of the specified object to ScopedPythonPtr.
  // The reference count of the specified py_object is not incremented.
  explicit ScopedPythonPtr(PyObjectStruct* py_object = NULL)
      : ptr_(py_object) {}

  // If a PyObject is owned, decrement its reference count.
  ~ScopedPythonPtr() { Py_XDECREF(ptr_); }

  // Deletes the current owned object, if any.
  // Then takes ownership of a new object without incrementing the reference
  // count.
  // This function must be called with a reference that you own.
  //   this->reset(this->get()) is wrong!
  //   this->reset(this->release()) is OK.
  PyObjectStruct* reset(PyObjectStruct* p = NULL) {
    Py_XDECREF(ptr_);
    ptr_ = p;
    return ptr_;
  }

  // Releases ownership of the object without decrementing the reference count.
  // The caller now owns the returned reference.
  PyObjectStruct* release() {
    PyObject* p = ptr_;
    ptr_ = NULL;
    return p;
  }

  PyObjectStruct* operator->() const {
    assert(ptr_ != NULL);
    return ptr_;
  }

  PyObjectStruct* get() const { return ptr_; }

  PyObject* as_pyobject() const { return reinterpret_cast<PyObject*>(ptr_); }

  // Increments the reference count fo the current object.
  // Should not be called when no object is held.
  void inc() const { Py_INCREF(ptr_); }

  // True when a ScopedPyObjectPtr and a raw pointer refer to the same object.
  // Comparison operators are non reflexive.
  bool operator==(const PyObjectStruct* p) const { return ptr_ == p; }
  bool operator!=(const PyObjectStruct* p) const { return ptr_ != p; }

 private:
  PyObjectStruct* ptr_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ScopedPythonPtr);
};

typedef ScopedPythonPtr<PyObject> ScopedPyObjectPtr;

}  // namespace python
}  // namespace protobuf
}  // namespace google
#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_SCOPED_PYOBJECT_PTR_H__
