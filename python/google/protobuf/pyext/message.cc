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

#include <google/protobuf/pyext/message.h>

#include <memory>
#ifndef _SHARED_PTR_H
#include <google/protobuf/stubs/shared_ptr.h>
#endif
#include <string>
#include <vector>
#include <structmember.h>  // A Python header file.

#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(type, size) PyObject_HEAD_INIT(type) size,
#endif
#ifndef Py_TYPE
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/descriptor_pool.h>
#include <google/protobuf/pyext/extension_dict.h>
#include <google/protobuf/pyext/repeated_composite_container.h>
#include <google/protobuf/pyext/repeated_scalar_container.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>
#include <google/protobuf/stubs/strutil.h>

#if PY_MAJOR_VERSION >= 3
  #define PyInt_Check PyLong_Check
  #define PyInt_AsLong PyLong_AsLong
  #define PyInt_FromLong PyLong_FromLong
  #define PyInt_FromSize_t PyLong_FromSize_t
  #define PyString_Check PyUnicode_Check
  #define PyString_FromString PyUnicode_FromString
  #define PyString_FromStringAndSize PyUnicode_FromStringAndSize
  #if PY_VERSION_HEX < 0x03030000
    #error "Python 3.0 - 3.2 are not supported."
  #else
  #define PyString_AsString(ob) \
    (PyUnicode_Check(ob)? PyUnicode_AsUTF8(ob): PyBytes_AsString(ob))
  #define PyString_AsStringAndSize(ob, charpp, sizep) \
    (PyUnicode_Check(ob)? \
       ((*(charpp) = PyUnicode_AsUTF8AndSize(ob, (sizep))) == NULL? -1: 0): \
       PyBytes_AsStringAndSize(ob, (charpp), (sizep)))
  #endif
#endif

namespace google {
namespace protobuf {
namespace python {

// Forward declarations
namespace cmessage {
static const FieldDescriptor* GetFieldDescriptor(
    CMessage* self, PyObject* name);
static const Descriptor* GetMessageDescriptor(PyTypeObject* cls);
static string GetMessageName(CMessage* self);
int InternalReleaseFieldByDescriptor(
    const FieldDescriptor* field_descriptor,
    PyObject* composite_field,
    Message* parent_message);
}  // namespace cmessage

// ---------------------------------------------------------------------
// Visiting the composite children of a CMessage

struct ChildVisitor {
  // Returns 0 on success, -1 on failure.
  int VisitRepeatedCompositeContainer(RepeatedCompositeContainer* container) {
    return 0;
  }

  // Returns 0 on success, -1 on failure.
  int VisitRepeatedScalarContainer(RepeatedScalarContainer* container) {
    return 0;
  }

