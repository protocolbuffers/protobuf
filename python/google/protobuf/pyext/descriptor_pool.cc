// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Implements the DescriptorPool, which collects all descriptors.

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/const_init.h"
#include "absl/base/thread_annotations.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "google/protobuf/descriptor.pb.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_database.h"
#include "google/protobuf/pyext/descriptor.h"
#include "google/protobuf/pyext/descriptor_database.h"
#include "google/protobuf/pyext/descriptor_pool.h"
#include "google/protobuf/pyext/free_threading_mutex.h"
#include "google/protobuf/pyext/message.h"
#include "google/protobuf/pyext/message_factory.h"
#include "google/protobuf/pyext/scoped_pyobject_ptr.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#define PyString_AsStringAndSize(ob, charpp, sizep)              \
  (PyUnicode_Check(ob)                                           \
       ? ((*(charpp) = const_cast<char*>(                        \
               PyUnicode_AsUTF8AndSize(ob, (sizep)))) == nullptr \
              ? -1                                               \
              : 0)                                               \
       : PyBytes_AsStringAndSize(ob, (charpp), (sizep)))

namespace google {
namespace protobuf {
namespace python {

// A map to cache Python Pools per C++ pointer.
// Pointers are not owned here, and belong to the PyDescriptorPool.
static absl::flat_hash_map<const DescriptorPool*, PyDescriptorPool*>*
    descriptor_pool_map;

static FreeThreadingMutex descriptor_pool_map_mutex(absl::kConstInit);

// Create a shared_ptr to the pool, with a custom deleter that does nothing.
//
// This shared pointer does not truly share ownership with the raw pointer:
// when it's deallocated it will NOT delete the actual pool because of the empty
// deleter. Furthermore, we cannot guarantee that the underlying raw pointer
// will outlive surviving copies of this shared_ptr. We only do this to strictly
// fit the PyDescriptorPool structure without behavior changes.
static std::shared_ptr<const DescriptorPool> UnsafeSharedPointerFromRaw(
    const DescriptorPool* ptr) {
  return std::shared_ptr<const DescriptorPool>(ptr,
                                               [](const DescriptorPool*) {});
}

namespace cdescriptor_pool {

// Collects errors that occur during proto file building to allow them to be
// propagated in the python exception instead of only living in ERROR logs.
class BuildFileErrorCollector : public DescriptorPool::ErrorCollector {
 public:
  class ErrorState {
   public:
    explicit ErrorState(BuildFileErrorCollector* collector)
        : collector_(collector) {
      if (collector_ != nullptr) {
        FreeThreadingLockGuard lock(collector_->mutex_);
        collector_->thread_errors_[GetThreadId()] = this;
      }
    }
    ErrorState(const ErrorState&) = delete;
    ErrorState& operator=(const ErrorState&) = delete;
    ~ErrorState() {
      if (collector_ != nullptr) {
        FreeThreadingLockGuard lock(collector_->mutex_);
        collector_->thread_errors_.erase(GetThreadId());
      }
    }

    const std::optional<std::string>& error_message() const {
      return error_message_;
    }

   private:
    friend BuildFileErrorCollector;

    BuildFileErrorCollector* collector_;
    std::optional<std::string> error_message_;
  };

  BuildFileErrorCollector() = default;

  void RecordError(absl::string_view filename, absl::string_view element_name,
                   const Message* descriptor, ErrorLocation location,
                   absl::string_view message) override {
    FreeThreadingLockGuard lock(mutex_);
    auto it = thread_errors_.find(GetThreadId());
    if (it == thread_errors_.end()) return;
    ErrorState& state = *it->second;
    // Replicates the logging behavior that happens in the C++ implementation
    // when an error collector is not passed in.
    if (!state.error_message_.has_value()) {
      state.error_message_ = absl::StrCat(
          "Invalid proto descriptor for file \"", filename, "\":\n");
    }
    // As this only happens on failure and will result in the program not
    // running at all, no effort is made to optimize this string manipulation.
    absl::StrAppend(&*state.error_message_, "  ", element_name, ": ", message,
                    "\n");
  }

 private:
  static const void* GetThreadId() {
    static thread_local bool dummy_id = false;
    return &dummy_id;
  }

