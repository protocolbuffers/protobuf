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

#include <google/protobuf/pyext/repeated_scalar_container.h>

#include <memory>
#ifndef _SHARED_PTR_H
#include <google/protobuf/stubs/shared_ptr.h>
#endif

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

#if PY_MAJOR_VERSION >= 3
  #define PyInt_FromLong PyLong_FromLong
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

extern google::protobuf::DynamicMessageFactory* global_message_factory;

namespace repeated_scalar_container {

static int InternalAssignRepeatedField(
    RepeatedScalarContainer* self, PyObject* list) {
  self->message->GetReflection()->ClearField(self->message,
                                             self->parent_field->descriptor);
  for (Py_ssize_t i = 0; i < PyList_GET_SIZE(list); ++i) {
    PyObject* value = PyList_GET_ITEM(list, i);
    if (Append(self, value) == NULL) {
      return -1;
    }
  }
  return 0;
}

static Py_ssize_t Len(RepeatedScalarContainer* self) {
  google::protobuf::Message* message = self->message;
  return message->GetReflection()->FieldSize(*message,
                                             self->parent_field->descriptor);
}

static int AssignItem(RepeatedScalarContainer* self,
                      Py_ssize_t index,
                      PyObject* arg) {
  cmessage::AssureWritable(self->parent);
  google::protobuf::Message* message = self->message;
  const google::protobuf::FieldDescriptor* field_descriptor =
      self->parent_field->descriptor;
  if (!FIELD_BELONGS_TO_MESSAGE(field_descriptor, message)) {
    PyErr_SetString(
        PyExc_KeyError, "Field does not belong to message!");
    return -1;
  }

  const google::protobuf::Reflection* reflection = message->GetReflection();
  int field_size = reflection->FieldSize(*message, field_descriptor);
  if (index < 0) {
    index = field_size + index;
  }
  if (index < 0 || index >= field_size) {
    PyErr_Format(PyExc_IndexError,
                 "list assignment index (%d) out of range",
                 static_cast<int>(index));
    return -1;
  }

  if (arg == NULL) {
    ScopedPyObjectPtr py_index(PyLong_FromLong(index));
    return cmessage::InternalDeleteRepeatedField(message, field_descriptor,
                                                 py_index, NULL);
  }

  if (PySequence_Check(arg) && !(PyBytes_Check(arg) || PyUnicode_Check(arg))) {
    PyErr_SetString(PyExc_TypeError, "Value must be scalar");
    return -1;
  }

  switch (field_descriptor->cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
      GOOGLE_CHECK_GET_INT32(arg, value, -1);
      reflection->SetRepeatedInt32(message, field_descriptor, index, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
      GOOGLE_CHECK_GET_INT64(arg, value, -1);
      reflection->SetRepeatedInt64(message, field_descriptor, index, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
      GOOGLE_CHECK_GET_UINT32(arg, value, -1);
      reflection->SetRepeatedUInt32(message, field_descriptor, index, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
      GOOGLE_CHECK_GET_UINT64(arg, value, -1);
      reflection->SetRepeatedUInt64(message, field_descriptor, index, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
      GOOGLE_CHECK_GET_FLOAT(arg, value, -1);
      reflection->SetRepeatedFloat(message, field_descriptor, index, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
      GOOGLE_CHECK_GET_DOUBLE(arg, value, -1);
      reflection->SetRepeatedDouble(message, field_descriptor, index, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
      GOOGLE_CHECK_GET_BOOL(arg, value, -1);
      reflection->SetRepeatedBool(message, field_descriptor, index, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
      if (!CheckAndSetString(
          arg, message, field_descriptor, reflection, false, index)) {
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
        reflection->SetRepeatedEnum(message, field_descriptor, index,
                                    enum_value);
      } else {
        ScopedPyObjectPtr s(PyObject_Str(arg));
        if (s != NULL) {
          PyErr_Format(PyExc_ValueError, "Unknown enum value: %s",
                       PyString_AsString(s.get()));
        }
        return -1;
      }
      break;
    }
    default:
      PyErr_Format(
          PyExc_SystemError, "Adding value to a field of unknown type %d",
          field_descriptor->cpp_type());
      return -1;
  }
  return 0;
}

static PyObject* Item(RepeatedScalarContainer* self, Py_ssize_t index) {
  google::protobuf::Message* message = self->message;
  const google::protobuf::FieldDescriptor* field_descriptor =
      self->parent_field->descriptor;
  const google::protobuf::Reflection* reflection = message->GetReflection();

  int field_size = reflection->FieldSize(*message, field_descriptor);
  if (index < 0) {
    index = field_size + index;
  }
  if (index < 0 || index >= field_size) {
    PyErr_Format(PyExc_IndexError,
                 "list assignment index (%d) out of range",
                 static_cast<int>(index));
    return NULL;
  }

  PyObject* result = NULL;
  switch (field_descriptor->cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
      int32 value = reflection->GetRepeatedInt32(
          *message, field_descriptor, index);
      result = PyInt_FromLong(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
      int64 value = reflection->GetRepeatedInt64(
          *message, field_descriptor, index);
      result = PyLong_FromLongLong(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
      uint32 value = reflection->GetRepeatedUInt32(
          *message, field_descriptor, index);
      result = PyLong_FromLongLong(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
      uint64 value = reflection->GetRepeatedUInt64(
          *message, field_descriptor, index);
      result = PyLong_FromUnsignedLongLong(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
      float value = reflection->GetRepeatedFloat(
          *message, field_descriptor, index);
      result = PyFloat_FromDouble(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
      double value = reflection->GetRepeatedDouble(
          *message, field_descriptor, index);
      result = PyFloat_FromDouble(value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
      bool value = reflection->GetRepeatedBool(
          *message, field_descriptor, index);
      result = PyBool_FromLong(value ? 1 : 0);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
      const google::protobuf::EnumValueDescriptor* enum_value =
          message->GetReflection()->GetRepeatedEnum(
              *message, field_descriptor, index);
      result = PyInt_FromLong(enum_value->number());
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
      string value = reflection->GetRepeatedString(
          *message, field_descriptor, index);
      result = ToStringObject(field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
      PyObject* py_cmsg = PyObject_CallObject(reinterpret_cast<PyObject*>(
          &CMessage_Type), NULL);
      if (py_cmsg == NULL) {
        return NULL;
      }
      CMessage* cmsg = reinterpret_cast<CMessage*>(py_cmsg);
      const google::protobuf::Message& msg = reflection->GetRepeatedMessage(
          *message, field_descriptor, index);
      cmsg->owner = self->owner;
      cmsg->parent = self->parent;
      cmsg->message = const_cast<google::protobuf::Message*>(&msg);
      cmsg->read_only = false;
      result = reinterpret_cast<PyObject*>(py_cmsg);
      break;
    }
    default:
      PyErr_Format(
          PyExc_SystemError,
          "Getting value from a repeated field of unknown type %d",
          field_descriptor->cpp_type());
  }

  return result;
}

static PyObject* Subscript(RepeatedScalarContainer* self, PyObject* slice) {
  Py_ssize_t from;
  Py_ssize_t to;
  Py_ssize_t step;
  Py_ssize_t length;
  Py_ssize_t slicelength;
  bool return_list = false;
#if PY_MAJOR_VERSION < 3
  if (PyInt_Check(slice)) {
    from = to = PyInt_AsLong(slice);
  } else  // NOLINT
#endif
  if (PyLong_Check(slice)) {
    from = to = PyLong_AsLong(slice);
  } else if (PySlice_Check(slice)) {
    length = Len(self);
#if PY_MAJOR_VERSION >= 3
    if (PySlice_GetIndicesEx(slice,
#else
    if (PySlice_GetIndicesEx(reinterpret_cast<PySliceObject*>(slice),
#endif
                             length, &from, &to, &step, &slicelength) == -1) {
      return NULL;
    }
    return_list = true;
  } else {
    PyErr_SetString(PyExc_TypeError, "list indices must be integers");
    return NULL;
  }

  if (!return_list) {
    return Item(self, from);
  }

  PyObject* list = PyList_New(0);
  if (list == NULL) {
    return NULL;
  }
  if (from <= to) {
    if (step < 0) {
      return list;
    }
    for (Py_ssize_t index = from; index < to; index += step) {
      if (index < 0 || index >= length) {
        break;
      }
      ScopedPyObjectPtr s(Item(self, index));
      PyList_Append(list, s);
    }
  } else {
    if (step > 0) {
      return list;
    }
    for (Py_ssize_t index = from; index > to; index += step) {
      if (index < 0 || index >= length) {
        break;
      }
      ScopedPyObjectPtr s(Item(self, index));
      PyList_Append(list, s);
    }
  }
  return list;
}

PyObject* Append(RepeatedScalarContainer* self, PyObject* item) {
  cmessage::AssureWritable(self->parent);
  google::protobuf::Message* message = self->message;
  const google::protobuf::FieldDescriptor* field_descriptor =
      self->parent_field->descriptor;

  if (!FIELD_BELONGS_TO_MESSAGE(field_descriptor, message)) {
    PyErr_SetString(
        PyExc_KeyError, "Field does not belong to message!");
    return NULL;
  }

  const google::protobuf::Reflection* reflection = message->GetReflection();
  switch (field_descriptor->cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
      GOOGLE_CHECK_GET_INT32(item, value, NULL);
      reflection->AddInt32(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
      GOOGLE_CHECK_GET_INT64(item, value, NULL);
      reflection->AddInt64(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
      GOOGLE_CHECK_GET_UINT32(item, value, NULL);
      reflection->AddUInt32(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
      GOOGLE_CHECK_GET_UINT64(item, value, NULL);
      reflection->AddUInt64(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
      GOOGLE_CHECK_GET_FLOAT(item, value, NULL);
      reflection->AddFloat(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
      GOOGLE_CHECK_GET_DOUBLE(item, value, NULL);
      reflection->AddDouble(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
      GOOGLE_CHECK_GET_BOOL(item, value, NULL);
      reflection->AddBool(message, field_descriptor, value);
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
      if (!CheckAndSetString(
          item, message, field_descriptor, reflection, true, -1)) {
        return NULL;
      }
      break;
    }
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
      GOOGLE_CHECK_GET_INT32(item, value, NULL);
      const google::protobuf::EnumDescriptor* enum_descriptor =
          field_descriptor->enum_type();
      const google::protobuf::EnumValueDescriptor* enum_value =
          enum_descriptor->FindValueByNumber(value);
      if (enum_value != NULL) {
        reflection->AddEnum(message, field_descriptor, enum_value);
      } else {
        ScopedPyObjectPtr s(PyObject_Str(item));
        if (s != NULL) {
          PyErr_Format(PyExc_ValueError, "Unknown enum value: %s",
                       PyString_AsString(s.get()));
        }
        return NULL;
      }
      break;
    }
    default:
      PyErr_Format(
          PyExc_SystemError, "Adding value to a field of unknown type %d",
          field_descriptor->cpp_type());
      return NULL;
  }

  Py_RETURN_NONE;
}

static int AssSubscript(RepeatedScalarContainer* self,
                        PyObject* slice,
                        PyObject* value) {
  Py_ssize_t from;
  Py_ssize_t to;
  Py_ssize_t step;
  Py_ssize_t length;
  Py_ssize_t slicelength;
  bool create_list = false;

  cmessage::AssureWritable(self->parent);
  google::protobuf::Message* message = self->message;
  const google::protobuf::FieldDescriptor* field_descriptor =
      self->parent_field->descriptor;

#if PY_MAJOR_VERSION < 3
  if (PyInt_Check(slice)) {
    from = to = PyInt_AsLong(slice);
  } else
#endif
  if (PyLong_Check(slice)) {
    from = to = PyLong_AsLong(slice);
  } else if (PySlice_Check(slice)) {
    const google::protobuf::Reflection* reflection = message->GetReflection();
    length = reflection->FieldSize(*message, field_descriptor);
#if PY_MAJOR_VERSION >= 3
    if (PySlice_GetIndicesEx(slice,
#else
    if (PySlice_GetIndicesEx(reinterpret_cast<PySliceObject*>(slice),
#endif
                             length, &from, &to, &step, &slicelength) == -1) {
      return -1;
    }
    create_list = true;
  } else {
    PyErr_SetString(PyExc_TypeError, "list indices must be integers");
    return -1;
  }

  if (value == NULL) {
    return cmessage::InternalDeleteRepeatedField(
        message, field_descriptor, slice, NULL);
  }

  if (!create_list) {
    return AssignItem(self, from, value);
  }

  ScopedPyObjectPtr full_slice(PySlice_New(NULL, NULL, NULL));
  if (full_slice == NULL) {
    return -1;
  }
  ScopedPyObjectPtr new_list(Subscript(self, full_slice));
  if (new_list == NULL) {
    return -1;
  }
  if (PySequence_SetSlice(new_list, from, to, value) < 0) {
    return -1;
  }

  return InternalAssignRepeatedField(self, new_list);
}

PyObject* Extend(RepeatedScalarContainer* self, PyObject* value) {
  cmessage::AssureWritable(self->parent);
  if (PyObject_Not(value)) {
    Py_RETURN_NONE;
  }
  ScopedPyObjectPtr iter(PyObject_GetIter(value));
  if (iter == NULL) {
    PyErr_SetString(PyExc_TypeError, "Value must be iterable");
    return NULL;
  }
  ScopedPyObjectPtr next;
  while ((next.reset(PyIter_Next(iter))) != NULL) {
    if (Append(self, next) == NULL) {
      return NULL;
    }
  }
  if (PyErr_Occurred()) {
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* Insert(RepeatedScalarContainer* self, PyObject* args) {
  Py_ssize_t index;
  PyObject* value;
  if (!PyArg_ParseTuple(args, "lO", &index, &value)) {
    return NULL;
  }
  ScopedPyObjectPtr full_slice(PySlice_New(NULL, NULL, NULL));
  ScopedPyObjectPtr new_list(Subscript(self, full_slice));
  if (PyList_Insert(new_list, index, value) < 0) {
    return NULL;
  }
  int ret = InternalAssignRepeatedField(self, new_list);
  if (ret < 0) {
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* Remove(RepeatedScalarContainer* self, PyObject* value) {
  Py_ssize_t match_index = -1;
  for (Py_ssize_t i = 0; i < Len(self); ++i) {
    ScopedPyObjectPtr elem(Item(self, i));
    if (PyObject_RichCompareBool(elem, value, Py_EQ)) {
      match_index = i;
      break;
    }
  }
  if (match_index == -1) {
    PyErr_SetString(PyExc_ValueError, "remove(x): x not in container");
    return NULL;
  }
  if (AssignItem(self, match_index, NULL) < 0) {
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* RichCompare(RepeatedScalarContainer* self,
                             PyObject* other,
                             int opid) {
  if (opid != Py_EQ && opid != Py_NE) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  // Copy the contents of this repeated scalar container, and other if it is
  // also a repeated scalar container, into Python lists so we can delegate
  // to the list's compare method.

  ScopedPyObjectPtr full_slice(PySlice_New(NULL, NULL, NULL));
  if (full_slice == NULL) {
    return NULL;
  }

  ScopedPyObjectPtr other_list_deleter;
  if (PyObject_TypeCheck(other, &RepeatedScalarContainer_Type)) {
    other_list_deleter.reset(Subscript(
        reinterpret_cast<RepeatedScalarContainer*>(other), full_slice));
    other = other_list_deleter.get();
  }

  ScopedPyObjectPtr list(Subscript(self, full_slice));
  if (list == NULL) {
    return NULL;
  }
  return PyObject_RichCompare(list, other, opid);
}

PyObject* Reduce(RepeatedScalarContainer* unused_self) {
  PyErr_Format(
      PickleError_class,
      "can't pickle repeated message fields, convert to list first");
  return NULL;
}

static PyObject* Sort(RepeatedScalarContainer* self,
                      PyObject* args,
                      PyObject* kwds) {
  // Support the old sort_function argument for backwards
  // compatibility.
  if (kwds != NULL) {
    PyObject* sort_func = PyDict_GetItemString(kwds, "sort_function");
    if (sort_func != NULL) {
      // Must set before deleting as sort_func is a borrowed reference
      // and kwds might be the only thing keeping it alive.
      if (PyDict_SetItemString(kwds, "cmp", sort_func) == -1)
        return NULL;
      if (PyDict_DelItemString(kwds, "sort_function") == -1)
        return NULL;
    }
  }

  ScopedPyObjectPtr full_slice(PySlice_New(NULL, NULL, NULL));
  if (full_slice == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr list(Subscript(self, full_slice));
  if (list == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr m(PyObject_GetAttrString(list, "sort"));
  if (m == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr res(PyObject_Call(m, args, kwds));
  if (res == NULL) {
    return NULL;
  }
  int ret = InternalAssignRepeatedField(self, list);
  if (ret < 0) {
    return NULL;
  }
  Py_RETURN_NONE;
}

static int Init(RepeatedScalarContainer* self,
                PyObject* args,
                PyObject* kwargs) {
  PyObject* py_parent;
  PyObject* py_parent_field;
  if (!PyArg_UnpackTuple(args, "__init__()", 2, 2, &py_parent,
                         &py_parent_field)) {
    return -1;
  }

  if (!PyObject_TypeCheck(py_parent, &CMessage_Type)) {
    PyErr_Format(PyExc_TypeError,
                 "expect %s, but got %s",
                 CMessage_Type.tp_name,
                 Py_TYPE(py_parent)->tp_name);
    return -1;
  }

  if (!PyObject_TypeCheck(py_parent_field, &CFieldDescriptor_Type)) {
    PyErr_Format(PyExc_TypeError,
                 "expect %s, but got %s",
                 CFieldDescriptor_Type.tp_name,
                 Py_TYPE(py_parent_field)->tp_name);
    return -1;
  }

  CMessage* cmessage = reinterpret_cast<CMessage*>(py_parent);
  CFieldDescriptor* cdescriptor = reinterpret_cast<CFieldDescriptor*>(
      py_parent_field);

  if (!FIELD_BELONGS_TO_MESSAGE(cdescriptor->descriptor, cmessage->message)) {
    PyErr_SetString(
        PyExc_KeyError, "Field does not belong to message!");
    return -1;
  }

  self->message = cmessage->message;
  self->parent = cmessage;
  self->parent_field = cdescriptor;
  self->owner = cmessage->owner;
  return 0;
}

// Initializes the underlying Message object of "to" so it becomes a new parent
// repeated scalar, and copies all the values from "from" to it. A child scalar
// container can be released by passing it as both from and to (e.g. making it
// the recipient of the new parent message and copying the values from itself).
static int InitializeAndCopyToParentContainer(
    RepeatedScalarContainer* from,
    RepeatedScalarContainer* to) {
  ScopedPyObjectPtr full_slice(PySlice_New(NULL, NULL, NULL));
  if (full_slice == NULL) {
    return -1;
  }
  ScopedPyObjectPtr values(Subscript(from, full_slice));
  if (values == NULL) {
    return -1;
  }
  google::protobuf::Message* new_message = global_message_factory->GetPrototype(
      from->message->GetDescriptor())->New();
  to->parent = NULL;
  // TODO(anuraag): Document why it's OK to hang on to parent_field,
  //     even though it's a weak reference. It ought to be enough to
  //     hold on to the FieldDescriptor only.
  to->parent_field = from->parent_field;
  to->message = new_message;
  to->owner.reset(new_message);
  if (InternalAssignRepeatedField(to, values) < 0) {
    return -1;
  }
  return 0;
}

int Release(RepeatedScalarContainer* self) {
  return InitializeAndCopyToParentContainer(self, self);
}

PyObject* DeepCopy(RepeatedScalarContainer* self, PyObject* arg) {
  ScopedPyObjectPtr init_args(
      PyTuple_Pack(2, self->parent, self->parent_field));
  PyObject* clone = PyObject_CallObject(
      reinterpret_cast<PyObject*>(&RepeatedScalarContainer_Type), init_args);
  if (clone == NULL) {
    return NULL;
  }
  if (!PyObject_TypeCheck(clone, &RepeatedScalarContainer_Type)) {
    Py_DECREF(clone);
    return NULL;
  }
  if (InitializeAndCopyToParentContainer(
          self, reinterpret_cast<RepeatedScalarContainer*>(clone)) < 0) {
    Py_DECREF(clone);
    return NULL;
  }
  return clone;
}

static void Dealloc(RepeatedScalarContainer* self) {
  self->owner.reset();
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

void SetOwner(RepeatedScalarContainer* self,
              const shared_ptr<Message>& new_owner) {
  self->owner = new_owner;
}

static PySequenceMethods SqMethods = {
  (lenfunc)Len,           /* sq_length */
  0, /* sq_concat */
  0, /* sq_repeat */
  (ssizeargfunc)Item, /* sq_item */
  0, /* sq_slice */
  (ssizeobjargproc)AssignItem /* sq_ass_item */
};

static PyMappingMethods MpMethods = {
  (lenfunc)Len,               /* mp_length */
  (binaryfunc)Subscript,      /* mp_subscript */
  (objobjargproc)AssSubscript, /* mp_ass_subscript */
};

static PyMethodDef Methods[] = {
  { "__deepcopy__", (PyCFunction)DeepCopy, METH_VARARGS,
    "Makes a deep copy of the class." },
  { "__reduce__", (PyCFunction)Reduce, METH_NOARGS,
    "Outputs picklable representation of the repeated field." },
  { "append", (PyCFunction)Append, METH_O,
    "Appends an object to the repeated container." },
  { "extend", (PyCFunction)Extend, METH_O,
    "Appends objects to the repeated container." },
  { "insert", (PyCFunction)Insert, METH_VARARGS,
    "Appends objects to the repeated container." },
  { "remove", (PyCFunction)Remove, METH_O,
    "Removes an object from the repeated container." },
  { "sort", (PyCFunction)Sort, METH_VARARGS | METH_KEYWORDS,
    "Sorts the repeated container."},
  { NULL, NULL }
};

}  // namespace repeated_scalar_container

PyTypeObject RepeatedScalarContainer_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "google.protobuf.internal."
  "cpp._message.RepeatedScalarContainer",  // tp_name
  sizeof(RepeatedScalarContainer),     // tp_basicsize
  0,                                   //  tp_itemsize
  (destructor)repeated_scalar_container::Dealloc,  //  tp_dealloc
  0,                                   //  tp_print
  0,                                   //  tp_getattr
  0,                                   //  tp_setattr
  0,                                   //  tp_compare
  0,                                   //  tp_repr
  0,                                   //  tp_as_number
  &repeated_scalar_container::SqMethods,   //  tp_as_sequence
  &repeated_scalar_container::MpMethods,   //  tp_as_mapping
  0,                                   //  tp_hash
  0,                                   //  tp_call
  0,                                   //  tp_str
  0,                                   //  tp_getattro
  0,                                   //  tp_setattro
  0,                                   //  tp_as_buffer
  Py_TPFLAGS_DEFAULT,                  //  tp_flags
  "A Repeated scalar container",       //  tp_doc
  0,                                   //  tp_traverse
  0,                                   //  tp_clear
  (richcmpfunc)repeated_scalar_container::RichCompare,  //  tp_richcompare
  0,                                   //  tp_weaklistoffset
  0,                                   //  tp_iter
  0,                                   //  tp_iternext
  repeated_scalar_container::Methods,      //  tp_methods
  0,                                   //  tp_members
  0,                                   //  tp_getset
  0,                                   //  tp_base
  0,                                   //  tp_dict
  0,                                   //  tp_descr_get
  0,                                   //  tp_descr_set
  0,                                   //  tp_dictoffset
  (initproc)repeated_scalar_container::Init,  //  tp_init
};

}  // namespace python
}  // namespace protobuf
}  // namespace google
