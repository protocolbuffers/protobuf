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

// Author: petar@google.com (Petar Petrov)

#include <Python.h>
#include <string>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

#define C(str) const_cast<char*>(str)

#if PY_MAJOR_VERSION >= 3
  #define PyString_FromStringAndSize PyUnicode_FromStringAndSize
  #define PyInt_FromLong PyLong_FromLong
  #if PY_VERSION_HEX < 0x03030000
    #error "Python 3.0 - 3.2 are not supported."
  #else
  #define PyString_AsString(ob) \
    (PyUnicode_Check(ob)? PyUnicode_AsUTF8(ob): PyBytes_AS_STRING(ob))
  #endif
#endif

namespace google {
namespace protobuf {
namespace python {


#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(type, size) PyObject_HEAD_INIT(type) size,
#endif
#ifndef Py_TYPE
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif


static google::protobuf::DescriptorPool* g_descriptor_pool = NULL;

namespace cfield_descriptor {

static void Dealloc(CFieldDescriptor* self) {
  Py_CLEAR(self->descriptor_field);
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

static PyObject* GetFullName(CFieldDescriptor* self, void *closure) {
  return PyString_FromStringAndSize(
      self->descriptor->full_name().c_str(),
      self->descriptor->full_name().size());
}

static PyObject* GetName(CFieldDescriptor *self, void *closure) {
  return PyString_FromStringAndSize(
      self->descriptor->name().c_str(),
      self->descriptor->name().size());
}

static PyObject* GetCppType(CFieldDescriptor *self, void *closure) {
  return PyInt_FromLong(self->descriptor->cpp_type());
}

static PyObject* GetLabel(CFieldDescriptor *self, void *closure) {
  return PyInt_FromLong(self->descriptor->label());
}

static PyObject* GetID(CFieldDescriptor *self, void *closure) {
  return PyLong_FromVoidPtr(self);
}

static PyGetSetDef Getters[] = {
  { C("full_name"), (getter)GetFullName, NULL, "Full name", NULL},
  { C("name"), (getter)GetName, NULL, "last name", NULL},
  { C("cpp_type"), (getter)GetCppType, NULL, "C++ Type", NULL},
  { C("label"), (getter)GetLabel, NULL, "Label", NULL},
  { C("id"), (getter)GetID, NULL, "ID", NULL},
  {NULL}
};

}  // namespace cfield_descriptor

PyTypeObject CFieldDescriptor_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  C("google.protobuf.internal."
    "_net_proto2___python."
    "CFieldDescriptor"),                // tp_name
  sizeof(CFieldDescriptor),             // tp_basicsize
  0,                                    // tp_itemsize
  (destructor)cfield_descriptor::Dealloc,  // tp_dealloc
  0,                                    // tp_print
  0,                                    // tp_getattr
  0,                                    // tp_setattr
  0,                                    // tp_compare
  0,                                    // tp_repr
  0,                                    // tp_as_number
  0,                                    // tp_as_sequence
  0,                                    // tp_as_mapping
  0,                                    // tp_hash
  0,                                    // tp_call
  0,                                    // tp_str
  0,                                    // tp_getattro
  0,                                    // tp_setattro
  0,                                    // tp_as_buffer
  Py_TPFLAGS_DEFAULT,                   // tp_flags
  C("A Field Descriptor"),              // tp_doc
  0,                                    // tp_traverse
  0,                                    // tp_clear
  0,                                    // tp_richcompare
  0,                                    // tp_weaklistoffset
  0,                                    // tp_iter
  0,                                    // tp_iternext
  0,                                    // tp_methods
  0,                                    // tp_members
  cfield_descriptor::Getters,           // tp_getset
  0,                                    // tp_base
  0,                                    // tp_dict
  0,                                    // tp_descr_get
  0,                                    // tp_descr_set
  0,                                    // tp_dictoffset
  0,                                    // tp_init
  PyType_GenericAlloc,                  // tp_alloc
  PyType_GenericNew,                    // tp_new
  PyObject_Del,                         // tp_free
};

namespace cdescriptor_pool {

static void Dealloc(CDescriptorPool* self) {
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

static PyObject* NewCDescriptor(
    const google::protobuf::FieldDescriptor* field_descriptor) {
  CFieldDescriptor* cfield_descriptor = PyObject_New(
      CFieldDescriptor, &CFieldDescriptor_Type);
  if (cfield_descriptor == NULL) {
    return NULL;
  }
  cfield_descriptor->descriptor = field_descriptor;
  cfield_descriptor->descriptor_field = NULL;

  return reinterpret_cast<PyObject*>(cfield_descriptor);
}

PyObject* FindFieldByName(CDescriptorPool* self, PyObject* name) {
  const char* full_field_name = PyString_AsString(name);
  if (full_field_name == NULL) {
    return NULL;
  }

  const google::protobuf::FieldDescriptor* field_descriptor = NULL;

  field_descriptor = self->pool->FindFieldByName(full_field_name);

  if (field_descriptor == NULL) {
    PyErr_Format(PyExc_TypeError, "Couldn't find field %.200s",
                 full_field_name);
    return NULL;
  }

  return NewCDescriptor(field_descriptor);
}

PyObject* FindExtensionByName(CDescriptorPool* self, PyObject* arg) {
  const char* full_field_name = PyString_AsString(arg);
  if (full_field_name == NULL) {
    return NULL;
  }

  const google::protobuf::FieldDescriptor* field_descriptor =
      self->pool->FindExtensionByName(full_field_name);
  if (field_descriptor == NULL) {
    PyErr_Format(PyExc_TypeError, "Couldn't find field %.200s",
                 full_field_name);
    return NULL;
  }

  return NewCDescriptor(field_descriptor);
}

static PyMethodDef Methods[] = {
  { C("FindFieldByName"),
    (PyCFunction)FindFieldByName,
    METH_O,
    C("Searches for a field descriptor by full name.") },
  { C("FindExtensionByName"),
    (PyCFunction)FindExtensionByName,
    METH_O,
    C("Searches for extension descriptor by full name.") },
  {NULL}
};

}  // namespace cdescriptor_pool

PyTypeObject CDescriptorPool_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  C("google.protobuf.internal."
    "_net_proto2___python."
    "CFieldDescriptor"),               // tp_name
  sizeof(CDescriptorPool),             // tp_basicsize
  0,                                   // tp_itemsize
  (destructor)cdescriptor_pool::Dealloc,  // tp_dealloc
  0,                                   // tp_print
  0,                                   // tp_getattr
  0,                                   // tp_setattr
  0,                                   // tp_compare
  0,                                   // tp_repr
  0,                                   // tp_as_number
  0,                                   // tp_as_sequence
  0,                                   // tp_as_mapping
  0,                                   // tp_hash
  0,                                   // tp_call
  0,                                   // tp_str
  0,                                   // tp_getattro
  0,                                   // tp_setattro
  0,                                   // tp_as_buffer
  Py_TPFLAGS_DEFAULT,                  // tp_flags
  C("A Descriptor Pool"),              // tp_doc
  0,                                   // tp_traverse
  0,                                   // tp_clear
  0,                                   // tp_richcompare
  0,                                   // tp_weaklistoffset
  0,                                   // tp_iter
  0,                                   // tp_iternext
  cdescriptor_pool::Methods,           // tp_methods
  0,                                   // tp_members
  0,                                   // tp_getset
  0,                                   // tp_base
  0,                                   // tp_dict
  0,                                   // tp_descr_get
  0,                                   // tp_descr_set
  0,                                   // tp_dictoffset
  0,                                   // tp_init
  PyType_GenericAlloc,                 // tp_alloc
  PyType_GenericNew,                   // tp_new
  PyObject_Del,                        // tp_free
};

google::protobuf::DescriptorPool* GetDescriptorPool() {
  if (g_descriptor_pool == NULL) {
    g_descriptor_pool = new google::protobuf::DescriptorPool(
        google::protobuf::DescriptorPool::generated_pool());
  }
  return g_descriptor_pool;
}

PyObject* Python_NewCDescriptorPool(PyObject* ignored, PyObject* args) {
  CDescriptorPool* cdescriptor_pool = PyObject_New(
      CDescriptorPool, &CDescriptorPool_Type);
  if (cdescriptor_pool == NULL) {
    return NULL;
  }
  cdescriptor_pool->pool = GetDescriptorPool();
  return reinterpret_cast<PyObject*>(cdescriptor_pool);
}


// Collects errors that occur during proto file building to allow them to be
// propagated in the python exception instead of only living in ERROR logs.
class BuildFileErrorCollector : public google::protobuf::DescriptorPool::ErrorCollector {
 public:
  BuildFileErrorCollector() : error_message(""), had_errors(false) {}