  FreeThreadingMutex mutex_;
  absl::flat_hash_map<const void*, ErrorState*> thread_errors_
      ABSL_GUARDED_BY(mutex_);
};

using ErrorState = BuildFileErrorCollector::ErrorState;

// Create a Python DescriptorPool object, but does not fill the "pool"
// attribute.
static PyDescriptorPool* _CreateDescriptorPool() {
  PyDescriptorPool* cpool =
      PyObject_GC_New(PyDescriptorPool, &PyDescriptorPool_Type);
  if (cpool == nullptr) {
    return nullptr;
  }

  cpool->error_collector = nullptr;
  cpool->underlay = nullptr;
  cpool->is_mutable = false;
  cpool->pool = nullptr;
  cpool->database = nullptr;

  cpool->descriptor_options = new absl::flat_hash_map<const void*, PyObject*>();
  cpool->descriptor_features =
      new absl::flat_hash_map<const void*, PyObject*>();
  cpool->cache_mutex = new FreeThreadingMutex();

  cpool->py_message_factory =
      message_factory::NewMessageFactory(&PyMessageFactory_Type, cpool);
  if (cpool->py_message_factory == nullptr) {
    Py_DECREF(cpool);
    return nullptr;
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
  if (cpool == nullptr) {
    return nullptr;
  }
  cpool->error_collector = new BuildFileErrorCollector();
  cpool->backing_database = new SimpleThreadSafeDescriptorDatabase();
  auto pool = std::make_shared<DescriptorPool>(cpool->backing_database,
                                               cpool->error_collector);
  pool->internal_set_underlay(underlay);
  cpool->pool = new std::shared_ptr<const DescriptorPool>(std::move(pool));
  cpool->is_mutable = true;
  cpool->underlay = underlay;

  {
    FreeThreadingLockGuard lock(descriptor_pool_map_mutex);
    if (!descriptor_pool_map->insert(std::make_pair(cpool->pool->get(), cpool))
             .second) {
      // Should never happen -- would indicate an internal error / bug.
      PyErr_SetString(PyExc_ValueError, "DescriptorPool already registered");
      return nullptr;
    }
  }

  return cpool;
}

static PyDescriptorPool* PyDescriptorPool_NewWithDatabase(
    DescriptorDatabase* database) {
  PyDescriptorPool* cpool = _CreateDescriptorPool();
  if (cpool == nullptr) {
    return nullptr;
  }
  cpool->error_collector = new BuildFileErrorCollector();
  std::shared_ptr<DescriptorPool> pool_sp;
  if (database != nullptr) {
    pool_sp =
        std::make_shared<DescriptorPool>(database, cpool->error_collector);
    cpool->is_mutable = false;
    cpool->database = new std::shared_ptr<const DescriptorDatabase>(database);
  } else {
    cpool->backing_database = new SimpleThreadSafeDescriptorDatabase();
    pool_sp = std::make_shared<DescriptorPool>(cpool->backing_database,
                                               cpool->error_collector);
    cpool->is_mutable = true;
  }
  cpool->pool = new std::shared_ptr<const DescriptorPool>(pool_sp);

  {
    FreeThreadingLockGuard lock(descriptor_pool_map_mutex);
    if (!descriptor_pool_map->insert(std::make_pair(cpool->pool->get(), cpool))
             .second) {
      // Should never happen -- would indicate an internal error / bug.
      PyErr_SetString(PyExc_ValueError, "DescriptorPool already registered");
      return nullptr;
    }
  }

  return cpool;
}

// The public DescriptorPool constructor.
static PyObject* New(PyTypeObject* type, PyObject* args, PyObject* kwargs) {
  static const char* kwlist[] = {"descriptor_db", nullptr};
  PyObject* py_database = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O",
                                   const_cast<char**>(kwlist), &py_database)) {
    return nullptr;
  }
  DescriptorDatabase* database = nullptr;
  if (py_database && py_database != Py_None) {
    database = new PyDescriptorDatabase(py_database);
  }
  return reinterpret_cast<PyObject*>(PyDescriptorPool_NewWithDatabase(
      database));
}

static void Dealloc(PyObject* pself) {
  PyDescriptorPool* self = reinterpret_cast<PyDescriptorPool*>(pself);
  {
    FreeThreadingLockGuard lock(descriptor_pool_map_mutex);
    descriptor_pool_map->erase(self->pool->get());
  }
  Py_CLEAR(self->py_message_factory);
  for (auto it = self->descriptor_options->begin();
       it != self->descriptor_options->end(); ++it) {
    Py_DECREF(it->second);
  }
  delete self->descriptor_options;
  for (auto it = self->descriptor_features->begin();
       it != self->descriptor_features->end(); ++it) {
    Py_DECREF(it->second);
  }
  delete self->descriptor_features;
  delete self->cache_mutex;
  if (self->pool != nullptr) {
    delete self->pool;
  }
  if (self->is_mutable) {
    delete self->backing_database;
  } else {
    delete self->database;
  }
  delete self->error_collector;
  PyObject_GC_UnTrack(pself);
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

PyObject* SetErrorFromCollector(const ErrorState& error_state, PyObject* arg,
                                const char* error_type) {
  if (error_state.error_message().has_value()) {
    PyErr_Format(PyExc_KeyError, "Couldn't build file for %s %S\n%s",
                 error_type, arg, error_state.error_message()->c_str());
    return nullptr;
  }
  PyErr_Format(PyExc_KeyError, "Couldn't find %s %S", error_type, arg);
  return nullptr;
}

static PyObject* FindMessageByName(PyObject* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return nullptr;
  }

  PyDescriptorPool* py_pool = reinterpret_cast<PyDescriptorPool*>(self);
  ErrorState error_state(py_pool->error_collector);
  const Descriptor* message_descriptor =
      py_pool->pool->get()->FindMessageTypeByName(
          absl::string_view(name, name_size));

  if (message_descriptor == nullptr) {
    return SetErrorFromCollector(error_state, arg, "message");
  }

  return PyMessageDescriptor_FromDescriptor(message_descriptor);
}

static PyObject* FindFileByName(PyObject* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return nullptr;
  }

