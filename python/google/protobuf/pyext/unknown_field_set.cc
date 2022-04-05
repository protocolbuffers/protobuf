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

#include <google/protobuf/pyext/unknown_field_set.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <memory>
#include <set>

#include <google/protobuf/message.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

namespace google {
namespace protobuf {
namespace python {

namespace unknown_field_set {

static Py_ssize_t Len(PyObject* pself) {
  PyUnknownFieldSet* self = reinterpret_cast<PyUnknownFieldSet*>(pself);
  if (self->fields == nullptr) {
    PyErr_Format(PyExc_ValueError, "UnknownFieldSet does not exist. ");
    return -1;
  }
  return self->fields->field_count();
}

PyObject* NewPyUnknownField(PyUnknownFieldSet* parent, Py_ssize_t index);

static PyObject* Item(PyObject* pself, Py_ssize_t index) {
  PyUnknownFieldSet* self = reinterpret_cast<PyUnknownFieldSet*>(pself);
  if (self->fields == nullptr) {
    PyErr_Format(PyExc_ValueError, "UnknownFieldSet does not exist. ");
    return nullptr;
  }
  Py_ssize_t total_size = self->fields->field_count();
  if (index < 0) {
    index = total_size + index;
  }
  if (index < 0 || index >= total_size) {
    PyErr_Format(PyExc_IndexError, "index (%zd) out of range", index);
    return nullptr;
  }
  return unknown_field_set::NewPyUnknownField(self, index);
}

PyObject* New(PyTypeObject* type, PyObject* args, PyObject* kwargs) {
  if (args == nullptr || PyTuple_Size(args) != 1) {
    PyErr_SetString(PyExc_TypeError,
                    "Must provide a message to create UnknownFieldSet");
    return nullptr;
  }

  PyObject* c_message;
  if (!PyArg_ParseTuple(args, "O", &c_message)) {
    PyErr_SetString(PyExc_TypeError,
                    "Must provide a message to create UnknownFieldSet");
    return nullptr;
  }

  if (!PyObject_TypeCheck(c_message, CMessage_Type)) {
    PyErr_Format(PyExc_TypeError,
                 "Parameter to UnknownFieldSet() must be a message "
                 "got %s.",
                 Py_TYPE(c_message)->tp_name);
    return nullptr;
  }

  PyUnknownFieldSet* self = reinterpret_cast<PyUnknownFieldSet*>(
      PyType_GenericAlloc(&PyUnknownFieldSet_Type, 0));
  if (self == nullptr) {
    return nullptr;
  }

  // Top UnknownFieldSet should set parent nullptr.
  self->parent = nullptr;

  // Copy c_message's UnknownFieldSet.
  Message* message = reinterpret_cast<CMessage*>(c_message)->message;
  const Reflection* reflection = message->GetReflection();
  self->fields = new google::protobuf::UnknownFieldSet;
  self->fields->MergeFrom(reflection->GetUnknownFields(*message));
  return reinterpret_cast<PyObject*>(self);
}

PyObject* NewPyUnknownField(PyUnknownFieldSet* parent, Py_ssize_t index) {
  PyUnknownField* self = reinterpret_cast<PyUnknownField*>(
      PyType_GenericAlloc(&PyUnknownField_Type, 0));
  if (self == nullptr) {
    return nullptr;
  }

  Py_INCREF(parent);
  self->parent = parent;
  self->index = index;

  return reinterpret_cast<PyObject*>(self);
}

static void Dealloc(PyObject* pself) {
  PyUnknownFieldSet* self = reinterpret_cast<PyUnknownFieldSet*>(pself);
  if (self->parent == nullptr) {
    delete self->fields;
  } else {
    Py_CLEAR(self->parent);
  }
  auto* py_type = Py_TYPE(pself);
  self->~PyUnknownFieldSet();
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

}  // namespace unknown_field_set

PyTypeObject PyUnknownFieldSet_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) FULL_MODULE_NAME
    ".PyUnknownFieldSet",        // tp_name
    sizeof(PyUnknownFieldSet),   // tp_basicsize
    0,                           //  tp_itemsize
    unknown_field_set::Dealloc,  //  tp_dealloc
#if PY_VERSION_HEX < 0x03080000
    nullptr,  // tp_print
#else
    0,  // tp_vectorcall_offset
#endif
    nullptr,                        //  tp_getattr
    nullptr,                        //  tp_setattr
    nullptr,                        //  tp_compare
    nullptr,                        //  tp_repr
    nullptr,                        //  tp_as_number
    &unknown_field_set::SqMethods,  //  tp_as_sequence
    nullptr,                        //  tp_as_mapping
    PyObject_HashNotImplemented,    //  tp_hash
    nullptr,                        //  tp_call
    nullptr,                        //  tp_str
    nullptr,                        //  tp_getattro
    nullptr,                        //  tp_setattro
    nullptr,                        //  tp_as_buffer
    Py_TPFLAGS_DEFAULT,             //  tp_flags
    "unknown field set",            //  tp_doc
    nullptr,                        //  tp_traverse
    nullptr,                        //  tp_clear
    nullptr,                        //  tp_richcompare
    0,                              //  tp_weaklistoffset
    nullptr,                        //  tp_iter
    nullptr,                        //  tp_iternext
    nullptr,                        //  tp_methods
    nullptr,                        //  tp_members
    nullptr,                        //  tp_getset
    nullptr,                        //  tp_base
    nullptr,                        //  tp_dict
    nullptr,                        //  tp_descr_get
    nullptr,                        //  tp_descr_set
    0,                              //  tp_dictoffset
    nullptr,                        //  tp_init
    nullptr,                        //  tp_alloc
    unknown_field_set::New,         //  tp_new
};

namespace unknown_field {
static PyObject* PyUnknownFieldSet_FromUnknownFieldSet(
    PyUnknownFieldSet* parent, const UnknownFieldSet& fields) {
  PyUnknownFieldSet* self = reinterpret_cast<PyUnknownFieldSet*>(
      PyType_GenericAlloc(&PyUnknownFieldSet_Type, 0));
  if (self == nullptr) {
    return nullptr;
  }

  Py_INCREF(parent);
  self->parent = parent;
  self->fields = const_cast<UnknownFieldSet*>(&fields);

  return reinterpret_cast<PyObject*>(self);
}

const UnknownField* GetUnknownField(PyUnknownField* self) {
  const UnknownFieldSet* fields = self->parent->fields;
  if (fields == nullptr) {
    PyErr_Format(PyExc_ValueError, "UnknownField does not exist. ");
    return nullptr;
  }
  Py_ssize_t total_size = fields->field_count();
  if (self->index >= total_size) {
    PyErr_Format(PyExc_ValueError, "UnknownField does not exist. ");
    return nullptr;
  }
  return &fields->field(self->index);
}

static PyObject* GetFieldNumber(PyUnknownField* self, void* closure) {
  const UnknownField* unknown_field = GetUnknownField(self);
  if (unknown_field == nullptr) {
    return nullptr;
  }
  return PyLong_FromLong(unknown_field->number());
}

using internal::WireFormatLite;
static PyObject* GetWireType(PyUnknownField* self, void* closure) {
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

static PyObject* GetData(PyUnknownField* self, void* closure) {
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
      data =
          PyUnknownFieldSet_FromUnknownFieldSet(self->parent, field->group());
      break;
  }
  return data;
}

static void Dealloc(PyObject* pself) {
  PyUnknownField* self = reinterpret_cast<PyUnknownField*>(pself);
  Py_CLEAR(self->parent);
}

static PyGetSetDef Getters[] = {
    {"field_number", (getter)GetFieldNumber, nullptr},
    {"wire_type", (getter)GetWireType, nullptr},
    {"data", (getter)GetData, nullptr},
    {nullptr},
};

}  // namespace unknown_field

PyTypeObject PyUnknownField_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) FULL_MODULE_NAME
    ".PyUnknownField",       // tp_name
    sizeof(PyUnknownField),  //  tp_basicsize
    0,                       //  tp_itemsize
    unknown_field::Dealloc,  //  tp_dealloc
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
