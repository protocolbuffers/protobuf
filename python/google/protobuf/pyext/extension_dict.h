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
#ifndef _SHARED_PTR_H
#include <google/protobuf/stubs/shared_ptr.h>
#endif


namespace google {
namespace protobuf {

class Message;
class FieldDescriptor;

using internal::shared_ptr;

namespace python {

struct CMessage;
struct CFieldDescriptor;

typedef struct ExtensionDict {
  PyObject_HEAD;
  shared_ptr<Message> owner;
  CMessage* parent;
  Message* message;
  PyObject* values;
} ExtensionDict;

extern PyTypeObject ExtensionDict_Type;

namespace extension_dict {

// Gets the _cdescriptor reference to a CFieldDescriptor object given a
// python descriptor object.
//
// Returns a new reference.
CFieldDescriptor* InternalGetCDescriptorFromExtension(PyObject* extension);

// Gets the number of extension values in this ExtensionDict as a python object.
//
// Returns a new reference.
PyObject* len(ExtensionDict* self);

// Releases extensions referenced outside this dictionary to keep outside
// references alive.
//
// Returns 0 on success, -1 on failure.
int ReleaseExtension(ExtensionDict* self,
                     PyObject* extension,
                     const google::protobuf::FieldDescriptor* descriptor);

// Gets an extension from the dict for the given extension descriptor.
//
// Returns a new reference.
PyObject* subscript(ExtensionDict* self, PyObject* key);

// Assigns a value to an extension in the dict. Can only be used for singular
// simple types.
//
// Returns 0 on success, -1 on failure.
int ass_subscript(ExtensionDict* self, PyObject* key, PyObject* value);

// Clears an extension from the dict. Will release the extension if there
// is still an external reference left to it.
//
// Returns None on success.
PyObject* ClearExtension(ExtensionDict* self,
                                       PyObject* extension);

// Checks if the dict has an extension.
//
// Returns a new python boolean reference.
PyObject* HasExtension(ExtensionDict* self, PyObject* extension);

// Gets an extension from the dict given the extension name as opposed to
// descriptor.
//
// Returns a new reference.
PyObject* _FindExtensionByName(ExtensionDict* self, PyObject* name);

}  // namespace extension_dict
}  // namespace python
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_EXTENSION_DICT_H__
