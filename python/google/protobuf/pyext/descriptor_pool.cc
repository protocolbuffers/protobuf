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

#include <unordered_map>

#include <Python.h>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/descriptor_database.h>
#include <google/protobuf/pyext/descriptor_pool.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/message_factory.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>
#include <google/protobuf/stubs/hash.h>

#if PY_MAJOR_VERSION >= 3
  #define PyString_FromStringAndSize PyUnicode_FromStringAndSize
  #if PY_VERSION_HEX < 0x03030000
    #error "Python 3.0 - 3.2 are not supported."
  #endif
#define PyString_AsStringAndSize(ob, charpp, sizep)                           \
  (PyUnicode_Check(ob) ? ((*(charpp) = const_cast<char*>(                     \
                               PyUnicode_AsUTF8AndSize(ob, (sizep)))) == NULL \
                              ? -1                                            \
                              : 0)                                            \
                       : PyBytes_AsStringAndSize(ob, (charpp), (sizep)))
#endif

namespace google {
namespace protobuf {
namespace python {

// A map to cache Python Pools per C++ pointer.
// Pointers are not owned here, and belong to the PyDescriptorPool.
static std::unordered_map<const DescriptorPool*, PyDescriptorPool*>*
    descriptor_pool_map;

namespace cdescriptor_pool {

// Collects errors that occur during proto file building to allow them to be
// propagated in the python exception instead of only living in ERROR logs.
class BuildFileErrorCollector : public DescriptorPool::ErrorCollector {
 public:
  BuildFileErrorCollector() : error_message(""), had_errors_(false) {}

  void AddError(const std::string& filename, const std::string& element_name,
                const Message* descriptor, ErrorLocation location,
                const std::string& message) override {
    // Replicates the logging behavior that happens in the C++ implementation
    // when an error collector is not passed in.
    if (!had_errors_) {
      error_message +=
          ("Invalid proto descriptor for file \"" + filename + "\":\n");
      had_errors_ = true;
    }
    // As this only happens on failure and will result in the program not
    // running at all, no effort is made to optimize this string manipulation.
    error_message += ("  " + element_name + ": " + message + "\n");
  }

  void Clear() {
    had_errors_ = false;
    error_message = "";
  }

  std::string error_message;

