// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_REPEATED_COMPOSITE_CONTAINER_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_REPEATED_COMPOSITE_CONTAINER_H__

#include <Python.h>

#include <memory>
#ifndef _SHARED_PTR_H
#include <google/protobuf/stubs/shared_ptr.h>
#endif
#include <string>
#include <vector>


namespace google {
namespace protobuf {

class FieldDescriptor;
class Message;

using internal::shared_ptr;

namespace python {

struct CMessage;
struct CFieldDescriptor;

// A RepeatedCompositeContainer can be in one of two states: attached
// or released.
//
// When in the attached state all modifications to the container are
// done both on the 'message' and on the 'child_messages'
// list.  In this state all Messages refered to by the children in
// 'child_messages' are owner by the 'owner'.
//
// When in the released state 'message', 'owner', 'parent', and
// 'parent_field' are NULL.
typedef struct RepeatedCompositeContainer {
  PyObject_HEAD;

  // This is the top-level C++ Message object that owns the whole
  // proto tree.  Every Python RepeatedCompositeContainer holds a
  // reference to it in order to keep it alive as long as there's a
  // Python object that references any part of the tree.
  shared_ptr<Message> owner;

  // Weak reference to parent object. May be NULL. Used to make sure
  // the parent is writable before modifying the
  // RepeatedCompositeContainer.
  CMessage* parent;

  // A descriptor used to modify the underlying 'message'.
  CFieldDescriptor* parent_field;

  // Pointer to the C++ Message that contains this container.  The
  // RepeatedCompositeContainer does not own this pointer.
  //
  // If NULL, this message has been released from its parent (by
  // calling Clear() or ClearField() on the parent.
  Message* message;

  // A callable that is used to create new child messages.
  PyObject* subclass_init;

  // A list of child messages.
  PyObject* child_messages;
} RepeatedCompositeContainer;

extern PyTypeObject RepeatedCompositeContainer_Type;

namespace repeated_composite_container {

// Returns the number of items in this repeated composite container.
static Py_ssize_t Length(RepeatedCompositeContainer* self);

// Appends a new CMessage to the container and returns it.  The
// CMessage is initialized using the content of kwargs.
//
// Returns a new reference if successful; returns NULL and sets an
// exception if unsuccessful.
PyObject* Add(RepeatedCompositeContainer* self,
              PyObject* args,
              PyObject* kwargs);

// Appends all the CMessages in the input iterator to the container.
//
// Returns None if successful; returns NULL and sets an exception if
// unsuccessful.
PyObject* Extend(RepeatedCompositeContainer* self, PyObject* value);

// Appends a new message to the container for each message in the
// input iterator, merging each data element in. Equivalent to extend.
//
// Returns None if successful; returns NULL and sets an exception if
// unsuccessful.
PyObject* MergeFrom(RepeatedCompositeContainer* self, PyObject* other);

// Accesses messages in the container.
//
// Returns a new reference to the message for an integer parameter.
// Returns a new reference to a list of messages for a slice.
PyObject* Subscript(RepeatedCompositeContainer* self, PyObject* slice);

// Deletes items from the container (cannot be used for assignment).
//
// Returns 0 on success, -1 on failure.
int AssignSubscript(RepeatedCompositeContainer* self,
                    PyObject* slice,
                    PyObject* value);

// Releases the messages in the container to the given message.
//
// Returns 0 on success, -1 on failure.
int ReleaseToMessage(RepeatedCompositeContainer* self,
                     google::protobuf::Message* new_message);

// Releases the messages in the container to a new message.
//
// Returns 0 on success, -1 on failure.
int Release(RepeatedCompositeContainer* self);

// Returns 0 on success, -1 on failure.
int SetOwner(RepeatedCompositeContainer* self,
             const shared_ptr<Message>& new_owner);

// Removes the last element of the repeated message field 'field' on
// the Message 'message', and transfers the ownership of the released
// Message to 'cmessage'.
//
// Corresponds to reflection api method ReleaseMessage.
void ReleaseLastTo(const FieldDescriptor* field,
                   Message* message,
                   CMessage* cmessage);

}  // namespace repeated_composite_container
}  // namespace python
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_REPEATED_COMPOSITE_CONTAINER_H__
