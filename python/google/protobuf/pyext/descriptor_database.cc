// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file defines a C++ DescriptorDatabase, which wraps a Python Database
// and delegate all its operations to Python methods.

#include "google/protobuf/pyext/descriptor_database.h"

#include <Python.h>

#include <cstdint>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/log/absl_log.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/pyext/message.h"
#include "google/protobuf/pyext/scoped_pyobject_ptr.h"

namespace google {
namespace protobuf {
namespace python {

PyDescriptorDatabase::PyDescriptorDatabase(PyObject* py_database)
    : py_database_(py_database) {
  Py_INCREF(py_database_);
}

PyDescriptorDatabase::~PyDescriptorDatabase() { Py_DECREF(py_database_); }

// Convert a Python object to a FileDescriptorProto pointer.
// Handles all kinds of Python errors, which are simply logged.
static bool GetFileDescriptorProto(PyObject* py_descriptor,
                                   FileDescriptorProto* output) {
  if (py_descriptor == nullptr) {
    if (PyErr_ExceptionMatches(PyExc_KeyError)) {
      // Expected error: item was simply not found.
      PyErr_Clear();
    } else {
      ABSL_LOG(ERROR) << "DescriptorDatabase method raised an error";
      PyErr_Print();
    }
    return false;
  }
  if (py_descriptor == Py_None) {
    return false;
  }
  const Descriptor* filedescriptor_descriptor =
      FileDescriptorProto::default_instance().GetDescriptor();
  CMessage* message = reinterpret_cast<CMessage*>(py_descriptor);
  if (PyObject_TypeCheck(py_descriptor, CMessage_Type) &&
      message->message->GetDescriptor() == filedescriptor_descriptor) {
    // Fast path: Just use the pointer.
    FileDescriptorProto* file_proto =
        google::protobuf::DownCastMessage<FileDescriptorProto>(message->message);
    *output = *file_proto;
    return true;
  } else {
    // Slow path: serialize the message. This allows to use databases which
    // use a different implementation of FileDescriptorProto.
    ScopedPyObjectPtr serialized_pb(
        PyObject_CallMethod(py_descriptor, "SerializeToString", nullptr));
    if (serialized_pb == nullptr) {
      ABSL_LOG(ERROR)
          << "DescriptorDatabase method did not return a FileDescriptorProto";
      PyErr_Print();
      return false;
    }
    char* str;
    Py_ssize_t len;
    if (PyBytes_AsStringAndSize(serialized_pb.get(), &str, &len) < 0) {
      ABSL_LOG(ERROR)
          << "DescriptorDatabase method did not return a FileDescriptorProto";
      PyErr_Print();
      return false;
    }
    FileDescriptorProto file_proto;
    if (!file_proto.ParseFromArray(str, len)) {
      ABSL_LOG(ERROR)
          << "DescriptorDatabase method did not return a FileDescriptorProto";
      return false;
    }
    *output = file_proto;
    return true;
  }
}

// Find a file by file name.
bool PyDescriptorDatabase::FindFileByName(StringViewArg filename,
                                          FileDescriptorProto* output) {
  ScopedPyObjectPtr py_descriptor(PyObject_CallMethod(
      py_database_, "FindFileByName", "s#", filename.c_str(), filename.size()));
  return GetFileDescriptorProto(py_descriptor.get(), output);
}

// Find the file that declares the given fully-qualified symbol name.
bool PyDescriptorDatabase::FindFileContainingSymbol(
    StringViewArg symbol_name, FileDescriptorProto* output) {
  ScopedPyObjectPtr py_descriptor(
      PyObject_CallMethod(py_database_, "FindFileContainingSymbol", "s#",
                          symbol_name.c_str(), symbol_name.size()));
  return GetFileDescriptorProto(py_descriptor.get(), output);
}

// Find the file which defines an extension extending the given message type
// with the given field number.
// Python DescriptorDatabases are not required to implement this method.
bool PyDescriptorDatabase::FindFileContainingExtension(
    StringViewArg containing_type, int field_number,
    FileDescriptorProto* output) {
  ScopedPyObjectPtr py_method(
      PyObject_GetAttrString(py_database_, "FindFileContainingExtension"));
  if (py_method == nullptr) {
    // This method is not implemented, returns without error.
    PyErr_Clear();
    return false;
  }
  ScopedPyObjectPtr py_descriptor(
      PyObject_CallFunction(py_method.get(), "s#i", containing_type.c_str(),
                            containing_type.size(), field_number));
  return GetFileDescriptorProto(py_descriptor.get(), output);
}

// Finds the tag numbers used by all known extensions of
// containing_type, and appends them to output in an undefined
// order.
// Python DescriptorDatabases are not required to implement this method.
bool PyDescriptorDatabase::FindAllExtensionNumbers(
    StringViewArg containing_type, std::vector<int>* output) {
  ScopedPyObjectPtr py_method(
      PyObject_GetAttrString(py_database_, "FindAllExtensionNumbers"));
  if (py_method == nullptr) {
    // This method is not implemented, returns without error.
    PyErr_Clear();
    return false;
  }
  ScopedPyObjectPtr py_list(
      PyObject_CallFunction(py_method.get(), "s#", containing_type.c_str(),
                            containing_type.size()));
  if (py_list == nullptr) {
    PyErr_Print();
    return false;
  }
  Py_ssize_t size = PyList_Size(py_list.get());
  if (size == -1) {
    PyErr_Format(
        PyExc_RuntimeError,
        "FindAllExtensionNumbers() on fall back DB must return a list, not %S",
        py_list.get());
    PyErr_Print();
    return false;
  }
  int64_t item_value;
  for (Py_ssize_t i = 0 ; i < size; ++i) {
    ScopedPyObjectPtr item(PySequence_GetItem(py_list.get(), i));
    item_value = PyLong_AsLong(item.get());
    if (item_value < 0) {
      ABSL_LOG(ERROR) << "FindAllExtensionNumbers method did not return "
                      << "valid extension numbers.";
      PyErr_Print();
      return false;
    }
    output->push_back(item_value);
  }
  return true;
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
