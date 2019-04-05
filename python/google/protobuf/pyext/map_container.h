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

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_MAP_CONTAINER_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_MAP_CONTAINER_H__

#include <Python.h>

#include <memory>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/pyext/message.h>

namespace google {
namespace protobuf {

class Message;

namespace python {

struct CMessageClass;

// This struct is used directly for ScalarMap, and is the base class of
// MessageMapContainer, which is used for MessageMap.
struct MapContainer : public ContainerBase {
  // Use to get a mutable message when necessary.
  Message* GetMutableMessage();

  // Cache some descriptors, used to convert keys and values.
  const FieldDescriptor* key_field_descriptor;
  const FieldDescriptor* value_field_descriptor;
  // We bump this whenever we perform a mutation, to invalidate existing
  // iterators.
  uint64 version;
};

struct MessageMapContainer : public MapContainer {
  // The type used to create new child messages.
  CMessageClass* message_class;
};

bool InitMapContainers();

extern PyTypeObject* MessageMapContainer_Type;
extern PyTypeObject* ScalarMapContainer_Type;
extern PyTypeObject MapIterator_Type;  // Both map types use the same iterator.

// Builds a MapContainer object, from a parent message and a
// field descriptor.
extern MapContainer* NewScalarMapContainer(
    CMessage* parent, const FieldDescriptor* parent_field_descriptor);

// Builds a MessageMap object, from a parent message and a
// field descriptor.
extern MessageMapContainer* NewMessageMapContainer(
    CMessage* parent, const FieldDescriptor* parent_field_descriptor,
    CMessageClass* message_class);

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_MAP_CONTAINER_H__