  PyDescriptorPool* py_pool = reinterpret_cast<PyDescriptorPool*>(self);
  ErrorState error_state(py_pool->error_collector);
  const FileDescriptor* file_descriptor =
      py_pool->pool->get()->FindFileByName(absl::string_view(name, name_size));

  if (file_descriptor == nullptr) {
    return SetErrorFromCollector(error_state, arg, "file");
  }
  return PyFileDescriptor_FromDescriptor(file_descriptor);
}

PyObject* FindFieldByName(PyDescriptorPool* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return nullptr;
  }

  ErrorState error_state(self->error_collector);
  const FieldDescriptor* field_descriptor =
      self->pool->get()->FindFieldByName(absl::string_view(name, name_size));
  if (field_descriptor == nullptr) {
    return SetErrorFromCollector(error_state, arg, "field");
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
    return nullptr;
  }

  ErrorState error_state(self->error_collector);
  const FieldDescriptor* field_descriptor =
      self->pool->get()->FindExtensionByName(
          absl::string_view(name, name_size));
  if (field_descriptor == nullptr) {
    return SetErrorFromCollector(error_state, arg, "extension field");
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
    return nullptr;
  }

  ErrorState error_state(self->error_collector);
  const EnumDescriptor* enum_descriptor =
      self->pool->get()->FindEnumTypeByName(absl::string_view(name, name_size));
  if (enum_descriptor == nullptr) {
    return SetErrorFromCollector(error_state, arg, "enum");
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
    return nullptr;
  }

  ErrorState error_state(self->error_collector);
  const OneofDescriptor* oneof_descriptor =
      self->pool->get()->FindOneofByName(absl::string_view(name, name_size));
  if (oneof_descriptor == nullptr) {
    return SetErrorFromCollector(error_state, arg, "oneof");
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
    return nullptr;
  }

  PyDescriptorPool* py_pool = reinterpret_cast<PyDescriptorPool*>(self);
  ErrorState error_state(py_pool->error_collector);
  const ServiceDescriptor* service_descriptor =
      py_pool->pool->get()->FindServiceByName(
          absl::string_view(name, name_size));
  if (service_descriptor == nullptr) {
    return SetErrorFromCollector(error_state, arg, "service");
  }

  return PyServiceDescriptor_FromDescriptor(service_descriptor);
}

static PyObject* FindMethodByName(PyObject* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return nullptr;
  }

