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
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/descriptor_database.h>
#include <google/protobuf/pyext/descriptor_pool.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

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

// A map to cache Python Pools per C++ pointer.
// Pointers are not owned here, and belong to the PyDescriptorPool.
static hash_map<const DescriptorPool*, PyDescriptorPool*> descriptor_pool_map;

namespace cdescriptor_pool {

// Create a Python DescriptorPool object, but does not fill the "pool"
// attribute.
static PyDescriptorPool* _CreateDescriptorPool() {
  PyDescriptorPool* cpool = PyObject_New(
      PyDescriptorPool, &PyDescriptorPool_Type);
  if (cpool == NULL) {
    return NULL;
  }

  cpool->underlay = NULL;
  cpool->database = NULL;

  DynamicMessageFactory* message_factory = new DynamicMessageFactory();
  // This option might be the default some day.
  message_factory->SetDelegateToGeneratedFactory(true);
  cpool->message_factory = message_factory;

  // TODO(amauryfa): Rewrite the SymbolDatabase in C so that it uses the same
  // storage.
  cpool->classes_by_descriptor =
      new PyDescriptorPool::ClassesByMessageMap();
  cpool->descriptor_options =
      new hash_map<const void*, PyObject *>();

  return cpool;
}

// Create a Python DescriptorPool, using the given pool as an underlay:
// new messages will be added to a custom pool, not to the underlay.
//
// Ownership of the underlay is not transferred, its pointer should
// stay alive.
static PyDescriptorPool* PyDescriptorPool_NewWithUnderlay(
    const DescriptorPool* underlay) {
  PyDescriptorPool* cpool = _CreateDescriptorPool();
  if (cpool == NULL) {
    return NULL;
  }
  cpool->pool = new DescriptorPool(underlay);
  cpool->underlay = underlay;

  if (!descriptor_pool_map.insert(
      std::make_pair(cpool->pool, cpool)).second) {
    // Should never happen -- would indicate an internal error / bug.
    PyErr_SetString(PyExc_ValueError, "DescriptorPool already registered");
    return NULL;
  }

  return cpool;
}

static PyDescriptorPool* PyDescriptorPool_NewWithDatabase(
    DescriptorDatabase* database) {
  PyDescriptorPool* cpool = _CreateDescriptorPool();
  if (cpool == NULL) {
    return NULL;
  }
  if (database != NULL) {
    cpool->pool = new DescriptorPool(database);
    cpool->database = database;
  } else {
    cpool->pool = new DescriptorPool();
  }

  if (!descriptor_pool_map.insert(std::make_pair(cpool->pool, cpool)).second) {
    // Should never happen -- would indicate an internal error / bug.
    PyErr_SetString(PyExc_ValueError, "DescriptorPool already registered");
    return NULL;
  }

  return cpool;
}

// The public DescriptorPool constructor.
static PyObject* New(PyTypeObject* type,
                     PyObject* args, PyObject* kwargs) {
  static char* kwlist[] = {"descriptor_db", 0};
  PyObject* py_database = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &py_database)) {
    return NULL;
  }
  DescriptorDatabase* database = NULL;
  if (py_database && py_database != Py_None) {
    database = new PyDescriptorDatabase(py_database);
  }
  return reinterpret_cast<PyObject*>(
      PyDescriptorPool_NewWithDatabase(database));
}

static void Dealloc(PyDescriptorPool* self) {
  typedef PyDescriptorPool::ClassesByMessageMap::iterator iterator;
  descriptor_pool_map.erase(self->pool);
  for (iterator it = self->classes_by_descriptor->begin();
       it != self->classes_by_descriptor->end(); ++it) {
    Py_DECREF(it->second);
  }
  delete self->classes_by_descriptor;
  for (hash_map<const void*, PyObject*>::iterator it =
           self->descriptor_options->begin();
       it != self->descriptor_options->end(); ++it) {
    Py_DECREF(it->second);
  }
  delete self->descriptor_options;
  delete self->message_factory;
  delete self->database;
  delete self->pool;
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
    PyErr_Format(PyExc_KeyError, "Couldn't find message %.200s", name);
    return NULL;
  }

  return PyMessageDescriptor_FromDescriptor(message_descriptor);
}

