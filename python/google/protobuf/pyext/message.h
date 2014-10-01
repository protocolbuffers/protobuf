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

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_MESSAGE_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_MESSAGE_H__

#include <Python.h>

#include <memory>
#ifndef _SHARED_PTR_H
#include <google/protobuf/stubs/shared_ptr.h>
#endif
#include <string>


namespace google {
namespace protobuf {

class Message;
class Reflection;
class FieldDescriptor;

using internal::shared_ptr;

namespace python {

struct CFieldDescriptor;
struct ExtensionDict;

typedef struct CMessage {
  PyObject_HEAD;

  // This is the top-level C++ Message object that owns the whole
  // proto tree.  Every Python CMessage holds a reference to it in
  // order to keep it alive as long as there's a Python object that
  // references any part of the tree.
  shared_ptr<Message> owner;

  // Weak reference to a parent CMessage object. This is NULL for any top-level
  // message and is set for any child message (i.e. a child submessage or a
  // part of a repeated composite field).
  //
  // Used to make sure all ancestors are also mutable when first modifying
  // a child submessage (in other words, turning a default message instance
  // into a mutable one).
  //
  // If a submessage is released (becomes a new top-level message), this field
  // MUST be set to NULL. The parent may get deallocated and further attempts
  // to use this pointer will result in a crash.
  struct CMessage* parent;

  // Weak reference to the parent's descriptor that describes this submessage.
  // Used together with the parent's message when making a default message
  // instance mutable.
  // TODO(anuraag): With a bit of work on the Python/C++ layer, it should be
  // possible to make this a direct pointer to a C++ FieldDescriptor, this would
  // be easier if this implementation replaces upstream.
  CFieldDescriptor* parent_field;

  // Pointer to the C++ Message object for this CMessage.  The
  // CMessage does not own this pointer.
  Message* message;

  // Indicates this submessage is pointing to a default instance of a message.
  // Submessages are always first created as read only messages and are then
  // made writable, at which point this field is set to false.
  bool read_only;

  // A reference to a Python dictionary containing CMessage,
  // RepeatedCompositeContainer, and RepeatedScalarContainer
  // objects. Used as a cache to make sure we don't have to make a
  // Python wrapper for the C++ Message objects on every access, or
  // deal with the synchronization nightmare that could create.
  PyObject* composite_fields;

  // A reference to the dictionary containing the message's extensions.
  // Similar to composite_fields, acting as a cache, but also contains the
  // required extension dict logic.
  ExtensionDict* extensions;
} CMessage;

extern PyTypeObject CMessage_Type;

namespace cmessage {

// Create a new empty message that can be populated by the parent.
PyObject* NewEmpty(PyObject* type);

// Release a submessage from its proto tree, making it a new top-level messgae.
// A new message will be created if this is a read-only default instance.
//
// Corresponds to reflection api method ReleaseMessage.
int ReleaseSubMessage(google::protobuf::Message* message,
                      const google::protobuf::FieldDescriptor* field_descriptor,
                      CMessage* child_cmessage);

// Initializes a new CMessage instance for a submessage. Only called once per
// submessage as the result is cached in composite_fields.
//
// Corresponds to reflection api method GetMessage.
PyObject* InternalGetSubMessage(CMessage* self,
                                CFieldDescriptor* cfield_descriptor);

// Deletes a range of C++ submessages in a repeated field (following a
// removal in a RepeatedCompositeContainer).
//
// Releases messages to the provided cmessage_list if it is not NULL rather
// than just removing them from the underlying proto. This cmessage_list must
// have a CMessage for each underlying submessage. The CMessages refered to
// by slice will be removed from cmessage_list by this function.
//
// Corresponds to reflection api method RemoveLast.
int InternalDeleteRepeatedField(google::protobuf::Message* message,
                                const google::protobuf::FieldDescriptor* field_descriptor,
                                PyObject* slice, PyObject* cmessage_list);

// Sets the specified scalar value to the message.
int InternalSetScalar(CMessage* self,
                      const google::protobuf::FieldDescriptor* field_descriptor,
                      PyObject* value);

// Retrieves the specified scalar value from the message.
//
// Returns a new python reference.
PyObject* InternalGetScalar(CMessage* self,
                            const google::protobuf::FieldDescriptor* field_descriptor);

// Clears the message, removing all contained data. Extension dictionary and
// submessages are released first if there are remaining external references.
//
// Corresponds to message api method Clear.
PyObject* Clear(CMessage* self);

// Clears the data described by the given descriptor. Used to clear extensions
// (which don't have names). Extension release is handled by ExtensionDict
// class, not this function.
// TODO(anuraag): Try to make this discrepancy in release semantics with
//                ClearField less confusing.
//
// Corresponds to reflection api method ClearField.
PyObject* ClearFieldByDescriptor(
    CMessage* self,
    const google::protobuf::FieldDescriptor* descriptor);

// Clears the data for the given field name. The message is released if there
// are any external references.
//
// Corresponds to reflection api method ClearField.
PyObject* ClearField(CMessage* self, PyObject* arg);

// Checks if the message has the field described by the descriptor. Used for
// extensions (which have no name).
//
// Corresponds to reflection api method HasField
PyObject* HasFieldByDescriptor(
    CMessage* self, const google::protobuf::FieldDescriptor* field_descriptor);

// Checks if the message has the named field.
//
// Corresponds to reflection api method HasField.
PyObject* HasField(CMessage* self, PyObject* arg);

// Initializes constants/enum values on a message. This is called by
// RepeatedCompositeContainer and ExtensionDict after calling the constructor.
// TODO(anuraag): Make it always called from within the constructor since it can
int InitAttributes(CMessage* self, PyObject* descriptor, PyObject* kwargs);

PyObject* MergeFrom(CMessage* self, PyObject* arg);

// Retrieves an attribute named 'name' from CMessage 'self'. Returns
// the attribute value on success, or NULL on failure.
//
// Returns a new reference.
PyObject* GetAttr(CMessage* self, PyObject* name);

// Set the value of the attribute named 'name', for CMessage 'self',
// to the value 'value'. Returns -1 on failure.
int SetAttr(CMessage* self, PyObject* name, PyObject* value);

PyObject* FindInitializationErrors(CMessage* self);

// Set the owner field of self and any children of self, recursively.
// Used when self is being released and thus has a new owner (the
// released Message.)
int SetOwner(CMessage* self, const shared_ptr<Message>& new_owner);

int AssureWritable(CMessage* self);

}  // namespace cmessage

/* Is 64bit */
#define IS_64BIT (SIZEOF_LONG == 8)

#define FIELD_BELONGS_TO_MESSAGE(field_descriptor, message) \
    ((message)->GetDescriptor() == (field_descriptor)->containing_type())

#define FIELD_IS_REPEATED(field_descriptor)                 \
    ((field_descriptor)->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED)

#define GOOGLE_CHECK_GET_INT32(arg, value, err)                        \
    int32 value;                                            \
    if (!CheckAndGetInteger(arg, &value, kint32min_py, kint32max_py)) { \
      return err;                                          \
    }

#define GOOGLE_CHECK_GET_INT64(arg, value, err)                        \
    int64 value;                                            \
    if (!CheckAndGetInteger(arg, &value, kint64min_py, kint64max_py)) { \
      return err;                                          \
    }

#define GOOGLE_CHECK_GET_UINT32(arg, value, err)                       \
    uint32 value;                                           \
    if (!CheckAndGetInteger(arg, &value, kPythonZero, kuint32max_py)) { \
      return err;                                          \
    }

#define GOOGLE_CHECK_GET_UINT64(arg, value, err)                       \
    uint64 value;                                           \
    if (!CheckAndGetInteger(arg, &value, kPythonZero, kuint64max_py)) { \
      return err;                                          \
    }

#define GOOGLE_CHECK_GET_FLOAT(arg, value, err)                        \
    float value;                                            \
    if (!CheckAndGetFloat(arg, &value)) {                   \
      return err;                                          \
    }                                                       \

#define GOOGLE_CHECK_GET_DOUBLE(arg, value, err)                       \
    double value;                                           \
    if (!CheckAndGetDouble(arg, &value)) {                  \
      return err;                                          \
    }

#define GOOGLE_CHECK_GET_BOOL(arg, value, err)                         \
    bool value;                                             \
    if (!CheckAndGetBool(arg, &value)) {                    \
      return err;                                          \
    }


extern PyObject* kPythonZero;
extern PyObject* kint32min_py;
extern PyObject* kint32max_py;
extern PyObject* kuint32max_py;
extern PyObject* kint64min_py;
extern PyObject* kint64max_py;
extern PyObject* kuint64max_py;

#define C(str) const_cast<char*>(str)

void FormatTypeError(PyObject* arg, char* expected_types);
template<class T>
bool CheckAndGetInteger(
    PyObject* arg, T* value, PyObject* min, PyObject* max);
bool CheckAndGetDouble(PyObject* arg, double* value);
bool CheckAndGetFloat(PyObject* arg, float* value);
bool CheckAndGetBool(PyObject* arg, bool* value);
bool CheckAndSetString(
    PyObject* arg, google::protobuf::Message* message,
    const google::protobuf::FieldDescriptor* descriptor,
    const google::protobuf::Reflection* reflection,
    bool append,
    int index);
PyObject* ToStringObject(
    const google::protobuf::FieldDescriptor* descriptor, string value);

extern PyObject* PickleError_class;

}  // namespace python
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_MESSAGE_H__