  PyDescriptorPool* py_pool = reinterpret_cast<PyDescriptorPool*>(self);
  ErrorState error_state(py_pool->error_collector);
  const MethodDescriptor* method_descriptor =
      py_pool->pool->get()->FindMethodByName(
          absl::string_view(name, name_size));
  if (method_descriptor == nullptr) {
    return SetErrorFromCollector(error_state, arg, "method");
  }

  return PyMethodDescriptor_FromDescriptor(method_descriptor);
}

static PyObject* FindFileContainingSymbol(PyObject* self, PyObject* arg) {
  Py_ssize_t name_size;
  char* name;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return nullptr;
  }

  PyDescriptorPool* py_pool = reinterpret_cast<PyDescriptorPool*>(self);
  ErrorState error_state(py_pool->error_collector);
  const FileDescriptor* file_descriptor =
      py_pool->pool->get()->FindFileContainingSymbol(
          absl::string_view(name, name_size));
  if (file_descriptor == nullptr) {
    return SetErrorFromCollector(error_state, arg, "symbol");
  }

  return PyFileDescriptor_FromDescriptor(file_descriptor);
}

static PyObject* FindExtensionByNumber(PyObject* self, PyObject* args) {
  PyObject* message_descriptor;
  int number;
  if (!PyArg_ParseTuple(args, "Oi", &message_descriptor, &number)) {
    return nullptr;
  }
  const Descriptor* descriptor =
      PyMessageDescriptor_AsDescriptor(message_descriptor);
  if (descriptor == nullptr) {
    return nullptr;
  }

  PyDescriptorPool* py_pool = reinterpret_cast<PyDescriptorPool*>(self);
  ErrorState error_state(py_pool->error_collector);
  const FieldDescriptor* extension_descriptor =
      py_pool->pool->get()->FindExtensionByNumber(descriptor, number);
  if (extension_descriptor == nullptr) {
    if (error_state.error_message().has_value()) {
      PyErr_Format(PyExc_KeyError, "Couldn't build file for Extension %.d\n%s",
                   number, error_state.error_message()->c_str());
      return nullptr;
    }
    PyErr_Format(PyExc_KeyError, "Couldn't find Extension %d", number);
    return nullptr;
  }

  return PyFieldDescriptor_FromDescriptor(extension_descriptor);
}

static PyObject* FindAllExtensions(PyObject* self, PyObject* arg) {
  const Descriptor* descriptor = PyMessageDescriptor_AsDescriptor(arg);
  if (descriptor == nullptr) {
    return nullptr;
  }

  std::vector<const FieldDescriptor*> extensions;
  reinterpret_cast<PyDescriptorPool*>(self)->pool->get()->FindAllExtensions(
      descriptor, &extensions);

  ScopedPyObjectPtr result(PyList_New(extensions.size()));
  if (result == nullptr) {
    return nullptr;
  }
  for (int i = 0; i < extensions.size(); i++) {
    PyObject* extension = PyFieldDescriptor_FromDescriptor(extensions[i]);
    if (extension == nullptr) {
      return nullptr;
    }
    PyList_SET_ITEM(result.get(), i, extension);  // Steals the reference.
  }
  return result.release();
}

// The code below loads new Descriptors from a serialized FileDescriptorProto.
static PyObject* AddSerializedFile(PyObject* pself, PyObject* serialized_pb) {
  PyDescriptorPool* self = reinterpret_cast<PyDescriptorPool*>(pself);
  char* message_type;
  Py_ssize_t message_len;

  if (!self->is_mutable) {
    if (self->database != nullptr) {
      PyErr_SetString(
          PyExc_ValueError,
          "Cannot call Add on a DescriptorPool that uses a DescriptorDatabase. "
          "Add your file to the underlying database.");
    } else {
      PyErr_SetString(
          PyExc_ValueError,
          "This DescriptorPool is not mutable and cannot add new definitions.");
    }
    return nullptr;
  }

  if (PyBytes_AsStringAndSize(serialized_pb, &message_type, &message_len) < 0) {
    return nullptr;
  }

  FileDescriptorProto file_proto;
  if (!file_proto.ParseFromString(
          absl::string_view(message_type, message_len))) {
    PyErr_SetString(PyExc_TypeError, "Couldn't parse file content!");
    return nullptr;
  }

  // If the file was already part of a C++ library, all its descriptors are in
  // the underlying pool.  No need to do anything else.
  const FileDescriptor* generated_file = nullptr;
  if (self->underlay) {
    generated_file = self->underlay->FindFileByName(file_proto.name());
  }
  if (generated_file != nullptr) {
    return PyFileDescriptor_FromDescriptorWithSerializedPb(generated_file,
                                                           serialized_pb);
  }

  self->backing_database->Add(file_proto);
  ErrorState error_state(self->error_collector);
  const FileDescriptor* descriptor =
      self->pool->get()->FindFileByName(file_proto.name());
  if (descriptor == nullptr) {
    PyErr_Format(PyExc_TypeError,
                 "Couldn't build proto file into descriptor pool!\n%s",
                 error_state.error_message().has_value()
                     ? error_state.error_message()->c_str()
                     : "");
    return nullptr;
  }


  return PyFileDescriptor_FromDescriptorWithSerializedPb(descriptor,
                                                         serialized_pb);
}

