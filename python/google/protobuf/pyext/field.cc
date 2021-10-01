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

#include <google/protobuf/pyext/field.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/message.h>

namespace google {
namespace protobuf {
namespace python {

namespace field {

static PyObject* Repr(PyMessageFieldProperty* self) {
  return PyUnicode_FromFormat("<field property '%s'>",
                              self->field_descriptor->full_name().c_str());
}

static PyObject* DescrGet(PyMessageFieldProperty* self, PyObject* obj,
                          PyObject* type) {
  if (obj == NULL) {
    Py_INCREF(self);
    return reinterpret_cast<PyObject*>(self);
  }
  return cmessage::GetFieldValue(reinterpret_cast<CMessage*>(obj),
                                 self->field_descriptor);
}

static int DescrSet(PyMessageFieldProperty* self, PyObject* obj,
                    PyObject* value) {
  if (value == NULL) {
    PyErr_SetString(PyExc_AttributeError, "Cannot delete field attribute");
    return -1;
  }
  return cmessage::SetFieldValue(reinterpret_cast<CMessage*>(obj),
                                 self->field_descriptor, value);
}

static PyObject* GetDescriptor(PyMessageFieldProperty* self, void* closure) {
  return PyFieldDescriptor_FromDescriptor(self->field_descriptor);
}

static PyObject* GetDoc(PyMessageFieldProperty* self, void* closure) {
  return PyUnicode_FromFormat("Field %s",
                              self->field_descriptor->full_name().c_str());
}

static PyGetSetDef Getters[] = {
    {"DESCRIPTOR", (getter)GetDescriptor, NULL, "Field descriptor"},
    {"__doc__", (getter)GetDoc, NULL, NULL},
    {NULL}};
}  // namespace field

static PyTypeObject _CFieldProperty_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)    // head
    FULL_MODULE_NAME ".FieldProperty",        // tp_name
    sizeof(PyMessageFieldProperty),           // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    (reprfunc)field::Repr,                    // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    0,                                        // tp_hash
    0,                                        // tp_call
    0,                                        // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT,                       // tp_flags
    "Field property of a Message",            // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    0,                                        // tp_richcompare
    0,                                        // tp_weaklistoffset
    0,                                        // tp_iter
    0,                                        // tp_iternext
    0,                                        // tp_methods
    0,                                        // tp_members
    field::Getters,                           // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    (descrgetfunc)field::DescrGet,            // tp_descr_get
    (descrsetfunc)field::DescrSet,            // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    0,                                        // tp_new
};
PyTypeObject* CFieldProperty_Type = &_CFieldProperty_Type;

PyObject* NewFieldProperty(const FieldDescriptor* field_descriptor) {
  // Create a new descriptor object
  PyMessageFieldProperty* property =
      PyObject_New(PyMessageFieldProperty, CFieldProperty_Type);
  if (property == NULL) {
    return NULL;
  }
  property->field_descriptor = field_descriptor;
  return reinterpret_cast<PyObject*>(property);
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
