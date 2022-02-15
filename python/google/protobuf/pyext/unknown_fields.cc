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

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <set>
#include <memory>

#include <google/protobuf/message.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/wire_format_lite.h>

namespace google {
namespace protobuf {
namespace python {

namespace unknown_fields {

static Py_ssize_t Len(PyObject* pself) {
  PyUnknownFields* self =
      reinterpret_cast<PyUnknownFields*>(pself);
  if (self->fields == nullptr) {
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
  self->fields = nullptr;
  self->sub_unknown_fields.clear();
}

PyObject* NewPyUnknownFieldRef(PyUnknownFields* parent,
                               Py_ssize_t index);

static PyObject* Item(PyObject* pself, Py_ssize_t index) {
  PyUnknownFields* self =
      reinterpret_cast<PyUnknownFields*>(pself);
  if (self->fields == nullptr) {
    PyErr_Format(PyExc_ValueError,
                 "UnknownFields does not exist. "
                 "The parent message might be cleared.");
    return nullptr;
  }
  Py_ssize_t total_size = self->fields->field_count();
  if (index < 0) {
    index = total_size + index;
  }
  if (index < 0 || index >= total_size) {
    PyErr_Format(PyExc_IndexError,
                 "index (%zd) out of range",
                 index);
    return nullptr;
  }

  return unknown_fields::NewPyUnknownFieldRef(self, index);
}

PyObject* NewPyUnknownFields(CMessage* c_message) {
  PyUnknownFields* self = reinterpret_cast<PyUnknownFields*>(
      PyType_GenericAlloc(&PyUnknownFields_Type, 0));
  if (self == nullptr) {
    return nullptr;
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
  if (self == nullptr) {
    return nullptr;
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
  auto* py_type = Py_TYPE(pself);
  self->~PyUnknownFields();
  py_type->tp_free(pself);
}

static PySequenceMethods SqMethods = {
    Len,     /* sq_length */
    nullptr, /* sq_concat */
    nullptr, /* sq_repeat */
    Item,    /* sq_item */
    nullptr, /* sq_slice */
    nullptr, /* sq_ass_item */
};

}  // namespace unknown_fields

PyTypeObject PyUnknownFields_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) FULL_MODULE_NAME
    ".PyUnknownFields",       // tp_name
    sizeof(PyUnknownFields),  // tp_basicsize
    0,                        //  tp_itemsize
    unknown_fields::Dealloc,  //  tp_dealloc
#if PY_VERSION_HEX < 0x03080000
    nullptr,  // tp_print
#else
    0,  // tp_vectorcall_offset
#endif
    nullptr,                      //  tp_getattr
    nullptr,                      //  tp_setattr
    nullptr,                      //  tp_compare
    nullptr,                      //  tp_repr
    nullptr,                      //  tp_as_number
    &unknown_fields::SqMethods,   //  tp_as_sequence
    nullptr,                      //  tp_as_mapping
    PyObject_HashNotImplemented,  //  tp_hash
    nullptr,                      //  tp_call
    nullptr,                      //  tp_str
    nullptr,                      //  tp_getattro
    nullptr,                      //  tp_setattro
    nullptr,                      //  tp_as_buffer
    Py_TPFLAGS_DEFAULT,           //  tp_flags
    "unknown field set",          //  tp_doc
    nullptr,                      //  tp_traverse
    nullptr,                      //  tp_clear
    nullptr,                      //  tp_richcompare
    0,                            //  tp_weaklistoffset
    nullptr,                      //  tp_iter
    nullptr,                      //  tp_iternext
    nullptr,                      //  tp_methods
    nullptr,                      //  tp_members
    nullptr,                      //  tp_getset
    nullptr,                      //  tp_base
    nullptr,                      //  tp_dict
    nullptr,                      //  tp_descr_get
    nullptr,                      //  tp_descr_set
    0,                            //  tp_dictoffset
    nullptr,                      //  tp_init
};

namespace unknown_field {
static PyObject* PyUnknownFields_FromUnknownFieldSet(
    PyUnknownFields* parent, const UnknownFieldSet& fields) {
  PyUnknownFields* self = reinterpret_cast<PyUnknownFields*>(
      PyType_GenericAlloc(&PyUnknownFields_Type, 0));
  if (self == nullptr) {
    return nullptr;
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
  if (fields == nullptr) {
    PyErr_Format(PyExc_ValueError,
                 "UnknownField does not exist. "
                 "The parent message might be cleared.");
    return nullptr;
  }
  Py_ssize_t total_size = fields->field_count();
  if (self->index >= total_size) {
    PyErr_Format(PyExc_ValueError,
                 "UnknownField does not exist. "
                 "The parent message might be cleared.");
    return nullptr;
  }
  return &fields->field(self->index);
}

static PyObject* GetFieldNumber(PyUnknownFieldRef* self, void *closure) {
  const UnknownField* unknown_field = GetUnknownField(self);
  if (unknown_field == nullptr) {
    return nullptr;
  }
  return PyLong_FromLong(unknown_field->number());
}

using internal::WireFormatLite;
static PyObject* GetWireType(PyUnknownFieldRef* self, void *closure) {
  const UnknownField* unknown_field = GetUnknownField(self);
  if (unknown_field == nullptr) {
    return nullptr;
  }

  // Assign a default value to suppress may-uninitialized warnings (errors
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
  return PyLong_FromLong(wire_type);
}

static PyObject* GetData(PyUnknownFieldRef* self, void *closure) {
  const UnknownField* field = GetUnknownField(self);
  if (field == nullptr) {
    return nullptr;
  }
  PyObject* data = nullptr;
  switch (field->type()) {
    case UnknownField::TYPE_VARINT:
      data = PyLong_FromUnsignedLongLong(field->varint());
      break;
    case UnknownField::TYPE_FIXED32:
      data = PyLong_FromUnsignedLong(field->fixed32());
      break;
    case UnknownField::TYPE_FIXED64:
      data = PyLong_FromUnsignedLongLong(field->fixed64());
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
    {"field_number", (getter)GetFieldNumber, nullptr},
    {"wire_type", (getter)GetWireType, nullptr},
    {"data", (getter)GetData, nullptr},
    {nullptr},
};

}  // namespace unknown_field

PyTypeObject PyUnknownFieldRef_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) FULL_MODULE_NAME
    ".PyUnknownFieldRef",       // tp_name
    sizeof(PyUnknownFieldRef),  //  tp_basicsize
    0,                          //  tp_itemsize
    unknown_field::Dealloc,     //  tp_dealloc
#if PY_VERSION_HEX < 0x03080000
    nullptr,  // tp_print
#else
    0,  // tp_vectorcall_offset
#endif
    nullptr,                      //  tp_getattr
    nullptr,                      //  tp_setattr
    nullptr,                      //  tp_compare
    nullptr,                      //  tp_repr
    nullptr,                      //  tp_as_number
    nullptr,                      //  tp_as_sequence
    nullptr,                      //  tp_as_mapping
    PyObject_HashNotImplemented,  //  tp_hash
    nullptr,                      //  tp_call
    nullptr,                      //  tp_str
    nullptr,                      //  tp_getattro
    nullptr,                      //  tp_setattro
    nullptr,                      //  tp_as_buffer
    Py_TPFLAGS_DEFAULT,           //  tp_flags
    "unknown field",              //  tp_doc
    nullptr,                      //  tp_traverse
    nullptr,                      //  tp_clear
    nullptr,                      //  tp_richcompare
    0,                            //  tp_weaklistoffset
    nullptr,                      //  tp_iter
    nullptr,                      //  tp_iternext
    nullptr,                      //  tp_methods
    nullptr,                      //  tp_members
    unknown_field::Getters,       //  tp_getset
    nullptr,                      //  tp_base
    nullptr,                      //  tp_dict
    nullptr,                      //  tp_descr_get
    nullptr,                      //  tp_descr_set
    0,                            //  tp_dictoffset
    nullptr,                      //  tp_init
};

}  // namespace python
}  // namespace protobuf
}  // namespace google
