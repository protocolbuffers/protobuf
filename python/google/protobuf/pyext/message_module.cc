// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#include <cstdint>
#include <string>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "google/protobuf/descriptor.pb.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_database.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/proto_api.h"
#include "google/protobuf/pyext/descriptor.h"
#include "google/protobuf/pyext/descriptor_pool.h"
#include "google/protobuf/pyext/message.h"
#include "google/protobuf/pyext/message_factory.h"
#include "google/protobuf/stubs/status_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace {

class ProtoAPIDescriptorDatabase : public google::protobuf::DescriptorDatabase {
 public:
  explicit ProtoAPIDescriptorDatabase(PyObject* py_pool) : pool_(py_pool) {};

  ~ProtoAPIDescriptorDatabase() {};

  bool FindFileByName(absl::string_view filename,
                      google::protobuf::FileDescriptorProto* output) override {
    if (PyErr_Occurred()) {
      // If an error has already occurred, PyObject_CallMethod will assert.
      // Reset it so we can continue.
      PyErr_Clear();
    }

    PyObject* pyfile_name =
        PyUnicode_FromStringAndSize(filename.data(), filename.size());
    if (pyfile_name == nullptr) {
      PyErr_Format(PyExc_TypeError, "Fail to convert proto file name");
      return false;
    }

    PyObject* pyfile =
        PyObject_CallMethod(pool_, "FindFileByName", "O", pyfile_name);
    Py_DECREF(pyfile_name);
    if (pyfile == nullptr) {
      PyErr_Format(PyExc_TypeError, "Default python pool fail to find %s",
                   filename.data());
      return false;
    }

    PyObject* pyfile_serialized =
        PyObject_GetAttrString(pyfile, "serialized_pb");
    Py_DECREF(pyfile);
    if (pyfile_serialized == nullptr) {
      PyErr_Format(PyExc_TypeError,
                   "Python file has no attribute 'serialized_pb'");
      return false;
    }

    bool ok = output->ParsePartialFromArray(
        reinterpret_cast<uint8_t*>(PyBytes_AS_STRING(pyfile_serialized)),
        PyBytes_GET_SIZE(pyfile_serialized));
    if (!ok) {
      ABSL_LOG(ERROR) << "Failed to parse descriptor for " << filename;
    }
    Py_DECREF(pyfile_serialized);
    return ok;
  }

  bool FindFileContainingSymbol(absl::string_view symbol_name,
                                google::protobuf::FileDescriptorProto* output) override {
    return false;
  }

  bool FindFileContainingExtension(
      absl::string_view containing_type, int field_number,
      google::protobuf::FileDescriptorProto* output) override {
    return false;
  }

  PyObject* pool() { return pool_; }

 private:
  PyObject* pool_;
};

struct DescriptorPoolState {
  // clang-format off
  PyObject_HEAD

  std::shared_ptr<const google::protobuf::DescriptorDatabase> database;
  std::shared_ptr<const google::protobuf::DescriptorPool> pool;
};

void DeallocDescriptorPoolState (DescriptorPoolState* self) {
  self->pool.~shared_ptr();
  self->database.~shared_ptr();
  Py_TYPE(self)->tp_free((PyObject *)self);
}

PyTypeObject PyDescriptorPoolState_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) FULL_MODULE_NAME
    ".DescriptorPoolState",       //  tp_name
    sizeof(DescriptorPoolState),  //  tp_basicsize
    0,                            //  tp_itemsize
    (destructor)DeallocDescriptorPoolState,   //  tp_dealloc
    0,                            //  tp_vectorcall_offset
    nullptr,                      //  tp_getattr
    nullptr,                      //  tp_setattr
    nullptr,                      //  tp_compare
    nullptr,                      //  tp_repr
    nullptr,                      //  tp_as_number
    nullptr,                      //  tp_as_sequence
    nullptr,                      //  tp_as_mapping
    PyObject_HashNotImplemented,  //  tp_hash
    nullptr,                      //  tp_call
    nullptr,                      //  tp_str
    nullptr,                      //  tp_getattro
    nullptr,                      //  tp_setattro
    nullptr,                      //  tp_as_buffer
    Py_TPFLAGS_DEFAULT,           //  tp_flags
    "DescriptorPoolState",        //  tp_doc
};

