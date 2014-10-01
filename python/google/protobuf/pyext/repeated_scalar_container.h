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

#include <Python.h>

#include <memory>
#ifndef _SHARED_PTR_H
#include <google/protobuf/stubs/shared_ptr.h>
#endif


namespace google {
namespace protobuf {

class Message;

using internal::shared_ptr;

namespace python {

struct CFieldDescriptor;
struct CMessage;

typedef struct RepeatedScalarContainer {
  PyObject_HEAD;

  // This is the top-level C++ Message object that owns the whole
  // proto tree.  Every Python RepeatedScalarContainer holds a
  // reference to it in order to keep it alive as long as there's a
  // Python object that references any part of the tree.
  shared_ptr<Message> owner;

  // Pointer to the C++ Message that contains this container.  The
  // RepeatedScalarContainer does not own this pointer.
  Message* message;

  // Weak reference to a parent CMessage object (i.e. may be NULL.)
  //
  // Used to make sure all ancestors are also mutable when first
  // modifying the container.
  CMessage* parent;

  // Weak reference to the parent's descriptor that describes this
  // field.  Used together with the parent's message when making a
  // default message instance mutable.
  CFieldDescriptor* parent_field;
} RepeatedScalarContainer;

extern PyTypeObject RepeatedScalarContainer_Type;

namespace repeated_scalar_container {

// Appends the scalar 'item' to the end of the container 'self'.
//
// Returns None if successful; returns NULL and sets an exception if
// unsuccessful.
PyObject* Append(RepeatedScalarContainer* self, PyObject* item);

// Releases the messages in the container to a new message.
//
// Returns 0 on success, -1 on failure.
int Release(RepeatedScalarContainer* self);

// Appends all the elements in the input iterator to the container.
//
// Returns None if successful; returns NULL and sets an exception if
// unsuccessful.
PyObject* Extend(RepeatedScalarContainer* self, PyObject* value);

// Set the owner field of self and any children of self.
void SetOwner(RepeatedScalarContainer* self,
              const shared_ptr<Message>& new_owner);

}  // namespace repeated_scalar_container
}  // namespace python
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_REPEATED_SCALAR_CONTAINER_H__
