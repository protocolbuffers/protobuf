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
#include <google/protobuf/pyext/extension_dict.h>
#include <google/protobuf/pyext/repeated_composite_container.h>
#include <google/protobuf/pyext/repeated_scalar_container.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

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
    (PyUnicode_Check(ob)? PyUnicode_AsUTF8(ob): PyBytes_AS_STRING(ob))
  #endif
#endif

namespace google {
namespace protobuf {
namespace python {

// Forward declarations
namespace cmessage {
static PyObject* GetDescriptor(CMessage* self, PyObject* name);
static string GetMessageName(CMessage* self);
int InternalReleaseFieldByDescriptor(
    const google::protobuf::FieldDescriptor* field_descriptor,
    PyObject* composite_field,
    google::protobuf::Message* parent_message);
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
                    const google::protobuf::FieldDescriptor* field_descriptor) {
    return 0;
  }
};

// Apply a function to a composite field.  Does nothing if child is of
// non-composite type.
template<class Visitor>
static int VisitCompositeField(const FieldDescriptor* descriptor,
                               PyObject* child,
                               Visitor visitor) {
  if (descriptor->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
    if (descriptor->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
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
  } else if (descriptor->cpp_type() ==
             google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
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
  while (PyDict_Next(self->composite_fields, &pos, &key, &field)) {
    PyObject* cdescriptor = cmessage::GetDescriptor(self, key);
    if (cdescriptor != NULL) {
      const google::protobuf::FieldDescriptor* descriptor =
          reinterpret_cast<CFieldDescriptor*>(cdescriptor)->descriptor;
      if (VisitCompositeField(descriptor, field, visitor) == -1)
        return -1;
    }
  }

  // Visit extension fields.
  if (self->extensions != NULL) {
    while (PyDict_Next(self->extensions->values, &pos, &key, &field)) {
      CFieldDescriptor* cdescriptor =
          extension_dict::InternalGetCDescriptorFromExtension(key);
      if (cdescriptor == NULL)
        return -1;
      if (VisitCompositeField(cdescriptor->descriptor, field, visitor) == -1)
        return -1;
    }
  }

  return 0;
}

// ---------------------------------------------------------------------

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
static PyObject* k__descriptors;
static PyObject* kfull_name;
static PyObject* kname;
static PyObject* kmessage_type;
static PyObject* kis_extendable;
static PyObject* kextensions_by_name;
static PyObject* k_extensions_by_name;
static PyObject* k_extensions_by_number;
static PyObject* k_concrete_class;
static PyObject* kfields_by_name;

static CDescriptorPool* descriptor_pool;

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
    PyObject* arg, google::protobuf::Message* message,
    const google::protobuf::FieldDescriptor* descriptor,
    const google::protobuf::Reflection* reflection,
    bool append,
    int index) {
  GOOGLE_DCHECK(descriptor->type() == google::protobuf::FieldDescriptor::TYPE_STRING ||
         descriptor->type() == google::protobuf::FieldDescriptor::TYPE_BYTES);
  if (descriptor->type() == google::protobuf::FieldDescriptor::TYPE_STRING) {
    if (!PyBytes_Check(arg) && !PyUnicode_Check(arg)) {
      FormatTypeError(arg, "bytes, unicode");
      return false;
    }

    if (PyBytes_Check(arg)) {
      PyObject* unicode = PyUnicode_FromEncodedObject(arg, "ascii", NULL);
      if (unicode == NULL) {
        PyObject* repr = PyObject_Repr(arg);
        PyErr_Format(PyExc_ValueError,
                     "%s has type str, but isn't in 7-bit ASCII "
                     "encoding. Non-ASCII strings must be converted to "
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
  if (descriptor->type() == google::protobuf::FieldDescriptor::TYPE_STRING) {
    if (PyBytes_Check(arg)) {
#if PY_MAJOR_VERSION < 3
      encoded_string = PyString_AsEncodedObject(arg, "utf-8", NULL);
#else
      encoded_string = arg;  // Already encoded.
      Py_INCREF(encoded_string);
#endif
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

PyObject* ToStringObject(
    const google::protobuf::FieldDescriptor* descriptor, string value) {
  if (descriptor->type() != google::protobuf::FieldDescriptor::TYPE_STRING) {
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

google::protobuf::DynamicMessageFactory* global_message_factory;

namespace cmessage {

static int MaybeReleaseOverlappingOneofField(
    CMessage* cmessage,
    const google::protobuf::FieldDescriptor* field) {
#ifdef GOOGLE_PROTOBUF_HAS_ONEOF
  google::protobuf::Message* message = cmessage->message;
  const google::protobuf::Reflection* reflection = message->GetReflection();
  if (!field->containing_oneof() ||
      !reflection->HasOneof(*message, field->containing_oneof()) ||
      reflection->HasField(*message, field)) {
    // No other field in this oneof, no need to release.
    return 0;
  }

  const OneofDescriptor* oneof = field->containing_oneof();
  const FieldDescriptor* existing_field =
      reflection->GetOneofFieldDescriptor(*message, oneof);
  if (existing_field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    // Non-message fields don't need to be released.
    return 0;
  }
  const char* field_name = existing_field->name().c_str();
  PyObject* child_message = PyDict_GetItemString(
      cmessage->composite_fields, field_name);
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

static google::protobuf::Message* GetMutableMessage(
    CMessage* parent,
    const google::protobuf::FieldDescriptor* parent_field) {
  google::protobuf::Message* parent_message = parent->message;
  const google::protobuf::Reflection* reflection = parent_message->GetReflection();
  if (MaybeReleaseOverlappingOneofField(parent, parent_field) < 0) {
    return NULL;
  }
  return reflection->MutableMessage(
      parent_message, parent_field, global_message_factory);
}

struct FixupMessageReference : public ChildVisitor {
  // message must outlive this object.
  explicit FixupMessageReference(google::protobuf::Message* message) :
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
  google::protobuf::Message* message_;
};

int AssureWritable(CMessage* self) {
  if (self == NULL || !self->read_only) {
    return 0;
  }

  if (self->parent == NULL) {
    // If parent is NULL but we are trying to modify a read-only message, this
    // is a reference to a constant default instance that needs to be replaced
    // with a mutable top-level message.
    const Message* prototype = global_message_factory->GetPrototype(
        self->message->GetDescriptor());
    self->message = prototype->New();
    self->owner.reset(self->message);
  } else {
    // Otherwise, we need a mutable child message.
    if (AssureWritable(self->parent) == -1)
      return -1;

    // Make self->message writable.
    google::protobuf::Message* parent_message = self->parent->message;
    google::protobuf::Message* mutable_message = GetMutableMessage(
        self->parent,
        self->parent_field->descriptor);
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

static PyObject* GetDescriptor(CMessage* self, PyObject* name) {
  PyObject* descriptors =
      PyDict_GetItem(Py_TYPE(self)->tp_dict, k__descriptors);
  if (descriptors == NULL) {
    PyErr_SetString(PyExc_TypeError, "No __descriptors");
    return NULL;
  }

  return PyDict_GetItem(descriptors, name);
}

static const google::protobuf::Message* CreateMessage(const char* message_type) {
  string message_name(message_type);
  const google::protobuf::Descriptor* descriptor =
      GetDescriptorPool()->FindMessageTypeByName(message_name);
  if (descriptor == NULL) {
    PyErr_SetString(PyExc_TypeError, message_type);
    return NULL;
  }
  return global_message_factory->GetPrototype(descriptor);
}

// If cmessage_list is not NULL, this function releases values into the
// container CMessages instead of just removing. Repeated composite container
// needs to do this to make sure CMessages stay alive if they're still
// referenced after deletion. Repeated scalar container doesn't need to worry.
int InternalDeleteRepeatedField(
    google::protobuf::Message* message,
    const google::protobuf::FieldDescriptor* field_descriptor,
    PyObject* slice,
    PyObject* cmessage_list) {
  Py_ssize_t length, from, to, step, slice_length;
  const google::protobuf::Reflection* reflection = message->GetReflection();
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

int InitAttributes(CMessage* self, PyObject* arg, PyObject* kwargs) {
  ScopedPyObjectPtr descriptor;
  if (arg == NULL) {
    descriptor.reset(
        PyObject_GetAttr(reinterpret_cast<PyObject*>(self), kDESCRIPTOR));
    if (descriptor == NULL) {
      return NULL;
    }
  } else {
    descriptor.reset(arg);
    descriptor.inc();
  }
  ScopedPyObjectPtr is_extendable(PyObject_GetAttr(descriptor, kis_extendable));
  if (is_extendable == NULL) {
    return NULL;
  }
  int retcode = PyObject_IsTrue(is_extendable);
  if (retcode == -1) {
    return NULL;
  }
  if (retcode) {
    PyObject* py_extension_dict = PyObject_CallObject(
        reinterpret_cast<PyObject*>(&ExtensionDict_Type), NULL);
    if (py_extension_dict == NULL) {
      return NULL;
    }
    ExtensionDict* extension_dict = reinterpret_cast<ExtensionDict*>(
        py_extension_dict);
    extension_dict->parent = self;
    extension_dict->message = self->message;
    self->extensions = extension_dict;
  }

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
    PyObject* py_cdescriptor = GetDescriptor(self, name);
    if (py_cdescriptor == NULL) {
      PyErr_Format(PyExc_ValueError, "Protocol message has no \"%s\" field.",
                   PyString_AsString(name));
      return -1;
    }
    const google::protobuf::FieldDescriptor* descriptor =
        reinterpret_cast<CFieldDescriptor*>(py_cdescriptor)->descriptor;
    if (descriptor->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
      ScopedPyObjectPtr container(GetAttr(self, name));
      if (container == NULL) {
        return -1;
      }
      if (descriptor->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
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
    } else if (descriptor->cpp_type() ==
               google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
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

static PyObject* New(PyTypeObject* type, PyObject* args, PyObject* kwargs) {
  CMessage* self = reinterpret_cast<CMessage*>(type->tp_alloc(type, 0));
  if (self == NULL) {
    return NULL;
  }

  self->message = NULL;
  self->parent = NULL;
  self->parent_field = NULL;
  self->read_only = false;
  self->extensions = NULL;

  self->composite_fields = PyDict_New();
  if (self->composite_fields == NULL) {
    return NULL;
  }
  return reinterpret_cast<PyObject*>(self);
}

PyObject* NewEmpty(PyObject* type) {
  return New(reinterpret_cast<PyTypeObject*>(type), NULL, NULL);
}

static int Init(CMessage* self, PyObject* args, PyObject* kwargs) {
  if (kwargs == NULL) {
    // TODO(anuraag): Set error
    return -1;
  }

  PyObject* descriptor = PyTuple_GetItem(args, 0);
  if (descriptor == NULL || PyTuple_Size(args) != 1) {
    PyErr_SetString(PyExc_ValueError, "args must contain one arg: descriptor");
    return -1;
  }

  ScopedPyObjectPtr py_message_type(PyObject_GetAttr(descriptor, kfull_name));
  if (py_message_type == NULL) {
    return -1;
  }

  const char* message_type = PyString_AsString(py_message_type.get());
  const google::protobuf::Message* message = CreateMessage(message_type);
  if (message == NULL) {
    return -1;
  }

  self->message = message->New();
  self->owner.reset(self->message);

  if (InitAttributes(self, descriptor, kwargs) < 0) {
    return -1;
  }
  return 0;
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
                    const google::protobuf::FieldDescriptor* field_descriptor) {
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
    CMessage* self, const google::protobuf::FieldDescriptor* field_descriptor) {
  google::protobuf::Message* message = self->message;
  if (!FIELD_BELONGS_TO_MESSAGE(field_descriptor, message)) {
    PyErr_SetString(PyExc_KeyError,
                    "Field does not belong to message!");
    return NULL;
  }
  if (field_descriptor->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
    PyErr_SetString(PyExc_KeyError,
                    "Field is repeated. A singular method is required.");
    return NULL;
  }
  bool has_field =
      message->GetReflection()->HasField(*message, field_descriptor);
  return PyBool_FromLong(has_field ? 1 : 0);
}

const google::protobuf::FieldDescriptor* FindFieldWithOneofs(
    const google::protobuf::Message* message, const char* field_name, bool* in_oneof) {
  const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
  const google::protobuf::FieldDescriptor* field_descriptor =
      descriptor->FindFieldByName(field_name);
  if (field_descriptor == NULL) {
    const google::protobuf::OneofDescriptor* oneof_desc =
      message->GetDescriptor()->FindOneofByName(field_name);
    if (oneof_desc == NULL) {
      *in_oneof = false;
      return NULL;
    } else {
      *in_oneof = true;
      return message->GetReflection()->GetOneofFieldDescriptor(
          *message, oneof_desc);
    }
  }
  return field_descriptor;
}

PyObject* HasField(CMessage* self, PyObject* arg) {
#if PY_MAJOR_VERSION < 3
  char* field_name;
  if (PyString_AsStringAndSize(arg, &field_name, NULL) < 0) {
#else
  char* field_name = PyUnicode_AsUTF8(arg);
  if (!field_name) {
#endif
    return NULL;
  }

  google::protobuf::Message* message = self->message;
  const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
  bool is_in_oneof;
  const google::protobuf::FieldDescriptor* field_descriptor =
      FindFieldWithOneofs(message, field_name, &is_in_oneof);
  if (field_descriptor == NULL) {
    if (!is_in_oneof) {
      PyErr_Format(PyExc_ValueError, "Unknown field %s.", field_name);
      return NULL;
    } else {
      Py_RETURN_FALSE;
    }
  }

  if (field_descriptor->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
    PyErr_Format(PyExc_ValueError,
                 "Protocol message has no singular \"%s\" field.", field_name);
    return NULL;
  }

  bool has_field =
      message->GetReflection()->HasField(*message, field_descriptor);
  if (!has_field && field_descriptor->cpp_type() ==
      google::protobuf::FieldDescriptor::CPPTYPE_ENUM) {
    // We may have an invalid enum value stored in the UnknownFieldSet and need
    // to check presence in there as well.
    const google::protobuf::UnknownFieldSet& unknown_field_set =
        message->GetReflection()->GetUnknownFields(*message);
    for (int i = 0; i < unknown_field_set.field_count(); ++i) {
      if (unknown_field_set.field(i).number() == field_descriptor->number()) {
        Py_RETURN_TRUE;
      }
    }
    Py_RETURN_FALSE;
  }
  return PyBool_FromLong(has_field ? 1 : 0);
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
                    const google::protobuf::FieldDescriptor* field_descriptor) {
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
Message* ReleaseMessage(google::protobuf::Message* message,
                        const google::protobuf::Descriptor* descriptor,
                        const google::protobuf::FieldDescriptor* field_descriptor) {
  Message* released_message = message->GetReflection()->ReleaseMessage(
      message, field_descriptor, global_message_factory);
  // ReleaseMessage will return NULL which differs from
  // child_cmessage->message, if the field does not exist.  In this case,
  // the latter points to the default instance via a const_cast<>, so we
  // have to reset it to a new mutable object since we are taking ownership.
  if (released_message == NULL) {
    const Message* prototype = global_message_factory->GetPrototype(
        descriptor);
    GOOGLE_DCHECK(prototype != NULL);
    released_message = prototype->New();
  }

  return released_message;
}

int ReleaseSubMessage(google::protobuf::Message* message,
                      const google::protobuf::FieldDescriptor* field_descriptor,
                      CMessage* child_cmessage) {
  // Release the Message
  shared_ptr<Message> released_message(ReleaseMessage(
      message, child_cmessage->message->GetDescriptor(), field_descriptor));
  child_cmessage->message = released_message.get();
  child_cmessage->owner.swap(released_message);
  child_cmessage->parent = NULL;
  child_cmessage->parent_field = NULL;
  child_cmessage->read_only = false;
  return ForEachCompositeField(child_cmessage,
                               SetOwnerVisitor(child_cmessage->owner));
}

struct ReleaseChild : public ChildVisitor {
  // message must outlive this object.
  explicit ReleaseChild(google::protobuf::Message* parent_message) :
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
                    const google::protobuf::FieldDescriptor* field_descriptor) {
    return ReleaseSubMessage(parent_message_, field_descriptor,
        reinterpret_cast<CMessage*>(cmessage));
  }

  google::protobuf::Message* parent_message_;
};

int InternalReleaseFieldByDescriptor(
    const google::protobuf::FieldDescriptor* field_descriptor,
    PyObject* composite_field,
    google::protobuf::Message* parent_message) {
  return VisitCompositeField(
      field_descriptor,
      composite_field,
      ReleaseChild(parent_message));
}

int InternalReleaseField(CMessage* self, PyObject* composite_field,
                         PyObject* name) {
  PyObject* cdescriptor = GetDescriptor(self, name);
  if (cdescriptor != NULL) {
    const google::protobuf::FieldDescriptor* descriptor =
        reinterpret_cast<CFieldDescriptor*>(cdescriptor)->descriptor;
    return InternalReleaseFieldByDescriptor(
        descriptor, composite_field, self->message);
  }

  return 0;
}

PyObject* ClearFieldByDescriptor(
    CMessage* self,
    const google::protobuf::FieldDescriptor* descriptor) {
  if (!FIELD_BELONGS_TO_MESSAGE(descriptor, self->message)) {
    PyErr_SetString(PyExc_KeyError,
                    "Field does not belong to message!");
    return NULL;
  }
  AssureWritable(self);
  self->message->GetReflection()->ClearField(self->message, descriptor);
  Py_RETURN_NONE;
}

PyObject* ClearField(CMessage* self, PyObject* arg) {
  char* field_name;
  if (!PyString_Check(arg)) {
    PyErr_SetString(PyExc_TypeError, "field name must be a string");
    return NULL;
  }
#if PY_MAJOR_VERSION < 3
  if (PyString_AsStringAndSize(arg, &field_name, NULL) < 0) {
    return NULL;
  }
#else
  field_name = PyUnicode_AsUTF8(arg);
#endif
  AssureWritable(self);
  google::protobuf::Message* message = self->message;
  const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
  ScopedPyObjectPtr arg_in_oneof;
  bool is_in_oneof;
  const google::protobuf::FieldDescriptor* field_descriptor =
      FindFieldWithOneofs(message, field_name, &is_in_oneof);
  if (field_descriptor == NULL) {
    if (!is_in_oneof) {
      PyErr_Format(PyExc_ValueError,
                   "Protocol message has no \"%s\" field.", field_name);
      return NULL;
    } else {
      Py_RETURN_NONE;
    }
  } else if (is_in_oneof) {
    arg_in_oneof.reset(PyString_FromString(field_descriptor->name().c_str()));
    arg = arg_in_oneof.get();
  }

  PyObject* composite_field = PyDict_GetItem(self->composite_fields,
                                             arg);

  // Only release the field if there's a possibility that there are
  // references to it.
  if (composite_field != NULL) {
    if (InternalReleaseField(self, composite_field, arg) < 0) {
      return NULL;
    }
    PyDict_DelItem(self->composite_fields, arg);
  }
  message->GetReflection()->ClearField(message, field_descriptor);
  if (field_descriptor->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_ENUM) {
    google::protobuf::UnknownFieldSet* unknown_field_set =
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
    PyObject* py_extension_dict = PyObject_CallObject(
        reinterpret_cast<PyObject*>(&ExtensionDict_Type), NULL);
    if (py_extension_dict == NULL) {
      return NULL;
    }
    ExtensionDict* extension_dict = reinterpret_cast<ExtensionDict*>(
        py_extension_dict);
    extension_dict->parent = self;
    extension_dict->message = self->message;
    self->extensions = extension_dict;
  }
  PyDict_Clear(self->composite_fields);
  self->message->Clear();
  Py_RETURN_NONE;
}

// ---------------------------------------------------------------------

static string GetMessageName(CMessage* self) {
  if (self->parent_field != NULL) {
    return self->parent_field->descriptor->full_name();
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
    PyErr_Format(EncodeError_class, "Message %s is missing required fields: %s",
                 GetMessageName(self).c_str(), PyString_AsString(joined.get()));
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
class PythonFieldValuePrinter : public google::protobuf::TextFormat::FieldValuePrinter {
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
  google::protobuf::TextFormat::Printer printer;
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
  google::protobuf::io::CodedInputStream input(
      reinterpret_cast<const uint8*>(data), data_length);
  input.SetExtensionRegistry(GetDescriptorPool(), global_message_factory);
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
  if (PyObject_SetAttrString(extension_handle, "containing_type",
                             message_descriptor) < 0) {
    return NULL;
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

  CFieldDescriptor* cdescriptor =
      extension_dict::InternalGetCDescriptorFromExtension(extension_handle);
  ScopedPyObjectPtr py_cdescriptor(reinterpret_cast<PyObject*>(cdescriptor));
  if (cdescriptor == NULL) {
    return NULL;
  }
  Py_INCREF(extension_handle);
  cdescriptor->descriptor_field = extension_handle;
  const google::protobuf::FieldDescriptor* descriptor = cdescriptor->descriptor;
  // Check if it's a message set
  if (descriptor->is_extension() &&
      descriptor->containing_type()->options().message_set_wire_format() &&
      descriptor->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE &&
      descriptor->message_type() == descriptor->extension_scope() &&
      descriptor->label() == google::protobuf::FieldDescriptor::LABEL_OPTIONAL) {
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
  char* oneof_name;
  if (!PyString_Check(arg)) {
    PyErr_SetString(PyExc_TypeError, "field name must be a string");
    return NULL;
  }
  oneof_name = PyString_AsString(arg);
  if (oneof_name == NULL) {
    return NULL;
  }
  const google::protobuf::OneofDescriptor* oneof_desc =
      self->message->GetDescriptor()->FindOneofByName(oneof_name);
  if (oneof_desc == NULL) {
    PyErr_Format(PyExc_ValueError,
                 "Protocol message has no oneof \"%s\" field.", oneof_name);
    return NULL;
  }
  const google::protobuf::FieldDescriptor* field_in_oneof =
      self->message->GetReflection()->GetOneofFieldDescriptor(
          *self->message, oneof_desc);
  if (field_in_oneof == NULL) {
    Py_RETURN_NONE;
  } else {
    return PyString_FromString(field_in_oneof->name().c_str());
  }
}

static PyObject* ListFields(CMessage* self) {
  vector<const google::protobuf::FieldDescriptor*> fields;
  self->message->GetReflection()->ListFields(*self->message, &fields);

  PyObject* descriptor = PyDict_GetItem(Py_TYPE(self)->tp_dict, kDESCRIPTOR);
  if (descriptor == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr fields_by_name(
      PyObject_GetAttr(descriptor, kfields_by_name));
  if (fields_by_name == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr extensions_by_name(PyObject_GetAttr(
      reinterpret_cast<PyObject*>(Py_TYPE(self)), k_extensions_by_name));
  if (extensions_by_name == NULL) {
    PyErr_SetString(PyExc_ValueError, "no extensionsbyname");
    return NULL;
  }
  // Normally, the list will be exactly the size of the fields.
  PyObject* all_fields = PyList_New(fields.size());
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
      Py_DECREF(all_fields);
      return NULL;
    }

    if (fields[i]->is_extension()) {
      const string& field_name = fields[i]->full_name();
      PyObject* extension_field = PyDict_GetItemString(extensions_by_name,
                                                       field_name.c_str());
      if (extension_field == NULL) {
        // If we couldn't fetch extension_field, it means the module that
        // defines this extension has not been explicitly imported in Python
        // code, and the extension hasn't been registered. There's nothing much
        // we can do about this, so just skip it in the output to match the
        // behavior of the python implementation.
        continue;
      }
      PyObject* extensions = reinterpret_cast<PyObject*>(self->extensions);
      if (extensions == NULL) {
        Py_DECREF(all_fields);
        return NULL;
      }
      // 'extension' reference later stolen by PyTuple_SET_ITEM.
      PyObject* extension = PyObject_GetItem(extensions, extension_field);
      if (extension == NULL) {
        Py_DECREF(all_fields);
        return NULL;
      }
      Py_INCREF(extension_field);
      PyTuple_SET_ITEM(t.get(), 0, extension_field);
      // Steals reference to 'extension'
      PyTuple_SET_ITEM(t.get(), 1, extension);
    } else {
      const string& field_name = fields[i]->name();
      ScopedPyObjectPtr py_field_name(PyString_FromStringAndSize(
          field_name.c_str(), field_name.length()));
      if (py_field_name == NULL) {
        PyErr_SetString(PyExc_ValueError, "bad string");
        Py_DECREF(all_fields);
        return NULL;
      }
      PyObject* field_descriptor =
          PyDict_GetItem(fields_by_name, py_field_name);
      if (field_descriptor == NULL) {
        Py_DECREF(all_fields);
        return NULL;
      }

      PyObject* field_value = GetAttr(self, py_field_name);
      if (field_value == NULL) {
        PyErr_SetObject(PyExc_ValueError, py_field_name);
        Py_DECREF(all_fields);
        return NULL;
      }
      Py_INCREF(field_descriptor);
      PyTuple_SET_ITEM(t.get(), 0, field_descriptor);
      PyTuple_SET_ITEM(t.get(), 1, field_value);
    }
    PyList_SET_ITEM(all_fields, actual_size, t.release());
    ++actual_size;
  }
  Py_SIZE(all_fields) = actual_size;
  return all_fields;
}

PyObject* FindInitializationErrors(CMessage* self) {
  google::protobuf::Message* message = self->message;
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
    ScopedPyObjectPtr other_fields(ListFields(
        reinterpret_cast<CMessage*>(other)));
    return PyObject_RichCompare(self_fields, other_fields, opid);
  } else {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
}

PyObject* InternalGetScalar(
    CMessage* self,
    const google::protobuf::FieldDescriptor* field_descriptor) {
  google::protobuf::Message* message = self->message;
  const google::protobuf::Reflection* reflection = message->GetReflection();

  if (!FIELD_BELONGS_TO_MESSAGE(field_descriptor, message)) {
    PyErr_SetString(
        PyExc_KeyError, "Field does not belong to message!");
    return NULL;
  }

  PyObject* result = NULL;
  switch (field_descriptor->cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
      int32 value = reflection->GetInt32(*message, field_descriptor);
      result = PyInt_FromLong(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
      int64 value = reflection->GetInt64(*message, field_descriptor);
      result = PyLong_FromLongLong(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
      uint32 value = reflection->GetUInt32(*message, field_descriptor);
      result = PyInt_FromSize_t(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
      uint64 value = reflection->GetUInt64(*message, field_descriptor);
      result = PyLong_FromUnsignedLongLong(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
      float value = reflection->GetFloat(*message, field_descriptor);
      result = PyFloat_FromDouble(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
      double value = reflection->GetDouble(*message, field_descriptor);
      result = PyFloat_FromDouble(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
      bool value = reflection->GetBool(*message, field_descriptor);
      result = PyBool_FromLong(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
      string value = reflection->GetString(*message, field_descriptor);
      result = ToStringObject(field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
      if (!message->GetReflection()->HasField(*message, field_descriptor)) {
        // Look for the value in the unknown fields.
        google::protobuf::UnknownFieldSet* unknown_field_set =
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
        const google::protobuf::EnumValueDescriptor* enum_value =
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

PyObject* InternalGetSubMessage(CMessage* self,
                                CFieldDescriptor* cfield_descriptor) {
  PyObject* field = cfield_descriptor->descriptor_field;
  ScopedPyObjectPtr message_type(PyObject_GetAttr(field, kmessage_type));
  if (message_type == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr concrete_class(
      PyObject_GetAttr(message_type, k_concrete_class));
  if (concrete_class == NULL) {
    return NULL;
  }
  PyObject* py_cmsg = cmessage::NewEmpty(concrete_class);
  if (py_cmsg == NULL) {
    return NULL;
  }
  if (!PyObject_TypeCheck(py_cmsg, &CMessage_Type)) {
    PyErr_SetString(PyExc_TypeError, "Not a CMessage!");
  }
  CMessage* cmsg = reinterpret_cast<CMessage*>(py_cmsg);

  const google::protobuf::FieldDescriptor* field_descriptor =
      cfield_descriptor->descriptor;
  const google::protobuf::Reflection* reflection = self->message->GetReflection();
  const google::protobuf::Message& sub_message = reflection->GetMessage(
      *self->message, field_descriptor, global_message_factory);
  cmsg->owner = self->owner;
  cmsg->parent = self;
  cmsg->parent_field = cfield_descriptor;
  cmsg->read_only = !reflection->HasField(*self->message, field_descriptor);
  cmsg->message = const_cast<google::protobuf::Message*>(&sub_message);

  if (InitAttributes(cmsg, NULL, NULL) < 0) {
    Py_DECREF(py_cmsg);
    return NULL;
  }
  return py_cmsg;
}

int InternalSetScalar(
    CMessage* self,
    const google::protobuf::FieldDescriptor* field_descriptor,
    PyObject* arg) {
  google::protobuf::Message* message = self->message;
  const google::protobuf::Reflection* reflection = message->GetReflection();

  if (!FIELD_BELONGS_TO_MESSAGE(field_descriptor, message)) {
    PyErr_SetString(
        PyExc_KeyError, "Field does not belong to message!");
    return -1;
  }

  if (MaybeReleaseOverlappingOneofField(self, field_descriptor) < 0) {
    return -1;
  }

  switch (field_descriptor->cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
      GOOGLE_CHECK_GET_INT32(arg, value, -1);
      reflection->SetInt32(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
      GOOGLE_CHECK_GET_INT64(arg, value, -1);
      reflection->SetInt64(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
      GOOGLE_CHECK_GET_UINT32(arg, value, -1);
      reflection->SetUInt32(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
      GOOGLE_CHECK_GET_UINT64(arg, value, -1);
      reflection->SetUInt64(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
      GOOGLE_CHECK_GET_FLOAT(arg, value, -1);
      reflection->SetFloat(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
      GOOGLE_CHECK_GET_DOUBLE(arg, value, -1);
      reflection->SetDouble(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
      GOOGLE_CHECK_GET_BOOL(arg, value, -1);
      reflection->SetBool(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
      if (!CheckAndSetString(
          arg, message, field_descriptor, reflection, false, -1)) {
        return -1;
      }
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
      GOOGLE_CHECK_GET_INT32(arg, value, -1);
      const google::protobuf::EnumDescriptor* enum_descriptor =
          field_descriptor->enum_type();
      const google::protobuf::EnumValueDescriptor* enum_value =
          enum_descriptor->FindValueByNumber(value);
      if (enum_value != NULL) {
        reflection->SetEnum(message, field_descriptor, enum_value);
      } else {
        PyErr_Format(PyExc_ValueError, "Unknown enum value: %d", value);
        return -1;
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

  if (InitAttributes(cmsg, NULL, NULL) < 0) {
    Py_DECREF(py_cmsg);
    return NULL;
  }
  return py_cmsg;
}

static PyObject* AddDescriptors(PyTypeObject* cls,
                                PyObject* descriptor) {
  if (PyObject_SetAttr(reinterpret_cast<PyObject*>(cls),
                       k_extensions_by_name, PyDict_New()) < 0) {
    return NULL;
  }
  if (PyObject_SetAttr(reinterpret_cast<PyObject*>(cls),
                       k_extensions_by_number, PyDict_New()) < 0) {
    return NULL;
  }

  ScopedPyObjectPtr field_descriptors(PyDict_New());

  ScopedPyObjectPtr fields(PyObject_GetAttrString(descriptor, "fields"));
  if (fields == NULL) {
    return NULL;
  }

  ScopedPyObjectPtr _NUMBER_string(PyString_FromString("_FIELD_NUMBER"));
  if (_NUMBER_string == NULL) {
    return NULL;
  }

  const Py_ssize_t fields_size = PyList_GET_SIZE(fields.get());
  for (int i = 0; i < fields_size; ++i) {
    PyObject* field = PyList_GET_ITEM(fields.get(), i);
    ScopedPyObjectPtr field_name(PyObject_GetAttr(field, kname));
    ScopedPyObjectPtr full_field_name(PyObject_GetAttr(field, kfull_name));
    if (field_name == NULL || full_field_name == NULL) {
      PyErr_SetString(PyExc_TypeError, "Name is null");
      return NULL;
    }

    PyObject* field_descriptor =
        cdescriptor_pool::FindFieldByName(descriptor_pool, full_field_name);
    if (field_descriptor == NULL) {
      PyErr_SetString(PyExc_TypeError, "Couldn't find field");
      return NULL;
    }
    Py_INCREF(field);
    CFieldDescriptor* cfield_descriptor = reinterpret_cast<CFieldDescriptor*>(
        field_descriptor);
    cfield_descriptor->descriptor_field = field;
    if (PyDict_SetItem(field_descriptors, field_name, field_descriptor) < 0) {
      return NULL;
    }

    // The FieldDescriptor's name field might either be of type bytes or
    // of type unicode, depending on whether the FieldDescriptor was
    // parsed from a serialized message or read from the
    // <message>_pb2.py module.
    ScopedPyObjectPtr field_name_upcased(
         PyObject_CallMethod(field_name, "upper", NULL));
    if (field_name_upcased == NULL) {
      return NULL;
    }

    ScopedPyObjectPtr field_number_name(PyObject_CallMethod(
         field_name_upcased, "__add__", "(O)", _NUMBER_string.get()));
    if (field_number_name == NULL) {
      return NULL;
    }

    ScopedPyObjectPtr number(PyInt_FromLong(
        cfield_descriptor->descriptor->number()));
    if (number == NULL) {
      return NULL;
    }
    if (PyObject_SetAttr(reinterpret_cast<PyObject*>(cls),
                         field_number_name, number) == -1) {
      return NULL;
    }
  }

  PyDict_SetItem(cls->tp_dict, k__descriptors, field_descriptors);

  // Enum Values
  ScopedPyObjectPtr enum_types(PyObject_GetAttrString(descriptor,
                                                      "enum_types"));
  if (enum_types == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr type_iter(PyObject_GetIter(enum_types));
  if (type_iter == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr enum_type;
  while ((enum_type.reset(PyIter_Next(type_iter))) != NULL) {
    ScopedPyObjectPtr wrapped(PyObject_CallFunctionObjArgs(
        EnumTypeWrapper_class, enum_type.get(), NULL));
    if (wrapped == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr enum_name(PyObject_GetAttr(enum_type, kname));
    if (enum_name == NULL) {
      return NULL;
    }
    if (PyObject_SetAttr(reinterpret_cast<PyObject*>(cls),
                         enum_name, wrapped) == -1) {
      return NULL;
    }

    ScopedPyObjectPtr enum_values(PyObject_GetAttrString(enum_type, "values"));
    if (enum_values == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr values_iter(PyObject_GetIter(enum_values));
    if (values_iter == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr enum_value;
    while ((enum_value.reset(PyIter_Next(values_iter))) != NULL) {
      ScopedPyObjectPtr value_name(PyObject_GetAttr(enum_value, kname));
      if (value_name == NULL) {
        return NULL;
      }
      ScopedPyObjectPtr value_number(PyObject_GetAttrString(enum_value,
                                                            "number"));
      if (value_number == NULL) {
        return NULL;
      }
      if (PyObject_SetAttr(reinterpret_cast<PyObject*>(cls),
                           value_name, value_number) == -1) {
        return NULL;
      }
    }
    if (PyErr_Occurred()) {  // If PyIter_Next failed
      return NULL;
    }
  }
  if (PyErr_Occurred()) {  // If PyIter_Next failed
    return NULL;
  }

  ScopedPyObjectPtr extension_dict(
      PyObject_GetAttr(descriptor, kextensions_by_name));
  if (extension_dict == NULL || !PyDict_Check(extension_dict)) {
    PyErr_SetString(PyExc_TypeError, "extensions_by_name not a dict");
    return NULL;
  }
  Py_ssize_t pos = 0;
  PyObject* extension_name;
  PyObject* extension_field;

  while (PyDict_Next(extension_dict, &pos, &extension_name, &extension_field)) {
    if (PyObject_SetAttr(reinterpret_cast<PyObject*>(cls),
                         extension_name, extension_field) == -1) {
      return NULL;
    }
    ScopedPyObjectPtr py_cfield_descriptor(
        PyObject_GetAttrString(extension_field, "_cdescriptor"));
    if (py_cfield_descriptor == NULL) {
      return NULL;
    }
    CFieldDescriptor* cfield_descriptor =
        reinterpret_cast<CFieldDescriptor*>(py_cfield_descriptor.get());
    Py_INCREF(extension_field);
    cfield_descriptor->descriptor_field = extension_field;

    ScopedPyObjectPtr field_name_upcased(
        PyObject_CallMethod(extension_name, "upper", NULL));
    if (field_name_upcased == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr field_number_name(PyObject_CallMethod(
         field_name_upcased, "__add__", "(O)", _NUMBER_string.get()));
    if (field_number_name == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr number(PyInt_FromLong(
        cfield_descriptor->descriptor->number()));
    if (number == NULL) {
      return NULL;
    }
    if (PyObject_SetAttr(reinterpret_cast<PyObject*>(cls),
                         field_number_name, PyInt_FromLong(
            cfield_descriptor->descriptor->number())) == -1) {
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
  if (InitAttributes(reinterpret_cast<CMessage*>(clone), NULL, NULL) < 0) {
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
PyObject* _GetFieldDescriptor(PyObject* unused, PyObject* arg) {
  return cdescriptor_pool::FindFieldByName(descriptor_pool, arg);
}

PyObject* _GetExtensionDescriptor(PyObject* unused, PyObject* arg) {
  return cdescriptor_pool::FindExtensionByName(descriptor_pool, arg);
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
  { "_GetFieldDescriptor", (PyCFunction)_GetFieldDescriptor,
    METH_O | METH_STATIC, "Finds a field descriptor in the message pool." },
  { "_GetExtensionDescriptor", (PyCFunction)_GetExtensionDescriptor,
    METH_O | METH_STATIC,
    "Finds a extension descriptor in the message pool." },
  { NULL, NULL}
};

PyObject* GetAttr(CMessage* self, PyObject* name) {
  PyObject* value = PyDict_GetItem(self->composite_fields, name);
  if (value != NULL) {
    Py_INCREF(value);
    return value;
  }

  PyObject* descriptor = GetDescriptor(self, name);
  if (descriptor != NULL) {
    CFieldDescriptor* cdescriptor =
        reinterpret_cast<CFieldDescriptor*>(descriptor);
    const google::protobuf::FieldDescriptor* field_descriptor = cdescriptor->descriptor;
    if (field_descriptor->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
      if (field_descriptor->cpp_type() ==
          google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        PyObject* py_container = PyObject_CallObject(
            reinterpret_cast<PyObject*>(&RepeatedCompositeContainer_Type),
            NULL);
        if (py_container == NULL) {
          return NULL;
        }
        RepeatedCompositeContainer* container =
            reinterpret_cast<RepeatedCompositeContainer*>(py_container);
        PyObject* field = cdescriptor->descriptor_field;
        PyObject* message_type = PyObject_GetAttr(field, kmessage_type);
        if (message_type == NULL) {
          return NULL;
        }
        PyObject* concrete_class =
            PyObject_GetAttr(message_type, k_concrete_class);
        if (concrete_class == NULL) {
          return NULL;
        }
        container->parent = self;
        container->parent_field = cdescriptor;
        container->message = self->message;
        container->owner = self->owner;
        container->subclass_init = concrete_class;
        Py_DECREF(message_type);
        if (PyDict_SetItem(self->composite_fields, name, py_container) < 0) {
          Py_DECREF(py_container);
          return NULL;
        }
        return py_container;
      } else {
        ScopedPyObjectPtr init_args(PyTuple_Pack(2, self, cdescriptor));
        PyObject* py_container = PyObject_CallObject(
            reinterpret_cast<PyObject*>(&RepeatedScalarContainer_Type),
            init_args);
        if (py_container == NULL) {
          return NULL;
        }
        if (PyDict_SetItem(self->composite_fields, name, py_container) < 0) {
          Py_DECREF(py_container);
          return NULL;
        }
        return py_container;
      }
    } else {
      if (field_descriptor->cpp_type() ==
          google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        PyObject* sub_message = InternalGetSubMessage(self, cdescriptor);
        if (PyDict_SetItem(self->composite_fields, name, sub_message) < 0) {
          Py_DECREF(sub_message);
          return NULL;
        }
        return sub_message;
      } else {
        return InternalGetScalar(self, field_descriptor);
      }
    }
  }

  return CMessage_Type.tp_base->tp_getattro(reinterpret_cast<PyObject*>(self),
                                            name);
}

int SetAttr(CMessage* self, PyObject* name, PyObject* value) {
  if (PyDict_Contains(self->composite_fields, name)) {
    PyErr_SetString(PyExc_TypeError, "Can't set composite field");
    return -1;
  }

  PyObject* descriptor = GetDescriptor(self, name);
  if (descriptor != NULL) {
    AssureWritable(self);
    CFieldDescriptor* cdescriptor =
        reinterpret_cast<CFieldDescriptor*>(descriptor);
    const google::protobuf::FieldDescriptor* field_descriptor = cdescriptor->descriptor;
    if (field_descriptor->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
      PyErr_Format(PyExc_AttributeError, "Assignment not allowed to repeated "
                   "field \"%s\" in protocol message object.",
                   field_descriptor->name().c_str());
      return -1;
    } else {
      if (field_descriptor->cpp_type() ==
          google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
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
  "google.protobuf.internal."
  "cpp._message.CMessage",             // tp_name
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

static const google::protobuf::Message* GetCProtoInsidePyProtoImpl(PyObject* msg) {
  if (!PyObject_TypeCheck(msg, &CMessage_Type)) {
    return NULL;
  }
  CMessage* cmsg = reinterpret_cast<CMessage*>(msg);
  return cmsg->message;
}

static google::protobuf::Message* MutableCProtoInsidePyProtoImpl(PyObject* msg) {
  if (!PyObject_TypeCheck(msg, &CMessage_Type)) {
    return NULL;
  }
  CMessage* cmsg = reinterpret_cast<CMessage*>(msg);
  if (PyDict_Size(cmsg->composite_fields) != 0 ||
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
  k__descriptors = PyString_FromString("__descriptors");
  kfull_name = PyString_FromString("full_name");
  kis_extendable = PyString_FromString("is_extendable");
  kextensions_by_name = PyString_FromString("extensions_by_name");
  k_extensions_by_name = PyString_FromString("_extensions_by_name");
  k_extensions_by_number = PyString_FromString("_extensions_by_number");
  k_concrete_class = PyString_FromString("_concrete_class");
  kmessage_type = PyString_FromString("message_type");
  kname = PyString_FromString("name");
  kfields_by_name = PyString_FromString("fields_by_name");

  global_message_factory = new DynamicMessageFactory(GetDescriptorPool());
  global_message_factory->SetDelegateToGeneratedFactory(true);

  descriptor_pool = reinterpret_cast<google::protobuf::python::CDescriptorPool*>(
      Python_NewCDescriptorPool(NULL, NULL));
}

bool InitProto2MessageModule(PyObject *m) {
  InitGlobals();

  google::protobuf::python::CMessage_Type.tp_hash = PyObject_HashNotImplemented;
  if (PyType_Ready(&google::protobuf::python::CMessage_Type) < 0) {
    return false;
  }

  // All three of these are actually set elsewhere, directly onto the child
  // protocol buffer message class, but set them here as well to document that
  // subclasses need to set these.
  PyDict_SetItem(google::protobuf::python::CMessage_Type.tp_dict, kDESCRIPTOR, Py_None);
  PyDict_SetItem(google::protobuf::python::CMessage_Type.tp_dict,
                 k_extensions_by_name, Py_None);
  PyDict_SetItem(google::protobuf::python::CMessage_Type.tp_dict,
                 k_extensions_by_number, Py_None);

  PyModule_AddObject(m, "Message", reinterpret_cast<PyObject*>(
      &google::protobuf::python::CMessage_Type));

  google::protobuf::python::RepeatedScalarContainer_Type.tp_new = PyType_GenericNew;
  google::protobuf::python::RepeatedScalarContainer_Type.tp_hash =
      PyObject_HashNotImplemented;
  if (PyType_Ready(&google::protobuf::python::RepeatedScalarContainer_Type) < 0) {
    return false;
  }

  PyModule_AddObject(m, "RepeatedScalarContainer",
                     reinterpret_cast<PyObject*>(
                         &google::protobuf::python::RepeatedScalarContainer_Type));

  google::protobuf::python::RepeatedCompositeContainer_Type.tp_new = PyType_GenericNew;
  google::protobuf::python::RepeatedCompositeContainer_Type.tp_hash =
      PyObject_HashNotImplemented;
  if (PyType_Ready(&google::protobuf::python::RepeatedCompositeContainer_Type) < 0) {
    return false;
  }

  PyModule_AddObject(
      m, "RepeatedCompositeContainer",
      reinterpret_cast<PyObject*>(
          &google::protobuf::python::RepeatedCompositeContainer_Type));

  google::protobuf::python::ExtensionDict_Type.tp_new = PyType_GenericNew;
  google::protobuf::python::ExtensionDict_Type.tp_hash = PyObject_HashNotImplemented;
  if (PyType_Ready(&google::protobuf::python::ExtensionDict_Type) < 0) {
    return false;
  }

  PyModule_AddObject(
      m, "ExtensionDict",
      reinterpret_cast<PyObject*>(&google::protobuf::python::ExtensionDict_Type));

  if (!google::protobuf::python::InitDescriptor()) {
    return false;
  }

  PyObject* enum_type_wrapper = PyImport_ImportModule(
      "google.protobuf.internal.enum_type_wrapper");
  if (enum_type_wrapper == NULL) {
    return false;
  }
  google::protobuf::python::EnumTypeWrapper_class =
      PyObject_GetAttrString(enum_type_wrapper, "EnumTypeWrapper");
  Py_DECREF(enum_type_wrapper);

  PyObject* message_module = PyImport_ImportModule(
      "google.protobuf.message");
  if (message_module == NULL) {
    return false;
  }
  google::protobuf::python::EncodeError_class = PyObject_GetAttrString(message_module,
                                                             "EncodeError");
  google::protobuf::python::DecodeError_class = PyObject_GetAttrString(message_module,
                                                             "DecodeError");
  Py_DECREF(message_module);

  PyObject* pickle_module = PyImport_ImportModule("pickle");
  if (pickle_module == NULL) {
    return false;
  }
  google::protobuf::python::PickleError_class = PyObject_GetAttrString(pickle_module,
                                                             "PickleError");
  Py_DECREF(pickle_module);

  // Override {Get,Mutable}CProtoInsidePyProto.
  google::protobuf::python::GetCProtoInsidePyProtoPtr =
      google::protobuf::python::GetCProtoInsidePyProtoImpl;
  google::protobuf::python::MutableCProtoInsidePyProtoPtr =
      google::protobuf::python::MutableCProtoInsidePyProtoImpl;

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
