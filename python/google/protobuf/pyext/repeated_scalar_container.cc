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

#include "google/protobuf/pyext/repeated_scalar_container.h"

#include <cstdint>
#include <memory>
#include <string>

#include "google/protobuf/stubs/common.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
#include "google/protobuf/pyext/descriptor.h"
#include "google/protobuf/pyext/descriptor_pool.h"
#include "google/protobuf/pyext/message.h"
#include "google/protobuf/pyext/scoped_pyobject_ptr.h"

#define PyString_AsString(ob) \
  (PyUnicode_Check(ob) ? PyUnicode_AsUTF8(ob) : PyBytes_AsString(ob))

namespace google {
namespace protobuf {
namespace python {

namespace repeated_scalar_container {

static int InternalAssignRepeatedField(RepeatedScalarContainer* self,
                                       PyObject* list) {
  cmessage::AssureWritable(self->parent);
  Message* message = self->parent->message;
  message->GetReflection()->ClearField(message, self->parent_field_descriptor);
  for (Py_ssize_t i = 0; i < PyList_GET_SIZE(list); ++i) {
    PyObject* value = PyList_GET_ITEM(list, i);
    if (ScopedPyObjectPtr(Append(self, value)) == nullptr) {
      return -1;
    }
  }
  return 0;
}

static Py_ssize_t Len(PyObject* pself) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);
  Message* message = self->parent->message;
  return message->GetReflection()->FieldSize(*message,
                                             self->parent_field_descriptor);
}

static int AssignItem(PyObject* pself, Py_ssize_t index, PyObject* arg) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);

  cmessage::AssureWritable(self->parent);
  Message* message = self->parent->message;
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;

  const Reflection* reflection = message->GetReflection();
  int field_size = reflection->FieldSize(*message, field_descriptor);
  if (index < 0) {
    index = field_size + index;
  }
  if (index < 0 || index >= field_size) {
    PyErr_Format(PyExc_IndexError, "list assignment index (%d) out of range",
                 static_cast<int>(index));
    return -1;
  }

  if (arg == nullptr) {
    ScopedPyObjectPtr py_index(PyLong_FromLong(index));
    return cmessage::DeleteRepeatedField(self->parent, field_descriptor,
                                         py_index.get());
  }

