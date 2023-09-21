// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC nor the names of its
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

#include "upb/python/field.h"

#include "upb/python/descriptor.h"
#include "upb/python/message.h"
#include "upb/upb/reflection/def.h"

typedef struct {
  PyObject_HEAD;
  const upb_FieldDef* field;
} PyUpb_FieldProperty;

static PyObject* PyUpb_FieldProperty_Repr(PyUpb_FieldProperty* self) {
  return PyUnicode_FromString(upb_FieldDef_FullName(self->field));
}

static PyObject* PyUpb_FieldProperty_DescrGet(PyUpb_FieldProperty* self,
                                              PyObject* obj, PyObject* type) {
  if (obj == NULL) {
    Py_INCREF(self);
    return (PyObject*)(self);
  }
  const upb_FieldDef* field = self->field;
  if (upb_FieldDef_ContainingType(field) != PyUpb_Message_GetMsgdef(obj)) {
    PyErr_Format(PyExc_TypeError,
                 "descriptor to field '%s' doesn't apply to '%s' object",
                 upb_FieldDef_FullName(field),
                 upb_MessageDef_FullName(PyUpb_Message_GetMsgdef(obj)));
    return NULL;
  }
  return PyUpb_Message_GetFieldValue(obj, field);
}

static int PyUpb_FieldProperty_DescrSet(PyUpb_FieldProperty* self,
                                        PyObject* obj, PyObject* value) {
  if (value == NULL) {
    PyErr_SetString(PyExc_AttributeError, "Cannot delete field attribute");
    return -1;
  }
  return PyUpb_Message_SetFieldValue(obj, self->field, value,
                                     PyExc_AttributeError);
}

static PyObject* GetDescriptor(PyUpb_FieldProperty* self, void* closure) {
  return PyUpb_FieldDescriptor_Get(self->field);
}

static PyObject* GetDoc(PyUpb_FieldProperty* self, void* closure) {
  return PyUnicode_FromString(upb_FieldDef_FullName(self->field));
}

static PyGetSetDef PyUpb_FieldProperty_Getters[] = {
    {"DESCRIPTOR", (getter)GetDescriptor, NULL, "Field descriptor"},
    {"__doc__", (getter)GetDoc, NULL, NULL},
    {NULL}};

static PyType_Slot PyUpb_FieldProperty_Slots[] = {
    {Py_tp_repr, PyUpb_FieldProperty_Repr},
    {Py_tp_getset, PyUpb_FieldProperty_Getters},
    {Py_tp_descr_get, PyUpb_FieldProperty_DescrGet},
    {Py_tp_descr_set, PyUpb_FieldProperty_DescrSet},
    {0, NULL}};

static PyType_Spec PyUpb_FieldProperty_Spec = {
    PYUPB_MODULE_NAME ".FieldProperty",
    sizeof(PyUpb_FieldProperty),
    0,  // tp_itemsize
    Py_TPFLAGS_DEFAULT,
    PyUpb_FieldProperty_Slots,
};

// -----------------------------------------------------------------------------
// Top Level
// -----------------------------------------------------------------------------
PyObject* PyUpb_FieldProperty_New(const upb_FieldDef* field) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_Get();
  PyTypeObject* cls = state->field_property;
  PyUpb_FieldProperty* field_property = (void*)PyType_GenericAlloc(cls, 0);
  field_property->field = field;
  return &field_property->ob_base;
}

bool PyUpb_FieldProperty_Init(PyObject* m) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_GetFromModule(m);

  state->field_property = PyUpb_AddClass(m, &PyUpb_FieldProperty_Spec);
  return (state->field_property != NULL);
}