 private:
  bool had_errors_;
};

// Create a Python DescriptorPool object, but does not fill the "pool"
// attribute.
static PyDescriptorPool* _CreateDescriptorPool() {
  PyDescriptorPool* cpool = PyObject_GC_New(
      PyDescriptorPool, &PyDescriptorPool_Type);
  if (cpool == NULL) {
    return NULL;
  }

  cpool->error_collector = nullptr;
  cpool->underlay = NULL;
  cpool->database = NULL;

  cpool->descriptor_options = new std::unordered_map<const void*, PyObject*>();

  cpool->py_message_factory = message_factory::NewMessageFactory(
      &PyMessageFactory_Type, cpool);
  if (cpool->py_message_factory == NULL) {
    Py_DECREF(cpool);
    return NULL;
  }

  PyObject_GC_Track(cpool);

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

  if (!descriptor_pool_map->insert(
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
    cpool->error_collector = new BuildFileErrorCollector();
    cpool->pool = new DescriptorPool(database, cpool->error_collector);
    cpool->database = database;
  } else {
    cpool->pool = new DescriptorPool();
  }

  if (!descriptor_pool_map->insert(std::make_pair(cpool->pool, cpool)).second) {
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

static void Dealloc(PyObject* pself) {
  PyDescriptorPool* self = reinterpret_cast<PyDescriptorPool*>(pself);
  descriptor_pool_map->erase(self->pool);
  Py_CLEAR(self->py_message_factory);
  for (std::unordered_map<const void*, PyObject*>::iterator it =
           self->descriptor_options->begin();
       it != self->descriptor_options->end(); ++it) {
    Py_DECREF(it->second);
  }
  delete self->descriptor_options;
  delete self->database;
  delete self->pool;
  delete self->error_collector;
  Py_TYPE(self)->tp_free(pself);
}

static int GcTraverse(PyObject* pself, visitproc visit, void* arg) {
  PyDescriptorPool* self = reinterpret_cast<PyDescriptorPool*>(pself);
  Py_VISIT(self->py_message_factory);
  return 0;
}

static int GcClear(PyObject* pself) {
  PyDescriptorPool* self = reinterpret_cast<PyDescriptorPool*>(pself);
  Py_CLEAR(self->py_message_factory);
  return 0;
}

PyObject* SetErrorFromCollector(DescriptorPool::ErrorCollector* self,
                                char* name, char* error_type) {
  BuildFileErrorCollector* error_collector =
      reinterpret_cast<BuildFileErrorCollector*>(self);
  if (error_collector && !error_collector->error_message.empty()) {
    PyErr_Format(PyExc_KeyError, "Couldn't build file for %s %.200s\n%s",
                 error_type, name, error_collector->error_message.c_str());
    error_collector->Clear();
    return NULL;
  }
  PyErr_Format(PyExc_KeyError, "Couldn't find %s %.200s", error_type, name);
  return NULL;
}

static PyObject* FindMessageByName(PyObject* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const Descriptor* message_descriptor =
      reinterpret_cast<PyDescriptorPool*>(self)->pool->FindMessageTypeByName(
          StringParam(name, name_size));

  if (message_descriptor == NULL) {
    return SetErrorFromCollector(
        reinterpret_cast<PyDescriptorPool*>(self)->error_collector, name,
        "message");
  }


  return PyMessageDescriptor_FromDescriptor(message_descriptor);
}




static PyObject* FindFileByName(PyObject* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  PyDescriptorPool* py_pool = reinterpret_cast<PyDescriptorPool*>(self);
  const FileDescriptor* file_descriptor =
      py_pool->pool->FindFileByName(StringParam(name, name_size));

  if (file_descriptor == NULL) {
    return SetErrorFromCollector(py_pool->error_collector, name, "file");
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
      self->pool->FindFieldByName(StringParam(name, name_size));
  if (field_descriptor == NULL) {
    return SetErrorFromCollector(self->error_collector, name, "field");
  }


  return PyFieldDescriptor_FromDescriptor(field_descriptor);
}

static PyObject* FindFieldByNameMethod(PyObject* self, PyObject* arg) {
  return FindFieldByName(reinterpret_cast<PyDescriptorPool*>(self), arg);
}

PyObject* FindExtensionByName(PyDescriptorPool* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const FieldDescriptor* field_descriptor =
      self->pool->FindExtensionByName(StringParam(name, name_size));
  if (field_descriptor == NULL) {
    return SetErrorFromCollector(self->error_collector, name,
                                 "extension field");
  }


  return PyFieldDescriptor_FromDescriptor(field_descriptor);
}

static PyObject* FindExtensionByNameMethod(PyObject* self, PyObject* arg) {
  return FindExtensionByName(reinterpret_cast<PyDescriptorPool*>(self), arg);
}

PyObject* FindEnumTypeByName(PyDescriptorPool* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const EnumDescriptor* enum_descriptor =
      self->pool->FindEnumTypeByName(StringParam(name, name_size));
  if (enum_descriptor == NULL) {
    return SetErrorFromCollector(self->error_collector, name, "enum");
  }


  return PyEnumDescriptor_FromDescriptor(enum_descriptor);
}

static PyObject* FindEnumTypeByNameMethod(PyObject* self, PyObject* arg) {
  return FindEnumTypeByName(reinterpret_cast<PyDescriptorPool*>(self), arg);
}

PyObject* FindOneofByName(PyDescriptorPool* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const OneofDescriptor* oneof_descriptor =
      self->pool->FindOneofByName(StringParam(name, name_size));
  if (oneof_descriptor == NULL) {
    return SetErrorFromCollector(self->error_collector, name, "oneof");
  }


  return PyOneofDescriptor_FromDescriptor(oneof_descriptor);
}

static PyObject* FindOneofByNameMethod(PyObject* self, PyObject* arg) {
  return FindOneofByName(reinterpret_cast<PyDescriptorPool*>(self), arg);
}

static PyObject* FindServiceByName(PyObject* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const ServiceDescriptor* service_descriptor =
      reinterpret_cast<PyDescriptorPool*>(self)->pool->FindServiceByName(
          StringParam(name, name_size));
  if (service_descriptor == NULL) {
    return SetErrorFromCollector(
        reinterpret_cast<PyDescriptorPool*>(self)->error_collector, name,
        "service");
  }


  return PyServiceDescriptor_FromDescriptor(service_descriptor);
}

static PyObject* FindMethodByName(PyObject* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const MethodDescriptor* method_descriptor =
      reinterpret_cast<PyDescriptorPool*>(self)->pool->FindMethodByName(
          StringParam(name, name_size));
  if (method_descriptor == NULL) {
    return SetErrorFromCollector(
        reinterpret_cast<PyDescriptorPool*>(self)->error_collector, name,
        "method");
  }


  return PyMethodDescriptor_FromDescriptor(method_descriptor);
}

static PyObject* FindFileContainingSymbol(PyObject* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  const FileDescriptor* file_descriptor =
      reinterpret_cast<PyDescriptorPool*>(self)->pool->FindFileContainingSymbol(
          StringParam(name, name_size));
  if (file_descriptor == NULL) {
    return SetErrorFromCollector(
        reinterpret_cast<PyDescriptorPool*>(self)->error_collector, name,
        "symbol");
  }


  return PyFileDescriptor_FromDescriptor(file_descriptor);
}

static PyObject* FindExtensionByNumber(PyObject* self, PyObject* args) {
  PyObject* message_descriptor;
  int number;
  if (!PyArg_ParseTuple(args, "Oi", &message_descriptor, &number)) {
    return NULL;
  }
  const Descriptor* descriptor = PyMessageDescriptor_AsDescriptor(
      message_descriptor);
  if (descriptor == NULL) {
    return NULL;
  }

  const FieldDescriptor* extension_descriptor =
      reinterpret_cast<PyDescriptorPool*>(self)->pool->FindExtensionByNumber(
          descriptor, number);
  if (extension_descriptor == NULL) {
    BuildFileErrorCollector* error_collector =
        reinterpret_cast<BuildFileErrorCollector*>(
            reinterpret_cast<PyDescriptorPool*>(self)->error_collector);
    if (error_collector && !error_collector->error_message.empty()) {
      PyErr_Format(PyExc_KeyError, "Couldn't build file for Extension %.d\n%s",
                   number, error_collector->error_message.c_str());
      error_collector->Clear();
      return NULL;
    }
    PyErr_Format(PyExc_KeyError, "Couldn't find Extension %d", number);
    return NULL;
  }


  return PyFieldDescriptor_FromDescriptor(extension_descriptor);
}

static PyObject* FindAllExtensions(PyObject* self, PyObject* arg) {
  const Descriptor* descriptor = PyMessageDescriptor_AsDescriptor(arg);
  if (descriptor == NULL) {
    return NULL;
  }

  std::vector<const FieldDescriptor*> extensions;
  reinterpret_cast<PyDescriptorPool*>(self)->pool->FindAllExtensions(
      descriptor, &extensions);

  ScopedPyObjectPtr result(PyList_New(extensions.size()));
  if (result == NULL) {
    return NULL;
  }
  for (int i = 0; i < extensions.size(); i++) {
    PyObject* extension = PyFieldDescriptor_FromDescriptor(extensions[i]);
    if (extension == NULL) {
      return NULL;
    }
    PyList_SET_ITEM(result.get(), i, extension);  // Steals the reference.
  }
  return result.release();
}

// These functions should not exist -- the only valid way to create
// descriptors is to call Add() or AddSerializedFile().
// But these AddDescriptor() functions were created in Python and some people
// call them, so we support them for now for compatibility.
// However we do check that the existing descriptor already exists in the pool,
// which appears to always be true for existing calls -- but then why do people
// call a function that will just be a no-op?
// TODO(amauryfa): Need to investigate further.

static PyObject* AddFileDescriptor(PyObject* self, PyObject* descriptor) {
  const FileDescriptor* file_descriptor =
      PyFileDescriptor_AsDescriptor(descriptor);
  if (!file_descriptor) {
    return NULL;
  }
  if (file_descriptor !=
      reinterpret_cast<PyDescriptorPool*>(self)->pool->FindFileByName(
          file_descriptor->name())) {
    PyErr_Format(PyExc_ValueError,
                 "The file descriptor %s does not belong to this pool",
                 file_descriptor->name().c_str());
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* AddDescriptor(PyObject* self, PyObject* descriptor) {
  const Descriptor* message_descriptor =
      PyMessageDescriptor_AsDescriptor(descriptor);
  if (!message_descriptor) {
    return NULL;
  }
  if (message_descriptor !=
      reinterpret_cast<PyDescriptorPool*>(self)->pool->FindMessageTypeByName(
          message_descriptor->full_name())) {
    PyErr_Format(PyExc_ValueError,
                 "The message descriptor %s does not belong to this pool",
                 message_descriptor->full_name().c_str());
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* AddEnumDescriptor(PyObject* self, PyObject* descriptor) {
  const EnumDescriptor* enum_descriptor =
      PyEnumDescriptor_AsDescriptor(descriptor);
  if (!enum_descriptor) {
    return NULL;
  }
  if (enum_descriptor !=
      reinterpret_cast<PyDescriptorPool*>(self)->pool->FindEnumTypeByName(
          enum_descriptor->full_name())) {
    PyErr_Format(PyExc_ValueError,
                 "The enum descriptor %s does not belong to this pool",
                 enum_descriptor->full_name().c_str());
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* AddExtensionDescriptor(PyObject* self, PyObject* descriptor) {
  const FieldDescriptor* extension_descriptor =
      PyFieldDescriptor_AsDescriptor(descriptor);
  if (!extension_descriptor) {
    return NULL;
  }
  if (extension_descriptor !=
      reinterpret_cast<PyDescriptorPool*>(self)->pool->FindExtensionByName(
          extension_descriptor->full_name())) {
    PyErr_Format(PyExc_ValueError,
                 "The extension descriptor %s does not belong to this pool",
                 extension_descriptor->full_name().c_str());
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* AddServiceDescriptor(PyObject* self, PyObject* descriptor) {
  const ServiceDescriptor* service_descriptor =
      PyServiceDescriptor_AsDescriptor(descriptor);
  if (!service_descriptor) {
    return NULL;
  }
  if (service_descriptor !=
      reinterpret_cast<PyDescriptorPool*>(self)->pool->FindServiceByName(
          service_descriptor->full_name())) {
    PyErr_Format(PyExc_ValueError,
                 "The service descriptor %s does not belong to this pool",
                 service_descriptor->full_name().c_str());
    return NULL;
  }
  Py_RETURN_NONE;
}

// The code below loads new Descriptors from a serialized FileDescriptorProto.
static PyObject* AddSerializedFile(PyObject* pself, PyObject* serialized_pb) {
  PyDescriptorPool* self = reinterpret_cast<PyDescriptorPool*>(pself);
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

static PyObject* Add(PyObject* self, PyObject* file_descriptor_proto) {
  ScopedPyObjectPtr serialized_pb(
      PyObject_CallMethod(file_descriptor_proto, "SerializeToString", NULL));
  if (serialized_pb == NULL) {
    return NULL;
  }
  return AddSerializedFile(self, serialized_pb.get());
}

static PyMethodDef Methods[] = {
  { "Add", Add, METH_O,
    "Adds the FileDescriptorProto and its types to this pool." },
  { "AddSerializedFile", AddSerializedFile, METH_O,
    "Adds a serialized FileDescriptorProto to this pool." },

  // TODO(amauryfa): Understand why the Python implementation differs from
  // this one, ask users to use another API and deprecate these functions.
  { "AddFileDescriptor", AddFileDescriptor, METH_O,
    "No-op. Add() must have been called before." },
  { "AddDescriptor", AddDescriptor, METH_O,
    "No-op. Add() must have been called before." },
  { "AddEnumDescriptor", AddEnumDescriptor, METH_O,
    "No-op. Add() must have been called before." },
  { "AddExtensionDescriptor", AddExtensionDescriptor, METH_O,
    "No-op. Add() must have been called before." },
  { "AddServiceDescriptor", AddServiceDescriptor, METH_O,
    "No-op. Add() must have been called before." },

  { "FindFileByName", FindFileByName, METH_O,
    "Searches for a file descriptor by its .proto name." },
  { "FindMessageTypeByName", FindMessageByName, METH_O,
    "Searches for a message descriptor by full name." },
  { "FindFieldByName", FindFieldByNameMethod, METH_O,
    "Searches for a field descriptor by full name." },
  { "FindExtensionByName", FindExtensionByNameMethod, METH_O,
    "Searches for extension descriptor by full name." },
  { "FindEnumTypeByName", FindEnumTypeByNameMethod, METH_O,
    "Searches for enum type descriptor by full name." },
  { "FindOneofByName", FindOneofByNameMethod, METH_O,
    "Searches for oneof descriptor by full name." },
  { "FindServiceByName", FindServiceByName, METH_O,
    "Searches for service descriptor by full name." },
  { "FindMethodByName", FindMethodByName, METH_O,
    "Searches for method descriptor by full name." },

  { "FindFileContainingSymbol", FindFileContainingSymbol, METH_O,
    "Gets the FileDescriptor containing the specified symbol." },
  { "FindExtensionByNumber", FindExtensionByNumber, METH_VARARGS,
    "Gets the extension descriptor for the given number." },
  { "FindAllExtensions", FindAllExtensions, METH_O,
    "Gets all known extensions of the given message descriptor." },
  {NULL}
};

}  // namespace cdescriptor_pool

PyTypeObject PyDescriptorPool_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) FULL_MODULE_NAME
    ".DescriptorPool",                        // tp_name
    sizeof(PyDescriptorPool),                 // tp_basicsize
    0,                                        // tp_itemsize
    cdescriptor_pool::Dealloc,                // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    0,                                        // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    0,                                        // tp_hash
    0,                                        // tp_call
    0,                                        // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,  // tp_flags
    "A Descriptor Pool",                      // tp_doc
    cdescriptor_pool::GcTraverse,             // tp_traverse
    cdescriptor_pool::GcClear,                // tp_clear
    0,                                        // tp_richcompare
    0,                                        // tp_weaklistoffset
    0,                                        // tp_iter
    0,                                        // tp_iternext
    cdescriptor_pool::Methods,                // tp_methods
    0,                                        // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    cdescriptor_pool::New,                    // tp_new
    PyObject_GC_Del,                          // tp_free
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
  descriptor_pool_map =
      new std::unordered_map<const DescriptorPool*, PyDescriptorPool*>;
  python_generated_pool = cdescriptor_pool::PyDescriptorPool_NewWithUnderlay(
      DescriptorPool::generated_pool());
  if (python_generated_pool == NULL) {
    delete descriptor_pool_map;
    return false;
  }

  // Register this pool to be found for C++-generated descriptors.
  descriptor_pool_map->insert(
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
  std::unordered_map<const DescriptorPool*, PyDescriptorPool*>::iterator it =
      descriptor_pool_map->find(pool);
  if (it == descriptor_pool_map->end()) {
    PyErr_SetString(PyExc_KeyError, "Unknown descriptor pool");
    return NULL;
  }
  return it->second;
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