  void AddError(const string& filename, const string& element_name,
                const Message* descriptor, ErrorLocation location,
                const string& message) {
    // Replicates the logging behavior that happens in the C++ implementation
    // when an error collector is not passed in.
    if (!had_errors) {
      error_message +=
          ("Invalid proto descriptor for file \"" + filename + "\":\n");
    }
    // As this only happens on failure and will result in the program not
    // running at all, no effort is made to optimize this string manipulation.
    error_message += ("  " + element_name + ": " + message + "\n");
  }

  string error_message;
  bool had_errors;
};

PyObject* Python_BuildFile(PyObject* ignored, PyObject* arg) {
  char* message_type;
  Py_ssize_t message_len;

  if (PyBytes_AsStringAndSize(arg, &message_type, &message_len) < 0) {
    return NULL;
  }

  google::protobuf::FileDescriptorProto file_proto;
  if (!file_proto.ParseFromArray(message_type, message_len)) {
    PyErr_SetString(PyExc_TypeError, "Couldn't parse file content!");
    return NULL;
  }

  if (google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
          file_proto.name()) != NULL) {
    Py_RETURN_NONE;
  }

  BuildFileErrorCollector error_collector;
  const google::protobuf::FileDescriptor* descriptor =
      GetDescriptorPool()->BuildFileCollectingErrors(file_proto,
                                                     &error_collector);
  if (descriptor == NULL) {
    PyErr_Format(PyExc_TypeError,
                 "Couldn't build proto file into descriptor pool!\n%s",
                 error_collector.error_message.c_str());
    return NULL;
  }

  Py_RETURN_NONE;
}

bool InitDescriptor() {
  CFieldDescriptor_Type.tp_new = PyType_GenericNew;
  if (PyType_Ready(&CFieldDescriptor_Type) < 0)
    return false;

  CDescriptorPool_Type.tp_new = PyType_GenericNew;
  if (PyType_Ready(&CDescriptorPool_Type) < 0)
    return false;

  return true;
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
