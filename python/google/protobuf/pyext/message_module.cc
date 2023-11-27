// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "google/protobuf/message_lite.h"
#include "google/protobuf/pyext/descriptor.h"
#include "google/protobuf/pyext/descriptor_pool.h"
#include "google/protobuf/pyext/message.h"
#include "google/protobuf/pyext/message_factory.h"
#include "google/protobuf/proto_api.h"

namespace {

// C++ API.  Clients get at this via proto_api.h
struct ApiImplementation : google::protobuf::python::PyProto_API {
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
