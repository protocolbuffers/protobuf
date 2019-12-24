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

// This file can be included by other C++ libraries, typically extension modules
// which want to interact with the Python Messages coming from the "cpp"
// implementation of protocol buffers.
//
// Usage:
// Declare a (probably static) variable to hold the API:
//    const PyProto_API* py_proto_api;
// In some initialization function, write:
//    py_proto_api = static_cast<const PyProto_API*>(PyCapsule_Import(
//        PyProtoAPICapsuleName(), 0));
//    if (!py_proto_api) { ...handle ImportError... }
// Then use the methods of the returned class:
//    py_proto_api->GetMessagePointer(...);

#ifndef GOOGLE_PROTOBUF_PYTHON_PROTO_API_H__
#define GOOGLE_PROTOBUF_PYTHON_PROTO_API_H__

#include <Python.h>

#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/message.h>

namespace google {
namespace protobuf {
namespace python {

// Note on the implementation:
// This API is designed after
// https://docs.python.org/3/extending/extending.html#providing-a-c-api-for-an-extension-module
// The class below contains no mutable state, and all methods are "const";
// we use a C++ class instead of a C struct with functions pointers just because
// the code looks more readable.
struct PyProto_API {
  // The API object is created at initialization time and never freed.
  // This destructor is never called.
  virtual ~PyProto_API() {}

  // Operations on Messages.

  // If the passed object is a Python Message, returns its internal pointer.
  // Otherwise, returns NULL with an exception set.
  virtual const Message* GetMessagePointer(PyObject* msg) const = 0;

  // If the passed object is a Python Message, returns a mutable pointer.
  // Otherwise, returns NULL with an exception set.
  // This function will succeed only if there are no other Python objects
  // pointing to the message, like submessages or repeated containers.
  // With the current implementation, only empty messages are in this case.
  virtual Message* GetMutableMessagePointer(PyObject* msg) const = 0;

  // Expose the underlying DescriptorPool and MessageFactory to enable C++ code
  // to create Python-compatible message.
  virtual const DescriptorPool* GetDefaultDescriptorPool() const = 0;
  virtual MessageFactory* GetDefaultMessageFactory() const = 0;
};

inline const char* PyProtoAPICapsuleName() {
  static const char kCapsuleName[] =
      "google.protobuf.pyext._message.proto_API";
  return kCapsuleName;
}

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYTHON_PROTO_API_H__
