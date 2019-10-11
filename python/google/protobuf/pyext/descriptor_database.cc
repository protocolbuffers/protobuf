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

// This file defines a C++ DescriptorDatabase, which wraps a Python Database
// and delegate all its operations to Python methods.

#include <google/protobuf/pyext/descriptor_database.h>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

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
  if (py_descriptor == NULL) {
    if (PyErr_ExceptionMatches(PyExc_KeyError)) {
      // Expected error: item was simply not found.
      PyErr_Clear();
    } else {
      GOOGLE_LOG(ERROR) << "DescriptorDatabase method raised an error";
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
        static_cast<FileDescriptorProto*>(message->message);
    *output = *file_proto;
    return true;
  } else {
    // Slow path: serialize the message. This allows to use databases which
    // use a different implementation of FileDescriptorProto.
    ScopedPyObjectPtr serialized_pb(
        PyObject_CallMethod(py_descriptor, "SerializeToString", NULL));
    if (serialized_pb == NULL) {
      GOOGLE_LOG(ERROR)
          << "DescriptorDatabase method did not return a FileDescriptorProto";
      PyErr_Print();
      return false;
    }
    char* str;
    Py_ssize_t len;
    if (PyBytes_AsStringAndSize(serialized_pb.get(), &str, &len) < 0) {
      GOOGLE_LOG(ERROR)
          << "DescriptorDatabase method did not return a FileDescriptorProto";
      PyErr_Print();
      return false;
    }
    FileDescriptorProto file_proto;
    if (!file_proto.ParseFromArray(str, len)) {
      GOOGLE_LOG(ERROR)
          << "DescriptorDatabase method did not return a FileDescriptorProto";
      return false;
    }
    *output = file_proto;
    return true;
  }
}

// Find a file by file name.
bool PyDescriptorDatabase::FindFileByName(const std::string& filename,
                                          FileDescriptorProto* output) {
  ScopedPyObjectPtr py_descriptor(PyObject_CallMethod(
      py_database_, "FindFileByName", "s#", filename.c_str(), filename.size()));
  return GetFileDescriptorProto(py_descriptor.get(), output);
}

// Find the file that declares the given fully-qualified symbol name.
bool PyDescriptorDatabase::FindFileContainingSymbol(
    const std::string& symbol_name, FileDescriptorProto* output) {
  ScopedPyObjectPtr py_descriptor(
      PyObject_CallMethod(py_database_, "FindFileContainingSymbol", "s#",
                          symbol_name.c_str(), symbol_name.size()));
  return GetFileDescriptorProto(py_descriptor.get(), output);
}

// Find the file which defines an extension extending the given message type
// with the given field number.
// Python DescriptorDatabases are not required to implement this method.
bool PyDescriptorDatabase::FindFileContainingExtension(
    const std::string& containing_type, int field_number,
    FileDescriptorProto* output) {
  ScopedPyObjectPtr py_method(
      PyObject_GetAttrString(py_database_, "FindFileContainingExtension"));
  if (py_method == NULL) {
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
    const std::string& containing_type, std::vector<int>* output) {
  ScopedPyObjectPtr py_method(
      PyObject_GetAttrString(py_database_, "FindAllExtensionNumbers"));
  if (py_method == NULL) {
    // This method is not implemented, returns without error.
    PyErr_Clear();
    return false;
  }
  ScopedPyObjectPtr py_list(
      PyObject_CallFunction(py_method.get(), "s#", containing_type.c_str(),
                            containing_type.size()));
  if (py_list == NULL) {
    PyErr_Print();
    return false;
  }
  Py_ssize_t size = PyList_Size(py_list.get());
  int64 item_value;
  for (Py_ssize_t i = 0 ; i < size; ++i) {
    ScopedPyObjectPtr item(PySequence_GetItem(py_list.get(), i));
    item_value = PyLong_AsLong(item.get());
    if (item_value < 0) {
      GOOGLE_LOG(ERROR)
          << "FindAllExtensionNumbers method did not return "
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