static PyObject* Add(PyObject* self, PyObject* file_descriptor_proto) {
  ScopedPyObjectPtr serialized_pb(
      PyObject_CallMethod(file_descriptor_proto, "SerializeToString", nullptr));
  if (serialized_pb == nullptr) {
    return nullptr;
  }
  return AddSerializedFile(self, serialized_pb.get());
}

static PyObject* SetFeatureSetDefaults(PyObject* pself, PyObject* pdefaults) {
  PyDescriptorPool* self = reinterpret_cast<PyDescriptorPool*>(pself);

  if (!self->is_mutable) {
    PyErr_SetString(
        PyExc_RuntimeError,
        "This DescriptorPool is not mutable and cannot add new definitions.");
    return nullptr;
  }

  if (!PyObject_TypeCheck(pdefaults, CMessage_Type)) {
    PyErr_Format(PyExc_TypeError,
                 "SetFeatureSetDefaults called with invalid type: got %s.",
                 Py_TYPE(pdefaults)->tp_name);
    return nullptr;
  }

  CMessage* defaults = reinterpret_cast<CMessage*>(pdefaults);
  if (defaults->message->GetDescriptor() !=
      FeatureSetDefaults::GetDescriptor()) {
    PyErr_Format(
        PyExc_TypeError,
        "SetFeatureSetDefaults called with invalid type: "
        " got %s.",
        std::string(defaults->message->GetDescriptor()->full_name()).c_str());
    return nullptr;
  }

  absl::Status status =
      const_cast<DescriptorPool*>(self->pool->get())
          ->SetFeatureSetDefaults(
              *reinterpret_cast<const FeatureSetDefaults*>(defaults->message));
  if (!status.ok()) {
    PyErr_SetString(PyExc_ValueError, std::string(status.message()).c_str());
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyMethodDef Methods[] = {
    {"Add", Add, METH_O,
     "Adds the FileDescriptorProto and its types to this pool."},
    {"AddSerializedFile", AddSerializedFile, METH_O,
     "Adds a serialized FileDescriptorProto to this pool."},
    {"SetFeatureSetDefaults", SetFeatureSetDefaults, METH_O,
     "Sets the default feature mappings used during the build."},

    {"FindFileByName", FindFileByName, METH_O,
     "Searches for a file descriptor by its .proto name."},
    {"FindMessageTypeByName", FindMessageByName, METH_O,
     "Searches for a message descriptor by full name."},
    {"FindFieldByName", FindFieldByNameMethod, METH_O,
     "Searches for a field descriptor by full name."},
    {"FindExtensionByName", FindExtensionByNameMethod, METH_O,
     "Searches for extension descriptor by full name."},
    {"FindEnumTypeByName", FindEnumTypeByNameMethod, METH_O,
     "Searches for enum type descriptor by full name."},
    {"FindOneofByName", FindOneofByNameMethod, METH_O,
     "Searches for oneof descriptor by full name."},
    {"FindServiceByName", FindServiceByName, METH_O,
     "Searches for service descriptor by full name."},
    {"FindMethodByName", FindMethodByName, METH_O,
     "Searches for method descriptor by full name."},

    {"FindFileContainingSymbol", FindFileContainingSymbol, METH_O,
     "Gets the FileDescriptor containing the specified symbol."},
    {"FindExtensionByNumber", FindExtensionByNumber, METH_VARARGS,
     "Gets the extension descriptor for the given number."},
    {"FindAllExtensions", FindAllExtensions, METH_O,
     "Gets all known extensions of the given message descriptor."},
    {nullptr},
};

}  // namespace cdescriptor_pool

PyTypeObject PyDescriptorPool_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) FULL_MODULE_NAME
    ".DescriptorPool",          // tp_name
    sizeof(PyDescriptorPool),   // tp_basicsize
    0,                          // tp_itemsize
    cdescriptor_pool::Dealloc,  // tp_dealloc
