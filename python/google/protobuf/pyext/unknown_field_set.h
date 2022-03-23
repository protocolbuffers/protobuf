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

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_UNKNOWN_FIELD_SET_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_UNKNOWN_FIELD_SET_H__

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <memory>
#include <set>

#include <google/protobuf/pyext/message.h>

namespace google {
namespace protobuf {

class UnknownField;
class UnknownFieldSet;

namespace python {
struct CMessage;

struct PyUnknownFieldSet {
  PyObject_HEAD;
  // If parent is nullptr, it is a top UnknownFieldSet.
  PyUnknownFieldSet* parent;

  // Top UnknownFieldSet owns fields pointer. Sub UnknownFieldSet
  // does not own fields pointer.
  UnknownFieldSet* fields;
};

struct PyUnknownField {
  PyObject_HEAD;
  // Every Python PyUnknownField holds a reference to its parent
  // PyUnknownFieldSet in order to keep it alive.
  PyUnknownFieldSet* parent;

  // The UnknownField index in UnknownFieldSet.
  Py_ssize_t index;
};

extern PyTypeObject PyUnknownFieldSet_Type;
extern PyTypeObject PyUnknownField_Type;

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_UNKNOWN_FIELD_SET_H__
