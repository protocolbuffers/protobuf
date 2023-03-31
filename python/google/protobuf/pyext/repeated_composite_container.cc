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

#include "google/protobuf/pyext/repeated_composite_container.h"

#include <memory>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "google/protobuf/pyext/descriptor.h"
#include "google/protobuf/pyext/descriptor_pool.h"
#include "google/protobuf/pyext/message.h"
#include "google/protobuf/pyext/message_factory.h"
#include "google/protobuf/pyext/scoped_pyobject_ptr.h"

namespace google {
namespace protobuf {
namespace python {

namespace repeated_composite_container {

// ---------------------------------------------------------------------
// len()

static Py_ssize_t Length(PyObject* pself) {
  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(pself);

  Message* message = self->parent->message;
  return message->GetReflection()->FieldSize(*message,
                                             self->parent_field_descriptor);
}

// ---------------------------------------------------------------------
// add()

PyObject* Add(RepeatedCompositeContainer* self, PyObject* args,
              PyObject* kwargs) {
  if (cmessage::AssureWritable(self->parent) == -1) return nullptr;
  Message* message = self->parent->message;

  Message* sub_message = message->GetReflection()->AddMessage(
      message, self->parent_field_descriptor,
      self->child_message_class->py_message_factory->message_factory);
  CMessage* cmsg = self->parent->BuildSubMessageFromPointer(
      self->parent_field_descriptor, sub_message, self->child_message_class);

  if (cmessage::InitAttributes(cmsg, args, kwargs) < 0) {
    message->GetReflection()->RemoveLast(message,
                                         self->parent_field_descriptor);
    Py_DECREF(cmsg);
    return nullptr;
  }

  return cmsg->AsPyObject();
}

static PyObject* AddMethod(PyObject* self, PyObject* args, PyObject* kwargs) {
  return Add(reinterpret_cast<RepeatedCompositeContainer*>(self), args, kwargs);
}

// ---------------------------------------------------------------------
// append()

static PyObject* AddMessage(RepeatedCompositeContainer* self, PyObject* value) {
  cmessage::AssureWritable(self->parent);
  PyObject* py_cmsg;
  Message* message = self->parent->message;
  const Reflection* reflection = message->GetReflection();
  py_cmsg = Add(self, nullptr, nullptr);
  if (py_cmsg == nullptr) return nullptr;
  CMessage* cmsg = reinterpret_cast<CMessage*>(py_cmsg);
  if (ScopedPyObjectPtr(cmessage::MergeFrom(cmsg, value)) == nullptr) {
    reflection->RemoveLast(message, self->parent_field_descriptor);
    Py_DECREF(cmsg);
    return nullptr;
  }
  return py_cmsg;
}

static PyObject* AppendMethod(PyObject* pself, PyObject* value) {
  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(pself);
  ScopedPyObjectPtr py_cmsg(AddMessage(self, value));
  if (py_cmsg == nullptr) {
    return nullptr;
  }

  Py_RETURN_NONE;
}

// ---------------------------------------------------------------------
// insert()
static PyObject* Insert(PyObject* pself, PyObject* args) {
  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(pself);

  Py_ssize_t index;
  PyObject* value;
  if (!PyArg_ParseTuple(args, "nO", &index, &value)) {
    return nullptr;
  }

  ScopedPyObjectPtr py_cmsg(AddMessage(self, value));
  if (py_cmsg == nullptr) {
    return nullptr;
  }

  // Swap the element to right position.
  Message* message = self->parent->message;
  const Reflection* reflection = message->GetReflection();
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;
  Py_ssize_t length = reflection->FieldSize(*message, field_descriptor) - 1;
  Py_ssize_t end_index = index;
  if (end_index < 0) end_index += length;
  if (end_index < 0) end_index = 0;
  for (Py_ssize_t i = length; i > end_index; i--) {
    reflection->SwapElements(message, field_descriptor, i, i - 1);
  }

  Py_RETURN_NONE;
}

// ---------------------------------------------------------------------
// extend()

PyObject* Extend(RepeatedCompositeContainer* self, PyObject* value) {
  cmessage::AssureWritable(self->parent);
  ScopedPyObjectPtr iter(PyObject_GetIter(value));
  if (iter == nullptr) {
    PyErr_SetString(PyExc_TypeError, "Value must be iterable");
    return nullptr;
  }
  ScopedPyObjectPtr next;
  while ((next.reset(PyIter_Next(iter.get()))) != nullptr) {
    if (!PyObject_TypeCheck(next.get(), CMessage_Type)) {
      PyErr_SetString(PyExc_TypeError, "Not a cmessage");
      return nullptr;
    }
    ScopedPyObjectPtr new_message(Add(self, nullptr, nullptr));
    if (new_message == nullptr) {
      return nullptr;
    }
    CMessage* new_cmessage = reinterpret_cast<CMessage*>(new_message.get());
    if (ScopedPyObjectPtr(cmessage::MergeFrom(new_cmessage, next.get())) ==
        nullptr) {
      return nullptr;
    }
  }
  if (PyErr_Occurred()) {
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* ExtendMethod(PyObject* self, PyObject* value) {
  return Extend(reinterpret_cast<RepeatedCompositeContainer*>(self), value);
}

PyObject* MergeFrom(RepeatedCompositeContainer* self, PyObject* other) {
  return Extend(self, other);
}

static PyObject* MergeFromMethod(PyObject* self, PyObject* other) {
  return MergeFrom(reinterpret_cast<RepeatedCompositeContainer*>(self), other);
}

// This function does not check the bounds.
static PyObject* GetItem(RepeatedCompositeContainer* self, Py_ssize_t index,
                         Py_ssize_t length = -1) {
  if (length == -1) {
    Message* message = self->parent->message;
    const Reflection* reflection = message->GetReflection();
    length = reflection->FieldSize(*message, self->parent_field_descriptor);
  }
  if (index < 0 || index >= length) {
    PyErr_Format(PyExc_IndexError, "list index (%zd) out of range", index);
    return nullptr;
  }
  Message* message = self->parent->message;
  Message* sub_message = message->GetReflection()->MutableRepeatedMessage(
      message, self->parent_field_descriptor, index);
  return self->parent
      ->BuildSubMessageFromPointer(self->parent_field_descriptor, sub_message,
                                   self->child_message_class)
      ->AsPyObject();
}

PyObject* Subscript(RepeatedCompositeContainer* self, PyObject* item) {
  Message* message = self->parent->message;
  const Reflection* reflection = message->GetReflection();
  Py_ssize_t length =
      reflection->FieldSize(*message, self->parent_field_descriptor);

  if (PyIndex_Check(item)) {
    Py_ssize_t index;
    index = PyNumber_AsSsize_t(item, PyExc_IndexError);
    if (index == -1 && PyErr_Occurred()) return nullptr;
    if (index < 0) index += length;
    return GetItem(self, index, length);
  } else if (PySlice_Check(item)) {
    Py_ssize_t from, to, step, slicelength, cur, i;
    PyObject* result;

    if (PySlice_GetIndicesEx(item, length, &from, &to, &step, &slicelength) ==
        -1) {
      return nullptr;
    }

    if (slicelength <= 0) {
      return PyList_New(0);
    } else {
      result = PyList_New(slicelength);
      if (!result) return nullptr;

      for (cur = from, i = 0; i < slicelength; cur += step, i++) {
        PyList_SET_ITEM(result, i, GetItem(self, cur, length));
      }

      return result;
    }
  } else {
    PyErr_Format(PyExc_TypeError, "indices must be integers, not %.200s",
                 item->ob_type->tp_name);
    return nullptr;
  }
}

static PyObject* SubscriptMethod(PyObject* self, PyObject* slice) {
  return Subscript(reinterpret_cast<RepeatedCompositeContainer*>(self), slice);
}

int AssignSubscript(RepeatedCompositeContainer* self, PyObject* slice,
                    PyObject* value) {
  if (value != nullptr) {
    PyErr_SetString(PyExc_TypeError, "does not support assignment");
    return -1;
  }

  return cmessage::DeleteRepeatedField(self->parent,
                                       self->parent_field_descriptor, slice);
}

static int AssignSubscriptMethod(PyObject* self, PyObject* slice,
                                 PyObject* value) {
  return AssignSubscript(reinterpret_cast<RepeatedCompositeContainer*>(self),
                         slice, value);
}

static PyObject* Remove(PyObject* pself, PyObject* value) {
  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(pself);
  Py_ssize_t len = Length(reinterpret_cast<PyObject*>(self));

  for (Py_ssize_t i = 0; i < len; i++) {
    ScopedPyObjectPtr item(GetItem(self, i, len));
    if (item == nullptr) {
      return nullptr;
    }
    int result = PyObject_RichCompareBool(item.get(), value, Py_EQ);
    if (result < 0) {
      return nullptr;
    }
    if (result) {
      ScopedPyObjectPtr py_index(PyLong_FromSsize_t(i));
      if (AssignSubscript(self, py_index.get(), nullptr) < 0) {
        return nullptr;
      }
      Py_RETURN_NONE;
    }
  }
  PyErr_SetString(PyExc_ValueError, "Item to delete not in list");
  return nullptr;
}

static PyObject* RichCompare(PyObject* pself, PyObject* other, int opid) {
  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(pself);

  if (!PyObject_TypeCheck(other, &RepeatedCompositeContainer_Type)) {
    PyErr_SetString(PyExc_TypeError,
                    "Can only compare repeated composite fields "
                    "against other repeated composite fields.");
    return nullptr;
  }
  if (opid == Py_EQ || opid == Py_NE) {
    // TODO(anuraag): Don't make new lists just for this...
    ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
    if (full_slice == nullptr) {
      return nullptr;
    }
    ScopedPyObjectPtr list(Subscript(self, full_slice.get()));
    if (list == nullptr) {
      return nullptr;
    }
    ScopedPyObjectPtr other_list(
        Subscript(reinterpret_cast<RepeatedCompositeContainer*>(other),
                  full_slice.get()));
    if (other_list == nullptr) {
      return nullptr;
    }
    return PyObject_RichCompare(list.get(), other_list.get(), opid);
  } else {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
}

static PyObject* ToStr(PyObject* pself) {
  ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
  if (full_slice == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr list(Subscript(
      reinterpret_cast<RepeatedCompositeContainer*>(pself), full_slice.get()));
  if (list == nullptr) {
    return nullptr;
  }
  return PyObject_Repr(list.get());
}

// ---------------------------------------------------------------------
// sort()

static void ReorderAttached(RepeatedCompositeContainer* self,
                            PyObject* child_list) {
  Message* message = self->parent->message;
  const Reflection* reflection = message->GetReflection();
  const FieldDescriptor* descriptor = self->parent_field_descriptor;
  const Py_ssize_t length = Length(reinterpret_cast<PyObject*>(self));

  // We need to rearrange things to match python's sort order.
  for (Py_ssize_t i = 0; i < length; ++i) {
    reflection->UnsafeArenaReleaseLast(message, descriptor);
  }
  for (Py_ssize_t i = 0; i < length; ++i) {
    Message* child_message =
        reinterpret_cast<CMessage*>(PyList_GET_ITEM(child_list, i))->message;
    reflection->UnsafeArenaAddAllocatedMessage(message, descriptor,
                                               child_message);
  }
}

// Returns 0 if successful; returns -1 and sets an exception if
// unsuccessful.
static int SortPythonMessages(RepeatedCompositeContainer* self, PyObject* args,
                              PyObject* kwds) {
  ScopedPyObjectPtr child_list(
      PySequence_List(reinterpret_cast<PyObject*>(self)));
  if (child_list == nullptr) {
    return -1;
  }
  ScopedPyObjectPtr m(PyObject_GetAttrString(child_list.get(), "sort"));
  if (m == nullptr) return -1;
  if (ScopedPyObjectPtr(PyObject_Call(m.get(), args, kwds)) == nullptr)
    return -1;
  ReorderAttached(self, child_list.get());
  return 0;
}

static PyObject* Sort(PyObject* pself, PyObject* args, PyObject* kwds) {
  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(pself);

  // Support the old sort_function argument for backwards
  // compatibility.
  if (kwds != nullptr) {
    PyObject* sort_func = PyDict_GetItemString(kwds, "sort_function");
    if (sort_func != nullptr) {
      // Must set before deleting as sort_func is a borrowed reference
      // and kwds might be the only thing keeping it alive.
      PyDict_SetItemString(kwds, "cmp", sort_func);
      PyDict_DelItemString(kwds, "sort_function");
    }
  }

  if (SortPythonMessages(self, args, kwds) < 0) {
    return nullptr;
  }
  Py_RETURN_NONE;
}

// ---------------------------------------------------------------------
// reverse()

// Returns 0 if successful; returns -1 and sets an exception if
// unsuccessful.
static int ReversePythonMessages(RepeatedCompositeContainer* self) {
  ScopedPyObjectPtr child_list(
      PySequence_List(reinterpret_cast<PyObject*>(self)));
  if (child_list == nullptr) {
    return -1;
  }
  if (ScopedPyObjectPtr(
          PyObject_CallMethod(child_list.get(), "reverse", nullptr)) == nullptr)
    return -1;
  ReorderAttached(self, child_list.get());
  return 0;
}

static PyObject* Reverse(PyObject* pself) {
  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(pself);

  if (ReversePythonMessages(self) < 0) {
    return nullptr;
  }
  Py_RETURN_NONE;
}

// ---------------------------------------------------------------------

static PyObject* Item(PyObject* pself, Py_ssize_t index) {
  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(pself);
  return GetItem(self, index);
}

static PyObject* Pop(PyObject* pself, PyObject* args) {
  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(pself);

  Py_ssize_t index = -1;
  if (!PyArg_ParseTuple(args, "|n", &index)) {
    return nullptr;
  }
  Py_ssize_t length = Length(pself);
  if (index < 0) index += length;
  PyObject* item = GetItem(self, index, length);
  if (item == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr py_index(PyLong_FromSsize_t(index));
  if (AssignSubscript(self, py_index.get(), nullptr) < 0) {
    return nullptr;
  }
  return item;
}

PyObject* DeepCopy(PyObject* pself, PyObject* arg) {
  return reinterpret_cast<RepeatedCompositeContainer*>(pself)->DeepCopy();
}

// The private constructor of RepeatedCompositeContainer objects.
RepeatedCompositeContainer* NewContainer(
    CMessage* parent, const FieldDescriptor* parent_field_descriptor,
    CMessageClass* child_message_class) {
  if (!CheckFieldBelongsToMessage(parent_field_descriptor, parent->message)) {
    return nullptr;
  }

  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(
          PyType_GenericAlloc(&RepeatedCompositeContainer_Type, 0));
  if (self == nullptr) {
    return nullptr;
  }

  Py_INCREF(parent);
  self->parent = parent;
  self->parent_field_descriptor = parent_field_descriptor;
  Py_INCREF(child_message_class);
  self->child_message_class = child_message_class;
  return self;
}

static void Dealloc(PyObject* pself) {
  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(pself);
  self->RemoveFromParentCache();
  Py_CLEAR(self->child_message_class);
  Py_TYPE(self)->tp_free(pself);
}

static PySequenceMethods SqMethods = {
    Length,  /* sq_length */
    nullptr, /* sq_concat */
    nullptr, /* sq_repeat */
    Item     /* sq_item */
};

static PyMappingMethods MpMethods = {
    Length,                /* mp_length */
    SubscriptMethod,       /* mp_subscript */
    AssignSubscriptMethod, /* mp_ass_subscript */
};

static PyMethodDef Methods[] = {
    {"__deepcopy__", DeepCopy, METH_VARARGS, "Makes a deep copy of the class."},
    {"add", reinterpret_cast<PyCFunction>(AddMethod),
     METH_VARARGS | METH_KEYWORDS, "Adds an object to the repeated container."},
    {"append", AppendMethod, METH_O,
     "Appends a message to the end of the repeated container."},
    {"insert", Insert, METH_VARARGS,
     "Inserts a message before the specified index."},
    {"extend", ExtendMethod, METH_O, "Adds objects to the repeated container."},
    {"pop", Pop, METH_VARARGS,
     "Removes an object from the repeated container and returns it."},
    {"remove", Remove, METH_O,
     "Removes an object from the repeated container."},
    {"sort", reinterpret_cast<PyCFunction>(Sort), METH_VARARGS | METH_KEYWORDS,
     "Sorts the repeated container."},
    {"reverse", reinterpret_cast<PyCFunction>(Reverse), METH_NOARGS,
     "Reverses elements order of the repeated container."},
    {"MergeFrom", MergeFromMethod, METH_O,
     "Adds objects to the repeated container."},
    {nullptr, nullptr}};

}  // namespace repeated_composite_container

PyTypeObject RepeatedCompositeContainer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) FULL_MODULE_NAME
    ".RepeatedCompositeContainer",          // tp_name
    sizeof(RepeatedCompositeContainer),     // tp_basicsize
    0,                                      //  tp_itemsize
    repeated_composite_container::Dealloc,  //  tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
    0,  //  tp_vectorcall_offset
#else
    nullptr,             //  tp_print
#endif
    nullptr,                                   //  tp_getattr
    nullptr,                                   //  tp_setattr
    nullptr,                                   //  tp_compare
    repeated_composite_container::ToStr,       //  tp_repr
    nullptr,                                   //  tp_as_number
    &repeated_composite_container::SqMethods,  //  tp_as_sequence
    &repeated_composite_container::MpMethods,  //  tp_as_mapping
    PyObject_HashNotImplemented,               //  tp_hash
    nullptr,                                   //  tp_call
    nullptr,                                   //  tp_str
    nullptr,                                   //  tp_getattro
    nullptr,                                   //  tp_setattro
    nullptr,                                   //  tp_as_buffer
#if PY_VERSION_HEX >= 0x030A0000
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_SEQUENCE,  //  tp_flags
#else
    Py_TPFLAGS_DEFAULT,  //  tp_flags
#endif
    "A Repeated scalar container",              //  tp_doc
    nullptr,                                    //  tp_traverse
    nullptr,                                    //  tp_clear
    repeated_composite_container::RichCompare,  //  tp_richcompare
    0,                                          //  tp_weaklistoffset
    nullptr,                                    //  tp_iter
    nullptr,                                    //  tp_iternext
    repeated_composite_container::Methods,      //  tp_methods
    nullptr,                                    //  tp_members
    nullptr,                                    //  tp_getset
    nullptr,                                    //  tp_base
    nullptr,                                    //  tp_dict
    nullptr,                                    //  tp_descr_get
    nullptr,                                    //  tp_descr_set
    0,                                          //  tp_dictoffset
    nullptr,                                    //  tp_init
};

}  // namespace python
}  // namespace protobuf
}  // namespace google
