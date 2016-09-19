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

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_MESSAGE_FACTORY_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_MESSAGE_FACTORY_H__

#include <Python.h>

#include <google/protobuf/stubs/hash.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/pyext/descriptor_pool.h>

namespace google {
namespace protobuf {
class MessageFactory;

namespace python {

// The (meta) type of all Messages classes.
struct CMessageClass;

struct PyMessageFactory {
  PyObject_HEAD

  // DynamicMessageFactory used to create C++ instances of messages.
  // This object cache the descriptors that were used, so the DescriptorPool
  // needs to get rid of it before it can delete itself.
  //
  // Note: A C++ MessageFactory is different from the PyMessageFactory.
  // The C++ one creates messages, when the Python one creates classes.
  MessageFactory* message_factory;

  // borrowed reference to a Python DescriptorPool.
  // TODO(amauryfa): invert the dependency: the MessageFactory owns the
  // DescriptorPool, not the opposite.
  PyDescriptorPool* pool;

  // Make our own mapping to retrieve Python classes from C++ descriptors.
  //
  // Descriptor pointers stored here are owned by the DescriptorPool above.
  // Python references to classes are owned by this PyDescriptorPool.
  typedef hash_map<const Descriptor*, CMessageClass*> ClassesByMessageMap;
  ClassesByMessageMap* classes_by_descriptor;
};

extern PyTypeObject PyMessageFactory_Type;

namespace message_factory {

// Creates a new MessageFactory instance.
PyMessageFactory* NewMessageFactory(PyTypeObject* type, PyDescriptorPool* pool);

// Registers a new Python class for the given message descriptor.
// On error, returns -1 with a Python exception set.
int RegisterMessageClass(PyMessageFactory* self,
                         const Descriptor* message_descriptor,
                         CMessageClass* message_class);

// Retrieves the Python class registered with the given message descriptor.
//
// Returns a *borrowed* reference if found, otherwise returns NULL with an
// exception set.
CMessageClass* GetMessageClass(PyMessageFactory* self,
                               const Descriptor* message_descriptor);

}  // namespace message_factory

// Initialize objects used by this module.
// On error, returns false with a Python exception set.
bool InitMessageFactory();

}  // namespace python
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_MESSAGE_FACTORY_H__
