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

#include <google/protobuf/pyext/unknown_fields.h>

#include <Python.h>
#include <set>
#include <memory>

#include <google/protobuf/message.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/wire_format_lite.h>

#if PY_MAJOR_VERSION >= 3
  #define PyInt_FromLong PyLong_FromLong
#endif

namespace google {
namespace protobuf {
namespace python {

namespace unknown_fields {

static Py_ssize_t Len(PyObject* pself) {
  PyUnknownFields* self =
      reinterpret_cast<PyUnknownFields*>(pself);
  if (self->fields == NULL) {
    PyErr_Format(PyExc_ValueError,
                 "UnknownFields does not exist. "
                 "The parent message might be cleared.");
    return -1;
  }
  return self->fields->field_count();
}

void Clear(PyUnknownFields* self) {
  for (std::set<PyUnknownFields*>::iterator it =
           self->sub_unknown_fields.begin();
       it != self->sub_unknown_fields.end(); it++) {
    Clear(*it);
  }
  self->fields = NULL;
  self->sub_unknown_fields.clear();
}

PyObject* NewPyUnknownFieldRef(PyUnknownFields* parent,
                               Py_ssize_t index);

static PyObject* Item(PyObject* pself, Py_ssize_t index) {
  PyUnknownFields* self =
      reinterpret_cast<PyUnknownFields*>(pself);
  if (self->fields == NULL) {
    PyErr_Format(PyExc_ValueError,
                 "UnknownFields does not exist. "
                 "The parent message might be cleared.");
    return NULL;
  }
  Py_ssize_t total_size = self->fields->field_count();
  if (index < 0) {
    index = total_size + index;
  }
  if (index < 0 || index >= total_size) {
    PyErr_Format(PyExc_IndexError,
                 "index (%zd) out of range",
                 index);
    return NULL;
  }

  return unknown_fields::NewPyUnknownFieldRef(self, index);
}

PyObject* NewPyUnknownFields(CMessage* c_message) {
  PyUnknownFields* self = reinterpret_cast<PyUnknownFields*>(
      PyType_GenericAlloc(&PyUnknownFields_Type, 0));
  if (self == NULL) {
    return NULL;
  }
  // Call "placement new" to initialize PyUnknownFields.
  new (self) PyUnknownFields;

  Py_INCREF(c_message);
  self->parent = reinterpret_cast<PyObject*>(c_message);
  Message* message = c_message->message;
  const Reflection* reflection = message->GetReflection();
  self->fields = &reflection->GetUnknownFields(*message);

  return reinterpret_cast<PyObject*>(self);
}

PyObject* NewPyUnknownFieldRef(PyUnknownFields* parent,
                               Py_ssize_t index) {
  PyUnknownFieldRef* self = reinterpret_cast<PyUnknownFieldRef*>(
      PyType_GenericAlloc(&PyUnknownFieldRef_Type, 0));
  if (self == NULL) {
    return NULL;
  }

  Py_INCREF(parent);
  self->parent = parent;
  self->index = index;

  return reinterpret_cast<PyObject*>(self);
}

static void Dealloc(PyObject* pself) {
  PyUnknownFields* self =
      reinterpret_cast<PyUnknownFields*>(pself);
  if (PyObject_TypeCheck(self->parent, &PyUnknownFields_Type)) {
    reinterpret_cast<PyUnknownFields*>(
        self->parent)->sub_unknown_fields.erase(self);
  } else {
    reinterpret_cast<CMessage*>(self->parent)->unknown_field_set = nullptr;
  }
  Py_CLEAR(self->parent);
  self->~PyUnknownFields();
}

static PySequenceMethods SqMethods = {
  Len,        /* sq_length */
  0,          /* sq_concat */
  0,          /* sq_repeat */
  Item,       /* sq_item */
  0,          /* sq_slice */
  0,          /* sq_ass_item */
};

}  // namespace unknown_fields

PyTypeObject PyUnknownFields_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  FULL_MODULE_NAME ".PyUnknownFields",  // tp_name
  sizeof(PyUnknownFields),             // tp_basicsize
  0,                                   //  tp_itemsize
  unknown_fields::Dealloc,             //  tp_dealloc
  0,                                   //  tp_print
  0,                                   //  tp_getattr
  0,                                   //  tp_setattr
  0,                                   //  tp_compare
  0,                                   //  tp_repr
  0,                                   //  tp_as_number
  &unknown_fields::SqMethods,          //  tp_as_sequence
  0,                                   //  tp_as_mapping
  PyObject_HashNotImplemented,         //  tp_hash
  0,                                   //  tp_call
  0,                                   //  tp_str
  0,                                   //  tp_getattro
  0,                                   //  tp_setattro
  0,                                   //  tp_as_buffer
  Py_TPFLAGS_DEFAULT,                  //  tp_flags
  "unknown field set",                 //  tp_doc
  0,                                   //  tp_traverse
  0,                                   //  tp_clear
  0,                                   //  tp_richcompare
  0,                                   //  tp_weaklistoffset
  0,                                   //  tp_iter
  0,                                   //  tp_iternext
  0,                                   //  tp_methods
  0,                                   //  tp_members
  0,                                   //  tp_getset
  0,                                   //  tp_base
  0,                                   //  tp_dict
  0,                                   //  tp_descr_get
  0,                                   //  tp_descr_set
  0,                                   //  tp_dictoffset
  0,                                   //  tp_init
};

namespace unknown_field {
static PyObject* PyUnknownFields_FromUnknownFieldSet(
    PyUnknownFields* parent, const UnknownFieldSet& fields) {
  PyUnknownFields* self = reinterpret_cast<PyUnknownFields*>(
      PyType_GenericAlloc(&PyUnknownFields_Type, 0));
  if (self == NULL) {
    return NULL;
  }
  // Call "placement new" to initialize PyUnknownFields.
  new (self) PyUnknownFields;

  Py_INCREF(parent);
  self->parent = reinterpret_cast<PyObject*>(parent);
  self->fields = &fields;
  parent->sub_unknown_fields.emplace(self);

  return reinterpret_cast<PyObject*>(self);
}

const UnknownField* GetUnknownField(PyUnknownFieldRef* self) {
  const UnknownFieldSet* fields = self->parent->fields;
  if (fields == NULL) {
    PyErr_Format(PyExc_ValueError,
                 "UnknownField does not exist. "
                 "The parent message might be cleared.");
    return NULL;
  }
  ssize_t total_size = fields->field_count();
  if (self->index >= total_size) {
    PyErr_Format(PyExc_ValueError,
                 "UnknownField does not exist. "
                 "The parent message might be cleared.");
    return NULL;
  }
  return &fields->field(self->index);
}

static PyObject* GetFieldNumber(PyUnknownFieldRef* self, void *closure) {
  const UnknownField* unknown_field = GetUnknownField(self);
  if (unknown_field == NULL) {
    return NULL;
  }
  return PyInt_FromLong(unknown_field->number());
}

using internal::WireFormatLite;
static PyObject* GetWireType(PyUnknownFieldRef* self, void *closure) {
  const UnknownField* unknown_field = GetUnknownField(self);
  if (unknown_field == NULL) {
    return NULL;
  }

  // Assign a default value to suppress may-unintialized warnings (errors
  // when built in some places).
  WireFormatLite::WireType wire_type = WireFormatLite::WIRETYPE_VARINT;
  switch (unknown_field->type()) {
    case UnknownField::TYPE_VARINT:
      wire_type = WireFormatLite::WIRETYPE_VARINT;
      break;
    case UnknownField::TYPE_FIXED32:
      wire_type = WireFormatLite::WIRETYPE_FIXED32;
      break;
    case UnknownField::TYPE_FIXED64:
      wire_type = WireFormatLite::WIRETYPE_FIXED64;
      break;
    case UnknownField::TYPE_LENGTH_DELIMITED:
      wire_type = WireFormatLite::WIRETYPE_LENGTH_DELIMITED;
      break;
    case UnknownField::TYPE_GROUP:
      wire_type = WireFormatLite::WIRETYPE_START_GROUP;
      break;
  }
  return PyInt_FromLong(wire_type);
}

static PyObject* GetData(PyUnknownFieldRef* self, void *closure) {
  const UnknownField* field = GetUnknownField(self);
  if (field == NULL) {
    return NULL;
  }
  PyObject* data = NULL;
  switch (field->type()) {
    case UnknownField::TYPE_VARINT:
      data = PyInt_FromLong(field->varint());
      break;
    case UnknownField::TYPE_FIXED32:
      data = PyInt_FromLong(field->fixed32());
      break;
    case UnknownField::TYPE_FIXED64:
      data = PyInt_FromLong(field->fixed64());
      break;
    case UnknownField::TYPE_LENGTH_DELIMITED:
      data = PyBytes_FromStringAndSize(field->length_delimited().data(),
                                       field->GetLengthDelimitedSize());
      break;
    case UnknownField::TYPE_GROUP:
      data = PyUnknownFields_FromUnknownFieldSet(
          self->parent, field->group());
      break;
  }
  return data;
}

static void Dealloc(PyObject* pself) {
  PyUnknownFieldRef* self =
      reinterpret_cast<PyUnknownFieldRef*>(pself);
  Py_CLEAR(self->parent);
}

static PyGetSetDef Getters[] = {
  {"field_number", (getter)GetFieldNumber, NULL},
  {"wire_type", (getter)GetWireType, NULL},
  {"data", (getter)GetData, NULL},
  {NULL}
};

}  // namespace unknown_field

PyTypeObject PyUnknownFieldRef_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  FULL_MODULE_NAME ".PyUnknownFieldRef",  // tp_name
  sizeof(PyUnknownFieldRef),           //  tp_basicsize
  0,                                   //  tp_itemsize
  unknown_field::Dealloc,              //  tp_dealloc
  0,                                   //  tp_print
  0,                                   //  tp_getattr
  0,                                   //  tp_setattr
  0,                                   //  tp_compare
  0,                                   //  tp_repr
  0,                                   //  tp_as_number
  0,                                   //  tp_as_sequence
  0,                                   //  tp_as_mapping
  PyObject_HashNotImplemented,         //  tp_hash
  0,                                   //  tp_call
  0,                                   //  tp_str
  0,                                   //  tp_getattro
  0,                                   //  tp_setattro
  0,                                   //  tp_as_buffer
  Py_TPFLAGS_DEFAULT,                  //  tp_flags
  "unknown field",                     //  tp_doc
  0,                                   //  tp_traverse
  0,                                   //  tp_clear
  0,                                   //  tp_richcompare
  0,                                   //  tp_weaklistoffset
  0,                                   //  tp_iter
  0,                                   //  tp_iternext
  0,                                   //  tp_methods
  0,                                   //  tp_members
  unknown_field::Getters,              //  tp_getset
  0,                                   //  tp_base
  0,                                   //  tp_dict
  0,                                   //  tp_descr_get
  0,                                   //  tp_descr_set
  0,                                   //  tp_dictoffset
  0,                                   //  tp_init
};


}  // namespace python
}  // namespace protobuf
}  // namespace google
