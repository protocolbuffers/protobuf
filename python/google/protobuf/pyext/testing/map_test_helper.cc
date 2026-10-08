// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <Python.h>

#include <cstdint>
#include <vector>

#include "absl/status/statusor.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/proto_api.h"

namespace google {
namespace protobuf {
namespace python {

static const PyProto_API* GetProtoApi() {
  static const PyProto_API* py_proto_api = static_cast<const PyProto_API*>(
      PyCapsule_Import(PyProtoAPICapsuleName(), 0));
  ABSL_CHECK(py_proto_api);
  return py_proto_api;
}

// Recursively traverses all fields using C++ reflection (including repeated
// messages and map repeated fields) to force synchronization and caching of map
// repeated field representations. This is used in tests to verify that mutating
// submessages via retained pointers after a sync correctly invalidates the
// cached repeated field.
static int64_t SumInt32FieldsRecursive(const google::protobuf::Message& msg) {
  int64_t sum = 0;
  const google::protobuf::Reflection* reflection = msg.GetReflection();
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  reflection->ListFields(msg, &fields);

  for (const google::protobuf::FieldDescriptor* field : fields) {
    if (field->is_repeated()) {
      int size = reflection->FieldSize(msg, field);
      if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        for (int i = 0; i < size; ++i) {
          sum += SumInt32FieldsRecursive(
              reflection->GetRepeatedMessage(msg, field, i));
        }
      } else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_INT32) {
        for (int i = 0; i < size; ++i) {
          sum += reflection->GetRepeatedInt32(msg, field, i);
        }
      }
    } else {
      if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        sum += SumInt32FieldsRecursive(reflection->GetMessage(msg, field));
      } else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_INT32) {
        sum += reflection->GetInt32(msg, field);
      }
    }
  }
  return sum;
}

PyObject* TestSumAllInt32FieldsUsingRepeatedFields(PyObject* m,
                                                   PyObject* args) {
  PyObject* py_message;
  if (!PyArg_ParseTuple(args, "O", &py_message)) {
    return nullptr;
  }

  absl::StatusOr<PythonConstMessagePointer> msg_ptr =
      GetProtoApi()->GetConstMessagePointer(py_message);
  if (!msg_ptr.ok()) {
    if (!PyErr_Occurred()) {
      PyErr_SetString(PyExc_ValueError, "Message has been released or is null");
    }
    return nullptr;
  }

  int64_t total = SumInt32FieldsRecursive(msg_ptr->get());
  return PyLong_FromLongLong(total);
}

static PyMethodDef ModuleMethods[] = {
    {"TestSumAllInt32FieldsUsingRepeatedFields",
     TestSumAllInt32FieldsUsingRepeatedFields, METH_VARARGS,
     "Test helper: recursively sum all int32 fields using repeated fields."},
    {nullptr, nullptr}};

static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                     "_map_test_helper",
                                     "Test helper for map tests",
                                     -1,
                                     ModuleMethods,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr};

extern "C" {
PyMODINIT_FUNC PyInit__map_test_helper() {
  PyObject* module = PyModule_Create(&_module);
  if (module == nullptr) {
    return nullptr;
  }
  return module;
}
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