// Helper to convert C++ FileDescriptor to Python FileDescriptorProto
static PyObject* ConvertFileDescriptorToPythonProto(
    const google::protobuf::FileDescriptor* file_desc) {
  google::protobuf::FileDescriptorProto file_proto;
  file_desc->CopyTo(&file_proto);
  std::string serialized = file_proto.SerializeAsString();

  PyObject* desc_pb2 = PyImport_ImportModule("google.protobuf.descriptor_pb2");
  if (desc_pb2 == nullptr) return nullptr;
  PyObject* file_desc_proto_cls =
      PyObject_GetAttrString(desc_pb2, "FileDescriptorProto");
  Py_DECREF(desc_pb2);
  if (file_desc_proto_cls == nullptr) return nullptr;

  PyObject* serialized_bytes =
      PyBytes_FromStringAndSize(serialized.data(), serialized.size());
  if (serialized_bytes == nullptr) {
    Py_DECREF(file_desc_proto_cls);
    return nullptr;
  }

  PyObject* file_proto_obj = PyObject_CallMethod(
      file_desc_proto_cls, "FromString", "O", serialized_bytes);
  Py_DECREF(serialized_bytes);
  Py_DECREF(file_desc_proto_cls);

  return file_proto_obj;
}

struct PyCDescriptorDatabaseProxyObject {
  PyObject_HEAD
  std::shared_ptr<const google::protobuf::DescriptorPool> pool;
};

static PyObject* PyCDescriptorDatabaseProxy_FindFileByName(PyObject* self,
                                                           PyObject* args) {
  const char* filename_str;
  if (!PyArg_ParseTuple(args, "s", &filename_str)) {
    return nullptr;
  }
  auto* proxy = reinterpret_cast<PyCDescriptorDatabaseProxyObject*>(self);
  const google::protobuf::FileDescriptor* file_desc =
      proxy->pool->FindFileByName(filename_str);
  if (file_desc == nullptr) {
    PyErr_SetString(PyExc_KeyError, filename_str);
    return nullptr;
  }
  return ConvertFileDescriptorToPythonProto(file_desc);
}

static PyObject* PyCDescriptorDatabaseProxy_FindFileContainingSymbol(
    PyObject* self, PyObject* args) {
  const char* symbol_str;
  if (!PyArg_ParseTuple(args, "s", &symbol_str)) {
    return nullptr;
  }
  auto* proxy = reinterpret_cast<PyCDescriptorDatabaseProxyObject*>(self);
  const google::protobuf::FileDescriptor* file_desc =
      proxy->pool->FindFileContainingSymbol(symbol_str);
  if (file_desc == nullptr) {
    PyErr_SetString(PyExc_KeyError, symbol_str);
    return nullptr;
  }
  return ConvertFileDescriptorToPythonProto(file_desc);
}

static PyMethodDef PyCDescriptorDatabaseProxy_Methods[] = {
    {"FindFileByName", PyCDescriptorDatabaseProxy_FindFileByName, METH_VARARGS,
     "Find file by name"},
    {"FindFileContainingSymbol",
     PyCDescriptorDatabaseProxy_FindFileContainingSymbol, METH_VARARGS,
     "Find file containing symbol"},
    {nullptr, nullptr, 0, nullptr}};

void DeallocPyCDescriptorDatabaseProxy(PyCDescriptorDatabaseProxyObject* self) {
  self->pool.~shared_ptr();
  Py_TYPE(self)->tp_free((PyObject*)self);
}