  // Returns 0 on success, -1 on failure.
  int VisitCMessage(CMessage* cmessage,
                    const FieldDescriptor* field_descriptor) {
    return 0;
  }
};

// Apply a function to a composite field.  Does nothing if child is of
// non-composite type.
template<class Visitor>
static int VisitCompositeField(const FieldDescriptor* descriptor,
                               PyObject* child,
                               Visitor visitor) {
  if (descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
    if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      RepeatedCompositeContainer* container =
        reinterpret_cast<RepeatedCompositeContainer*>(child);
      if (visitor.VisitRepeatedCompositeContainer(container) == -1)
        return -1;
    } else {
      RepeatedScalarContainer* container =
        reinterpret_cast<RepeatedScalarContainer*>(child);
      if (visitor.VisitRepeatedScalarContainer(container) == -1)
        return -1;
    }
  } else if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    CMessage* cmsg = reinterpret_cast<CMessage*>(child);
    if (visitor.VisitCMessage(cmsg, descriptor) == -1)
      return -1;
  }
  // The ExtensionDict might contain non-composite fields, which we
  // skip here.
  return 0;
}

// Visit each composite field and extension field of this CMessage.
// Returns -1 on error and 0 on success.
template<class Visitor>
int ForEachCompositeField(CMessage* self, Visitor visitor) {
  Py_ssize_t pos = 0;
  PyObject* key;
  PyObject* field;

  // Visit normal fields.
  if (self->composite_fields) {
    // Never use self->message in this function, it may be already freed.
    const Descriptor* message_descriptor =
        cmessage::GetMessageDescriptor(Py_TYPE(self));
    while (PyDict_Next(self->composite_fields, &pos, &key, &field)) {
      Py_ssize_t key_str_size;
      char *key_str_data;
      if (PyString_AsStringAndSize(key, &key_str_data, &key_str_size) != 0)
        return -1;
      const string key_str(key_str_data, key_str_size);
      const FieldDescriptor* descriptor =
        message_descriptor->FindFieldByName(key_str);
      if (descriptor != NULL) {
        if (VisitCompositeField(descriptor, field, visitor) == -1)
          return -1;
      }
    }
  }

  // Visit extension fields.
  if (self->extensions != NULL) {
    pos = 0;
    while (PyDict_Next(self->extensions->values, &pos, &key, &field)) {
      const FieldDescriptor* descriptor = cmessage::GetExtensionDescriptor(key);
      if (descriptor == NULL)
        return -1;
      if (VisitCompositeField(descriptor, field, visitor) == -1)
        return -1;
    }
  }

  return 0;
}

// ---------------------------------------------------------------------

static DynamicMessageFactory* message_factory;

// Constants used for integer type range checking.
PyObject* kPythonZero;
PyObject* kint32min_py;
PyObject* kint32max_py;
PyObject* kuint32max_py;
PyObject* kint64min_py;
PyObject* kint64max_py;
PyObject* kuint64max_py;

PyObject* EnumTypeWrapper_class;
PyObject* EncodeError_class;
PyObject* DecodeError_class;
PyObject* PickleError_class;

// Constant PyString values used for GetAttr/GetItem.
static PyObject* kDESCRIPTOR;
static PyObject* k_cdescriptor;
static PyObject* kfull_name;
static PyObject* k_extensions_by_name;
static PyObject* k_extensions_by_number;

/* Is 64bit */
void FormatTypeError(PyObject* arg, char* expected_types) {
  PyObject* repr = PyObject_Repr(arg);
  if (repr) {
    PyErr_Format(PyExc_TypeError,
                 "%.100s has type %.100s, but expected one of: %s",
                 PyString_AsString(repr),
                 Py_TYPE(arg)->tp_name,
                 expected_types);
    Py_DECREF(repr);
  }
}

template<class T>
bool CheckAndGetInteger(
    PyObject* arg, T* value, PyObject* min, PyObject* max) {
  bool is_long = PyLong_Check(arg);
#if PY_MAJOR_VERSION < 3
  if (!PyInt_Check(arg) && !is_long) {
    FormatTypeError(arg, "int, long");
    return false;
  }
  if (PyObject_Compare(min, arg) > 0 || PyObject_Compare(max, arg) < 0) {
#else
  if (!is_long) {
    FormatTypeError(arg, "int");
    return false;
  }
  if (PyObject_RichCompareBool(min, arg, Py_LE) != 1 ||
      PyObject_RichCompareBool(max, arg, Py_GE) != 1) {
#endif
    PyObject *s = PyObject_Str(arg);
    if (s) {
      PyErr_Format(PyExc_ValueError,
                   "Value out of range: %s",
                   PyString_AsString(s));
      Py_DECREF(s);
    }
    return false;
  }
#if PY_MAJOR_VERSION < 3
  if (!is_long) {
    *value = static_cast<T>(PyInt_AsLong(arg));
  } else  // NOLINT
#endif
  {
    if (min == kPythonZero) {
      *value = static_cast<T>(PyLong_AsUnsignedLongLong(arg));
    } else {
      *value = static_cast<T>(PyLong_AsLongLong(arg));
    }
  }
  return true;
}

// These are referenced by repeated_scalar_container, and must
// be explicitly instantiated.
template bool CheckAndGetInteger<int32>(
    PyObject*, int32*, PyObject*, PyObject*);
template bool CheckAndGetInteger<int64>(
    PyObject*, int64*, PyObject*, PyObject*);
template bool CheckAndGetInteger<uint32>(
    PyObject*, uint32*, PyObject*, PyObject*);
template bool CheckAndGetInteger<uint64>(
    PyObject*, uint64*, PyObject*, PyObject*);

bool CheckAndGetDouble(PyObject* arg, double* value) {
  if (!PyInt_Check(arg) && !PyLong_Check(arg) &&
      !PyFloat_Check(arg)) {
    FormatTypeError(arg, "int, long, float");
    return false;
  }
  *value = PyFloat_AsDouble(arg);
  return true;
}

bool CheckAndGetFloat(PyObject* arg, float* value) {
  double double_value;
  if (!CheckAndGetDouble(arg, &double_value)) {
    return false;
  }
  *value = static_cast<float>(double_value);
  return true;
}

bool CheckAndGetBool(PyObject* arg, bool* value) {
  if (!PyInt_Check(arg) && !PyBool_Check(arg) && !PyLong_Check(arg)) {
    FormatTypeError(arg, "int, long, bool");
    return false;
  }
  *value = static_cast<bool>(PyInt_AsLong(arg));
  return true;
}

bool CheckAndSetString(
    PyObject* arg, Message* message,
    const FieldDescriptor* descriptor,
    const Reflection* reflection,
    bool append,
    int index) {
  GOOGLE_DCHECK(descriptor->type() == FieldDescriptor::TYPE_STRING ||
         descriptor->type() == FieldDescriptor::TYPE_BYTES);
  if (descriptor->type() == FieldDescriptor::TYPE_STRING) {
    if (!PyBytes_Check(arg) && !PyUnicode_Check(arg)) {
      FormatTypeError(arg, "bytes, unicode");
      return false;
    }

    if (PyBytes_Check(arg)) {
      PyObject* unicode = PyUnicode_FromEncodedObject(arg, "utf-8", NULL);
      if (unicode == NULL) {
        PyObject* repr = PyObject_Repr(arg);
        PyErr_Format(PyExc_ValueError,
                     "%s has type str, but isn't valid UTF-8 "
                     "encoding. Non-UTF-8 strings must be converted to "
                     "unicode objects before being added.",
                     PyString_AsString(repr));
        Py_DECREF(repr);
        return false;
      } else {
        Py_DECREF(unicode);
      }
    }
  } else if (!PyBytes_Check(arg)) {
    FormatTypeError(arg, "bytes");
    return false;
  }

  PyObject* encoded_string = NULL;
  if (descriptor->type() == FieldDescriptor::TYPE_STRING) {
    if (PyBytes_Check(arg)) {
      // The bytes were already validated as correctly encoded UTF-8 above.
      encoded_string = arg;  // Already encoded.
      Py_INCREF(encoded_string);
    } else {
      encoded_string = PyUnicode_AsEncodedObject(arg, "utf-8", NULL);
    }
  } else {
    // In this case field type is "bytes".
    encoded_string = arg;
    Py_INCREF(encoded_string);
  }

  if (encoded_string == NULL) {
    return false;
  }

  char* value;
  Py_ssize_t value_len;
  if (PyBytes_AsStringAndSize(encoded_string, &value, &value_len) < 0) {
    Py_DECREF(encoded_string);
    return false;
  }

  string value_string(value, value_len);
  if (append) {
    reflection->AddString(message, descriptor, value_string);
  } else if (index < 0) {
    reflection->SetString(message, descriptor, value_string);
  } else {
    reflection->SetRepeatedString(message, descriptor, index, value_string);
  }
  Py_DECREF(encoded_string);
  return true;
}

PyObject* ToStringObject(const FieldDescriptor* descriptor, string value) {
  if (descriptor->type() != FieldDescriptor::TYPE_STRING) {
    return PyBytes_FromStringAndSize(value.c_str(), value.length());
  }

  PyObject* result = PyUnicode_DecodeUTF8(value.c_str(), value.length(), NULL);
  // If the string can't be decoded in UTF-8, just return a string object that
  // contains the raw bytes. This can't happen if the value was assigned using
  // the members of the Python message object, but can happen if the values were
  // parsed from the wire (binary).
  if (result == NULL) {
    PyErr_Clear();
    result = PyBytes_FromStringAndSize(value.c_str(), value.length());
  }
  return result;
}

bool CheckFieldBelongsToMessage(const FieldDescriptor* field_descriptor,
                                const Message* message) {
  if (message->GetDescriptor() == field_descriptor->containing_type()) {
    return true;
  }
  PyErr_Format(PyExc_KeyError, "Field '%s' does not belong to message '%s'",
               field_descriptor->full_name().c_str(),
               message->GetDescriptor()->full_name().c_str());
  return false;
}

namespace cmessage {

DynamicMessageFactory* GetMessageFactory() {
  return message_factory;
}

static int MaybeReleaseOverlappingOneofField(
    CMessage* cmessage,
    const FieldDescriptor* field) {
#ifdef GOOGLE_PROTOBUF_HAS_ONEOF
  Message* message = cmessage->message;
  const Reflection* reflection = message->GetReflection();
  if (!field->containing_oneof() ||
      !reflection->HasOneof(*message, field->containing_oneof()) ||
      reflection->HasField(*message, field)) {
    // No other field in this oneof, no need to release.
    return 0;
  }

  const OneofDescriptor* oneof = field->containing_oneof();
  const FieldDescriptor* existing_field =
      reflection->GetOneofFieldDescriptor(*message, oneof);
  if (existing_field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    // Non-message fields don't need to be released.
    return 0;
  }
  const char* field_name = existing_field->name().c_str();
  PyObject* child_message = cmessage->composite_fields ?
      PyDict_GetItemString(cmessage->composite_fields, field_name) : NULL;
  if (child_message == NULL) {
    // No python reference to this field so no need to release.
    return 0;
  }

  if (InternalReleaseFieldByDescriptor(
          existing_field, child_message, message) < 0) {
    return -1;
  }
  return PyDict_DelItemString(cmessage->composite_fields, field_name);
#else
  return 0;
#endif
}

// ---------------------------------------------------------------------
// Making a message writable

static Message* GetMutableMessage(
    CMessage* parent,
    const FieldDescriptor* parent_field) {
  Message* parent_message = parent->message;
  const Reflection* reflection = parent_message->GetReflection();
  if (MaybeReleaseOverlappingOneofField(parent, parent_field) < 0) {
    return NULL;
  }
  return reflection->MutableMessage(
      parent_message, parent_field, message_factory);
}

struct FixupMessageReference : public ChildVisitor {
  // message must outlive this object.
  explicit FixupMessageReference(Message* message) :
      message_(message) {}

  int VisitRepeatedCompositeContainer(RepeatedCompositeContainer* container) {
    container->message = message_;
    return 0;
  }

  int VisitRepeatedScalarContainer(RepeatedScalarContainer* container) {
    container->message = message_;
    return 0;
  }

 private:
  Message* message_;
};

int AssureWritable(CMessage* self) {
  if (self == NULL || !self->read_only) {
    return 0;
  }

  if (self->parent == NULL) {
    // If parent is NULL but we are trying to modify a read-only message, this
    // is a reference to a constant default instance that needs to be replaced
    // with a mutable top-level message.
    const Message* prototype = message_factory->GetPrototype(
        self->message->GetDescriptor());
    self->message = prototype->New();
    self->owner.reset(self->message);
  } else {
    // Otherwise, we need a mutable child message.
    if (AssureWritable(self->parent) == -1)
      return -1;

    // Make self->message writable.
    Message* parent_message = self->parent->message;
    Message* mutable_message = GetMutableMessage(
        self->parent,
        self->parent_field_descriptor);
    if (mutable_message == NULL) {
      return -1;
    }
    self->message = mutable_message;
  }
  self->read_only = false;

  // When a CMessage is made writable its Message pointer is updated
  // to point to a new mutable Message.  When that happens we need to
  // update any references to the old, read-only CMessage.  There are
  // three places such references occur: RepeatedScalarContainer,
  // RepeatedCompositeContainer, and ExtensionDict.
  if (self->extensions != NULL)
    self->extensions->message = self->message;
  if (ForEachCompositeField(self, FixupMessageReference(self->message)) == -1)
    return -1;

  return 0;
}

// --- Globals:

// Retrieve the C++ Descriptor of a message class.
// On error, returns NULL with an exception set.
static const Descriptor* GetMessageDescriptor(PyTypeObject* cls) {
  ScopedPyObjectPtr descriptor(PyObject_GetAttr(
      reinterpret_cast<PyObject*>(cls), kDESCRIPTOR));
  if (descriptor == NULL) {
    PyErr_SetString(PyExc_TypeError, "Message class has no DESCRIPTOR");
    return NULL;
  }
  if (!PyObject_TypeCheck(descriptor, &PyMessageDescriptor_Type)) {
    PyErr_Format(PyExc_TypeError, "Expected a message Descriptor, got %s",
                 descriptor->ob_type->tp_name);
    return NULL;
  }
  return PyMessageDescriptor_AsDescriptor(descriptor);
}

// Retrieve a C++ FieldDescriptor for a message attribute.
// The C++ message must be valid.
// TODO(amauryfa): This function should stay internal, because exception
// handling is not consistent.
static const FieldDescriptor* GetFieldDescriptor(
    CMessage* self, PyObject* name) {
  const Descriptor *message_descriptor = self->message->GetDescriptor();
  char* field_name;
  Py_ssize_t size;
  if (PyString_AsStringAndSize(name, &field_name, &size) < 0) {
    return NULL;
  }
  const FieldDescriptor *field_descriptor =
      message_descriptor->FindFieldByName(string(field_name, size));
  if (field_descriptor == NULL) {
    // Note: No exception is set!
    return NULL;
  }
  return field_descriptor;
}

// Retrieve a C++ FieldDescriptor for an extension handle.
const FieldDescriptor* GetExtensionDescriptor(PyObject* extension) {
  ScopedPyObjectPtr cdescriptor;
  if (!PyObject_TypeCheck(extension, &PyFieldDescriptor_Type)) {
    // Most callers consider extensions as a plain dictionary.  We should
    // allow input which is not a field descriptor, and simply pretend it does
    // not exist.
    PyErr_SetObject(PyExc_KeyError, extension);
    return NULL;
  }
  return PyFieldDescriptor_AsDescriptor(extension);
}

// If cmessage_list is not NULL, this function releases values into the
// container CMessages instead of just removing. Repeated composite container
// needs to do this to make sure CMessages stay alive if they're still
// referenced after deletion. Repeated scalar container doesn't need to worry.
int InternalDeleteRepeatedField(
    Message* message,
    const FieldDescriptor* field_descriptor,
    PyObject* slice,
    PyObject* cmessage_list) {
  Py_ssize_t length, from, to, step, slice_length;
  const Reflection* reflection = message->GetReflection();
  int min, max;
  length = reflection->FieldSize(*message, field_descriptor);

  if (PyInt_Check(slice) || PyLong_Check(slice)) {
    from = to = PyLong_AsLong(slice);
    if (from < 0) {
      from = to = length + from;
    }
    step = 1;
    min = max = from;

    // Range check.
    if (from < 0 || from >= length) {
      PyErr_Format(PyExc_IndexError, "list assignment index out of range");
      return -1;
    }
  } else if (PySlice_Check(slice)) {
    from = to = step = slice_length = 0;
    PySlice_GetIndicesEx(
#if PY_MAJOR_VERSION < 3
        reinterpret_cast<PySliceObject*>(slice),
#else
        slice,
#endif
        length, &from, &to, &step, &slice_length);
    if (from < to) {
      min = from;
      max = to - 1;
    } else {
      min = to + 1;
      max = from;
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "list indices must be integers");
    return -1;
  }

  Py_ssize_t i = from;
  std::vector<bool> to_delete(length, false);
  while (i >= min && i <= max) {
    to_delete[i] = true;
    i += step;
  }

  to = 0;
  for (i = 0; i < length; ++i) {
    if (!to_delete[i]) {
      if (i != to) {
        reflection->SwapElements(message, field_descriptor, i, to);
        if (cmessage_list != NULL) {
          // If a list of cmessages is passed in (i.e. from a repeated
          // composite container), swap those as well to correspond to the
          // swaps in the underlying message so they're in the right order
          // when we start releasing.
          PyObject* tmp = PyList_GET_ITEM(cmessage_list, i);
          PyList_SET_ITEM(cmessage_list, i,
                          PyList_GET_ITEM(cmessage_list, to));
          PyList_SET_ITEM(cmessage_list, to, tmp);
        }
      }
      ++to;
    }
  }

  while (i > to) {
    if (cmessage_list == NULL) {
      reflection->RemoveLast(message, field_descriptor);
    } else {
      CMessage* last_cmessage = reinterpret_cast<CMessage*>(
          PyList_GET_ITEM(cmessage_list, PyList_GET_SIZE(cmessage_list) - 1));
      repeated_composite_container::ReleaseLastTo(
          field_descriptor, message, last_cmessage);
      if (PySequence_DelItem(cmessage_list, -1) < 0) {
        return -1;
      }
    }
    --i;
  }

  return 0;
}

// Initializes fields of a message. Used in constructors.
int InitAttributes(CMessage* self, PyObject* kwargs) {
  if (kwargs == NULL) {
    return 0;
  }

  Py_ssize_t pos = 0;
  PyObject* name;
  PyObject* value;
  while (PyDict_Next(kwargs, &pos, &name, &value)) {
    if (!PyString_Check(name)) {
      PyErr_SetString(PyExc_ValueError, "Field name must be a string");
      return -1;
    }
    const FieldDescriptor* descriptor = GetFieldDescriptor(self, name);
    if (descriptor == NULL) {
      PyErr_Format(PyExc_ValueError, "Protocol message has no \"%s\" field.",
                   PyString_AsString(name));
      return -1;
    }
    if (descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
      ScopedPyObjectPtr container(GetAttr(self, name));
      if (container == NULL) {
        return -1;
      }
      if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        if (repeated_composite_container::Extend(
                reinterpret_cast<RepeatedCompositeContainer*>(container.get()),
                value)
            == NULL) {
          return -1;
        }
      } else {
        if (repeated_scalar_container::Extend(
                reinterpret_cast<RepeatedScalarContainer*>(container.get()),
                value) ==
            NULL) {
          return -1;
        }
      }
    } else if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      ScopedPyObjectPtr message(GetAttr(self, name));
      if (message == NULL) {
        return -1;
      }
      if (MergeFrom(reinterpret_cast<CMessage*>(message.get()),
                             value) == NULL) {
        return -1;
      }
    } else {
      if (SetAttr(self, name, value) < 0) {
        return -1;
      }
    }
  }
  return 0;
}

// Allocates an incomplete Python Message: the caller must fill self->message,
// self->owner and eventually self->parent.
CMessage* NewEmptyMessage(PyObject* type, const Descriptor *descriptor) {
  CMessage* self = reinterpret_cast<CMessage*>(
      PyType_GenericAlloc(reinterpret_cast<PyTypeObject*>(type), 0));
  if (self == NULL) {
    return NULL;
  }

  self->message = NULL;
  self->parent = NULL;
  self->parent_field_descriptor = NULL;
  self->read_only = false;
  self->extensions = NULL;

  self->composite_fields = NULL;

  // If there are extension_ranges, the message is "extendable". Allocate a
  // dictionary to store the extension fields.
  if (descriptor->extension_range_count() > 0) {
    // TODO(amauryfa): Delay the construction of this dict until extensions are
    // really used on the object.
    ExtensionDict* extension_dict = extension_dict::NewExtensionDict(self);
    if (extension_dict == NULL) {
      return NULL;
    }
    self->extensions = extension_dict;
  }

  return self;
}

// The __new__ method of Message classes.
// Creates a new C++ message and takes ownership.
static PyObject* New(PyTypeObject* type,
                     PyObject* unused_args, PyObject* unused_kwargs) {
  // Retrieve the message descriptor and the default instance (=prototype).
  const Descriptor* message_descriptor = GetMessageDescriptor(type);
  if (message_descriptor == NULL) {
    return NULL;
  }
  const Message* default_message =
      message_factory->GetPrototype(message_descriptor);
  if (default_message == NULL) {
    PyErr_SetString(PyExc_TypeError, message_descriptor->full_name().c_str());
    return NULL;
  }

  CMessage* self = NewEmptyMessage(reinterpret_cast<PyObject*>(type),
                                   message_descriptor);
  if (self == NULL) {
    return NULL;
  }
  self->message = default_message->New();
  self->owner.reset(self->message);

  return reinterpret_cast<PyObject*>(self);
}

// The __init__ method of Message classes.
// It initializes fields from keywords passed to the constructor.
static int Init(CMessage* self, PyObject* args, PyObject* kwargs) {
  if (PyTuple_Size(args) != 0) {
    PyErr_SetString(PyExc_TypeError, "No positional arguments allowed");
    return -1;
  }

  return InitAttributes(self, kwargs);
}

// ---------------------------------------------------------------------
// Deallocating a CMessage
//
// Deallocating a CMessage requires that we clear any weak references
// from children to the message being deallocated.

// Clear the weak reference from the child to the parent.
struct ClearWeakReferences : public ChildVisitor {
  int VisitRepeatedCompositeContainer(RepeatedCompositeContainer* container) {
    container->parent = NULL;
    // The elements in the container have the same parent as the
    // container itself, so NULL out that pointer as well.
    const Py_ssize_t n = PyList_GET_SIZE(container->child_messages);
    for (Py_ssize_t i = 0; i < n; ++i) {
      CMessage* child_cmessage = reinterpret_cast<CMessage*>(
          PyList_GET_ITEM(container->child_messages, i));
      child_cmessage->parent = NULL;
    }
    return 0;
  }