// Add a message class to our database.
int RegisterMessageClass(PyDescriptorPool* self,
                         const Descriptor *message_descriptor,
                         PyObject *message_class) {
  Py_INCREF(message_class);
  typedef PyDescriptorPool::ClassesByMessageMap::iterator iterator;
  std::pair<iterator, bool> ret = self->classes_by_descriptor->insert(
      std::make_pair(message_descriptor, message_class));
  if (!ret.second) {
    // Update case: DECREF the previous value.
    Py_DECREF(ret.first->second);
    ret.first->second = message_class;
  }
  return 0;
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

PyObject* FindFileByName(PyDescriptorPool* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const FileDescriptor* file_descriptor =
      self->pool->FindFileByName(string(name, name_size));
  if (file_descriptor == NULL) {
    PyErr_Format(PyExc_KeyError, "Couldn't find file %.200s",
                 name);
    return NULL;
  }

  return PyFileDescriptor_FromDescriptor(file_descriptor);
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
    PyErr_Format(PyExc_KeyError, "Couldn't find field %.200s",
                 name);
    return NULL;
  }

  return PyFieldDescriptor_FromDescriptor(field_descriptor);
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
    PyErr_Format(PyExc_KeyError, "Couldn't find extension field %.200s", name);
    return NULL;
  }

  return PyFieldDescriptor_FromDescriptor(field_descriptor);
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
    PyErr_Format(PyExc_KeyError, "Couldn't find enum %.200s", name);
    return NULL;
  }

  return PyEnumDescriptor_FromDescriptor(enum_descriptor);
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
    PyErr_Format(PyExc_KeyError, "Couldn't find oneof %.200s", name);
    return NULL;
  }

  return PyOneofDescriptor_FromDescriptor(oneof_descriptor);
}

PyObject* FindFileContainingSymbol(PyDescriptorPool* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const FileDescriptor* file_descriptor =
      self->pool->FindFileContainingSymbol(string(name, name_size));
  if (file_descriptor == NULL) {
    PyErr_Format(PyExc_KeyError, "Couldn't find symbol %.200s", name);
    return NULL;
  }

  return PyFileDescriptor_FromDescriptor(file_descriptor);
}

// These functions should not exist -- the only valid way to create
// descriptors is to call Add() or AddSerializedFile().
// But these AddDescriptor() functions were created in Python and some people
// call them, so we support them for now for compatibility.
// However we do check that the existing descriptor already exists in the pool,
// which appears to always be true for existing calls -- but then why do people
// call a function that will just be a no-op?
// TODO(amauryfa): Need to investigate further.

PyObject* AddFileDescriptor(PyDescriptorPool* self, PyObject* descriptor) {
  const FileDescriptor* file_descriptor =
      PyFileDescriptor_AsDescriptor(descriptor);
  if (!file_descriptor) {
    return NULL;
  }
  if (file_descriptor !=
      self->pool->FindFileByName(file_descriptor->name())) {
    PyErr_Format(PyExc_ValueError,
                 "The file descriptor %s does not belong to this pool",
                 file_descriptor->name().c_str());
    return NULL;
  }
  Py_RETURN_NONE;
}

PyObject* AddDescriptor(PyDescriptorPool* self, PyObject* descriptor) {
  const Descriptor* message_descriptor =
      PyMessageDescriptor_AsDescriptor(descriptor);
  if (!message_descriptor) {
    return NULL;
  }
  if (message_descriptor !=
      self->pool->FindMessageTypeByName(message_descriptor->full_name())) {
    PyErr_Format(PyExc_ValueError,
                 "The message descriptor %s does not belong to this pool",
                 message_descriptor->full_name().c_str());
    return NULL;
  }
  Py_RETURN_NONE;
}

PyObject* AddEnumDescriptor(PyDescriptorPool* self, PyObject* descriptor) {
  const EnumDescriptor* enum_descriptor =
      PyEnumDescriptor_AsDescriptor(descriptor);
  if (!enum_descriptor) {
    return NULL;
  }
  if (enum_descriptor !=
      self->pool->FindEnumTypeByName(enum_descriptor->full_name())) {
    PyErr_Format(PyExc_ValueError,
                 "The enum descriptor %s does not belong to this pool",
                 enum_descriptor->full_name().c_str());
    return NULL;
  }
  Py_RETURN_NONE;
}

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
      had_errors = true;
    }
    // As this only happens on failure and will result in the program not
    // running at all, no effort is made to optimize this string manipulation.
    error_message += ("  " + element_name + ": " + message + "\n");
  }

  string error_message;
  bool had_errors;
};

PyObject* AddSerializedFile(PyDescriptorPool* self, PyObject* serialized_pb) {
  char* message_type;
  Py_ssize_t message_len;

  if (self->database != NULL) {
    PyErr_SetString(
        PyExc_ValueError,
        "Cannot call Add on a DescriptorPool that uses a DescriptorDatabase. "
        "Add your file to the underlying database.");
    return NULL;
  }

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
  const FileDescriptor* generated_file = NULL;
  if (self->underlay) {
    generated_file = self->underlay->FindFileByName(file_proto.name());
  }
  if (generated_file != NULL) {
    return PyFileDescriptor_FromDescriptorWithSerializedPb(
        generated_file, serialized_pb);
  }

  BuildFileErrorCollector error_collector;
  const FileDescriptor* descriptor =
      self->pool->BuildFileCollectingErrors(file_proto,
                                            &error_collector);
  if (descriptor == NULL) {
    PyErr_Format(PyExc_TypeError,
                 "Couldn't build proto file into descriptor pool!\n%s",
                 error_collector.error_message.c_str());
    return NULL;
  }

  return PyFileDescriptor_FromDescriptorWithSerializedPb(
      descriptor, serialized_pb);
}

