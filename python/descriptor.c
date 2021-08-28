/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "python/descriptor.h"

#include "python/protobuf.h"
#include "upb/def.h"

// -----------------------------------------------------------------------------
// DescriptorBase
// -----------------------------------------------------------------------------

// This representation is used by all concrete descriptors.

typedef struct {
  PyObject_HEAD
  PyObject *pool;  // We own a ref.
  const void *def;  // Type depends on the class. Kept alive by "pool".
} PyUpb_DescriptorBase;

PyObject *PyUpb_AnyDescriptor_GetPool(PyObject *desc) {
  PyUpb_DescriptorBase *base = (void *)desc;
  return base->pool;
}

static PyObject *PyUpb_DescriptorBase_New(PyTypeObject *subtype, PyObject *args,
                                          PyObject *kwds) {
  return PyErr_Format(PyExc_RuntimeError,
                      "Creating descriptors directly is not allowed.");
}

static PyObject *PyUpb_DescriptorBase_NewInternal(PyTypeObject *type,
                                                  const void *def,
                                                  PyObject *pool) {
  PyUpb_DescriptorBase *base = (PyUpb_DescriptorBase *)PyUpb_ObjCache_Get(def);

  if (!base) {
    base = PyObject_New(PyUpb_DescriptorBase, type);
    base->pool = pool;
    base->def = def;
    Py_INCREF(pool);
    PyUpb_ObjCache_Add(def, &base->ob_base);
  }

  return &base->ob_base;
}

static void PyUpb_DescriptorBase_Dealloc(PyUpb_DescriptorBase *self) {
  PyUpb_DescriptorBase *base = (PyUpb_DescriptorBase *)self;
  PyUpb_ObjCache_Delete(base->def);
  Py_DECREF(base->pool);
  PyObject_Del(self);
}

#define DESCRIPTOR_BASE_SLOTS                            \
  {Py_tp_new, (void *)&PyUpb_DescriptorBase_New},        \
  {Py_tp_dealloc, (void *)&PyUpb_DescriptorBase_Dealloc}

// -----------------------------------------------------------------------------
// FieldDescriptor
// -----------------------------------------------------------------------------

static PyObject *PyUpb_FieldDescriptor_GetType(PyUpb_DescriptorBase *self,
                                               void *closure) {
  return PyLong_FromLong(upb_fielddef_descriptortype(self->def));
}

static PyObject *PyUpb_FieldDescriptor_GetLabel(PyUpb_DescriptorBase *self,
                                                void *closure) {
  return PyLong_FromLong(upb_fielddef_label(self->def));
}

static PyObject *PyUpb_FieldDescriptor_GetNumber(PyUpb_DescriptorBase *self,
                                                 void *closure) {
  return PyLong_FromLong(upb_fielddef_number(self->def));
}

static PyGetSetDef PyUpb_FieldDescriptor_Getters[] = {
    /*
    { "full_name", (getter)GetFullName, NULL, "Full name"},
    { "name", (getter)GetName, NULL, "Unqualified name"},
    { "camelcase_name", (getter)GetCamelcaseName, NULL, "Camelcase name"},
    { "json_name", (getter)GetJsonName, NULL, "Json name"},
    { "file", (getter)GetFile, NULL, "File Descriptor"},
    */
    {"type", (getter)PyUpb_FieldDescriptor_GetType, NULL, "Type"},
    /*
    { "cpp_type", (getter)PyUpb_FieldDescriptor_GetCppType, NULL, "C++ Type"},
    */
    {"label", (getter)PyUpb_FieldDescriptor_GetLabel, NULL, "Label"},
    {"number", (getter)PyUpb_FieldDescriptor_GetNumber, NULL, "Number"},
    /*
    { "index", (getter)GetIndex, NULL, "Index"},
    { "default_value", (getter)GetDefaultValue, NULL, "Default Value"},
    { "has_default_value", (getter)HasDefaultValue},
    { "is_extension", (getter)IsExtension, NULL, "ID"},
    { "id", (getter)GetID, NULL, "ID"},
    { "_cdescriptor", (getter)GetCDescriptor, NULL, "HAACK REMOVE ME"},

    { "message_type", (getter)GetMessageType, (setter)SetMessageType,
      "Message type"},
    { "enum_type", (getter)GetEnumType, (setter)SetEnumType, "Enum type"},
    { "containing_type", (getter)GetContainingType, (setter)SetContainingType,
      "Containing type"},
    { "extension_scope", (getter)GetExtensionScope, (setter)NULL,
      "Extension scope"},
    { "containing_oneof", (getter)GetContainingOneof,
    (setter)SetContainingOneof, "Containing oneof"}, { "has_options",
    (getter)GetHasOptions, (setter)SetHasOptions, "Has Options"}, { "_options",
    (getter)NULL, (setter)SetOptions, "Options"}, { "_serialized_options",
    (getter)NULL, (setter)SetSerializedOptions, "Serialized Options"},
  */
    {NULL}};

