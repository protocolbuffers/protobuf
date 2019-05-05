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

#include <google/protobuf/pyext/repeated_composite_container.h>

#include <memory>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/descriptor_pool.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/message_factory.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>
#include <google/protobuf/reflection.h>
#include <google/protobuf/stubs/map_util.h>

#if PY_MAJOR_VERSION >= 3
  #define PyInt_Check PyLong_Check
  #define PyInt_AsLong PyLong_AsLong
  #define PyInt_FromLong PyLong_FromLong
#endif

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
  if (cmessage::AssureWritable(self->parent) == -1)
    return NULL;
  Message* message = self->parent->message;

  Message* sub_message =
      message->GetReflection()->AddMessage(
          message,
          self->parent_field_descriptor,
          self->child_message_class->py_message_factory->message_factory);
  CMessage* cmsg = self->parent->BuildSubMessageFromPointer(
      self->parent_field_descriptor, sub_message, self->child_message_class);

  if (cmessage::InitAttributes(cmsg, args, kwargs) < 0) {
    message->GetReflection()->RemoveLast(
        message, self->parent_field_descriptor);
    Py_DECREF(cmsg);
    return NULL;
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
    reflection->RemoveLast(
        message, self->parent_field_descriptor);
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
  for (Py_ssize_t i = length; i > end_index; i --) {
    reflection->SwapElements(message, field_descriptor, i, i - 1);
  }

  Py_RETURN_NONE;
}

// ---------------------------------------------------------------------
// extend()