#if PY_VERSION_HEX < 0x03080000
    nullptr,  // tp_print
#else
    0,  // tp_vectorcall_offset
#endif
    nullptr,                                  // tp_getattr
    nullptr,                                  // tp_setattr
    nullptr,                                  // tp_compare
    nullptr,                                  // tp_repr
    nullptr,                                  // tp_as_number
    nullptr,                                  // tp_as_sequence
    nullptr,                                  // tp_as_mapping
    nullptr,                                  // tp_hash
    nullptr,                                  // tp_call
    nullptr,                                  // tp_str
    nullptr,                                  // tp_getattro
    nullptr,                                  // tp_setattro
    nullptr,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,  // tp_flags
    "A Descriptor Pool",                      // tp_doc
    cdescriptor_pool::GcTraverse,             // tp_traverse
    cdescriptor_pool::GcClear,                // tp_clear
    nullptr,                                  // tp_richcompare
    0,                                        // tp_weaklistoffset
    nullptr,                                  // tp_iter
    nullptr,                                  // tp_iternext
    cdescriptor_pool::Methods,                // tp_methods
    nullptr,                                  // tp_members
    nullptr,                                  // tp_getset
    nullptr,                                  // tp_base
    nullptr,                                  // tp_dict
    nullptr,                                  // tp_descr_get
    nullptr,                                  // tp_descr_set
    0,                                        // tp_dictoffset
    nullptr,                                  // tp_init
    nullptr,                                  // tp_alloc
    cdescriptor_pool::New,                    // tp_new
    PyObject_GC_Del,                          // tp_free
};

// This is the DescriptorPool which contains all the definitions from the
// generated _pb2.py modules.
static PyDescriptorPool* python_generated_pool = nullptr;

bool InitDescriptorPool() {
  if (PyType_Ready(&PyDescriptorPool_Type) < 0) return false;

  // The Pool of messages declared in Python libraries.
  // generated_pool() contains all messages already linked in C++ libraries, and
  // is used as underlay.
  descriptor_pool_map =
      new absl::flat_hash_map<const DescriptorPool*, PyDescriptorPool*>;
  python_generated_pool = cdescriptor_pool::PyDescriptorPool_NewWithUnderlay(
      DescriptorPool::generated_pool());
  if (python_generated_pool == nullptr) {
    delete descriptor_pool_map;
    return false;
  }

  // Register this pool to be found for C++-generated descriptors.
  descriptor_pool_map->insert(
      std::make_pair(DescriptorPool::generated_pool(), python_generated_pool));
  return true;
}

// The default DescriptorPool used everywhere in this module.
// Today it's the python_generated_pool.
// TODO: Remove all usages of this function: the pool should be
// derived from the context.
PyDescriptorPool* GetDefaultDescriptorPool() { return python_generated_pool; }

PyDescriptorPool* GetDescriptorPool_FromPool(const DescriptorPool* pool) {
  // Fast path for standard descriptors.
  if (pool == python_generated_pool->pool->get() ||
      pool == DescriptorPool::generated_pool()) {
    return python_generated_pool;
  }
  FreeThreadingLockGuard lock(descriptor_pool_map_mutex);
  auto it = descriptor_pool_map->find(pool);
  if (it == descriptor_pool_map->end()) {
    PyErr_SetString(PyExc_KeyError, "Unknown descriptor pool");
    return nullptr;
  }
  return it->second;
}