PyTypeObject PyCDescriptorDatabaseProxy_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) FULL_MODULE_NAME
    ".CDescriptorDatabaseProxy",       //  tp_name
    sizeof(PyCDescriptorDatabaseProxyObject),  //  tp_basicsize
    0,                            //  tp_itemsize
    (destructor)DeallocPyCDescriptorDatabaseProxy,   //  tp_dealloc
    0,                            //  tp_vectorcall_offset
    nullptr,                      //  tp_getattr
    nullptr,                      //  tp_setattr
    nullptr,                      //  tp_compare
    nullptr,                      //  tp_repr
    nullptr,                      //  tp_as_number
    nullptr,                      //  tp_as_sequence
    nullptr,                      //  tp_as_mapping
    PyObject_HashNotImplemented,  //  tp_hash
    nullptr,                      //  tp_call
    nullptr,                      //  tp_str
    nullptr,                      //  tp_getattro
    nullptr,                      //  tp_setattro
    nullptr,                      //  tp_as_buffer
    Py_TPFLAGS_DEFAULT,           //  tp_flags
    "CDescriptorDatabaseProxy",        //  tp_doc
    nullptr,                      //  tp_traverse
    nullptr,                      //  tp_clear
    nullptr,                      //  tp_richcompare
    0,                            //  tp_weaklistoffset
    nullptr,                      //  tp_iter
    nullptr,                      //  tp_iternext
    PyCDescriptorDatabaseProxy_Methods,  //  tp_methods
};


PyObject* PyDescriptorPoolState_New(PyObject* pyfile_pool) {
  PyObject* pypool_state = PyType_GenericAlloc(&PyDescriptorPoolState_Type, 0);
  if (pypool_state == nullptr) {
    PyErr_SetString(PyExc_MemoryError,
                    "Fail to new PyDescriptorPoolState_Type");
    return nullptr;
  }
  DescriptorPoolState* pool_state =
      reinterpret_cast<DescriptorPoolState*>(pypool_state);
  auto db = std::make_shared<ProtoAPIDescriptorDatabase>(pyfile_pool);
  auto pool = std::make_shared<google::protobuf::DescriptorPool>(db.get());
  new (&pool_state->database)
      std::shared_ptr<const google::protobuf::DescriptorDatabase>(db);
  new (&pool_state->pool)
      std::shared_ptr<const google::protobuf::DescriptorPool>(pool);
  return pypool_state;
}

PyObject* InitAndGetPoolMap() {
  static PyObject* pypool_map = nullptr;
  if (pypool_map != nullptr) {
    return pypool_map;
  }
#if PY_VERSION_HEX >= 0x030C0000
  // Returns a WeakKeyDictionary. The key will be a python pool and
  // the value will be PyDescriptorPoolState_Type.
  // PyDescriptorPoolState_Type should be ready for the usage.
  if (PyType_Ready(&PyDescriptorPoolState_Type) < 0) {
    return nullptr;
  }
  PyObject* weakref = PyImport_ImportModule("weakref");
  pypool_map = PyObject_CallMethod(weakref, "WeakKeyDictionary", NULL);
  Py_DECREF(weakref);
#else
  pypool_map = PyDict_New();
#endif
  return pypool_map;
}

