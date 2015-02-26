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

// Implements the DescriptorPool, which collects all descriptors.

#include <Python.h>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/pyext/descriptor_pool.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

#define C(str) const_cast<char*>(str)

#if PY_MAJOR_VERSION >= 3
  #define PyString_FromStringAndSize PyUnicode_FromStringAndSize
  #if PY_VERSION_HEX < 0x03030000
    #error "Python 3.0 - 3.2 are not supported."
  #endif
  #define PyString_AsStringAndSize(ob, charpp, sizep) \
    (PyUnicode_Check(ob)? \
       ((*(charpp) = PyUnicode_AsUTF8AndSize(ob, (sizep))) == NULL? -1: 0): \
       PyBytes_AsStringAndSize(ob, (charpp), (sizep)))
#endif

namespace google {
namespace protobuf {
namespace python {

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
  cdescriptor_pool->pool = new DescriptorPool(DescriptorPool::generated_pool());

  // TODO(amauryfa): Rewrite the SymbolDatabase in C so that it uses the same
  // storage.
  cdescriptor_pool->classes_by_descriptor =
      new PyDescriptorPool::ClassesByMessageMap();
  cdescriptor_pool->interned_descriptors =
      new hash_map<const void*, PyObject *>();
  cdescriptor_pool->descriptor_options =
      new hash_map<const void*, PyObject *>();

  return cdescriptor_pool;
}

static void Dealloc(PyDescriptorPool* self) {
  typedef PyDescriptorPool::ClassesByMessageMap::iterator iterator;
  for (iterator it = self->classes_by_descriptor->begin();
       it != self->classes_by_descriptor->end(); ++it) {
    Py_DECREF(it->second);
  }
  delete self->classes_by_descriptor;
  delete self->interned_descriptors;  // its references were borrowed.
  for (hash_map<const void*, PyObject*>::iterator it =
           self->descriptor_options->begin();
       it != self->descriptor_options->end(); ++it) {
    Py_DECREF(it->second);
  }
  delete self->descriptor_options;
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

PyObject* FindMessageByName(PyDescriptorPool* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const Descriptor* message_descriptor =
      self->pool->FindMessageTypeByName(string(name, name_size));

  if (message_descriptor == NULL) {
    PyErr_Format(PyExc_TypeError, "Couldn't find message %.200s", name);
    return NULL;
  }

  return PyMessageDescriptor_New(message_descriptor);
}

// Add a message class to our database.
const Descriptor* RegisterMessageClass(
    PyDescriptorPool* self, PyObject *message_class, PyObject* descriptor) {
  ScopedPyObjectPtr full_message_name(
      PyObject_GetAttrString(descriptor, "full_name"));
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(full_message_name, &name, &name_size) < 0) {
    return NULL;
  }
  const Descriptor *message_descriptor =
      self->pool->FindMessageTypeByName(string(name, name_size));
  if (!message_descriptor) {
    PyErr_Format(PyExc_TypeError, "Could not find C++ descriptor for '%s'",
                 name);
    return NULL;
  }
  Py_INCREF(message_class);
  typedef PyDescriptorPool::ClassesByMessageMap::iterator iterator;
  std::pair<iterator, bool> ret = self->classes_by_descriptor->insert(
      std::make_pair(message_descriptor, message_class));
  if (!ret.second) {
    // Update case: DECREF the previous value.
    Py_DECREF(ret.first->second);
    ret.first->second = message_class;
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

PyObject* FindFieldByName(PyDescriptorPool* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const FieldDescriptor* field_descriptor =
      self->pool->FindFieldByName(string(name, name_size));
  if (field_descriptor == NULL) {
    PyErr_Format(PyExc_TypeError, "Couldn't find field %.200s",
                 name);
    return NULL;
  }

  return PyFieldDescriptor_New(field_descriptor);
}

PyObject* FindExtensionByName(PyDescriptorPool* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const FieldDescriptor* field_descriptor =
      self->pool->FindExtensionByName(string(name, name_size));
  if (field_descriptor == NULL) {
    PyErr_Format(PyExc_TypeError, "Couldn't find field %.200s", name);
    return NULL;
  }

  return PyFieldDescriptor_New(field_descriptor);
}

PyObject* FindEnumTypeByName(PyDescriptorPool* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const EnumDescriptor* enum_descriptor =
      self->pool->FindEnumTypeByName(string(name, name_size));
  if (enum_descriptor == NULL) {
    PyErr_Format(PyExc_TypeError, "Couldn't find enum %.200s", name);
    return NULL;
  }

  return PyEnumDescriptor_New(enum_descriptor);
}

PyObject* FindOneofByName(PyDescriptorPool* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const OneofDescriptor* oneof_descriptor =
      self->pool->FindOneofByName(string(name, name_size));
  if (oneof_descriptor == NULL) {
    PyErr_Format(PyExc_TypeError, "Couldn't find oneof %.200s", name);
    return NULL;
  }

  return PyOneofDescriptor_New(oneof_descriptor);
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
    "_message.DescriptorPool"),        // tp_name
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

// The code below loads new Descriptors from a serialized FileDescriptorProto.


// Collects errors that occur during proto file building to allow them to be
// propagated in the python exception instead of only living in ERROR logs.
class BuildFileErrorCollector : public DescriptorPool::ErrorCollector {
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

PyObject* Python_BuildFile(PyObject* ignored, PyObject* serialized_pb) {
  char* message_type;
  Py_ssize_t message_len;

  if (PyBytes_AsStringAndSize(serialized_pb, &message_type, &message_len) < 0) {
    return NULL;
  }

  FileDescriptorProto file_proto;
  if (!file_proto.ParseFromArray(message_type, message_len)) {
    PyErr_SetString(PyExc_TypeError, "Couldn't parse file content!");
    return NULL;
  }

  // If the file was already part of a C++ library, all its descriptors are in
  // the underlying pool.  No need to do anything else.
  const FileDescriptor* generated_file =
      DescriptorPool::generated_pool()->FindFileByName(file_proto.name());
  if (generated_file != NULL) {
    return PyFileDescriptor_NewWithPb(generated_file, serialized_pb);
  }

  BuildFileErrorCollector error_collector;
  const FileDescriptor* descriptor =
      GetDescriptorPool()->pool->BuildFileCollectingErrors(file_proto,
                                                           &error_collector);
  if (descriptor == NULL) {
    PyErr_Format(PyExc_TypeError,
                 "Couldn't build proto file into descriptor pool!\n%s",
                 error_collector.error_message.c_str());
    return NULL;
  }

  return PyFileDescriptor_NewWithPb(descriptor, serialized_pb);
}

static PyDescriptorPool* global_cdescriptor_pool = NULL;

bool InitDescriptorPool() {
  if (PyType_Ready(&PyDescriptorPool_Type) < 0)
    return false;

  global_cdescriptor_pool = cdescriptor_pool::NewDescriptorPool();
  if (global_cdescriptor_pool == NULL) {
    return false;
  }

  return true;
}

PyDescriptorPool* GetDescriptorPool() {
  return global_cdescriptor_pool;
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