  if (PySequence_Check(arg) && !(PyBytes_Check(arg) || PyUnicode_Check(arg))) {
    PyErr_SetString(PyExc_TypeError, "Value must be scalar");
    return -1;
  }

  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: {
      PROTOBUF_CHECK_GET_INT32(arg, value, -1);
      reflection->SetRepeatedInt32(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      PROTOBUF_CHECK_GET_INT64(arg, value, -1);
      reflection->SetRepeatedInt64(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT32: {
      PROTOBUF_CHECK_GET_UINT32(arg, value, -1);
      reflection->SetRepeatedUInt32(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT64: {
      PROTOBUF_CHECK_GET_UINT64(arg, value, -1);
      reflection->SetRepeatedUInt64(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      PROTOBUF_CHECK_GET_FLOAT(arg, value, -1);
      reflection->SetRepeatedFloat(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      PROTOBUF_CHECK_GET_DOUBLE(arg, value, -1);
      reflection->SetRepeatedDouble(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_BOOL: {
      PROTOBUF_CHECK_GET_BOOL(arg, value, -1);
      reflection->SetRepeatedBool(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      if (!CheckAndSetString(arg, message, field_descriptor, reflection, false,
                             index)) {
        return -1;
      }
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      PROTOBUF_CHECK_GET_INT32(arg, value, -1);
      if (!field_descriptor->legacy_enum_field_treated_as_closed()) {
        reflection->SetRepeatedEnumValue(message, field_descriptor, index,
                                         value);
      } else {
        const EnumDescriptor* enum_descriptor = field_descriptor->enum_type();
        const EnumValueDescriptor* enum_value =
            enum_descriptor->FindValueByNumber(value);
        if (enum_value != nullptr) {
          reflection->SetRepeatedEnum(message, field_descriptor, index,
                                      enum_value);
        } else {
          ScopedPyObjectPtr s(PyObject_Str(arg));
          if (s != nullptr) {
            PyErr_Format(PyExc_ValueError, "Unknown enum value: %s",
                         PyString_AsString(s.get()));
          }
          return -1;
        }
      }
      break;
    }
    default:
      PyErr_Format(PyExc_SystemError,
                   "Adding value to a field of unknown type %d",
                   field_descriptor->cpp_type());
      return -1;
  }
  return 0;
}

static PyObject* Item(PyObject* pself, Py_ssize_t index) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);

  Message* message = self->parent->message;
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;
  const Reflection* reflection = message->GetReflection();

  int field_size = reflection->FieldSize(*message, field_descriptor);
  if (index < 0) {
    index = field_size + index;
  }
  if (index < 0 || index >= field_size) {
    PyErr_Format(PyExc_IndexError, "list index (%zd) out of range", index);
    return nullptr;
  }

  PyObject* result = nullptr;
  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: {
      int32_t value =
          reflection->GetRepeatedInt32(*message, field_descriptor, index);
      result = PyLong_FromLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      int64_t value =
          reflection->GetRepeatedInt64(*message, field_descriptor, index);
      result = PyLong_FromLongLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT32: {
      uint32_t value =
          reflection->GetRepeatedUInt32(*message, field_descriptor, index);
      result = PyLong_FromLongLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT64: {
      uint64_t value =
          reflection->GetRepeatedUInt64(*message, field_descriptor, index);
      result = PyLong_FromUnsignedLongLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      float value =
          reflection->GetRepeatedFloat(*message, field_descriptor, index);
      result = PyFloat_FromDouble(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      double value =
          reflection->GetRepeatedDouble(*message, field_descriptor, index);
      result = PyFloat_FromDouble(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_BOOL: {
      bool value =
          reflection->GetRepeatedBool(*message, field_descriptor, index);
      result = PyBool_FromLong(value ? 1 : 0);
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      const EnumValueDescriptor* enum_value =
          message->GetReflection()->GetRepeatedEnum(*message, field_descriptor,
                                                    index);
      result = PyLong_FromLong(enum_value->number());
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      std::string scratch;
      const std::string& value = reflection->GetRepeatedStringReference(
          *message, field_descriptor, index, &scratch);
      result = ToStringObject(field_descriptor, value);
      break;
    }
    default:
      PyErr_Format(PyExc_SystemError,
                   "Getting value from a repeated field of unknown type %d",
                   field_descriptor->cpp_type());
  }

  return result;
}

static PyObject* Subscript(PyObject* pself, PyObject* slice) {
  Py_ssize_t from;
  Py_ssize_t to;
  Py_ssize_t step;
  Py_ssize_t length;
  Py_ssize_t slicelength;
  bool return_list = false;
  if (PyLong_Check(slice)) {
    from = to = PyLong_AsLong(slice);
  } else if (PyIndex_Check(slice)) {
    from = to = PyNumber_AsSsize_t(slice, PyExc_ValueError);
    if (from == -1 && PyErr_Occurred()) {
      return nullptr;
    }
  } else if (PySlice_Check(slice)) {
    length = Len(pself);
    if (PySlice_GetIndicesEx(slice, length, &from, &to, &step, &slicelength) ==
        -1) {
      return nullptr;
    }
    return_list = true;
  } else {
    PyErr_SetString(PyExc_TypeError, "list indices must be integers");
    return nullptr;
  }

  if (!return_list) {
    return Item(pself, from);
  }

  PyObject* list = PyList_New(0);
  if (list == nullptr) {
    return nullptr;
  }
  if (from <= to) {
    if (step < 0) {
      return list;
    }
    for (Py_ssize_t index = from; index < to; index += step) {
      if (index < 0 || index >= length) {
        break;
      }
      ScopedPyObjectPtr s(Item(pself, index));
      PyList_Append(list, s.get());
    }
  } else {
    if (step > 0) {
      return list;
    }
    for (Py_ssize_t index = from; index > to; index += step) {
      if (index < 0 || index >= length) {
        break;
      }
      ScopedPyObjectPtr s(Item(pself, index));
      PyList_Append(list, s.get());
    }
  }
  return list;
}

PyObject* Append(RepeatedScalarContainer* self, PyObject* item) {
  cmessage::AssureWritable(self->parent);
  Message* message = self->parent->message;
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;

  const Reflection* reflection = message->GetReflection();
  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: {
      PROTOBUF_CHECK_GET_INT32(item, value, nullptr);
      reflection->AddInt32(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      PROTOBUF_CHECK_GET_INT64(item, value, nullptr);
      reflection->AddInt64(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT32: {
      PROTOBUF_CHECK_GET_UINT32(item, value, nullptr);
      reflection->AddUInt32(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT64: {
      PROTOBUF_CHECK_GET_UINT64(item, value, nullptr);
      reflection->AddUInt64(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      PROTOBUF_CHECK_GET_FLOAT(item, value, nullptr);
      reflection->AddFloat(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      PROTOBUF_CHECK_GET_DOUBLE(item, value, nullptr);
      reflection->AddDouble(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_BOOL: {
      PROTOBUF_CHECK_GET_BOOL(item, value, nullptr);
      reflection->AddBool(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      if (!CheckAndSetString(item, message, field_descriptor, reflection, true,
                             -1)) {
        return nullptr;
      }
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      PROTOBUF_CHECK_GET_INT32(item, value, nullptr);
      if (!field_descriptor->legacy_enum_field_treated_as_closed()) {
        reflection->AddEnumValue(message, field_descriptor, value);
      } else {
        const EnumDescriptor* enum_descriptor = field_descriptor->enum_type();
        const EnumValueDescriptor* enum_value =
            enum_descriptor->FindValueByNumber(value);
        if (enum_value != nullptr) {
          reflection->AddEnum(message, field_descriptor, enum_value);
        } else {
          ScopedPyObjectPtr s(PyObject_Str(item));
          if (s != nullptr) {
            PyErr_Format(PyExc_ValueError, "Unknown enum value: %s",
                         PyString_AsString(s.get()));
          }
          return nullptr;
        }
      }
      break;
    }
    default:
      PyErr_Format(PyExc_SystemError,
                   "Adding value to a field of unknown type %d",
                   field_descriptor->cpp_type());
      return nullptr;
  }

  Py_RETURN_NONE;
}

static PyObject* AppendMethod(PyObject* self, PyObject* item) {
  return Append(reinterpret_cast<RepeatedScalarContainer*>(self), item);
}

static int AssSubscript(PyObject* pself, PyObject* slice, PyObject* value) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);

  Py_ssize_t from;
  Py_ssize_t to;
  Py_ssize_t step;
  Py_ssize_t length;
  Py_ssize_t slicelength;
  bool create_list = false;

  cmessage::AssureWritable(self->parent);
  Message* message = self->parent->message;
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;

  if (PyLong_Check(slice)) {
    from = to = PyLong_AsLong(slice);
  } else if (PySlice_Check(slice)) {
    const Reflection* reflection = message->GetReflection();
    length = reflection->FieldSize(*message, field_descriptor);
    if (PySlice_GetIndicesEx(slice, length, &from, &to, &step, &slicelength) ==
        -1) {
      return -1;
    }
    create_list = true;
  } else {
    PyErr_SetString(PyExc_TypeError, "list indices must be integers");
    return -1;
  }

  if (value == nullptr) {
    return cmessage::DeleteRepeatedField(self->parent, field_descriptor, slice);
  }

  if (!create_list) {
    return AssignItem(pself, from, value);
  }

  ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
  if (full_slice == nullptr) {
    return -1;
  }
  ScopedPyObjectPtr new_list(Subscript(pself, full_slice.get()));
  if (new_list == nullptr) {
    return -1;
  }
  if (PySequence_SetSlice(new_list.get(), from, to, value) < 0) {
    return -1;
  }

  return InternalAssignRepeatedField(self, new_list.get());
}

PyObject* Extend(RepeatedScalarContainer* self, PyObject* value) {
  cmessage::AssureWritable(self->parent);

  // TODO(b/286557203): Remove this in OSS
  if (value == Py_None) {
    PyErr_Warn(nullptr,
               "Value is not iterable. Please remove the wrong usage."
               " This will be changed to raise TypeError soon.");
    Py_RETURN_NONE;
  }
  if ((Py_TYPE(value)->tp_as_sequence == nullptr) && PyObject_Not(value)) {
    PyErr_Warn(nullptr,
               "Value is not iterable. Please remove the wrong usage."
               " This will be changed to raise TypeError soon.");
    Py_RETURN_NONE;
  }

  ScopedPyObjectPtr iter(PyObject_GetIter(value));
  if (iter == nullptr) {
    PyErr_SetString(PyExc_TypeError, "Value must be iterable");
    return nullptr;
  }
  ScopedPyObjectPtr next;
  while ((next.reset(PyIter_Next(iter.get()))) != nullptr) {
    if (ScopedPyObjectPtr(Append(self, next.get())) == nullptr) {
      return nullptr;
    }
  }
  if (PyErr_Occurred()) {
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* Insert(PyObject* pself, PyObject* args) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);

  Py_ssize_t index;
  PyObject* value;
  if (!PyArg_ParseTuple(args, "lO", &index, &value)) {
    return nullptr;
  }
  ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
  ScopedPyObjectPtr new_list(Subscript(pself, full_slice.get()));
  if (PyList_Insert(new_list.get(), index, value) < 0) {
    return nullptr;
  }
  int ret = InternalAssignRepeatedField(self, new_list.get());
  if (ret < 0) {
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* Remove(PyObject* pself, PyObject* value) {
  Py_ssize_t match_index = -1;
  for (Py_ssize_t i = 0; i < Len(pself); ++i) {
    ScopedPyObjectPtr elem(Item(pself, i));
    if (PyObject_RichCompareBool(elem.get(), value, Py_EQ)) {
      match_index = i;
      break;
    }
  }
  if (match_index == -1) {
    PyErr_SetString(PyExc_ValueError, "remove(x): x not in container");
    return nullptr;
  }
  if (AssignItem(pself, match_index, nullptr) < 0) {
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* ExtendMethod(PyObject* self, PyObject* value) {
  return Extend(reinterpret_cast<RepeatedScalarContainer*>(self), value);
}

static PyObject* RichCompare(PyObject* pself, PyObject* other, int opid) {
  if (opid != Py_EQ && opid != Py_NE) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  // Copy the contents of this repeated scalar container, and other if it is
  // also a repeated scalar container, into Python lists so we can delegate
  // to the list's compare method.

  ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
  if (full_slice == nullptr) {
    return nullptr;
  }

  ScopedPyObjectPtr other_list_deleter;
  if (PyObject_TypeCheck(other, &RepeatedScalarContainer_Type)) {
    other_list_deleter.reset(Subscript(other, full_slice.get()));
    other = other_list_deleter.get();
  }

  ScopedPyObjectPtr list(Subscript(pself, full_slice.get()));
  if (list == nullptr) {
    return nullptr;
  }
  return PyObject_RichCompare(list.get(), other, opid);
}

PyObject* Reduce(PyObject* unused_self, PyObject* unused_other) {
  PyErr_Format(PickleError_class,
               "can't pickle repeated message fields, convert to list first");
  return nullptr;
}

static PyObject* Sort(PyObject* pself, PyObject* args, PyObject* kwds) {
  // Support the old sort_function argument for backwards
  // compatibility.
  if (kwds != nullptr) {
    PyObject* sort_func = PyDict_GetItemString(kwds, "sort_function");
    if (sort_func != nullptr) {
      // Must set before deleting as sort_func is a borrowed reference
      // and kwds might be the only thing keeping it alive.
      if (PyDict_SetItemString(kwds, "cmp", sort_func) == -1) return nullptr;
      if (PyDict_DelItemString(kwds, "sort_function") == -1) return nullptr;
    }
  }

  ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
  if (full_slice == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr list(Subscript(pself, full_slice.get()));
  if (list == nullptr) {
    return nullptr;
  }
  if (PyList_GET_SIZE(list.get()) == 0) {
    Py_RETURN_NONE;
  }
  ScopedPyObjectPtr m(PyObject_GetAttrString(list.get(), "sort"));
  if (m == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr res(PyObject_Call(m.get(), args, kwds));
  if (res == nullptr) {
    return nullptr;
  }
  int ret = InternalAssignRepeatedField(
      reinterpret_cast<RepeatedScalarContainer*>(pself), list.get());
  if (ret < 0) {
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* Reverse(PyObject* pself) {
  ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
  if (full_slice == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr list(Subscript(pself, full_slice.get()));
  if (list == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr res(PyObject_CallMethod(list.get(), "reverse", nullptr));
  if (res == nullptr) {
    return nullptr;
  }
  int ret = InternalAssignRepeatedField(
      reinterpret_cast<RepeatedScalarContainer*>(pself), list.get());
  if (ret < 0) {
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* Pop(PyObject* pself, PyObject* args) {
  Py_ssize_t index = -1;
  if (!PyArg_ParseTuple(args, "|n", &index)) {
    return nullptr;
  }
  PyObject* item = Item(pself, index);
  if (item == nullptr) {
    PyErr_Format(PyExc_IndexError, "list index (%zd) out of range", index);
    return nullptr;
  }
  if (AssignItem(pself, index, nullptr) < 0) {
    return nullptr;
  }
  return item;
}

static PyObject* ToStr(PyObject* pself) {
  ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
  if (full_slice == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr list(Subscript(pself, full_slice.get()));
  if (list == nullptr) {
    return nullptr;
  }
  return PyObject_Repr(list.get());
}

static PyObject* MergeFrom(PyObject* pself, PyObject* arg) {
  return Extend(reinterpret_cast<RepeatedScalarContainer*>(pself), arg);
}

// The private constructor of RepeatedScalarContainer objects.
RepeatedScalarContainer* NewContainer(
    CMessage* parent, const FieldDescriptor* parent_field_descriptor) {
  if (!CheckFieldBelongsToMessage(parent_field_descriptor, parent->message)) {
    return nullptr;
  }

  RepeatedScalarContainer* self = reinterpret_cast<RepeatedScalarContainer*>(
      PyType_GenericAlloc(&RepeatedScalarContainer_Type, 0));
  if (self == nullptr) {
    return nullptr;
  }

  Py_INCREF(parent);
  self->parent = parent;
  self->parent_field_descriptor = parent_field_descriptor;

  return self;
}

PyObject* DeepCopy(PyObject* pself, PyObject* arg) {
  return reinterpret_cast<RepeatedScalarContainer*>(pself)->DeepCopy();
}

static void Dealloc(PyObject* pself) {
  reinterpret_cast<RepeatedScalarContainer*>(pself)->RemoveFromParentCache();
  Py_TYPE(pself)->tp_free(pself);
}

static PySequenceMethods SqMethods = {
    Len,       /* sq_length */
    nullptr,   /* sq_concat */
    nullptr,   /* sq_repeat */
    Item,      /* sq_item */
    nullptr,   /* sq_slice */
    AssignItem /* sq_ass_item */
};

static PyMappingMethods MpMethods = {
    Len,          /* mp_length */
    Subscript,    /* mp_subscript */
    AssSubscript, /* mp_ass_subscript */
};

static PyMethodDef Methods[] = {
    {"__deepcopy__", DeepCopy, METH_VARARGS, "Makes a deep copy of the class."},
    {"__reduce__", Reduce, METH_NOARGS,
     "Outputs picklable representation of the repeated field."},
    {"append", AppendMethod, METH_O,
     "Appends an object to the repeated container."},
    {"extend", ExtendMethod, METH_O,
     "Appends objects to the repeated container."},
    {"insert", Insert, METH_VARARGS,
     "Inserts an object at the specified position in the container."},
    {"pop", Pop, METH_VARARGS,
     "Removes an object from the repeated container and returns it."},
    {"remove", Remove, METH_O,
     "Removes an object from the repeated container."},
    {"sort", reinterpret_cast<PyCFunction>(Sort), METH_VARARGS | METH_KEYWORDS,
     "Sorts the repeated container."},
    {"reverse", reinterpret_cast<PyCFunction>(Reverse), METH_NOARGS,
     "Reverses elements order of the repeated container."},
    {"MergeFrom", static_cast<PyCFunction>(MergeFrom), METH_O,
     "Merges a repeated container into the current container."},
    {nullptr, nullptr}};

}  // namespace repeated_scalar_container

PyTypeObject RepeatedScalarContainer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) FULL_MODULE_NAME
    ".RepeatedScalarContainer",          // tp_name
    sizeof(RepeatedScalarContainer),     // tp_basicsize
    0,                                   //  tp_itemsize
    repeated_scalar_container::Dealloc,  //  tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
    0,  //  tp_vectorcall_offset
#else
    nullptr,             //  tp_print
#endif
    nullptr,                                //  tp_getattr
    nullptr,                                //  tp_setattr
    nullptr,                                //  tp_compare
    repeated_scalar_container::ToStr,       //  tp_repr
    nullptr,                                //  tp_as_number
    &repeated_scalar_container::SqMethods,  //  tp_as_sequence
    &repeated_scalar_container::MpMethods,  //  tp_as_mapping
    PyObject_HashNotImplemented,            //  tp_hash
    nullptr,                                //  tp_call
    nullptr,                                //  tp_str
    nullptr,                                //  tp_getattro
    nullptr,                                //  tp_setattro
    nullptr,                                //  tp_as_buffer
#if PY_VERSION_HEX >= 0x030A0000
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_SEQUENCE,  //  tp_flags
#else
    Py_TPFLAGS_DEFAULT,  //  tp_flags
#endif
    "A Repeated scalar container",           //  tp_doc
    nullptr,                                 //  tp_traverse
    nullptr,                                 //  tp_clear
    repeated_scalar_container::RichCompare,  //  tp_richcompare
    0,                                       //  tp_weaklistoffset
    nullptr,                                 //  tp_iter
    nullptr,                                 //  tp_iternext
    repeated_scalar_container::Methods,      //  tp_methods
    nullptr,                                 //  tp_members
    nullptr,                                 //  tp_getset
    nullptr,                                 //  tp_base
    nullptr,                                 //  tp_dict
    nullptr,                                 //  tp_descr_get
    nullptr,                                 //  tp_descr_set
    0,                                       //  tp_dictoffset
    nullptr,                                 //  tp_init
};

}  // namespace python
}  // namespace protobuf
}  // namespace google