absl::StatusOr<const google::protobuf::Descriptor*> FindMessageDescriptor(
    PyObject* pyfile, const char* descriptor_full_name) {
  static PyObject* pypool_map = InitAndGetPoolMap();
  if (pypool_map == nullptr) {
    return absl::InternalError("Fail to create pypool_map");
  }
  PyObject* pyfile_name = nullptr;
  PyObject* pyfile_pool = nullptr;
  PyObject* pypool_state = nullptr;
  const google::protobuf::DescriptorPool* pool;
  DescriptorPoolState* pool_state;
  const char* pyfile_name_char_ptr;
  const google::protobuf::FileDescriptor* file_descriptor;
  absl::StatusOr<const google::protobuf::Descriptor*> ret;

  pyfile_name = PyObject_GetAttrString(pyfile, "name");
  if (pyfile_name == nullptr) {
    ret = absl::InvalidArgumentError("FileDescriptor has no attribute 'name'");
    goto err;
  }
  pyfile_pool = PyObject_GetAttrString(pyfile, "pool");
  if (pyfile_pool == nullptr) {
    ret = absl::InvalidArgumentError("FileDescriptor has no attribute 'pool'");
    goto err;
  }

  pypool_state = PyObject_GetItem(pypool_map, pyfile_pool);
  if (pypool_state == nullptr) {
    if (PyErr_ExceptionMatches(PyExc_KeyError)) {
      // Ignore the KeyError
      PyErr_Clear();
    }
    PyErr_Print();
    pypool_state = PyDescriptorPoolState_New(pyfile_pool);
    if (pypool_state == nullptr) {
      ret = absl::InternalError("Fail to create PyDescriptorPoolState_Type");
      goto err;
    }
    if (PyObject_SetItem(pypool_map, pyfile_pool, pypool_state) < 0) {
      ret = absl::InternalError(
          "Fail to insert PyDescriptorPoolState_Type into pypool_map");
      goto err;
    }
  }
  pool_state = reinterpret_cast<DescriptorPoolState*>(pypool_state);
  pool = pool_state->pool.get();
  pyfile_name_char_ptr = PyUnicode_AsUTF8(pyfile_name);
  if (pyfile_name_char_ptr == nullptr) {
    ret = absl::InvalidArgumentError(
        "FileDescriptor 'name' PyUnicode_AsUTF8() failure.");
    goto err;
  }
  file_descriptor = pool->FindFileByName(pyfile_name_char_ptr);
  if (file_descriptor == nullptr) {
    // This error should never be reached.
    ABSL_DLOG(ERROR) << "MEANT TO BE UNREACHABLE.";
    std::string error_msg = absl::StrCat("Fail to find/build file ",
                                         pyfile_name_char_ptr);
    ret = absl::InternalError(error_msg);
    goto err;
  }

  ret = pool->FindMessageTypeByName(descriptor_full_name);
  if (ret.value() == nullptr) {
    ret = absl::InternalError("Fail to find descriptor by name.");
  }

err:
  Py_XDECREF(pyfile_name);
  Py_XDECREF(pyfile_pool);
  Py_XDECREF(pypool_state);
  return ret;
}

google::protobuf::DynamicMessageFactory* GetFactory() {
  static google::protobuf::DynamicMessageFactory* factory =
      new google::protobuf::DynamicMessageFactory;
  return factory;
}

absl::StatusOr<google::protobuf::Message*> CreateNewMessage(PyObject* py_msg) {
  PyObject* pyd = PyObject_GetAttrString(py_msg, "DESCRIPTOR");
  if (pyd == nullptr) {
    return absl::InvalidArgumentError("py_msg has no attribute 'DESCRIPTOR'");
  }

  PyObject* fn = PyObject_GetAttrString(pyd, "full_name");
  if (fn == nullptr) {
    return absl::InvalidArgumentError(
        "DESCRIPTOR has no attribute 'full_name'");
  }

  const char* descriptor_full_name = PyUnicode_AsUTF8(fn);
  if (descriptor_full_name == nullptr) {
    return absl::InternalError("Fail to convert descriptor full name");
  }

  PyObject* pyfile = PyObject_GetAttrString(pyd, "file");
  Py_DECREF(pyd);
  if (pyfile == nullptr) {
    return absl::InvalidArgumentError("DESCRIPTOR has no attribute 'file'");
  }
  auto gen_d = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(
      descriptor_full_name);
  if (gen_d) {
    Py_DECREF(pyfile);
    Py_DECREF(fn);
    return google::protobuf::MessageFactory::generated_factory()
        ->GetPrototype(gen_d)
        ->New();
  }
  auto d = FindMessageDescriptor(pyfile, descriptor_full_name);
  Py_DECREF(pyfile);
  Py_DECREF(fn);
  RETURN_IF_ERROR(d.status());
  return GetFactory()->GetPrototype(*d)->New();
}

