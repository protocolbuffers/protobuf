// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#include <Python.h>

namespace google {
class ScopedPyObjectPtr {
 public:
  // Constructor.  Defaults to intializing with NULL.
  // There is no way to create an uninitialized ScopedPyObjectPtr.
  explicit ScopedPyObjectPtr(PyObject* p = NULL) : ptr_(p) { }

  // Destructor.  If there is a PyObject object, delete it.
  ~ScopedPyObjectPtr() {
    Py_XDECREF(ptr_);
  }

  // Reset.  Deletes the current owned object, if any.
  // Then takes ownership of a new object, if given.
  // this->reset(this->get()) works.
  PyObject* reset(PyObject* p = NULL) {
    if (p != ptr_) {
      Py_XDECREF(ptr_);
      ptr_ = p;
    }
    return ptr_;
  }

  // Releases ownership of the object.
  PyObject* release() {
    PyObject* p = ptr_;
    ptr_ = NULL;
    return p;
  }

  operator PyObject*() { return ptr_; }

  PyObject* operator->() const  {
    assert(ptr_ != NULL);
    return ptr_;
  }

  PyObject* get() const { return ptr_; }

  Py_ssize_t refcnt() const { return Py_REFCNT(ptr_); }

  void inc() const { Py_INCREF(ptr_); }

  // Comparison operators.
  // These return whether a ScopedPyObjectPtr and a raw pointer
  // refer to the same object, not just to two different but equal
  // objects.
  bool operator==(const PyObject* p) const { return ptr_ == p; }
  bool operator!=(const PyObject* p) const { return ptr_ != p; }

 private:
  PyObject* ptr_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ScopedPyObjectPtr);
};

}  // namespace google
#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_SCOPED_PYOBJECT_PTR_H__