static PyMethodDef PyUpb_FieldDescriptor_Methods[] = {
    /*
    { "GetOptions", (PyCFunction)GetOptions, METH_NOARGS, },
    */
    {NULL}};

static PyType_Slot PyUpb_FieldDescriptor_Slots[] = {
    DESCRIPTOR_BASE_SLOTS,
    {Py_tp_methods, PyUpb_FieldDescriptor_Methods},
    {Py_tp_getset, PyUpb_FieldDescriptor_Getters},
    {0, NULL}};

static PyType_Spec PyUpb_FieldDescriptor_Spec = {
    PYUPB_MODULE_NAME ".FieldDescriptor",
    sizeof(PyUpb_DescriptorBase),
    0,  // tp_itemsize
    Py_TPFLAGS_DEFAULT,
    PyUpb_FieldDescriptor_Slots,
};

PyObject *PyUpb_FieldDescriptor_GetOrCreateWrapper(const upb_fielddef *field,
                                                   PyObject *pool) {
  PyUpb_ModuleState *state = PyUpb_ModuleState_Get();
  return PyUpb_DescriptorBase_NewInternal(state->field_descriptor_type, field,
                                          pool);
}

// -----------------------------------------------------------------------------
// FileDescriptor
// -----------------------------------------------------------------------------
//
static PyObject *PyUpb_FileDescriptor_GetName(PyUpb_DescriptorBase *self,
                                              void *closure) {
  return PyUnicode_FromString(upb_filedef_name(self->def));
}

static PyGetSetDef PyUpb_FileDescriptor_Getters[] =
    {
        /*
        { "pool", (getter)GetPool, NULL, "pool"},
        */
        {"name", (getter)PyUpb_FileDescriptor_GetName, NULL, "name"},
        /*
        { "package", (getter)GetPackage, NULL, "package"},
        { "serialized_pb", (getter)GetSerializedPb},
        { "message_types_by_name", PyUpb_FileDescriptor_GetMessageTypesByName,
        NULL, "Messages by name"}, { "enum_types_by_name",
        PyUpb_FileDescriptor_GetEnumTypesByName, NULL, "Enums by name"}, {
        "extensions_by_name", (getter)GetExtensionsByName, NULL, "Extensions by
        name"}, { "services_by_name", (getter)GetServicesByName, NULL, "Services
        by name"}, { "dependencies", (getter)GetDependencies, NULL,
        "Dependencies"}, { "public_dependencies", (getter)GetPublicDependencies,
        NULL, "Dependencies"},

        { "has_options", (getter)GetHasOptions, (setter)SetHasOptions, "Has
        Options"}, { "_options", (getter)NULL, (setter)SetOptions, "Options"},
        { "_serialized_options", (getter)NULL, (setter)SetSerializedOptions,
          "Serialized Options"},
        { "syntax", (getter)GetSyntax, (setter)NULL, "Syntax"},
      */
        {NULL}};

static PyMethodDef PyUpb_FileDescriptor_Methods[] = {
    /*
      { "GetOptions", (PyCFunction)GetOptions, METH_NOARGS, },
      { "CopyToProto", (PyCFunction)CopyToProto, METH_O, },
    */
    {NULL}};

static PyType_Slot PyUpb_FileDescriptor_Slots[] = {
    DESCRIPTOR_BASE_SLOTS,
    {Py_tp_methods, PyUpb_FileDescriptor_Methods},
    {Py_tp_getset, PyUpb_FileDescriptor_Getters},
    {0, NULL}};

static PyType_Spec PyUpb_FileDescriptor_Spec = {
    PYUPB_MODULE_NAME ".FileDescriptor",  // tp_name
    sizeof(PyUpb_DescriptorBase),         // tp_basicsize
    0,                                    // tp_itemsize
    Py_TPFLAGS_DEFAULT,                   // tp_flags
    PyUpb_FileDescriptor_Slots,
};

PyObject *PyUpb_FileDescriptor_GetOrCreateWrapper(const upb_filedef *file,
                                                  PyObject *pool) {
  PyUpb_ModuleState *state = PyUpb_ModuleState_Get();
  return PyUpb_DescriptorBase_NewInternal(state->file_descriptor_type, file,
                                          pool);
}

const upb_filedef *PyUpb_FileDescriptor_GetDef(PyObject *_self) {
  PyUpb_DescriptorBase *self = (void *)_self;
  return self->def;
}

// -----------------------------------------------------------------------------
// Top Level
// -----------------------------------------------------------------------------

bool PyUpb_InitDescriptor(PyObject *m) {
  PyUpb_ModuleState *s = PyUpb_ModuleState_Get();

  s->field_descriptor_type =
      AddObject(m, "FieldDescriptor", &PyUpb_FieldDescriptor_Spec);
  s->file_descriptor_type =
      AddObject(m, "FileDescriptor", &PyUpb_FileDescriptor_Spec);

  return s->field_descriptor_type && s->file_descriptor_type;
}