bool CopyToOwnedMsg(google::protobuf::Message** copy, const google::protobuf::Message& message) {
  *copy = message.New();
  std::string wire;
  // TODO: Remove this suppression.
      (void)message.SerializePartialToString(&wire);
  // TODO: Remove this suppression.
      (void)(*copy)->ParsePartialFromString(wire);
  return true;
}

// C++ API.  Clients get at this via proto_api.h
struct ApiImplementation : google::protobuf::python::PyProto_API {
  absl::StatusOr<google::protobuf::python::PythonMessageMutator> GetClearedMessageMutator(
      PyObject* py_msg) const override {
    if (PyObject_TypeCheck(py_msg, google::protobuf::python::CMessage_Type)) {
      google::protobuf::Message* message =
          google::protobuf::python::PyMessage_GetMutableMessagePointer(py_msg);
      if (message == nullptr) {
        return absl::InternalError(
            "Fail to get message pointer. The message "
            "may already had a reference.");
      }
      message->Clear();
      return CreatePythonMessageMutator(nullptr, message, py_msg);
    }

    auto msg = CreateNewMessage(py_msg);
    RETURN_IF_ERROR(msg.status());
    return CreatePythonMessageMutator(*msg, *msg, py_msg);
  }

  absl::StatusOr<google::protobuf::python::PythonConstMessagePointer>
  GetConstMessagePointer(PyObject* py_msg) const override {
    if (PyObject_TypeCheck(py_msg, google::protobuf::python::CMessage_Type)) {
      const google::protobuf::Message* message =
          google::protobuf::python::PyMessage_GetMessagePointer(py_msg);
      google::protobuf::Message* owned_msg = nullptr;
      ABSL_DCHECK(CopyToOwnedMsg(&owned_msg, *message));
      return CreatePythonConstMessagePointer(owned_msg, message, py_msg);
    }
    auto msg = CreateNewMessage(py_msg);
    RETURN_IF_ERROR(msg.status());
    PyObject* serialized_pb(
        PyObject_CallMethod(py_msg, "SerializePartialToString", nullptr));
    if (serialized_pb == nullptr) {
      return absl::InternalError("Fail to serialize py_msg");
    }
    char* data;
    Py_ssize_t len;
    if (PyBytes_AsStringAndSize(serialized_pb, &data, &len) < 0) {
      Py_DECREF(serialized_pb);
      return absl::InternalError(
          "Fail to get bytes from py_msg serialized data");
    }
    if (!(*msg)->ParsePartialFromString(absl::string_view(data, len))) {
      Py_DECREF(serialized_pb);
      return absl::InternalError(
          "Couldn't parse py_message to google::protobuf::Message*!");
    }
    Py_DECREF(serialized_pb);
    return CreatePythonConstMessagePointer(*msg, *msg, py_msg);
  }

  const google::protobuf::Message* GetMessagePointer(PyObject* msg) const override {
    return google::protobuf::python::PyMessage_GetMessagePointer(msg);
  }
  google::protobuf::Message* GetMutableMessagePointer(PyObject* msg) const override {
    return google::protobuf::python::PyMessage_GetMutableMessagePointer(msg);
  }
  const google::protobuf::Descriptor* MessageDescriptor_AsDescriptor(
      PyObject* desc) const override {
    return google::protobuf::python::PyMessageDescriptor_AsDescriptor(desc);
  }
  const google::protobuf::EnumDescriptor* EnumDescriptor_AsDescriptor(
      PyObject* enum_desc) const override {
    return google::protobuf::python::PyEnumDescriptor_AsDescriptor(enum_desc);
  }

  const google::protobuf::DescriptorPool* GetDefaultDescriptorPool() const override {
    return google::protobuf::python::GetDefaultDescriptorPool()->pool;
  }

