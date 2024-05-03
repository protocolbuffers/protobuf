// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_UNKNOWN_FIELDS_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_UNKNOWN_FIELDS_H__

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <memory>
#include <set>

#include "google/protobuf/pyext/message.h"

namespace google {
namespace protobuf {

class UnknownField;
class UnknownFieldSet;

namespace python {
struct CMessage;

typedef struct PyUnknownFields {
  PyObject_HEAD;
  // Strong pointer to the parent CMessage or PyUnknownFields.
  // The top PyUnknownFields holds a reference to its parent CMessage
  // object before release.
  // Sub PyUnknownFields holds reference to parent PyUnknownFields.
  PyObject* parent;

  // Pointer to the C++ UnknownFieldSet.
  // PyUnknownFields does not own this pointer.
  const UnknownFieldSet* fields;

  // Weak references to child unknown fields.
  std::set<PyUnknownFields*> sub_unknown_fields;
} PyUnknownFields;

typedef struct PyUnknownFieldRef {
  PyObject_HEAD;
  // Every Python PyUnknownFieldRef holds a reference to its parent
  // PyUnknownFields in order to keep it alive.
  PyUnknownFields* parent;

  // The UnknownField index in UnknownFields.
  Py_ssize_t index;
} UknownFieldRef;

extern PyTypeObject PyUnknownFields_Type;
extern PyTypeObject PyUnknownFieldRef_Type;

namespace unknown_fields {

// Builds an PyUnknownFields for a specific message.
PyObject* NewPyUnknownFields(CMessage *parent);
void Clear(PyUnknownFields* self);

}  // namespace unknown_fields
}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_UNKNOWN_FIELDS_H__
