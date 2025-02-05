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
  ProtoAPIDescriptorDatabase() {
    PyObject* descriptor_pool =
        PyImport_ImportModule("google.protobuf.descriptor_pool");
    if (descriptor_pool == nullptr) {
      ABSL_LOG(ERROR)
          << "Failed to import google.protobuf.descriptor_pool module.";
    }

    pool_ = PyObject_CallMethod(descriptor_pool, "Default", nullptr);
    if (pool_ == nullptr) {
      ABSL_LOG(ERROR) << "Failed to get python Default pool.";
    }
    Py_DECREF(descriptor_pool);
  };

  ~ProtoAPIDescriptorDatabase() {
    // Objects of this class are meant to be `static`ally initialized and
    // never destroyed. This is a commonly used approach, because the order
    // in which destructors of static objects run is unpredictable. In
    // particular, it is possible that the Python interpreter may have been
    // finalized already.
    ABSL_DLOG(ERROR) << "MEANT TO BE UNREACHABLE.";
  };

  bool FindFileByName(const std::string& filename,
                      google::protobuf::FileDescriptorProto* output) override {
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

    bool ok = output->ParseFromArray(
        reinterpret_cast<uint8_t*>(PyBytes_AS_STRING(pyfile_serialized)),
        PyBytes_GET_SIZE(pyfile_serialized));
    if (!ok) {
      ABSL_LOG(ERROR) << "Failed to parse descriptor for " << filename;
    }
    Py_DECREF(pyfile_serialized);
    return ok;
  }

  bool FindFileContainingSymbol(const std::string& symbol_name,
                                google::protobuf::FileDescriptorProto* output) override {
    return false;
  }

  bool FindFileContainingExtension(
      const std::string& containing_type, int field_number,
      google::protobuf::FileDescriptorProto* output) override {
    return false;
  }

  PyObject* pool() { return pool_; }

 private:
  PyObject* pool_;
};

absl::StatusOr<const google::protobuf::Descriptor*> FindMessageDescriptor(
    PyObject* pyfile, const char* descriptor_full_name) {
  static auto* database = new ProtoAPIDescriptorDatabase();
  static auto* pool = new google::protobuf::DescriptorPool(database);
  PyObject* pyfile_name = PyObject_GetAttrString(pyfile, "name");
  if (pyfile_name == nullptr) {
    return absl::InvalidArgumentError("FileDescriptor has no attribute 'name'");
  }
  PyObject* pyfile_pool = PyObject_GetAttrString(pyfile, "pool");
  if (pyfile_pool == nullptr) {
    Py_DECREF(pyfile_name);
    return absl::InvalidArgumentError("FileDescriptor has no attribute 'pool'");
  }
  // Check the file descriptor is from generated pool.
  bool is_from_generated_pool = database->pool() == pyfile_pool;
  Py_DECREF(pyfile_pool);
  const char* pyfile_name_char_ptr = PyUnicode_AsUTF8(pyfile_name);
  if (pyfile_name_char_ptr == nullptr) {
    Py_DECREF(pyfile_name);
    return absl::InvalidArgumentError(
        "FileDescriptor 'name' PyUnicode_AsUTF8() failure.");
  }
  if (!is_from_generated_pool) {
    std::string error_msg = absl::StrCat(pyfile_name_char_ptr,
                                         " is not from generated pool");
    Py_DECREF(pyfile_name);
    return absl::InvalidArgumentError(error_msg);
  }
  const google::protobuf::FileDescriptor* file_descriptor =
      pool->FindFileByName(pyfile_name_char_ptr);
  Py_DECREF(pyfile_name);
  if (file_descriptor == nullptr) {
    // Already checked the file is from generated pool above, this
    // error should never be reached.
    ABSL_DLOG(ERROR) << "MEANT TO BE UNREACHABLE.";
    std::string error_msg = absl::StrCat("Fail to find/build file ",
                                         pyfile_name_char_ptr);
    return absl::InternalError(error_msg);
  }

  const google::protobuf::Descriptor* descriptor =
      pool->FindMessageTypeByName(descriptor_full_name);
  if (descriptor == nullptr) {
    return absl::InternalError("Fail to find descriptor by name.");
  }
  return descriptor;
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
  RETURN_IF_ERROR(d.status());
  Py_DECREF(fn);
  return GetFactory()->GetPrototype(*d)->New();
}

bool CopyToOwnedMsg(google::protobuf::Message** copy, const google::protobuf::Message& message) {
  *copy = message.New();
  std::string wire;
  message.SerializeToString(&wire);
  (*copy)->ParseFromArray(wire.data(), wire.size());
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
        PyObject_CallMethod(py_msg, "SerializeToString", nullptr));
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
    if (!(*msg)->ParseFromArray(data, len)) {
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