  google::protobuf::MessageFactory* GetDefaultMessageFactory() const override {
    return google::protobuf::python::GetDefaultDescriptorPool()
        ->py_message_factory->message_factory;
  }
  PyObject* NewMessage(const google::protobuf::Descriptor* descriptor,
                       PyObject* py_message_factory) const override {
    return google::protobuf::python::PyMessage_New(descriptor, py_message_factory);
  }
  PyObject* NewMessageOwnedExternally(
      google::protobuf::Message* msg, PyObject* py_message_factory) const override {
    return google::protobuf::python::PyMessage_NewMessageOwnedExternally(
        msg, py_message_factory);
  }
  PyObject* DescriptorPool_FromPool(
      const google::protobuf::DescriptorPool* pool) const override {
    return google::protobuf::python::PyDescriptorPool_FromPool(pool);
  }
  PyObject* DescriptorPool_FromPool(
      std::unique_ptr<const google::protobuf::DescriptorPool> pool,
      std::unique_ptr<const google::protobuf::DescriptorDatabase> database)
      const override {
    return google::protobuf::python::PyDescriptorPool_FromSharedPool(
        std::shared_ptr<const google::protobuf::DescriptorPool>(std::move(pool)),
        std::shared_ptr<const google::protobuf::DescriptorDatabase>(std::move(database)));
  }

  PyObject* DescriptorPool_FromSharedPool(
      std::shared_ptr<const google::protobuf::DescriptorPool> pool,
      std::shared_ptr<const google::protobuf::DescriptorDatabase> database)
      const override {
    bool is_cpp = false;
    PyObject* api_module = PyImport_ImportModule(
        "google.protobuf.internal.api_implementation");
    if (api_module != nullptr) {
      PyObject* type_func = PyObject_GetAttrString(api_module, "Type");
      Py_DECREF(api_module);
      if (type_func != nullptr) {
        PyObject* type_str = PyObject_CallObject(type_func, nullptr);
        Py_DECREF(type_func);
        if (type_str != nullptr) {
          const char* type_c_str = PyUnicode_AsUTF8(type_str);
          if (type_c_str != nullptr && strcmp(type_c_str, "cpp") == 0) {
            is_cpp = true;
          }
          Py_DECREF(type_str);
        }
      }
    } else {
      PyErr_Clear();
    }

    if (is_cpp) {
      return google::protobuf::python::PyDescriptorPool_FromSharedPool(pool, database);
    }

    // !is_cpp case
    if (pool.get() == google::protobuf::DescriptorPool::generated_pool()) {
      static PyObject* default_func = nullptr;
      if (default_func == nullptr) {
        PyObject* pool_module =
            PyImport_ImportModule("google.protobuf.descriptor_pool");
        if (pool_module != nullptr) {
          default_func = PyObject_GetAttrString(pool_module, "Default");
          Py_DECREF(pool_module);
        }
      }
      if (default_func != nullptr) {
        PyObject* default_pool = PyObject_CallObject(default_func, nullptr);
        if (default_pool != nullptr) {
          return default_pool;
        }
      }
      PyErr_Clear();
    }

    // First, check if this pool is already in pypool_map
    static PyObject* pypool_map = InitAndGetPoolMap();
    if (pypool_map != nullptr) {
      PyObject* items = PyObject_CallMethod(pypool_map, "items", nullptr);
      if (items != nullptr) {
        PyObject* iter = PyObject_GetIter(items);
        Py_DECREF(items);
        if (iter != nullptr) {
          PyObject* item;
          while ((item = PyIter_Next(iter)) != nullptr) {
            PyObject* py_pool_candidate = PyTuple_GetItem(item, 0);
            PyObject* pypool_state = PyTuple_GetItem(item, 1);
            auto* pool_state =
                reinterpret_cast<DescriptorPoolState*>(pypool_state);
            if (pool_state->pool.get() == pool.get()) {
              Py_INCREF(py_pool_candidate);
              Py_DECREF(item);
              Py_DECREF(iter);
              return py_pool_candidate;
            }
            Py_DECREF(item);
          }
          Py_DECREF(iter);
        }
      }
      PyErr_Clear();
    }

    // Create standard Python pool
    PyObject* pool_module =
        PyImport_ImportModule("google.protobuf.descriptor_pool");
    if (pool_module == nullptr) return nullptr;
    PyObject* pool_cls = PyObject_GetAttrString(pool_module, "DescriptorPool");
    Py_DECREF(pool_module);
    if (pool_cls == nullptr) return nullptr;

    // Create Proxy database
    PyObject* proxy_db =
        PyType_GenericAlloc(&PyCDescriptorDatabaseProxy_Type, 0);
    if (proxy_db == nullptr) {
      Py_DECREF(pool_cls);
      return nullptr;
    }
    auto* proxy_obj =
        reinterpret_cast<PyCDescriptorDatabaseProxyObject*>(proxy_db);
    new (&proxy_obj->pool) std::shared_ptr<const google::protobuf::DescriptorPool>(pool);

    // Call DescriptorPool(proxy_db)
    PyObject* args = PyTuple_Pack(1, proxy_db);
    Py_DECREF(proxy_db);
    if (args == nullptr) {
      Py_DECREF(pool_cls);
      return nullptr;
    }
    PyObject* py_pool = PyObject_CallObject(pool_cls, args);
    Py_DECREF(args);
    Py_DECREF(pool_cls);
    if (py_pool == nullptr) return nullptr;

    // Store in pypool_map
    if (pypool_map == nullptr) {
      Py_DECREF(py_pool);
      return nullptr;
    }

    // Create DescriptorPoolState
    PyObject* pypool_state = PyType_GenericAlloc(
        &PyDescriptorPoolState_Type, 0);
    if (pypool_state == nullptr) {
      Py_DECREF(py_pool);
      return nullptr;
    }
    auto* pool_state = reinterpret_cast<DescriptorPoolState*>(pypool_state);
    new (&pool_state->database)
        std::shared_ptr<const google::protobuf::DescriptorDatabase>(database);
    new (&pool_state->pool) std::shared_ptr<const google::protobuf::DescriptorPool>(pool);

    if (PyObject_SetItem(pypool_map, py_pool, pypool_state) < 0) {
      Py_DECREF(pypool_state);
      Py_DECREF(py_pool);
      return nullptr;
    }
    Py_DECREF(pypool_state);

    return py_pool;
  }
  const google::protobuf::DescriptorPool* DescriptorPool_AsPool(
      PyObject* pool) const override {
    return google::protobuf::python::PyDescriptorPool_AsPool(pool);
  }
};

}  // namespace

