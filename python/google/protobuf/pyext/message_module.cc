// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/proto_api.h"
#include "google/protobuf/pyext/descriptor.h"
#include "google/protobuf/pyext/descriptor_pool.h"
#include "google/protobuf/pyext/message.h"
#include "google/protobuf/pyext/message_factory.h"

namespace {

class ProtoAPIDescriptorDatabase : public google::protobuf::DescriptorDatabase {
 public:
  ProtoAPIDescriptorDatabase() {
    PyObject* descriptor_pool =
        PyImport_ImportModule("google.protobuf.descriptor_pool");
    if (descriptor_pool == nullptr) {
      LOG(FATAL) << "Failed to import google.protobuf.descriptor_pool module.";
    }

    pool_ = PyObject_CallMethod(descriptor_pool, "Default", nullptr);
    if (pool_ == nullptr) {
      LOG(FATAL) << "Failed to get python Default pool.";
    }
    Py_DECREF(descriptor_pool);
  };

  ~ProtoAPIDescriptorDatabase() {
    // Objects of this class are meant to be `static`ally initialized and
    // never destroyed. This is a commonly used approach, because the order
    // in which destructors of static objects run is unpredictable. In
    // particular, it is possible that the Python interpreter may have been
    // finalized already.
    DLOG(FATAL) << "MEANT TO BE UNREACHABLE.";
  };

  bool FindFileByName(const std::string& filename,
                      google::protobuf::FileDescriptorProto* output) override {
    PyObject* pyfile_name =
        PyUnicode_FromStringAndSize(filename.data(), filename.size());
    if (pyfile_name == nullptr) {
      // Ideally this would be raise from.
      PyErr_Format(PyExc_TypeError, "Fail to convert proto file name");
      return false;
    }

    PyObject* pyfile =
        PyObject_CallMethod(pool_, "FindFileByName", "O", pyfile_name);
    Py_DECREF(pyfile_name);
    if (pyfile == nullptr) {
      // Ideally this would be raise from.
      PyErr_Format(PyExc_TypeError, "Default python pool fail to find %s",
                   filename.data());
      return false;
    }

    PyObject* pyfile_serialized =
        PyObject_GetAttrString(pyfile, "serialized_pb");
    Py_DECREF(pyfile);
    if (pyfile_serialized == nullptr) {
      // Ideally this would be raise from.
      PyErr_Format(PyExc_TypeError,
                   "Python file has no attribute 'serialized_pb'");
      return false;
    }

    bool ok = output->ParseFromArray(
        reinterpret_cast<uint8_t*>(PyBytes_AS_STRING(pyfile_serialized)),
        PyBytes_GET_SIZE(pyfile_serialized));
    if (!ok) {
      LOG(ERROR) << "Failed to parse descriptor for " << filename;
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

const google::protobuf::Descriptor* FindMessageDescriptor(
    PyObject* pyfile, const char* descritor_full_name) {
  static auto* database = new ProtoAPIDescriptorDatabase();
  static auto* pool = new google::protobuf::DescriptorPool(database);
  PyObject* pyfile_name = PyObject_GetAttrString(pyfile, "name");
  if (pyfile_name == nullptr) {
    // Ideally this would be raise from.
    PyErr_Format(PyExc_TypeError, "FileDescriptor has no attribute 'name'");
    return nullptr;
  }
  PyObject* pyfile_pool = PyObject_GetAttrString(pyfile, "pool");
  if (pyfile_pool == nullptr) {
    Py_DECREF(pyfile_name);
    // Ideally this would be raise from.
    PyErr_Format(PyExc_TypeError, "FileDescriptor has no attribute 'pool'");
    return nullptr;
  }
  bool is_from_generated_pool = database->pool() == pyfile_pool;
  Py_DECREF(pyfile_pool);
  const char* pyfile_name_char_ptr = PyUnicode_AsUTF8(pyfile_name);
  if (pyfile_name_char_ptr == nullptr) {
    Py_DECREF(pyfile_name);
    // Ideally this would be raise from.
    PyErr_Format(PyExc_TypeError,
                 "FileDescriptor 'name' PyUnicode_AsUTF8() failure.");
    return nullptr;
  }
  if (!is_from_generated_pool) {
    PyErr_Format(PyExc_TypeError, "%s is not from generated pool",
                 pyfile_name_char_ptr);
    Py_DECREF(pyfile_name);
    return nullptr;
  }
  pool->FindFileByName(pyfile_name_char_ptr);
  Py_DECREF(pyfile_name);

  return pool->FindMessageTypeByName(descritor_full_name);
}

google::protobuf::DynamicMessageFactory* GetFactory() {
  static google::protobuf::DynamicMessageFactory* factory =
      new google::protobuf::DynamicMessageFactory;
  return factory;
}

// C++ API.  Clients get at this via proto_api.h
struct ApiImplementation : google::protobuf::python::PyProto_API {
  google::protobuf::Message* NewCppMessage(PyObject* msg) const override {
    CHECK(!PyErr_Occurred());
    const google::protobuf::Message* message = GetMessagePointer(msg);
    if (message == nullptr) {
      // Clear the errors from GetMessagePointer()
      PyErr_Clear();

      PyObject* pyd = PyObject_GetAttrString(msg, "DESCRIPTOR");
      if (pyd == nullptr) {
        PyErr_Format(PyExc_TypeError, "msg has no attribute 'DESCRIPTOR'");
        return nullptr;
      }

      PyObject* fn = PyObject_GetAttrString(pyd, "full_name");
      if (fn == nullptr) {
        PyErr_Format(PyExc_TypeError,
                     "DESCRIPTOR has no attribute 'full_name'");
        return nullptr;
      }

      const char* descritor_full_name = PyUnicode_AsUTF8(fn);
      if (descritor_full_name == nullptr) {
        PyErr_Format(PyExc_ValueError, "Fail to convert descriptor full name");
        return nullptr;
      }

      PyObject* pyfile = PyObject_GetAttrString(pyd, "file");
      Py_DECREF(pyd);
      if (pyfile == nullptr) {
        PyErr_Format(PyExc_TypeError, "DESCRIPTOR has no attribute 'file'");
        return nullptr;
      }

      const google::protobuf::Descriptor* d =
          FindMessageDescriptor(pyfile, descritor_full_name);
      Py_DECREF(pyfile);
      if (d == nullptr) {
        PyErr_Format(PyExc_ValueError, "Fail to find descriptor %s.",
                     descritor_full_name);
        return nullptr;
      }
      return GetFactory()->GetPrototype(d)->New();
    } else {
      return message->New();
    }
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