PyObject* Extend(RepeatedCompositeContainer* self, PyObject* value) {
  cmessage::AssureWritable(self->parent);
  ScopedPyObjectPtr iter(PyObject_GetIter(value));
  if (iter == NULL) {
    PyErr_SetString(PyExc_TypeError, "Value must be iterable");
    return NULL;
  }
  ScopedPyObjectPtr next;
  while ((next.reset(PyIter_Next(iter.get()))) != NULL) {
    if (!PyObject_TypeCheck(next.get(), CMessage_Type)) {
      PyErr_SetString(PyExc_TypeError, "Not a cmessage");
      return NULL;
    }
    ScopedPyObjectPtr new_message(Add(self, NULL, NULL));
    if (new_message == NULL) {
      return NULL;
    }
    CMessage* new_cmessage = reinterpret_cast<CMessage*>(new_message.get());
    if (ScopedPyObjectPtr(cmessage::MergeFrom(new_cmessage, next.get())) ==
        NULL) {
      return NULL;
    }
  }
  if (PyErr_Occurred()) {
    return NULL;
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
    return NULL;
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
    if (index == -1 && PyErr_Occurred()) return NULL;
    if (index < 0) index += length;
    return GetItem(self, index, length);
  } else if (PySlice_Check(item)) {
    Py_ssize_t from, to, step, slicelength, cur, i;
    PyObject* result;

#if PY_MAJOR_VERSION >= 3
    if (PySlice_GetIndicesEx(item,
                             length, &from, &to, &step, &slicelength) == -1) {
#else
    if (PySlice_GetIndicesEx(reinterpret_cast<PySliceObject*>(item),
                             length, &from, &to, &step, &slicelength) == -1) {
#endif
      return NULL;
    }

    if (slicelength <= 0) {
      return PyList_New(0);
    } else {
      result = PyList_New(slicelength);
      if (!result) return NULL;

      for (cur = from, i = 0; i < slicelength; cur += step, i++) {
        PyList_SET_ITEM(result, i, GetItem(self, cur, length));
      }

      return result;
    }
  } else {
    PyErr_Format(PyExc_TypeError, "indices must be integers, not %.200s",
                 item->ob_type->tp_name);
    return NULL;
  }
}

static PyObject* SubscriptMethod(PyObject* self, PyObject* slice) {
  return Subscript(reinterpret_cast<RepeatedCompositeContainer*>(self), slice);
}

int AssignSubscript(RepeatedCompositeContainer* self,
                    PyObject* slice,
                    PyObject* value) {
  if (value != NULL) {
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
    if (item == NULL) {
      return NULL;
    }
    int result = PyObject_RichCompareBool(item.get(), value, Py_EQ);
    if (result < 0) {
      return NULL;
    }
    if (result) {
      ScopedPyObjectPtr py_index(PyLong_FromSsize_t(i));
      if (AssignSubscript(self, py_index.get(), NULL) < 0) {
        return NULL;
      }
      Py_RETURN_NONE;
    }
  }
  PyErr_SetString(PyExc_ValueError, "Item to delete not in list");
  return NULL;
}

static PyObject* RichCompare(PyObject* pself, PyObject* other, int opid) {
  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(pself);

  if (!PyObject_TypeCheck(other, &RepeatedCompositeContainer_Type)) {
    PyErr_SetString(PyExc_TypeError,
                    "Can only compare repeated composite fields "
                    "against other repeated composite fields.");
    return NULL;
  }
  if (opid == Py_EQ || opid == Py_NE) {
    // TODO(anuraag): Don't make new lists just for this...
    ScopedPyObjectPtr full_slice(PySlice_New(NULL, NULL, NULL));
    if (full_slice == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr list(Subscript(self, full_slice.get()));
    if (list == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr other_list(
        Subscript(reinterpret_cast<RepeatedCompositeContainer*>(other),
                  full_slice.get()));
    if (other_list == NULL) {
      return NULL;
    }
    return PyObject_RichCompare(list.get(), other_list.get(), opid);
  } else {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
}

static PyObject* ToStr(PyObject* pself) {
  ScopedPyObjectPtr full_slice(PySlice_New(NULL, NULL, NULL));
  if (full_slice == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr list(Subscript(
      reinterpret_cast<RepeatedCompositeContainer*>(pself), full_slice.get()));
  if (list == NULL) {
    return NULL;
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

  // Since Python protobuf objects are never arena-allocated, adding and
  // removing message pointers to the underlying array is just updating
  // pointers.
  for (Py_ssize_t i = 0; i < length; ++i)
    reflection->ReleaseLast(message, descriptor);

  for (Py_ssize_t i = 0; i < length; ++i) {
    CMessage* py_cmsg = reinterpret_cast<CMessage*>(
        PyList_GET_ITEM(child_list, i));
    reflection->AddAllocatedMessage(message, descriptor, py_cmsg->message);
  }
}

// Returns 0 if successful; returns -1 and sets an exception if
// unsuccessful.
static int SortPythonMessages(RepeatedCompositeContainer* self,
                               PyObject* args,
                               PyObject* kwds) {
  ScopedPyObjectPtr child_list(
      PySequence_List(reinterpret_cast<PyObject*>(self)));
  if (child_list == NULL) {
    return -1;
  }
  ScopedPyObjectPtr m(PyObject_GetAttrString(child_list.get(), "sort"));
  if (m == NULL)
    return -1;
  if (ScopedPyObjectPtr(PyObject_Call(m.get(), args, kwds)) == NULL)
    return -1;
  ReorderAttached(self, child_list.get());
  return 0;
}

static PyObject* Sort(PyObject* pself, PyObject* args, PyObject* kwds) {
  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(pself);

  // Support the old sort_function argument for backwards
  // compatibility.
  if (kwds != NULL) {
    PyObject* sort_func = PyDict_GetItemString(kwds, "sort_function");
    if (sort_func != NULL) {
      // Must set before deleting as sort_func is a borrowed reference
      // and kwds might be the only thing keeping it alive.
      PyDict_SetItemString(kwds, "cmp", sort_func);
      PyDict_DelItemString(kwds, "sort_function");
    }
  }

  if (SortPythonMessages(self, args, kwds) < 0) {
    return NULL;
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
    return NULL;
  }
  Py_ssize_t length = Length(pself);
  if (index < 0) index += length;
  PyObject* item = GetItem(self, index, length);
  if (item == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr py_index(PyLong_FromSsize_t(index));
  if (AssignSubscript(self, py_index.get(), NULL) < 0) {
    return NULL;
  }
  return item;
}

PyObject* DeepCopy(PyObject* pself, PyObject* arg) {
  return reinterpret_cast<RepeatedCompositeContainer*>(pself)->DeepCopy();
}

// The private constructor of RepeatedCompositeContainer objects.
RepeatedCompositeContainer *NewContainer(
    CMessage* parent,
    const FieldDescriptor* parent_field_descriptor,
    CMessageClass* child_message_class) {
  if (!CheckFieldBelongsToMessage(parent_field_descriptor, parent->message)) {
    return NULL;
  }

  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(
          PyType_GenericAlloc(&RepeatedCompositeContainer_Type, 0));
  if (self == NULL) {
    return NULL;
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
  Length,                 /* sq_length */
  0,                      /* sq_concat */
  0,                      /* sq_repeat */
  Item                    /* sq_item */
};

static PyMappingMethods MpMethods = {
  Length,                 /* mp_length */
  SubscriptMethod,        /* mp_subscript */
  AssignSubscriptMethod,  /* mp_ass_subscript */
};

static PyMethodDef Methods[] = {
  { "__deepcopy__", DeepCopy, METH_VARARGS,
    "Makes a deep copy of the class." },
  { "add", (PyCFunction)AddMethod, METH_VARARGS | METH_KEYWORDS,
    "Adds an object to the repeated container." },
  { "append", AppendMethod, METH_O,
    "Appends a message to the end of the repeated container."},
  { "insert", Insert, METH_VARARGS,
    "Inserts a message before the specified index." },
  { "extend", ExtendMethod, METH_O,
    "Adds objects to the repeated container." },
  { "pop", Pop, METH_VARARGS,
    "Removes an object from the repeated container and returns it." },
  { "remove", Remove, METH_O,
    "Removes an object from the repeated container." },
  { "sort", (PyCFunction)Sort, METH_VARARGS | METH_KEYWORDS,
    "Sorts the repeated container." },
  { "MergeFrom", MergeFromMethod, METH_O,
    "Adds objects to the repeated container." },
  { NULL, NULL }
};

}  // namespace repeated_composite_container

PyTypeObject RepeatedCompositeContainer_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  FULL_MODULE_NAME ".RepeatedCompositeContainer",  // tp_name
  sizeof(RepeatedCompositeContainer),  // tp_basicsize
  0,                                   //  tp_itemsize
  repeated_composite_container::Dealloc,  //  tp_dealloc
  0,                                   //  tp_print
  0,                                   //  tp_getattr
  0,                                   //  tp_setattr
  0,                                   //  tp_compare
  repeated_composite_container::ToStr,      //  tp_repr
  0,                                   //  tp_as_number
  &repeated_composite_container::SqMethods,   //  tp_as_sequence
  &repeated_composite_container::MpMethods,   //  tp_as_mapping
  PyObject_HashNotImplemented,         //  tp_hash
  0,                                   //  tp_call
  0,                                   //  tp_str
  0,                                   //  tp_getattro
  0,                                   //  tp_setattro
  0,                                   //  tp_as_buffer
  Py_TPFLAGS_DEFAULT,                  //  tp_flags
  "A Repeated scalar container",       //  tp_doc
  0,                                   //  tp_traverse
  0,                                   //  tp_clear
  repeated_composite_container::RichCompare,  //  tp_richcompare
  0,                                   //  tp_weaklistoffset
  0,                                   //  tp_iter
  0,                                   //  tp_iternext
  repeated_composite_container::Methods,   //  tp_methods
  0,                                   //  tp_members
  0,                                   //  tp_getset
  0,                                   //  tp_base
  0,                                   //  tp_dict
  0,                                   //  tp_descr_get
  0,                                   //  tp_descr_set
  0,                                   //  tp_dictoffset
  0,                                   //  tp_init
};

}  // namespace python
}  // namespace protobuf
}  // namespace google