static const char module_docstring[] =
    "python-proto2 is a module that can be used to enhance proto2 Python API\n"
    "performance.\n"
    "\n"
    "It provides access to the protocol buffers C++ reflection API that\n"
    "implements the basic protocol buffer functions.";

static PyMethodDef ModuleMethods[] = {
    {"SetAllowOversizeProtos",
     (PyCFunction)google::protobuf::python::cmessage::SetAllowOversizeProtos, METH_O,
     "Enable/disable oversize proto parsing."},
    // DO NOT USE: For migration and testing only.
    {nullptr, nullptr}};

static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                     "_message",
                                     module_docstring,
                                     -1,
                                     ModuleMethods, /* m_methods */
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr};

PyMODINIT_FUNC PyInit__message() {
  PyObject* m;
  m = PyModule_Create(&_module);
  if (m == nullptr) {
    return nullptr;
  }

  if (PyType_Ready(&PyCDescriptorDatabaseProxy_Type) < 0) {
    Py_DECREF(m);
    return nullptr;
  }

  if (!google::protobuf::python::InitProto2MessageModule(m)) {
    Py_DECREF(m);
    return nullptr;
  }

  // Adds the C++ API
  if (PyObject* api = PyCapsule_New(
          new ApiImplementation(), google::protobuf::python::PyProtoAPICapsuleName(),
          [](PyObject* o) {
            delete (ApiImplementation*)PyCapsule_GetPointer(
                o, google::protobuf::python::PyProtoAPICapsuleName());
          })) {
    PyModule_AddObject(m, "proto_API", api);
  } else {
    return nullptr;
  }

  return m;
}

#include "google/protobuf/port_undef.inc"
