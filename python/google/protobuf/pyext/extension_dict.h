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

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_EXTENSION_DICT_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_EXTENSION_DICT_H__

#include <Python.h>

#include <memory>

#include <google/protobuf/pyext/message.h>

namespace google {
namespace protobuf {

class Message;
class FieldDescriptor;

namespace python {

typedef struct ExtensionDict {
  PyObject_HEAD;

  // This is the top-level C++ Message object that owns the whole
  // proto tree.  Every Python container class holds a
  // reference to it in order to keep it alive as long as there's a
  // Python object that references any part of the tree.
  CMessage::OwnerRef owner;

  // Weak reference to parent message. Used to make sure
  // the parent is writable when an extension field is modified.
  CMessage* parent;

  // Pointer to the C++ Message that this ExtensionDict extends.
  // Not owned by us.
  Message* message;

  // A dict of child messages, indexed by Extension descriptors.
  // Similar to CMessage::composite_fields.
  PyObject* values;
} ExtensionDict;

extern PyTypeObject ExtensionDict_Type;

namespace extension_dict {

// Builds an Extensions dict for a specific message.
ExtensionDict* NewExtensionDict(CMessage *parent);

}  // namespace extension_dict
}  // namespace python
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_EXTENSION_DICT_H__