PyObject* PyDescriptorPool_FromPool(const DescriptorPool* pool) {
  PyDescriptorPool* existing_pool = GetDescriptorPool_FromPool(pool);
  if (existing_pool != nullptr) {
    Py_INCREF(existing_pool);
    return reinterpret_cast<PyObject*>(existing_pool);
  } else {
    PyErr_Clear();
  }

  PyDescriptorPool* cpool = cdescriptor_pool::_CreateDescriptorPool();
  if (cpool == nullptr) {
    return nullptr;
  }
  cpool->pool = new std::shared_ptr<const DescriptorPool>(
      UnsafeSharedPointerFromRaw(pool));
  cpool->is_mutable = false;
  cpool->underlay = nullptr;
  {
    FreeThreadingLockGuard lock(descriptor_pool_map_mutex);
    if (!descriptor_pool_map->insert(std::make_pair(cpool->pool->get(), cpool))
             .second) {
      // Should never happen -- We already checked the existence above.
      PyErr_SetString(PyExc_ValueError, "DescriptorPool already registered");
      return nullptr;
    }
  }

  return reinterpret_cast<PyObject*>(cpool);
}

PyObject* PyDescriptorPool_FromSharedPool(
    std::shared_ptr<const DescriptorPool> pool,
    std::shared_ptr<const DescriptorDatabase> database) {
  if (pool == nullptr) {
    PyErr_SetString(PyExc_ValueError, "DescriptorPool is null");
    return nullptr;
  }
  PyDescriptorPool* existing_pool = GetDescriptorPool_FromPool(pool.get());
  if (existing_pool != nullptr) {
    Py_INCREF(existing_pool);
    return reinterpret_cast<PyObject*>(existing_pool);
  } else {
    PyErr_Clear();
  }

  PyDescriptorPool* cpool = cdescriptor_pool::_CreateDescriptorPool();
  if (cpool == nullptr) {
    return nullptr;
  }
  cpool->pool = new std::shared_ptr<const DescriptorPool>(pool);
  if (database != nullptr) {
    cpool->database = new std::shared_ptr<const DescriptorDatabase>(database);
  }
  cpool->is_mutable = false;
  cpool->underlay = nullptr;
  {
    FreeThreadingLockGuard lock(descriptor_pool_map_mutex);
    if (!descriptor_pool_map->insert(std::make_pair(cpool->pool->get(), cpool))
             .second) {
      PyErr_SetString(PyExc_ValueError, "DescriptorPool already registered");
      return nullptr;
    }
  }

  return reinterpret_cast<PyObject*>(cpool);
}

PyObject* PyDescriptorPool_FromPool(
    std::unique_ptr<const google::protobuf::DescriptorPool> pool,
    std::unique_ptr<const google::protobuf::DescriptorDatabase> database) {
  if (pool == nullptr) {
    PyErr_SetString(PyExc_ValueError, "DescriptorPool is null");
    return nullptr;
  }
  // There should not be any Python-side pool for this C++ object.
  PyDescriptorPool* existing_pool = GetDescriptorPool_FromPool(pool.get());
  if (existing_pool != nullptr) {
    PyErr_SetString(PyExc_ValueError, "DescriptorPool already registered");
    return nullptr;
  } else {
    PyErr_Clear();
  }

  PyDescriptorPool* cpool = cdescriptor_pool::_CreateDescriptorPool();
  if (cpool == nullptr) {
    return nullptr;
  }
  cpool->pool = new std::shared_ptr<const DescriptorPool>(std::move(pool));
  if (database != nullptr) {
    cpool->database =
        new std::shared_ptr<const DescriptorDatabase>(std::move(database));
  }
  cpool->is_mutable = false;
  cpool->underlay = nullptr;

  {
    FreeThreadingLockGuard lock(descriptor_pool_map_mutex);
    if (!descriptor_pool_map->insert(std::make_pair(cpool->pool->get(), cpool))
             .second) {
      // Should never happen -- We already checked the existence above.
      PyErr_SetString(PyExc_ValueError, "DescriptorPool already registered");
      return nullptr;
    }
  }

  return reinterpret_cast<PyObject*>(cpool);
}

const DescriptorPool* PyDescriptorPool_AsPool(PyObject* pool) {
  if (!PyObject_TypeCheck(pool, &PyDescriptorPool_Type)) {
    PyErr_SetString(PyExc_TypeError, "Not a DescriptorPool");
    return nullptr;
  }
  PyDescriptorPool* cpool = reinterpret_cast<PyDescriptorPool*>(pool);
  return cpool->pool != nullptr ? cpool->pool->get() : nullptr;
}

}  // namespace python
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