  int VisitRepeatedScalarContainer(RepeatedScalarContainer* container) {
    container->parent = NULL;
    return 0;
  }

  int VisitCMessage(CMessage* cmessage,
                    const FieldDescriptor* field_descriptor) {
    cmessage->parent = NULL;
    return 0;
  }
};

static void Dealloc(CMessage* self) {
  // Null out all weak references from children to this message.
  GOOGLE_CHECK_EQ(0, ForEachCompositeField(self, ClearWeakReferences()));

  Py_CLEAR(self->extensions);
  Py_CLEAR(self->composite_fields);
  self->owner.reset();
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

// ---------------------------------------------------------------------


PyObject* IsInitialized(CMessage* self, PyObject* args) {
  PyObject* errors = NULL;
  if (PyArg_ParseTuple(args, "|O", &errors) < 0) {
    return NULL;
  }
  if (self->message->IsInitialized()) {
    Py_RETURN_TRUE;
  }
  if (errors != NULL) {
    ScopedPyObjectPtr initialization_errors(
        FindInitializationErrors(self));
    if (initialization_errors == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr extend_name(PyString_FromString("extend"));
    if (extend_name == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr result(PyObject_CallMethodObjArgs(
        errors,
        extend_name.get(),
        initialization_errors.get(),
        NULL));
    if (result == NULL) {
      return NULL;
    }
  }
  Py_RETURN_FALSE;
}

PyObject* HasFieldByDescriptor(
    CMessage* self, const FieldDescriptor* field_descriptor) {
  Message* message = self->message;
  if (!CheckFieldBelongsToMessage(field_descriptor, message)) {
    return NULL;
  }
  if (field_descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
    PyErr_SetString(PyExc_KeyError,
                    "Field is repeated. A singular method is required.");
    return NULL;
  }
  bool has_field =
      message->GetReflection()->HasField(*message, field_descriptor);
  return PyBool_FromLong(has_field ? 1 : 0);
}

const FieldDescriptor* FindFieldWithOneofs(
    const Message* message, const string& field_name, bool* in_oneof) {
  *in_oneof = false;
  const Descriptor* descriptor = message->GetDescriptor();
  const FieldDescriptor* field_descriptor =
      descriptor->FindFieldByName(field_name);
  if (field_descriptor != NULL) {
    return field_descriptor;
  }
  const OneofDescriptor* oneof_desc =
      descriptor->FindOneofByName(field_name);
  if (oneof_desc != NULL) {
    *in_oneof = true;
    return message->GetReflection()->GetOneofFieldDescriptor(*message,
                                                             oneof_desc);
  }
  return NULL;
}

bool CheckHasPresence(const FieldDescriptor* field_descriptor, bool in_oneof) {
  if (field_descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
    PyErr_Format(PyExc_ValueError,
                 "Protocol message has no singular \"%s\" field.",
                 field_descriptor->name().c_str());
    return false;
  }

  if (field_descriptor->file()->syntax() == FileDescriptor::SYNTAX_PROTO3) {
    // HasField() for a oneof *itself* isn't supported.
    if (in_oneof) {
      PyErr_Format(PyExc_ValueError,
                   "Can't test oneof field \"%s\" for presence in proto3, use "
                   "WhichOneof instead.",
                   field_descriptor->containing_oneof()->name().c_str());
      return false;
    }

    // ...but HasField() for fields *in* a oneof is supported.
    if (field_descriptor->containing_oneof() != NULL) {
      return true;
    }

    if (field_descriptor->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
      PyErr_Format(
          PyExc_ValueError,
          "Can't test non-submessage field \"%s\" for presence in proto3.",
          field_descriptor->name().c_str());
      return false;
    }
  }

  return true;
}

PyObject* HasField(CMessage* self, PyObject* arg) {
  char* field_name;
  Py_ssize_t size;
#if PY_MAJOR_VERSION < 3
  if (PyString_AsStringAndSize(arg, &field_name, &size) < 0) {
    return NULL;
  }
#else
  field_name = PyUnicode_AsUTF8AndSize(arg, &size);
  if (!field_name) {
    return NULL;
  }
#endif

  Message* message = self->message;
  bool is_in_oneof;
  const FieldDescriptor* field_descriptor =
      FindFieldWithOneofs(message, string(field_name, size), &is_in_oneof);
  if (field_descriptor == NULL) {
    if (!is_in_oneof) {
      PyErr_Format(PyExc_ValueError, "Unknown field %s.", field_name);
      return NULL;
    } else {
      Py_RETURN_FALSE;
    }
  }

  if (!CheckHasPresence(field_descriptor, is_in_oneof)) {
    return NULL;
  }

  if (message->GetReflection()->HasField(*message, field_descriptor)) {
    Py_RETURN_TRUE;
  }
  if (!message->GetReflection()->SupportsUnknownEnumValues() &&
      field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
    // Special case: Python HasField() differs in semantics from C++
    // slightly: we return HasField('enum_field') == true if there is
    // an unknown enum value present. To implement this we have to
    // look in the UnknownFieldSet.
    const UnknownFieldSet& unknown_field_set =
        message->GetReflection()->GetUnknownFields(*message);
    for (int i = 0; i < unknown_field_set.field_count(); ++i) {
      if (unknown_field_set.field(i).number() == field_descriptor->number()) {
        Py_RETURN_TRUE;
      }
    }
  }
  Py_RETURN_FALSE;
}

PyObject* ClearExtension(CMessage* self, PyObject* arg) {
  if (self->extensions != NULL) {
    return extension_dict::ClearExtension(self->extensions, arg);
  }
  PyErr_SetString(PyExc_TypeError, "Message is not extendable");
  return NULL;
}

PyObject* HasExtension(CMessage* self, PyObject* arg) {
  if (self->extensions != NULL) {
    return extension_dict::HasExtension(self->extensions, arg);
  }
  PyErr_SetString(PyExc_TypeError, "Message is not extendable");
  return NULL;
}

// ---------------------------------------------------------------------
// Releasing messages
//
// The Python API's ClearField() and Clear() methods behave
// differently than their C++ counterparts.  While the C++ versions
// clears the children the Python versions detaches the children,
// without touching their content.  This impedance mismatch causes
// some complexity in the implementation, which is captured in this
// section.
//
// When a CMessage field is cleared we need to:
//
// * Release the Message used as the backing store for the CMessage
//   from its parent.
//
// * Change the owner field of the released CMessage and all of its
//   children to point to the newly released Message.
//
// * Clear the weak references from the released CMessage to the
//   parent.
//
// When a RepeatedCompositeContainer field is cleared we need to:
//
// * Release all the Message used as the backing store for the
//   CMessages stored in the container.
//
// * Change the owner field of all the released CMessage and all of
//   their children to point to the newly released Messages.
//
// * Clear the weak references from the released container to the
//   parent.

struct SetOwnerVisitor : public ChildVisitor {
  // new_owner must outlive this object.
  explicit SetOwnerVisitor(const shared_ptr<Message>& new_owner)
      : new_owner_(new_owner) {}

  int VisitRepeatedCompositeContainer(RepeatedCompositeContainer* container) {
    repeated_composite_container::SetOwner(container, new_owner_);
    return 0;
  }

  int VisitRepeatedScalarContainer(RepeatedScalarContainer* container) {
    repeated_scalar_container::SetOwner(container, new_owner_);
    return 0;
  }

  int VisitCMessage(CMessage* cmessage,
                    const FieldDescriptor* field_descriptor) {
    return SetOwner(cmessage, new_owner_);
  }

 private:
  const shared_ptr<Message>& new_owner_;
};

// Change the owner of this CMessage and all its children, recursively.
int SetOwner(CMessage* self, const shared_ptr<Message>& new_owner) {
  self->owner = new_owner;
  if (ForEachCompositeField(self, SetOwnerVisitor(new_owner)) == -1)
    return -1;
  return 0;
}

// Releases the message specified by 'field' and returns the
// pointer. If the field does not exist a new message is created using
// 'descriptor'. The caller takes ownership of the returned pointer.
Message* ReleaseMessage(Message* message,
                        const Descriptor* descriptor,
                        const FieldDescriptor* field_descriptor) {
  Message* released_message = message->GetReflection()->ReleaseMessage(
      message, field_descriptor, message_factory);
  // ReleaseMessage will return NULL which differs from
  // child_cmessage->message, if the field does not exist.  In this case,
  // the latter points to the default instance via a const_cast<>, so we
  // have to reset it to a new mutable object since we are taking ownership.
  if (released_message == NULL) {
    const Message* prototype = message_factory->GetPrototype(descriptor);
    GOOGLE_DCHECK(prototype != NULL);
    released_message = prototype->New();
  }

  return released_message;
}

int ReleaseSubMessage(Message* message,
                      const FieldDescriptor* field_descriptor,
                      CMessage* child_cmessage) {
  // Release the Message
  shared_ptr<Message> released_message(ReleaseMessage(
      message, child_cmessage->message->GetDescriptor(), field_descriptor));
  child_cmessage->message = released_message.get();
  child_cmessage->owner.swap(released_message);
  child_cmessage->parent = NULL;
  child_cmessage->parent_field_descriptor = NULL;
  child_cmessage->read_only = false;
  return ForEachCompositeField(child_cmessage,
                               SetOwnerVisitor(child_cmessage->owner));
}

struct ReleaseChild : public ChildVisitor {
  // message must outlive this object.
  explicit ReleaseChild(Message* parent_message) :
      parent_message_(parent_message) {}

  int VisitRepeatedCompositeContainer(RepeatedCompositeContainer* container) {
    return repeated_composite_container::Release(
        reinterpret_cast<RepeatedCompositeContainer*>(container));
  }

  int VisitRepeatedScalarContainer(RepeatedScalarContainer* container) {
    return repeated_scalar_container::Release(
        reinterpret_cast<RepeatedScalarContainer*>(container));
  }

  int VisitCMessage(CMessage* cmessage,
                    const FieldDescriptor* field_descriptor) {
    return ReleaseSubMessage(parent_message_, field_descriptor,
        reinterpret_cast<CMessage*>(cmessage));
  }

  Message* parent_message_;
};

int InternalReleaseFieldByDescriptor(
    const FieldDescriptor* field_descriptor,
    PyObject* composite_field,
    Message* parent_message) {
  return VisitCompositeField(
      field_descriptor,
      composite_field,
      ReleaseChild(parent_message));
}

PyObject* ClearFieldByDescriptor(
    CMessage* self,
    const FieldDescriptor* descriptor) {
  if (!CheckFieldBelongsToMessage(descriptor, self->message)) {
    return NULL;
  }
  AssureWritable(self);
  self->message->GetReflection()->ClearField(self->message, descriptor);
  Py_RETURN_NONE;
}

PyObject* ClearField(CMessage* self, PyObject* arg) {
  if (!PyString_Check(arg)) {
    PyErr_SetString(PyExc_TypeError, "field name must be a string");
    return NULL;
  }
#if PY_MAJOR_VERSION < 3
  const char* field_name = PyString_AS_STRING(arg);
  Py_ssize_t size = PyString_GET_SIZE(arg);
#else
  Py_ssize_t size;
  const char* field_name = PyUnicode_AsUTF8AndSize(arg, &size);
#endif
  AssureWritable(self);
  Message* message = self->message;
  ScopedPyObjectPtr arg_in_oneof;
  bool is_in_oneof;
  const FieldDescriptor* field_descriptor =
      FindFieldWithOneofs(message, string(field_name, size), &is_in_oneof);
  if (field_descriptor == NULL) {
    if (!is_in_oneof) {
      PyErr_Format(PyExc_ValueError,
                   "Protocol message has no \"%s\" field.", field_name);
      return NULL;
    } else {
      Py_RETURN_NONE;
    }
  } else if (is_in_oneof) {
    const string& name = field_descriptor->name();
    arg_in_oneof.reset(PyString_FromStringAndSize(name.c_str(), name.size()));
    arg = arg_in_oneof.get();
  }

  PyObject* composite_field = self->composite_fields ?
      PyDict_GetItem(self->composite_fields, arg) : NULL;

  // Only release the field if there's a possibility that there are
  // references to it.
  if (composite_field != NULL) {
    if (InternalReleaseFieldByDescriptor(field_descriptor,
                                         composite_field, message) < 0) {
      return NULL;
    }
    PyDict_DelItem(self->composite_fields, arg);
  }
  message->GetReflection()->ClearField(message, field_descriptor);
  if (field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_ENUM &&
      !message->GetReflection()->SupportsUnknownEnumValues()) {
    UnknownFieldSet* unknown_field_set =
        message->GetReflection()->MutableUnknownFields(message);
    unknown_field_set->DeleteByNumber(field_descriptor->number());
  }

  Py_RETURN_NONE;
}

PyObject* Clear(CMessage* self) {
  AssureWritable(self);
  if (ForEachCompositeField(self, ReleaseChild(self->message)) == -1)
    return NULL;

  // The old ExtensionDict still aliases this CMessage, but all its
  // fields have been released.
  if (self->extensions != NULL) {
    Py_CLEAR(self->extensions);
    ExtensionDict* extension_dict = extension_dict::NewExtensionDict(self);
    if (extension_dict == NULL) {
      return NULL;
    }
    self->extensions = extension_dict;
  }
  if (self->composite_fields) {
    PyDict_Clear(self->composite_fields);
  }
  self->message->Clear();
  Py_RETURN_NONE;
}

// ---------------------------------------------------------------------

static string GetMessageName(CMessage* self) {
  if (self->parent_field_descriptor != NULL) {
    return self->parent_field_descriptor->full_name();
  } else {
    return self->message->GetDescriptor()->full_name();
  }
}

static PyObject* SerializeToString(CMessage* self, PyObject* args) {
  if (!self->message->IsInitialized()) {
    ScopedPyObjectPtr errors(FindInitializationErrors(self));
    if (errors == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr comma(PyString_FromString(","));
    if (comma == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr joined(
        PyObject_CallMethod(comma.get(), "join", "O", errors.get()));
    if (joined == NULL) {
      return NULL;
    }

    // TODO(haberman): this is a (hopefully temporary) hack.  The unit testing
    // infrastructure reloads all pure-Python modules for every test, but not
    // C++ modules (because that's generally impossible:
    // http://bugs.python.org/issue1144263).  But if we cache EncodeError, we'll
    // return the EncodeError from a previous load of the module, which won't
    // match a user's attempt to catch EncodeError.  So we have to look it up
    // again every time.
    ScopedPyObjectPtr message_module(PyImport_ImportModule(
        "google.protobuf.message"));
    if (message_module.get() == NULL) {
      return NULL;
    }

    ScopedPyObjectPtr encode_error(
        PyObject_GetAttrString(message_module, "EncodeError"));
    if (encode_error.get() == NULL) {
      return NULL;
    }
    PyErr_Format(encode_error.get(),
                 "Message %s is missing required fields: %s",
                 GetMessageName(self).c_str(), PyString_AsString(joined));
    return NULL;
  }
  int size = self->message->ByteSize();
  if (size <= 0) {
    return PyBytes_FromString("");
  }
  PyObject* result = PyBytes_FromStringAndSize(NULL, size);
  if (result == NULL) {
    return NULL;
  }
  char* buffer = PyBytes_AS_STRING(result);
  self->message->SerializeWithCachedSizesToArray(
      reinterpret_cast<uint8*>(buffer));
  return result;
}

static PyObject* SerializePartialToString(CMessage* self) {
  string contents;
  self->message->SerializePartialToString(&contents);
  return PyBytes_FromStringAndSize(contents.c_str(), contents.size());
}

// Formats proto fields for ascii dumps using python formatting functions where
// appropriate.
class PythonFieldValuePrinter : public TextFormat::FieldValuePrinter {
 public:
  PythonFieldValuePrinter() : float_holder_(PyFloat_FromDouble(0)) {}

  // Python has some differences from C++ when printing floating point numbers.
  //
  // 1) Trailing .0 is always printed.
  // 2) Outputted is rounded to 12 digits.
  //
  // We override floating point printing with the C-API function for printing
  // Python floats to ensure consistency.
  string PrintFloat(float value) const { return PrintDouble(value); }
  string PrintDouble(double value) const {
    reinterpret_cast<PyFloatObject*>(float_holder_.get())->ob_fval = value;
    ScopedPyObjectPtr s(PyObject_Str(float_holder_.get()));
    if (s == NULL) return string();
#if PY_MAJOR_VERSION < 3
    char *cstr = PyBytes_AS_STRING(static_cast<PyObject*>(s));
#else
    char *cstr = PyUnicode_AsUTF8(s);
#endif
    return string(cstr);
  }

 private:
  // Holder for a python float object which we use to allow us to use
  // the Python API for printing doubles. We initialize once and then
  // directly modify it for every float printed to save on allocations
  // and refcounting.
  ScopedPyObjectPtr float_holder_;
};

static PyObject* ToStr(CMessage* self) {
  TextFormat::Printer printer;
  // Passes ownership
  printer.SetDefaultFieldValuePrinter(new PythonFieldValuePrinter());
  printer.SetHideUnknownFields(true);
  string output;
  if (!printer.PrintToString(*self->message, &output)) {
    PyErr_SetString(PyExc_ValueError, "Unable to convert message to str");
    return NULL;
  }
  return PyString_FromString(output.c_str());
}

PyObject* MergeFrom(CMessage* self, PyObject* arg) {
  CMessage* other_message;
  if (!PyObject_TypeCheck(reinterpret_cast<PyObject *>(arg), &CMessage_Type)) {
    PyErr_SetString(PyExc_TypeError, "Must be a message");
    return NULL;
  }

  other_message = reinterpret_cast<CMessage*>(arg);
  if (other_message->message->GetDescriptor() !=
      self->message->GetDescriptor()) {
    PyErr_Format(PyExc_TypeError,
                 "Tried to merge from a message with a different type. "
                 "to: %s, from: %s",
                 self->message->GetDescriptor()->full_name().c_str(),
                 other_message->message->GetDescriptor()->full_name().c_str());
    return NULL;
  }
  AssureWritable(self);

  // TODO(tibell): Message::MergeFrom might turn some child Messages
  // into mutable messages, invalidating the message field in the
  // corresponding CMessages.  We should run a FixupMessageReferences
  // pass here.

  self->message->MergeFrom(*other_message->message);
  Py_RETURN_NONE;
}

static PyObject* CopyFrom(CMessage* self, PyObject* arg) {
  CMessage* other_message;
  if (!PyObject_TypeCheck(reinterpret_cast<PyObject *>(arg), &CMessage_Type)) {
    PyErr_SetString(PyExc_TypeError, "Must be a message");
    return NULL;
  }

  other_message = reinterpret_cast<CMessage*>(arg);

  if (self == other_message) {
    Py_RETURN_NONE;
  }

  if (other_message->message->GetDescriptor() !=
      self->message->GetDescriptor()) {
    PyErr_Format(PyExc_TypeError,
                 "Tried to copy from a message with a different type. "
                 "to: %s, from: %s",
                 self->message->GetDescriptor()->full_name().c_str(),
                 other_message->message->GetDescriptor()->full_name().c_str());
    return NULL;
  }

  AssureWritable(self);

  // CopyFrom on the message will not clean up self->composite_fields,
  // which can leave us in an inconsistent state, so clear it out here.
  Clear(self);

  self->message->CopyFrom(*other_message->message);

  Py_RETURN_NONE;
}

static PyObject* MergeFromString(CMessage* self, PyObject* arg) {
  const void* data;
  Py_ssize_t data_length;
  if (PyObject_AsReadBuffer(arg, &data, &data_length) < 0) {
    return NULL;
  }

  AssureWritable(self);
  io::CodedInputStream input(
      reinterpret_cast<const uint8*>(data), data_length);
  input.SetExtensionRegistry(GetDescriptorPool()->pool, message_factory);
  bool success = self->message->MergePartialFromCodedStream(&input);
  if (success) {
    return PyInt_FromLong(input.CurrentPosition());
  } else {
    PyErr_Format(DecodeError_class, "Error parsing message");
    return NULL;
  }
}

static PyObject* ParseFromString(CMessage* self, PyObject* arg) {
  if (Clear(self) == NULL) {
    return NULL;
  }
  return MergeFromString(self, arg);
}

static PyObject* ByteSize(CMessage* self, PyObject* args) {
  return PyLong_FromLong(self->message->ByteSize());
}

static PyObject* RegisterExtension(PyObject* cls,
                                   PyObject* extension_handle) {
  ScopedPyObjectPtr message_descriptor(PyObject_GetAttr(cls, kDESCRIPTOR));
  if (message_descriptor == NULL) {
    return NULL;
  }

  const FieldDescriptor* descriptor =
      GetExtensionDescriptor(extension_handle);
  if (descriptor == NULL) {
    return NULL;
  }
  const Descriptor* cmessage_descriptor = GetMessageDescriptor(
      reinterpret_cast<PyTypeObject*>(cls));

  if (cmessage_descriptor != descriptor->containing_type()) {
    if (PyObject_SetAttrString(extension_handle, "containing_type",
                               message_descriptor) < 0) {
      return NULL;
    }
  }

  ScopedPyObjectPtr extensions_by_name(
      PyObject_GetAttr(cls, k_extensions_by_name));
  if (extensions_by_name == NULL) {
    PyErr_SetString(PyExc_TypeError, "no extensions_by_name on class");
    return NULL;
  }
  ScopedPyObjectPtr full_name(PyObject_GetAttr(extension_handle, kfull_name));
  if (full_name == NULL) {
    return NULL;
  }

  // If the extension was already registered, check that it is the same.
  PyObject* existing_extension = PyDict_GetItem(extensions_by_name, full_name);
  if (existing_extension != NULL) {
    const FieldDescriptor* existing_extension_descriptor =
        GetExtensionDescriptor(existing_extension);
    if (existing_extension_descriptor != descriptor) {
      PyErr_SetString(PyExc_ValueError, "Double registration of Extensions");
      return NULL;
    }
    // Nothing else to do.
    Py_RETURN_NONE;
  }

  if (PyDict_SetItem(extensions_by_name, full_name, extension_handle) < 0) {
    return NULL;
  }

  // Also store a mapping from extension number to implementing class.
  ScopedPyObjectPtr extensions_by_number(
      PyObject_GetAttr(cls, k_extensions_by_number));
  if (extensions_by_number == NULL) {
    PyErr_SetString(PyExc_TypeError, "no extensions_by_number on class");
    return NULL;
  }
  ScopedPyObjectPtr number(PyObject_GetAttrString(extension_handle, "number"));
  if (number == NULL) {
    return NULL;
  }
  if (PyDict_SetItem(extensions_by_number, number, extension_handle) < 0) {
    return NULL;
  }

  // Check if it's a message set
  if (descriptor->is_extension() &&
      descriptor->containing_type()->options().message_set_wire_format() &&
      descriptor->type() == FieldDescriptor::TYPE_MESSAGE &&
      descriptor->message_type() == descriptor->extension_scope() &&
      descriptor->label() == FieldDescriptor::LABEL_OPTIONAL) {
    ScopedPyObjectPtr message_name(PyString_FromStringAndSize(
        descriptor->message_type()->full_name().c_str(),
        descriptor->message_type()->full_name().size()));
    if (message_name == NULL) {
      return NULL;
    }
    PyDict_SetItem(extensions_by_name, message_name, extension_handle);
  }

  Py_RETURN_NONE;
}

static PyObject* SetInParent(CMessage* self, PyObject* args) {
  AssureWritable(self);
  Py_RETURN_NONE;
}

static PyObject* WhichOneof(CMessage* self, PyObject* arg) {
  Py_ssize_t name_size;
  char *name_data;
  if (PyString_AsStringAndSize(arg, &name_data, &name_size) < 0)
    return NULL;
  string oneof_name = string(name_data, name_size);
  const OneofDescriptor* oneof_desc =
      self->message->GetDescriptor()->FindOneofByName(oneof_name);
  if (oneof_desc == NULL) {
    PyErr_Format(PyExc_ValueError,
                 "Protocol message has no oneof \"%s\" field.",
                 oneof_name.c_str());
    return NULL;
  }
  const FieldDescriptor* field_in_oneof =
      self->message->GetReflection()->GetOneofFieldDescriptor(
          *self->message, oneof_desc);
  if (field_in_oneof == NULL) {
    Py_RETURN_NONE;
  } else {
    const string& name = field_in_oneof->name();
    return PyString_FromStringAndSize(name.c_str(), name.size());
  }
}

static PyObject* ListFields(CMessage* self) {
  vector<const FieldDescriptor*> fields;
  self->message->GetReflection()->ListFields(*self->message, &fields);

  // Normally, the list will be exactly the size of the fields.
  ScopedPyObjectPtr all_fields(PyList_New(fields.size()));
  if (all_fields == NULL) {
    return NULL;
  }

  // When there are unknown extensions, the py list will *not* contain
  // the field information.  Thus the actual size of the py list will be
  // smaller than the size of fields.  Set the actual size at the end.
  Py_ssize_t actual_size = 0;
  for (Py_ssize_t i = 0; i < fields.size(); ++i) {
    ScopedPyObjectPtr t(PyTuple_New(2));
    if (t == NULL) {
      return NULL;
    }

    if (fields[i]->is_extension()) {
      ScopedPyObjectPtr extension_field(PyFieldDescriptor_New(fields[i]));
      if (extension_field == NULL) {
        return NULL;
      }
      // With C++ descriptors, the field can always be retrieved, but for
      // unknown extensions which have not been imported in Python code, there
      // is no message class and we cannot retrieve the value.
      // TODO(amauryfa): consider building the class on the fly!
      if (fields[i]->message_type() != NULL &&
          cdescriptor_pool::GetMessageClass(
              GetDescriptorPool(), fields[i]->message_type()) == NULL) {
        PyErr_Clear();
        continue;
      }
      PyObject* extensions = reinterpret_cast<PyObject*>(self->extensions);
      if (extensions == NULL) {
        return NULL;
      }
      // 'extension' reference later stolen by PyTuple_SET_ITEM.
      PyObject* extension = PyObject_GetItem(extensions, extension_field);
      if (extension == NULL) {
        return NULL;
      }
      PyTuple_SET_ITEM(t.get(), 0, extension_field.release());
      // Steals reference to 'extension'
      PyTuple_SET_ITEM(t.get(), 1, extension);
    } else {
      const string& field_name = fields[i]->name();
      ScopedPyObjectPtr py_field_name(PyString_FromStringAndSize(
          field_name.c_str(), field_name.length()));
      if (py_field_name == NULL) {
        PyErr_SetString(PyExc_ValueError, "bad string");
        return NULL;
      }
      ScopedPyObjectPtr field_descriptor(PyFieldDescriptor_New(fields[i]));
      if (field_descriptor == NULL) {
        return NULL;
      }

      PyObject* field_value = GetAttr(self, py_field_name);
      if (field_value == NULL) {
        PyErr_SetObject(PyExc_ValueError, py_field_name);
        return NULL;
      }
      PyTuple_SET_ITEM(t.get(), 0, field_descriptor.release());
      PyTuple_SET_ITEM(t.get(), 1, field_value);
    }
    PyList_SET_ITEM(all_fields.get(), actual_size, t.release());
    ++actual_size;
  }
  Py_SIZE(all_fields.get()) = actual_size;
  return all_fields.release();
}

PyObject* FindInitializationErrors(CMessage* self) {
  Message* message = self->message;
  vector<string> errors;
  message->FindInitializationErrors(&errors);

  PyObject* error_list = PyList_New(errors.size());
  if (error_list == NULL) {
    return NULL;
  }
  for (Py_ssize_t i = 0; i < errors.size(); ++i) {
    const string& error = errors[i];
    PyObject* error_string = PyString_FromStringAndSize(
        error.c_str(), error.length());
    if (error_string == NULL) {
      Py_DECREF(error_list);
      return NULL;
    }
    PyList_SET_ITEM(error_list, i, error_string);
  }
  return error_list;
}

static PyObject* RichCompare(CMessage* self, PyObject* other, int opid) {
  if (!PyObject_TypeCheck(other, &CMessage_Type)) {
    if (opid == Py_EQ) {
      Py_RETURN_FALSE;
    } else if (opid == Py_NE) {
      Py_RETURN_TRUE;
    }
  }
  if (opid == Py_EQ || opid == Py_NE) {
    ScopedPyObjectPtr self_fields(ListFields(self));
    if (!self_fields) {
      return NULL;
    }
    ScopedPyObjectPtr other_fields(ListFields(
        reinterpret_cast<CMessage*>(other)));
    if (!other_fields) {
      return NULL;
    }
    return PyObject_RichCompare(self_fields, other_fields, opid);
  } else {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
}

PyObject* InternalGetScalar(
    CMessage* self,
    const FieldDescriptor* field_descriptor) {
  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();

  if (!CheckFieldBelongsToMessage(field_descriptor, message)) {
    return NULL;
  }

  PyObject* result = NULL;
  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: {
      int32 value = reflection->GetInt32(*message, field_descriptor);
      result = PyInt_FromLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      int64 value = reflection->GetInt64(*message, field_descriptor);
      result = PyLong_FromLongLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT32: {
      uint32 value = reflection->GetUInt32(*message, field_descriptor);
      result = PyInt_FromSize_t(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT64: {
      uint64 value = reflection->GetUInt64(*message, field_descriptor);
      result = PyLong_FromUnsignedLongLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      float value = reflection->GetFloat(*message, field_descriptor);
      result = PyFloat_FromDouble(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      double value = reflection->GetDouble(*message, field_descriptor);
      result = PyFloat_FromDouble(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_BOOL: {
      bool value = reflection->GetBool(*message, field_descriptor);
      result = PyBool_FromLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      string value = reflection->GetString(*message, field_descriptor);
      result = ToStringObject(field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      if (!message->GetReflection()->SupportsUnknownEnumValues() &&
          !message->GetReflection()->HasField(*message, field_descriptor)) {
        // Look for the value in the unknown fields.
        UnknownFieldSet* unknown_field_set =
            message->GetReflection()->MutableUnknownFields(message);
        for (int i = 0; i < unknown_field_set->field_count(); ++i) {
          if (unknown_field_set->field(i).number() ==
              field_descriptor->number()) {
            result = PyInt_FromLong(unknown_field_set->field(i).varint());
            break;
          }
        }
      }

      if (result == NULL) {
        const EnumValueDescriptor* enum_value =
            message->GetReflection()->GetEnum(*message, field_descriptor);
        result = PyInt_FromLong(enum_value->number());
      }
      break;
    }
    default:
      PyErr_Format(
          PyExc_SystemError, "Getting a value from a field of unknown type %d",
          field_descriptor->cpp_type());
  }

  return result;
}

PyObject* InternalGetSubMessage(
    CMessage* self, const FieldDescriptor* field_descriptor) {
  const Reflection* reflection = self->message->GetReflection();
  const Message& sub_message = reflection->GetMessage(
      *self->message, field_descriptor, message_factory);

  PyObject *message_class = cdescriptor_pool::GetMessageClass(
      GetDescriptorPool(), field_descriptor->message_type());
  if (message_class == NULL) {
    return NULL;
  }

  CMessage* cmsg = cmessage::NewEmptyMessage(message_class,
                                             sub_message.GetDescriptor());
  if (cmsg == NULL) {
    return NULL;
  }

  cmsg->owner = self->owner;
  cmsg->parent = self;
  cmsg->parent_field_descriptor = field_descriptor;
  cmsg->read_only = !reflection->HasField(*self->message, field_descriptor);
  cmsg->message = const_cast<Message*>(&sub_message);

  return reinterpret_cast<PyObject*>(cmsg);
}

int InternalSetScalar(
    CMessage* self,
    const FieldDescriptor* field_descriptor,
    PyObject* arg) {
  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();

  if (!CheckFieldBelongsToMessage(field_descriptor, message)) {
    return -1;
  }

  if (MaybeReleaseOverlappingOneofField(self, field_descriptor) < 0) {
    return -1;
  }

  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: {
      GOOGLE_CHECK_GET_INT32(arg, value, -1);
      reflection->SetInt32(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      GOOGLE_CHECK_GET_INT64(arg, value, -1);
      reflection->SetInt64(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT32: {
      GOOGLE_CHECK_GET_UINT32(arg, value, -1);
      reflection->SetUInt32(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT64: {
      GOOGLE_CHECK_GET_UINT64(arg, value, -1);
      reflection->SetUInt64(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      GOOGLE_CHECK_GET_FLOAT(arg, value, -1);
      reflection->SetFloat(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      GOOGLE_CHECK_GET_DOUBLE(arg, value, -1);
      reflection->SetDouble(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_BOOL: {
      GOOGLE_CHECK_GET_BOOL(arg, value, -1);
      reflection->SetBool(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      if (!CheckAndSetString(
          arg, message, field_descriptor, reflection, false, -1)) {
        return -1;
      }
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      GOOGLE_CHECK_GET_INT32(arg, value, -1);
      if (reflection->SupportsUnknownEnumValues()) {
        reflection->SetEnumValue(message, field_descriptor, value);
      } else {
        const EnumDescriptor* enum_descriptor = field_descriptor->enum_type();
        const EnumValueDescriptor* enum_value =
            enum_descriptor->FindValueByNumber(value);
        if (enum_value != NULL) {
          reflection->SetEnum(message, field_descriptor, enum_value);
        } else {
          PyErr_Format(PyExc_ValueError, "Unknown enum value: %d", value);
          return -1;
        }
      }
      break;
    }
    default:
      PyErr_Format(
          PyExc_SystemError, "Setting value to a field of unknown type %d",
          field_descriptor->cpp_type());
      return -1;
  }

  return 0;
}

PyObject* FromString(PyTypeObject* cls, PyObject* serialized) {
  PyObject* py_cmsg = PyObject_CallObject(
      reinterpret_cast<PyObject*>(cls), NULL);
  if (py_cmsg == NULL) {
    return NULL;
  }
  CMessage* cmsg = reinterpret_cast<CMessage*>(py_cmsg);

  ScopedPyObjectPtr py_length(MergeFromString(cmsg, serialized));
  if (py_length == NULL) {
    Py_DECREF(py_cmsg);
    return NULL;
  }

  return py_cmsg;
}

// Add the number of a field descriptor to the containing message class.
// Equivalent to:
//   _cls.<field>_FIELD_NUMBER = <number>
static bool AddFieldNumberToClass(
    PyObject* cls, const FieldDescriptor* field_descriptor) {
  string constant_name = field_descriptor->name() + "_FIELD_NUMBER";
  UpperString(&constant_name);
  ScopedPyObjectPtr attr_name(PyString_FromStringAndSize(
      constant_name.c_str(), constant_name.size()));
  if (attr_name == NULL) {
    return false;
  }
  ScopedPyObjectPtr number(PyInt_FromLong(field_descriptor->number()));
  if (number == NULL) {
    return false;
  }
  if (PyObject_SetAttr(cls, attr_name, number) == -1) {
    return false;
  }
  return true;
}


// Finalize the creation of the Message class.
// Called from its metaclass: GeneratedProtocolMessageType.__init__().
static PyObject* AddDescriptors(PyObject* cls, PyObject* descriptor) {
  const Descriptor* message_descriptor =
      cdescriptor_pool::RegisterMessageClass(
          GetDescriptorPool(), cls, descriptor);
  if (message_descriptor == NULL) {
    return NULL;
  }

  // If there are extension_ranges, the message is "extendable", and extension
  // classes will register themselves in this class.
  if (message_descriptor->extension_range_count() > 0) {
    ScopedPyObjectPtr by_name(PyDict_New());
    if (PyObject_SetAttr(cls, k_extensions_by_name, by_name) < 0) {
      return NULL;
    }
    ScopedPyObjectPtr by_number(PyDict_New());
    if (PyObject_SetAttr(cls, k_extensions_by_number, by_number) < 0) {
      return NULL;
    }
  }

  // For each field set: cls.<field>_FIELD_NUMBER = <number>
  for (int i = 0; i < message_descriptor->field_count(); ++i) {
    if (!AddFieldNumberToClass(cls, message_descriptor->field(i))) {
      return NULL;
    }
  }

  // For each enum set cls.<enum name> = EnumTypeWrapper(<enum descriptor>).
  //
  // The enum descriptor we get from
  // <messagedescriptor>.enum_types_by_name[name]
  // which was built previously.
  for (int i = 0; i < message_descriptor->enum_type_count(); ++i) {
    const EnumDescriptor* enum_descriptor = message_descriptor->enum_type(i);
    ScopedPyObjectPtr enum_type(PyEnumDescriptor_New(enum_descriptor));
    if (enum_type == NULL) {
      return NULL;
     }
    // Add wrapped enum type to message class.
    ScopedPyObjectPtr wrapped(PyObject_CallFunctionObjArgs(
        EnumTypeWrapper_class, enum_type.get(), NULL));
    if (wrapped == NULL) {
      return NULL;
    }
    if (PyObject_SetAttrString(
            cls, enum_descriptor->name().c_str(), wrapped) == -1) {
      return NULL;
    }

    // For each enum value add cls.<name> = <number>
    for (int j = 0; j < enum_descriptor->value_count(); ++j) {
      const EnumValueDescriptor* enum_value_descriptor =
          enum_descriptor->value(j);
      ScopedPyObjectPtr value_number(PyInt_FromLong(
          enum_value_descriptor->number()));
      if (value_number == NULL) {
        return NULL;
      }
      if (PyObject_SetAttrString(
              cls, enum_value_descriptor->name().c_str(), value_number) == -1) {
        return NULL;
      }
    }
  }

  // For each extension set cls.<extension name> = <extension descriptor>.
  //
  // Extension descriptors come from
  // <message descriptor>.extensions_by_name[name]
  // which was defined previously.
  for (int i = 0; i < message_descriptor->extension_count(); ++i) {
    const google::protobuf::FieldDescriptor* field = message_descriptor->extension(i);
    ScopedPyObjectPtr extension_field(PyFieldDescriptor_New(field));
    if (extension_field == NULL) {
      return NULL;
    }

    // Add the extension field to the message class.
    if (PyObject_SetAttrString(
            cls, field->name().c_str(), extension_field) == -1) {
      return NULL;
    }

    // For each extension set cls.<extension name>_FIELD_NUMBER = <number>.
    if (!AddFieldNumberToClass(cls, field)) {
      return NULL;
    }
  }

  Py_RETURN_NONE;
}

PyObject* DeepCopy(CMessage* self, PyObject* arg) {
  PyObject* clone = PyObject_CallObject(
      reinterpret_cast<PyObject*>(Py_TYPE(self)), NULL);
  if (clone == NULL) {
    return NULL;
  }
  if (!PyObject_TypeCheck(clone, &CMessage_Type)) {
    Py_DECREF(clone);
    return NULL;
  }
  if (MergeFrom(reinterpret_cast<CMessage*>(clone),
                reinterpret_cast<PyObject*>(self)) == NULL) {
    Py_DECREF(clone);
    return NULL;
  }
  return clone;
}

PyObject* ToUnicode(CMessage* self) {
  // Lazy import to prevent circular dependencies
  ScopedPyObjectPtr text_format(
      PyImport_ImportModule("google.protobuf.text_format"));
  if (text_format == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr method_name(PyString_FromString("MessageToString"));
  if (method_name == NULL) {
    return NULL;
  }
  Py_INCREF(Py_True);
  ScopedPyObjectPtr encoded(PyObject_CallMethodObjArgs(text_format, method_name,
                                                       self, Py_True, NULL));
  Py_DECREF(Py_True);
  if (encoded == NULL) {
    return NULL;
  }
#if PY_MAJOR_VERSION < 3
  PyObject* decoded = PyString_AsDecodedObject(encoded, "utf-8", NULL);
#else
  PyObject* decoded = PyUnicode_FromEncodedObject(encoded, "utf-8", NULL);
#endif
  if (decoded == NULL) {
    return NULL;
  }
  return decoded;
}

PyObject* Reduce(CMessage* self) {
  ScopedPyObjectPtr constructor(reinterpret_cast<PyObject*>(Py_TYPE(self)));
  constructor.inc();
  ScopedPyObjectPtr args(PyTuple_New(0));
  if (args == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr state(PyDict_New());
  if (state == NULL) {
    return  NULL;
  }
  ScopedPyObjectPtr serialized(SerializePartialToString(self));
  if (serialized == NULL) {
    return NULL;
  }
  if (PyDict_SetItemString(state, "serialized", serialized) < 0) {
    return NULL;
  }
  return Py_BuildValue("OOO", constructor.get(), args.get(), state.get());
}

PyObject* SetState(CMessage* self, PyObject* state) {
  if (!PyDict_Check(state)) {
    PyErr_SetString(PyExc_TypeError, "state not a dict");
    return NULL;
  }
  PyObject* serialized = PyDict_GetItemString(state, "serialized");
  if (serialized == NULL) {
    return NULL;
  }
  if (ParseFromString(self, serialized) == NULL) {
    return NULL;
  }
  Py_RETURN_NONE;
}

// CMessage static methods:
PyObject* _GetMessageDescriptor(PyObject* unused, PyObject* arg) {
  return cdescriptor_pool::FindMessageByName(GetDescriptorPool(), arg);
}

PyObject* _GetFieldDescriptor(PyObject* unused, PyObject* arg) {
  return cdescriptor_pool::FindFieldByName(GetDescriptorPool(), arg);
}

PyObject* _GetExtensionDescriptor(PyObject* unused, PyObject* arg) {
  return cdescriptor_pool::FindExtensionByName(GetDescriptorPool(), arg);
}

PyObject* _GetEnumDescriptor(PyObject* unused, PyObject* arg) {
  return cdescriptor_pool::FindEnumTypeByName(GetDescriptorPool(), arg);
}

PyObject* _GetOneofDescriptor(PyObject* unused, PyObject* arg) {
  return cdescriptor_pool::FindOneofByName(GetDescriptorPool(), arg);
}

PyObject* _CheckCalledFromGeneratedFile(PyObject* unused,
                                        PyObject* unused_arg) {
  if (!_CalledFromGeneratedFile(1)) {
    PyErr_SetString(PyExc_TypeError,
                    "Descriptors should not be created directly, "
                    "but only retrieved from their parent.");
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyMemberDef Members[] = {
  {"Extensions", T_OBJECT_EX, offsetof(CMessage, extensions), 0,
   "Extension dict"},
  {NULL}
};

static PyMethodDef Methods[] = {
  { "__deepcopy__", (PyCFunction)DeepCopy, METH_VARARGS,
    "Makes a deep copy of the class." },
  { "__reduce__", (PyCFunction)Reduce, METH_NOARGS,
    "Outputs picklable representation of the message." },
  { "__setstate__", (PyCFunction)SetState, METH_O,
    "Inputs picklable representation of the message." },
  { "__unicode__", (PyCFunction)ToUnicode, METH_NOARGS,
    "Outputs a unicode representation of the message." },
  { "AddDescriptors", (PyCFunction)AddDescriptors, METH_O | METH_CLASS,
    "Adds field descriptors to the class" },
  { "ByteSize", (PyCFunction)ByteSize, METH_NOARGS,
    "Returns the size of the message in bytes." },
  { "Clear", (PyCFunction)Clear, METH_NOARGS,
    "Clears the message." },
  { "ClearExtension", (PyCFunction)ClearExtension, METH_O,
    "Clears a message field." },
  { "ClearField", (PyCFunction)ClearField, METH_O,
    "Clears a message field." },
  { "CopyFrom", (PyCFunction)CopyFrom, METH_O,
    "Copies a protocol message into the current message." },
  { "FindInitializationErrors", (PyCFunction)FindInitializationErrors,
    METH_NOARGS,
    "Finds unset required fields." },
  { "FromString", (PyCFunction)FromString, METH_O | METH_CLASS,
    "Creates new method instance from given serialized data." },
  { "HasExtension", (PyCFunction)HasExtension, METH_O,
    "Checks if a message field is set." },
  { "HasField", (PyCFunction)HasField, METH_O,
    "Checks if a message field is set." },
  { "IsInitialized", (PyCFunction)IsInitialized, METH_VARARGS,
    "Checks if all required fields of a protocol message are set." },
  { "ListFields", (PyCFunction)ListFields, METH_NOARGS,
    "Lists all set fields of a message." },
  { "MergeFrom", (PyCFunction)MergeFrom, METH_O,
    "Merges a protocol message into the current message." },
  { "MergeFromString", (PyCFunction)MergeFromString, METH_O,
    "Merges a serialized message into the current message." },
  { "ParseFromString", (PyCFunction)ParseFromString, METH_O,
    "Parses a serialized message into the current message." },
  { "RegisterExtension", (PyCFunction)RegisterExtension, METH_O | METH_CLASS,
    "Registers an extension with the current message." },
  { "SerializePartialToString", (PyCFunction)SerializePartialToString,
    METH_NOARGS,
    "Serializes the message to a string, even if it isn't initialized." },
  { "SerializeToString", (PyCFunction)SerializeToString, METH_NOARGS,
    "Serializes the message to a string, only for initialized messages." },
  { "SetInParent", (PyCFunction)SetInParent, METH_NOARGS,
    "Sets the has bit of the given field in its parent message." },
  { "WhichOneof", (PyCFunction)WhichOneof, METH_O,
    "Returns the name of the field set inside a oneof, "
    "or None if no field is set." },

  // Static Methods.
  { "_BuildFile", (PyCFunction)Python_BuildFile, METH_O | METH_STATIC,
    "Registers a new protocol buffer file in the global C++ descriptor pool." },
  { "_GetMessageDescriptor", (PyCFunction)_GetMessageDescriptor,
    METH_O | METH_STATIC, "Finds a message descriptor in the message pool." },
  { "_GetFieldDescriptor", (PyCFunction)_GetFieldDescriptor,
    METH_O | METH_STATIC, "Finds a field descriptor in the message pool." },
  { "_GetExtensionDescriptor", (PyCFunction)_GetExtensionDescriptor,
    METH_O | METH_STATIC,
    "Finds a extension descriptor in the message pool." },
  { "_GetEnumDescriptor", (PyCFunction)_GetEnumDescriptor,
    METH_O | METH_STATIC,
    "Finds an enum descriptor in the message pool." },
  { "_GetOneofDescriptor", (PyCFunction)_GetOneofDescriptor,
    METH_O | METH_STATIC,
    "Finds an oneof descriptor in the message pool." },
  { "_CheckCalledFromGeneratedFile", (PyCFunction)_CheckCalledFromGeneratedFile,
    METH_NOARGS | METH_STATIC,
    "Raises TypeError if the caller is not in a _pb2.py file."},
  { NULL, NULL}
};

static bool SetCompositeField(
    CMessage* self, PyObject* name, PyObject* value) {
  if (self->composite_fields == NULL) {
    self->composite_fields = PyDict_New();
    if (self->composite_fields == NULL) {
      return false;
    }
  }
  return PyDict_SetItem(self->composite_fields, name, value) == 0;
}

PyObject* GetAttr(CMessage* self, PyObject* name) {
  PyObject* value = self->composite_fields ?
      PyDict_GetItem(self->composite_fields, name) : NULL;
  if (value != NULL) {
    Py_INCREF(value);
    return value;
  }

  const FieldDescriptor* field_descriptor = GetFieldDescriptor(self, name);
  if (field_descriptor == NULL) {
    return CMessage_Type.tp_base->tp_getattro(
        reinterpret_cast<PyObject*>(self), name);
  }

  if (field_descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
    PyObject* py_container = NULL;
    if (field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      PyObject *message_class = cdescriptor_pool::GetMessageClass(
          GetDescriptorPool(), field_descriptor->message_type());
      if (message_class == NULL) {
        return NULL;
      }
      py_container = repeated_composite_container::NewContainer(
          self, field_descriptor, message_class);
    } else {
      py_container = repeated_scalar_container::NewContainer(
          self, field_descriptor);
    }
    if (py_container == NULL) {
      return NULL;
    }
    if (!SetCompositeField(self, name, py_container)) {
      Py_DECREF(py_container);
      return NULL;
    }
    return py_container;
  }

  if (field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    PyObject* sub_message = InternalGetSubMessage(self, field_descriptor);
    if (!SetCompositeField(self, name, sub_message)) {
      Py_DECREF(sub_message);
      return NULL;
    }
    return sub_message;
  }

  return InternalGetScalar(self, field_descriptor);
}

int SetAttr(CMessage* self, PyObject* name, PyObject* value) {
  if (self->composite_fields && PyDict_Contains(self->composite_fields, name)) {
    PyErr_SetString(PyExc_TypeError, "Can't set composite field");
    return -1;
  }

  const FieldDescriptor* field_descriptor = GetFieldDescriptor(self, name);
  if (field_descriptor != NULL) {
    AssureWritable(self);
    if (field_descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
      PyErr_Format(PyExc_AttributeError, "Assignment not allowed to repeated "
                   "field \"%s\" in protocol message object.",
                   field_descriptor->name().c_str());
      return -1;
    } else {
      if (field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        PyErr_Format(PyExc_AttributeError, "Assignment not allowed to "
                     "field \"%s\" in protocol message object.",
                     field_descriptor->name().c_str());
        return -1;
      } else {
        return InternalSetScalar(self, field_descriptor, value);
      }
    }
  }

  PyErr_Format(PyExc_AttributeError, "Assignment not allowed");
  return -1;
}

}  // namespace cmessage

PyTypeObject CMessage_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "google.protobuf."
  "pyext._message.CMessage",           // tp_name
  sizeof(CMessage),                    // tp_basicsize
  0,                                   //  tp_itemsize
  (destructor)cmessage::Dealloc,       //  tp_dealloc
  0,                                   //  tp_print
  0,                                   //  tp_getattr
  0,                                   //  tp_setattr
  0,                                   //  tp_compare
  0,                                   //  tp_repr
  0,                                   //  tp_as_number
  0,                                   //  tp_as_sequence
  0,                                   //  tp_as_mapping
  0,                                   //  tp_hash
  0,                                   //  tp_call
  (reprfunc)cmessage::ToStr,           //  tp_str
  (getattrofunc)cmessage::GetAttr,     //  tp_getattro
  (setattrofunc)cmessage::SetAttr,     //  tp_setattro
  0,                                   //  tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  //  tp_flags
  "A ProtocolMessage",                 //  tp_doc
  0,                                   //  tp_traverse
  0,                                   //  tp_clear
  (richcmpfunc)cmessage::RichCompare,  //  tp_richcompare
  0,                                   //  tp_weaklistoffset
  0,                                   //  tp_iter
  0,                                   //  tp_iternext
  cmessage::Methods,                   //  tp_methods
  cmessage::Members,                   //  tp_members
  0,                                   //  tp_getset
  0,                                   //  tp_base
  0,                                   //  tp_dict
  0,                                   //  tp_descr_get
  0,                                   //  tp_descr_set
  0,                                   //  tp_dictoffset
  (initproc)cmessage::Init,            //  tp_init
  0,                                   //  tp_alloc
  cmessage::New,                       //  tp_new
};

// --- Exposing the C proto living inside Python proto to C code:

const Message* (*GetCProtoInsidePyProtoPtr)(PyObject* msg);
Message* (*MutableCProtoInsidePyProtoPtr)(PyObject* msg);

static const Message* GetCProtoInsidePyProtoImpl(PyObject* msg) {
  if (!PyObject_TypeCheck(msg, &CMessage_Type)) {
    return NULL;
  }
  CMessage* cmsg = reinterpret_cast<CMessage*>(msg);
  return cmsg->message;
}

static Message* MutableCProtoInsidePyProtoImpl(PyObject* msg) {
  if (!PyObject_TypeCheck(msg, &CMessage_Type)) {
    return NULL;
  }
  CMessage* cmsg = reinterpret_cast<CMessage*>(msg);
  if ((cmsg->composite_fields && PyDict_Size(cmsg->composite_fields) != 0) ||
      (cmsg->extensions != NULL &&
       PyDict_Size(cmsg->extensions->values) != 0)) {
    // There is currently no way of accurately syncing arbitrary changes to
    // the underlying C++ message back to the CMessage (e.g. removed repeated
    // composite containers). We only allow direct mutation of the underlying
    // C++ message if there is no child data in the CMessage.
    return NULL;
  }
  cmessage::AssureWritable(cmsg);
  return cmsg->message;
}

static const char module_docstring[] =
"python-proto2 is a module that can be used to enhance proto2 Python API\n"
"performance.\n"
"\n"
"It provides access to the protocol buffers C++ reflection API that\n"
"implements the basic protocol buffer functions.";

void InitGlobals() {
  // TODO(gps): Check all return values in this function for NULL and propagate
  // the error (MemoryError) on up to result in an import failure.  These should
  // also be freed and reset to NULL during finalization.
  kPythonZero = PyInt_FromLong(0);
  kint32min_py = PyInt_FromLong(kint32min);
  kint32max_py = PyInt_FromLong(kint32max);
  kuint32max_py = PyLong_FromLongLong(kuint32max);
  kint64min_py = PyLong_FromLongLong(kint64min);
  kint64max_py = PyLong_FromLongLong(kint64max);
  kuint64max_py = PyLong_FromUnsignedLongLong(kuint64max);

  kDESCRIPTOR = PyString_FromString("DESCRIPTOR");
  k_cdescriptor = PyString_FromString("_cdescriptor");
  kfull_name = PyString_FromString("full_name");
  k_extensions_by_name = PyString_FromString("_extensions_by_name");
  k_extensions_by_number = PyString_FromString("_extensions_by_number");

  message_factory = new DynamicMessageFactory(GetDescriptorPool()->pool);
  message_factory->SetDelegateToGeneratedFactory(true);
}

bool InitProto2MessageModule(PyObject *m) {
  // Initialize types and globals in descriptor.cc
  if (!InitDescriptor()) {
    return false;
  }

  // Initialize types and globals in descriptor_pool.cc
  if (!InitDescriptorPool()) {
    return false;
  }

  // Initialize constants defined in this file.
  InitGlobals();

  CMessage_Type.tp_hash = PyObject_HashNotImplemented;
  if (PyType_Ready(&CMessage_Type) < 0) {
    return false;
  }

  // DESCRIPTOR is set on each protocol buffer message class elsewhere, but set
  // it here as well to document that subclasses need to set it.
  PyDict_SetItem(CMessage_Type.tp_dict, kDESCRIPTOR, Py_None);
  // Subclasses with message extensions will override _extensions_by_name and
  // _extensions_by_number with fresh mutable dictionaries in AddDescriptors.
  // All other classes can share this same immutable mapping.
  ScopedPyObjectPtr empty_dict(PyDict_New());
  if (empty_dict == NULL) {
    return false;
  }
  ScopedPyObjectPtr immutable_dict(PyDictProxy_New(empty_dict));
  if (immutable_dict == NULL) {
    return false;
  }
  if (PyDict_SetItem(CMessage_Type.tp_dict,
                     k_extensions_by_name, immutable_dict) < 0) {
    return false;
  }
  if (PyDict_SetItem(CMessage_Type.tp_dict,
                     k_extensions_by_number, immutable_dict) < 0) {
    return false;
  }

  PyModule_AddObject(m, "Message", reinterpret_cast<PyObject*>(&CMessage_Type));

  RepeatedScalarContainer_Type.tp_hash =
      PyObject_HashNotImplemented;
  if (PyType_Ready(&RepeatedScalarContainer_Type) < 0) {
    return false;
  }

  PyModule_AddObject(m, "RepeatedScalarContainer",
                     reinterpret_cast<PyObject*>(
                         &RepeatedScalarContainer_Type));

  RepeatedCompositeContainer_Type.tp_hash = PyObject_HashNotImplemented;
  if (PyType_Ready(&RepeatedCompositeContainer_Type) < 0) {
    return false;
  }

  PyModule_AddObject(
      m, "RepeatedCompositeContainer",
      reinterpret_cast<PyObject*>(
          &RepeatedCompositeContainer_Type));

  ExtensionDict_Type.tp_hash = PyObject_HashNotImplemented;
  if (PyType_Ready(&ExtensionDict_Type) < 0) {
    return false;
  }

  PyModule_AddObject(
      m, "ExtensionDict",
      reinterpret_cast<PyObject*>(&ExtensionDict_Type));

  // This implementation provides full Descriptor types, we advertise it so that
  // descriptor.py can use them in replacement of the Python classes.
  PyModule_AddIntConstant(m, "_USE_C_DESCRIPTORS", 1);

  PyModule_AddObject(m, "Descriptor", reinterpret_cast<PyObject*>(
      &PyMessageDescriptor_Type));
  PyModule_AddObject(m, "FieldDescriptor", reinterpret_cast<PyObject*>(
      &PyFieldDescriptor_Type));
  PyModule_AddObject(m, "EnumDescriptor", reinterpret_cast<PyObject*>(
      &PyEnumDescriptor_Type));
  PyModule_AddObject(m, "EnumValueDescriptor", reinterpret_cast<PyObject*>(
      &PyEnumValueDescriptor_Type));
  PyModule_AddObject(m, "FileDescriptor", reinterpret_cast<PyObject*>(
      &PyFileDescriptor_Type));
  PyModule_AddObject(m, "OneofDescriptor", reinterpret_cast<PyObject*>(
      &PyOneofDescriptor_Type));

  PyObject* enum_type_wrapper = PyImport_ImportModule(
      "google.protobuf.internal.enum_type_wrapper");
  if (enum_type_wrapper == NULL) {
    return false;
  }
  EnumTypeWrapper_class =
      PyObject_GetAttrString(enum_type_wrapper, "EnumTypeWrapper");
  Py_DECREF(enum_type_wrapper);

  PyObject* message_module = PyImport_ImportModule(
      "google.protobuf.message");
  if (message_module == NULL) {
    return false;
  }
  EncodeError_class = PyObject_GetAttrString(message_module, "EncodeError");
  DecodeError_class = PyObject_GetAttrString(message_module, "DecodeError");
  Py_DECREF(message_module);

  PyObject* pickle_module = PyImport_ImportModule("pickle");
  if (pickle_module == NULL) {
    return false;
  }
  PickleError_class = PyObject_GetAttrString(pickle_module, "PickleError");
  Py_DECREF(pickle_module);

  // Override {Get,Mutable}CProtoInsidePyProto.
  GetCProtoInsidePyProtoPtr = GetCProtoInsidePyProtoImpl;
  MutableCProtoInsidePyProtoPtr = MutableCProtoInsidePyProtoImpl;

  return true;
}

}  // namespace python
}  // namespace protobuf


#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef _module = {
  PyModuleDef_HEAD_INIT,
  "_message",
  google::protobuf::python::module_docstring,
  -1,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
#define INITFUNC PyInit__message
#define INITFUNC_ERRORVAL NULL
#else  // Python 2
#define INITFUNC init_message
#define INITFUNC_ERRORVAL
#endif

extern "C" {
  PyMODINIT_FUNC INITFUNC(void) {
    PyObject* m;
#if PY_MAJOR_VERSION >= 3
    m = PyModule_Create(&_module);
#else
    m = Py_InitModule3("_message", NULL, google::protobuf::python::module_docstring);
#endif
    if (m == NULL) {
      return INITFUNC_ERRORVAL;
    }

    if (!google::protobuf::python::InitProto2MessageModule(m)) {
      Py_DECREF(m);
      return INITFUNC_ERRORVAL;
    }

#if PY_MAJOR_VERSION >= 3
    return m;
#endif
  }
}
}  // namespace google
