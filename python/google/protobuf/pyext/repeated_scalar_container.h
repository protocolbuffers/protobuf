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

// Author: anuraag@google.com (Anuraag Agrawal)
// Author: tibell@google.com (Johan Tibell)

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_REPEATED_SCALAR_CONTAINER_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_REPEATED_SCALAR_CONTAINER_H__

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <memory>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/pyext/message.h>

namespace google {
namespace protobuf {
namespace python {

typedef struct RepeatedScalarContainer : public ContainerBase {
} RepeatedScalarContainer;

extern PyTypeObject RepeatedScalarContainer_Type;

namespace repeated_scalar_container {

// Builds a RepeatedScalarContainer object, from a parent message and a
// field descriptor.
extern RepeatedScalarContainer* NewContainer(
    CMessage* parent, const FieldDescriptor* parent_field_descriptor);

// Appends the scalar 'item' to the end of the container 'self'.
//
// Returns None if successful; returns NULL and sets an exception if
// unsuccessful.
PyObject* Append(RepeatedScalarContainer* self, PyObject* item);

// Appends all the elements in the input iterator to the container.
//
// Returns None if successful; returns NULL and sets an exception if
// unsuccessful.
PyObject* Extend(RepeatedScalarContainer* self, PyObject* value);

}  // namespace repeated_scalar_container
}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_REPEATED_SCALAR_CONTAINER_H__
