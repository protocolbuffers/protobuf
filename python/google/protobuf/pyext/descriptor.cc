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
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

#define C(str) const_cast<char*>(str)

#if PY_MAJOR_VERSION >= 3
  #define PyString_FromStringAndSize PyUnicode_FromStringAndSize
  #define PyInt_FromLong PyLong_FromLong
  #if PY_VERSION_HEX < 0x03030000
    #error "Python 3.0 - 3.2 are not supported."
  #else
  #define PyString_AsString(ob) \
    (PyUnicode_Check(ob)? PyUnicode_AsUTF8(ob): PyBytes_AsString(ob))
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

namespace cmessage_descriptor {

static void Dealloc(CMessageDescriptor* self) {
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

static PyObject* GetFullName(CMessageDescriptor* self, void *closure) {
  return PyString_FromStringAndSize(
      self->descriptor->full_name().c_str(),
      self->descriptor->full_name().size());
}

static PyObject* GetName(CMessageDescriptor *self, void *closure) {
  return PyString_FromStringAndSize(
      self->descriptor->name().c_str(),
      self->descriptor->name().size());
}

static PyGetSetDef Getters[] = {
  { C("full_name"), (getter)GetFullName, NULL, "Full name", NULL},
  { C("name"), (getter)GetName, NULL, "Unqualified name", NULL},
  {NULL}
};

}  // namespace cmessage_descriptor

PyTypeObject CMessageDescriptor_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  C("google.protobuf.internal."
    "_net_proto2___python."
    "CMessageDescriptor"),              // tp_name
  sizeof(CMessageDescriptor),           // tp_basicsize
  0,                                    // tp_itemsize
  (destructor)cmessage_descriptor::Dealloc,  // tp_dealloc
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
  C("A Message Descriptor"),            // tp_doc
  0,                                    // tp_traverse
  0,                                    // tp_clear
  0,                                    // tp_richcompare
  0,                                    // tp_weaklistoffset
  0,                                    // tp_iter
  0,                                    // tp_iternext
  0,                                    // tp_methods
  0,                                    // tp_members
  cmessage_descriptor::Getters,         // tp_getset
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


namespace cfield_descriptor {

static void Dealloc(CFieldDescriptor* self) {
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
  { C("name"), (getter)GetName, NULL, "Unqualified name", NULL},
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

PyDescriptorPool* NewDescriptorPool() {
  PyDescriptorPool* cdescriptor_pool = PyObject_New(
      PyDescriptorPool, &PyDescriptorPool_Type);
  if (cdescriptor_pool == NULL) {
    return NULL;
  }

  // Build a DescriptorPool for messages only declared in Python libraries.
  // generated_pool() contains all messages linked in C++ libraries, and is used
  // as underlay.
  cdescriptor_pool->pool = new google::protobuf::DescriptorPool(
      google::protobuf::DescriptorPool::generated_pool());

  // TODO(amauryfa): Rewrite the SymbolDatabase in C so that it uses the same
  // storage.
  cdescriptor_pool->classes_by_descriptor =
      new PyDescriptorPool::ClassesByMessageMap();

  return cdescriptor_pool;
}

static void Dealloc(PyDescriptorPool* self) {
  typedef PyDescriptorPool::ClassesByMessageMap::iterator iterator;
  for (iterator it = self->classes_by_descriptor->begin();
       it != self->classes_by_descriptor->end(); ++it) {
    Py_DECREF(it->second);
  }
  delete self->classes_by_descriptor;
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

const google::protobuf::Descriptor* FindMessageTypeByName(PyDescriptorPool* self,
                                                const string& name) {
  return self->pool->FindMessageTypeByName(name);
}

static PyObject* NewCMessageDescriptor(
    const google::protobuf::Descriptor* message_descriptor) {
  CMessageDescriptor* cmessage_descriptor = PyObject_New(
      CMessageDescriptor, &CMessageDescriptor_Type);
  if (cmessage_descriptor == NULL) {
    return NULL;
  }
  cmessage_descriptor->descriptor = message_descriptor;

  return reinterpret_cast<PyObject*>(cmessage_descriptor);
}

static PyObject* NewCFieldDescriptor(
    const google::protobuf::FieldDescriptor* field_descriptor) {
  CFieldDescriptor* cfield_descriptor = PyObject_New(
      CFieldDescriptor, &CFieldDescriptor_Type);
  if (cfield_descriptor == NULL) {
    return NULL;
  }
  cfield_descriptor->descriptor = field_descriptor;

  return reinterpret_cast<PyObject*>(cfield_descriptor);
}

// Add a message class to our database.
const google::protobuf::Descriptor* RegisterMessageClass(
    PyDescriptorPool* self, PyObject *message_class, PyObject* descriptor) {
  ScopedPyObjectPtr full_message_name(
      PyObject_GetAttrString(descriptor, "full_name"));
  const char* full_name = PyString_AsString(full_message_name);
  if (full_name == NULL) {
    return NULL;
  }
  const Descriptor *message_descriptor =
      self->pool->FindMessageTypeByName(full_name);
  if (!message_descriptor) {
    PyErr_Format(PyExc_TypeError, "Could not find C++ descriptor for '%s'",
                 full_name);
    return NULL;
  }
  Py_INCREF(message_class);
  typedef PyDescriptorPool::ClassesByMessageMap::iterator iterator;
  std::pair<iterator, bool> ret = self->classes_by_descriptor->insert(
      make_pair(message_descriptor, message_class));
  if (!ret.second) {
    // Update case: DECREF the previous value.
    Py_DECREF(ret.first->second);
    ret.first->second = message_class;
  }

  // Also add the C++ descriptor to the Python descriptor class.
  ScopedPyObjectPtr cdescriptor(NewCMessageDescriptor(message_descriptor));
  if (cdescriptor == NULL) {
    return NULL;
  }
  if (PyObject_SetAttrString(
          descriptor, "_cdescriptor", cdescriptor) < 0) {
      return NULL;
  }
  return message_descriptor;
}

// Retrieve the message class added to our database.
PyObject *GetMessageClass(PyDescriptorPool* self,
                          const Descriptor *message_descriptor) {
  typedef PyDescriptorPool::ClassesByMessageMap::iterator iterator;
  iterator ret = self->classes_by_descriptor->find(message_descriptor);
  if (ret == self->classes_by_descriptor->end()) {
    PyErr_Format(PyExc_TypeError, "No message class registered for '%s'",
                 message_descriptor->full_name().c_str());
    return NULL;
  } else {
    return ret->second;
  }
}

PyObject* FindFieldByName(PyDescriptorPool* self, PyObject* name) {
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

  return NewCFieldDescriptor(field_descriptor);
}

PyObject* FindExtensionByName(PyDescriptorPool* self, PyObject* arg) {
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

  return NewCFieldDescriptor(field_descriptor);
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

PyTypeObject PyDescriptorPool_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  C("google.protobuf.internal."
    "_net_proto2___python."
    "CFieldDescriptor"),               // tp_name
  sizeof(PyDescriptorPool),            // tp_basicsize
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
  0,                                   // tp_alloc
  0,                                   // tp_new
  PyObject_Del,                        // tp_free
};


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

  // If the file was already part of a C++ library, all its descriptors are in
  // the underlying pool.  No need to do anything else.
  if (google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
          file_proto.name()) != NULL) {
    Py_RETURN_NONE;
  }

  BuildFileErrorCollector error_collector;
  const google::protobuf::FileDescriptor* descriptor =
      GetDescriptorPool()->pool->BuildFileCollectingErrors(file_proto,
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
  if (PyType_Ready(&CMessageDescriptor_Type) < 0)
    return false;
  if (PyType_Ready(&CFieldDescriptor_Type) < 0)
    return false;

  PyDescriptorPool_Type.tp_new = PyType_GenericNew;
  if (PyType_Ready(&PyDescriptorPool_Type) < 0)
    return false;

  return true;
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
