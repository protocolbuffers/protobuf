// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/pyext/field.h"

#include "google/protobuf/descriptor.h"
#include "google/protobuf/pyext/descriptor.h"
#include "google/protobuf/pyext/message.h"

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
  if (obj == nullptr) {
    Py_INCREF(self);
    return reinterpret_cast<PyObject*>(self);
  }
  return cmessage::GetFieldValue(reinterpret_cast<CMessage*>(obj),
                                 self->field_descriptor);
}

static int DescrSet(PyMessageFieldProperty* self, PyObject* obj,
                    PyObject* value) {
  if (value == nullptr) {
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
    {"DESCRIPTOR", (getter)GetDescriptor, nullptr, "Field descriptor"},
    {"__doc__", (getter)GetDoc, nullptr, nullptr},
    {nullptr},
};
}  // namespace field

static PyTypeObject _CFieldProperty_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // head
    FULL_MODULE_NAME ".FieldProperty",      // tp_name
    sizeof(PyMessageFieldProperty),         // tp_basicsize
    0,                                      // tp_itemsize
    nullptr,                                // tp_dealloc
#if PY_VERSION_HEX < 0x03080000
    nullptr,  // tp_print
#else
    0,  // tp_vectorcall_offset
#endif
    nullptr,                        // tp_getattr
    nullptr,                        // tp_setattr
    nullptr,                        // tp_compare
    (reprfunc)field::Repr,          // tp_repr
    nullptr,                        // tp_as_number
    nullptr,                        // tp_as_sequence
    nullptr,                        // tp_as_mapping
    nullptr,                        // tp_hash
    nullptr,                        // tp_call
    nullptr,                        // tp_str
    nullptr,                        // tp_getattro
    nullptr,                        // tp_setattro
    nullptr,                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT,             // tp_flags
    "Field property of a Message",  // tp_doc
    nullptr,                        // tp_traverse
    nullptr,                        // tp_clear
    nullptr,                        // tp_richcompare
    0,                              // tp_weaklistoffset
    nullptr,                        // tp_iter
    nullptr,                        // tp_iternext
    nullptr,                        // tp_methods
    nullptr,                        // tp_members
    field::Getters,                 // tp_getset
    nullptr,                        // tp_base
    nullptr,                        // tp_dict
    (descrgetfunc)field::DescrGet,  // tp_descr_get
    (descrsetfunc)field::DescrSet,  // tp_descr_set
    0,                              // tp_dictoffset
    nullptr,                        // tp_init
    nullptr,                        // tp_alloc
    nullptr,                        // tp_new
};
PyTypeObject* CFieldProperty_Type = &_CFieldProperty_Type;

PyObject* NewFieldProperty(const FieldDescriptor* field_descriptor) {
  // Create a new descriptor object
  PyMessageFieldProperty* property =
      PyObject_New(PyMessageFieldProperty, CFieldProperty_Type);
  if (property == nullptr) {
    return nullptr;
  }
  property->field_descriptor = field_descriptor;
  return reinterpret_cast<PyObject*>(property);
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