PyObject* Add(PyDescriptorPool* self, PyObject* file_descriptor_proto) {
  ScopedPyObjectPtr serialized_pb(
      PyObject_CallMethod(file_descriptor_proto, "SerializeToString", NULL));
  if (serialized_pb == NULL) {
    return NULL;
  }
  return AddSerializedFile(self, serialized_pb.get());
}

static PyMethodDef Methods[] = {
  { "Add", (PyCFunction)Add, METH_O,
    "Adds the FileDescriptorProto and its types to this pool." },
  { "AddSerializedFile", (PyCFunction)AddSerializedFile, METH_O,
    "Adds a serialized FileDescriptorProto to this pool." },

  // TODO(amauryfa): Understand why the Python implementation differs from
  // this one, ask users to use another API and deprecate these functions.
  { "AddFileDescriptor", (PyCFunction)AddFileDescriptor, METH_O,
    "No-op. Add() must have been called before." },
  { "AddDescriptor", (PyCFunction)AddDescriptor, METH_O,
    "No-op. Add() must have been called before." },
  { "AddEnumDescriptor", (PyCFunction)AddEnumDescriptor, METH_O,
    "No-op. Add() must have been called before." },

  { "FindFileByName", (PyCFunction)FindFileByName, METH_O,
    "Searches for a file descriptor by its .proto name." },
  { "FindMessageTypeByName", (PyCFunction)FindMessageByName, METH_O,
    "Searches for a message descriptor by full name." },
  { "FindFieldByName", (PyCFunction)FindFieldByName, METH_O,
    "Searches for a field descriptor by full name." },
  { "FindExtensionByName", (PyCFunction)FindExtensionByName, METH_O,
    "Searches for extension descriptor by full name." },
  { "FindEnumTypeByName", (PyCFunction)FindEnumTypeByName, METH_O,
    "Searches for enum type descriptor by full name." },
  { "FindOneofByName", (PyCFunction)FindOneofByName, METH_O,
    "Searches for oneof descriptor by full name." },

  { "FindFileContainingSymbol", (PyCFunction)FindFileContainingSymbol, METH_O,
    "Gets the FileDescriptor containing the specified symbol." },
  {NULL}
};

}  // namespace cdescriptor_pool

PyTypeObject PyDescriptorPool_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  FULL_MODULE_NAME ".DescriptorPool",  // tp_name
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
  "A Descriptor Pool",                 // tp_doc
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
  cdescriptor_pool::New,               // tp_new
  PyObject_Del,                        // tp_free
};

// This is the DescriptorPool which contains all the definitions from the
// generated _pb2.py modules.
static PyDescriptorPool* python_generated_pool = NULL;

bool InitDescriptorPool() {
  if (PyType_Ready(&PyDescriptorPool_Type) < 0)
    return false;

  // The Pool of messages declared in Python libraries.
  // generated_pool() contains all messages already linked in C++ libraries, and
  // is used as underlay.
  python_generated_pool = cdescriptor_pool::PyDescriptorPool_NewWithUnderlay(
      DescriptorPool::generated_pool());
  if (python_generated_pool == NULL) {
    return false;
  }
  // Register this pool to be found for C++-generated descriptors.
  descriptor_pool_map.insert(
      std::make_pair(DescriptorPool::generated_pool(),
                     python_generated_pool));

  return true;
}

// The default DescriptorPool used everywhere in this module.
// Today it's the python_generated_pool.
// TODO(amauryfa): Remove all usages of this function: the pool should be
// derived from the context.
PyDescriptorPool* GetDefaultDescriptorPool() {
  return python_generated_pool;
}

PyDescriptorPool* GetDescriptorPool_FromPool(const DescriptorPool* pool) {
  // Fast path for standard descriptors.
  if (pool == python_generated_pool->pool ||
      pool == DescriptorPool::generated_pool()) {
    return python_generated_pool;
  }
  hash_map<const DescriptorPool*, PyDescriptorPool*>::iterator it =
      descriptor_pool_map.find(pool);
  if (it == descriptor_pool_map.end()) {
    PyErr_SetString(PyExc_KeyError, "Unknown descriptor pool");
    return NULL;
  }
  return it->second;
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
