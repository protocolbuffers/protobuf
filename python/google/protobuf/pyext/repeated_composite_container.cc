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
  #define PyInt_Check PyLong_Check
  #define PyInt_AsLong PyLong_AsLong
  #define PyInt_FromLong PyLong_FromLong
#endif

namespace google {
namespace protobuf {
namespace python {

extern google::protobuf::DynamicMessageFactory* global_message_factory;

namespace repeated_composite_container {

// TODO(tibell): We might also want to check:
//   GOOGLE_CHECK_NOTNULL((self)->owner.get());
#define GOOGLE_CHECK_ATTACHED(self)             \
  do {                                   \
    GOOGLE_CHECK_NOTNULL((self)->message);      \
    GOOGLE_CHECK_NOTNULL((self)->parent_field); \
  } while (0);

#define GOOGLE_CHECK_RELEASED(self)             \
  do {                                   \
    GOOGLE_CHECK((self)->owner.get() == NULL);  \
    GOOGLE_CHECK((self)->message == NULL);      \
    GOOGLE_CHECK((self)->parent_field == NULL); \
    GOOGLE_CHECK((self)->parent == NULL);       \
  } while (0);

// Returns a new reference.
static PyObject* GetKey(PyObject* x) {
  // Just the identity function.
  Py_INCREF(x);
  return x;
}

#define GET_KEY(keyfunc, value)                                         \
  ((keyfunc) == NULL ?                                                  \
  GetKey((value)) :                                                     \
  PyObject_CallFunctionObjArgs((keyfunc), (value), NULL))

// Converts a comparison function that returns -1, 0, or 1 into a
// less-than predicate.
//
// Returns -1 on error, 1 if x < y, 0 if x >= y.
static int islt(PyObject *x, PyObject *y, PyObject *compare) {
  if (compare == NULL)
    return PyObject_RichCompareBool(x, y, Py_LT);

  ScopedPyObjectPtr res(PyObject_CallFunctionObjArgs(compare, x, y, NULL));
  if (res == NULL)
    return -1;
  if (!PyInt_Check(res)) {
    PyErr_Format(PyExc_TypeError,
                 "comparison function must return int, not %.200s",
                 Py_TYPE(res)->tp_name);
    return -1;
  }
  return PyInt_AsLong(res) < 0;
}

// Copied from uarrsort.c but swaps memcpy swaps with protobuf/python swaps
// TODO(anuraag): Is there a better way to do this then reinventing the wheel?
static int InternalQuickSort(RepeatedCompositeContainer* self,
                             Py_ssize_t start,
                             Py_ssize_t limit,
                             PyObject* cmp,
                             PyObject* keyfunc) {
  if (limit - start <= 1)
    return 0;  // Nothing to sort.

  GOOGLE_CHECK_ATTACHED(self);

  google::protobuf::Message* message = self->message;
  const google::protobuf::Reflection* reflection = message->GetReflection();
  const google::protobuf::FieldDescriptor* descriptor = self->parent_field->descriptor;
  Py_ssize_t left;
  Py_ssize_t right;

  PyObject* children = self->child_messages;

  do {
    left = start;
    right = limit;
    ScopedPyObjectPtr mid(
        GET_KEY(keyfunc, PyList_GET_ITEM(children, (start + limit) / 2)));
    do {
      ScopedPyObjectPtr key(GET_KEY(keyfunc, PyList_GET_ITEM(children, left)));
      int is_lt = islt(key, mid, cmp);
      if (is_lt == -1)
        return -1;
      /* array[left]<x */
      while (is_lt) {
        ++left;
        ScopedPyObjectPtr key(GET_KEY(keyfunc,
                                      PyList_GET_ITEM(children, left)));
        is_lt = islt(key, mid, cmp);
        if (is_lt == -1)
          return -1;
      }
      key.reset(GET_KEY(keyfunc, PyList_GET_ITEM(children, right - 1)));
      is_lt = islt(mid, key, cmp);
      if (is_lt == -1)
        return -1;
      while (is_lt) {
        --right;
        ScopedPyObjectPtr key(GET_KEY(keyfunc,
                                      PyList_GET_ITEM(children, right - 1)));
        is_lt = islt(mid, key, cmp);
        if (is_lt == -1)
          return -1;
      }
      if (left < right) {
        --right;
        if (left < right) {
          reflection->SwapElements(message, descriptor, left, right);
          PyObject* tmp = PyList_GET_ITEM(children, left);
          PyList_SET_ITEM(children, left, PyList_GET_ITEM(children, right));
          PyList_SET_ITEM(children, right, tmp);
        }
        ++left;
      }
    } while (left < right);

    if ((right - start) < (limit - left)) {
      /* sort [start..right[ */
      if (start < (right - 1)) {
        InternalQuickSort(self, start, right, cmp, keyfunc);
      }

      /* sort [left..limit[ */
      start = left;
    } else {
      /* sort [left..limit[ */
      if (left < (limit - 1)) {
        InternalQuickSort(self, left, limit, cmp, keyfunc);
      }

      /* sort [start..right[ */
      limit = right;
    }
  } while (start < (limit - 1));

  return 0;
}

#undef GET_KEY

// ---------------------------------------------------------------------
// len()

static Py_ssize_t Length(RepeatedCompositeContainer* self) {
  google::protobuf::Message* message = self->message;
  if (message != NULL) {
    return message->GetReflection()->FieldSize(*message,
                                               self->parent_field->descriptor);
  } else {
    // The container has been released (i.e. by a call to Clear() or
    // ClearField() on the parent) and thus there's no message.
    return PyList_GET_SIZE(self->child_messages);
  }
}

// Returns 0 if successful; returns -1 and sets an exception if
// unsuccessful.
static int UpdateChildMessages(RepeatedCompositeContainer* self) {
  if (self->message == NULL)
    return 0;

  // A MergeFrom on a parent message could have caused extra messages to be
  // added in the underlying protobuf so add them to our list. They can never
  // be removed in such a way so there's no need to worry about that.
  Py_ssize_t message_length = Length(self);
  Py_ssize_t child_length = PyList_GET_SIZE(self->child_messages);
  google::protobuf::Message* message = self->message;
  const google::protobuf::Reflection* reflection = message->GetReflection();
  for (Py_ssize_t i = child_length; i < message_length; ++i) {
    const Message& sub_message = reflection->GetRepeatedMessage(
        *(self->message), self->parent_field->descriptor, i);
    ScopedPyObjectPtr py_cmsg(cmessage::NewEmpty(self->subclass_init));
    if (py_cmsg == NULL) {
      return -1;
    }
    CMessage* cmsg = reinterpret_cast<CMessage*>(py_cmsg.get());
    cmsg->owner = self->owner;
    cmsg->message = const_cast<google::protobuf::Message*>(&sub_message);
    cmsg->parent = self->parent;
    if (cmessage::InitAttributes(cmsg, NULL, NULL) < 0) {
      return -1;
    }
    PyList_Append(self->child_messages, py_cmsg);
  }
  return 0;
}

// ---------------------------------------------------------------------
// add()

static PyObject* AddToAttached(RepeatedCompositeContainer* self,
                               PyObject* args,
                               PyObject* kwargs) {
  GOOGLE_CHECK_ATTACHED(self);

  if (UpdateChildMessages(self) < 0) {
    return NULL;
  }
  if (cmessage::AssureWritable(self->parent) == -1)
    return NULL;
  google::protobuf::Message* message = self->message;
  google::protobuf::Message* sub_message =
      message->GetReflection()->AddMessage(message,
                                           self->parent_field->descriptor);
  PyObject* py_cmsg = cmessage::NewEmpty(self->subclass_init);
  if (py_cmsg == NULL) {
    return NULL;
  }
  CMessage* cmsg = reinterpret_cast<CMessage*>(py_cmsg);

  cmsg->owner = self->owner;
  cmsg->message = sub_message;
  cmsg->parent = self->parent;
  // cmessage::InitAttributes must be called after cmsg->message has
  // been set.
  if (cmessage::InitAttributes(cmsg, NULL, kwargs) < 0) {
    Py_DECREF(py_cmsg);
    return NULL;
  }
  PyList_Append(self->child_messages, py_cmsg);
  return py_cmsg;
}

static PyObject* AddToReleased(RepeatedCompositeContainer* self,
                               PyObject* args,
                               PyObject* kwargs) {
  GOOGLE_CHECK_RELEASED(self);

  // Create the CMessage
  PyObject* py_cmsg = PyObject_CallObject(self->subclass_init, NULL);
  if (py_cmsg == NULL)
    return NULL;
  CMessage* cmsg = reinterpret_cast<CMessage*>(py_cmsg);
  if (cmessage::InitAttributes(cmsg, NULL, kwargs) < 0) {
    Py_DECREF(py_cmsg);
    return NULL;
  }

  // The Message got created by the call to subclass_init above and
  // it set self->owner to the newly allocated message.

  PyList_Append(self->child_messages, py_cmsg);
  return py_cmsg;
}

PyObject* Add(RepeatedCompositeContainer* self,
              PyObject* args,
              PyObject* kwargs) {
  if (self->message == NULL)
    return AddToReleased(self, args, kwargs);
  else
    return AddToAttached(self, args, kwargs);
}

// ---------------------------------------------------------------------
// extend()

PyObject* Extend(RepeatedCompositeContainer* self, PyObject* value) {
  cmessage::AssureWritable(self->parent);
  if (UpdateChildMessages(self) < 0) {
    return NULL;
  }
  ScopedPyObjectPtr iter(PyObject_GetIter(value));
  if (iter == NULL) {
    PyErr_SetString(PyExc_TypeError, "Value must be iterable");
    return NULL;
  }
  ScopedPyObjectPtr next;
  while ((next.reset(PyIter_Next(iter))) != NULL) {
    if (!PyObject_TypeCheck(next, &CMessage_Type)) {
      PyErr_SetString(PyExc_TypeError, "Not a cmessage");
      return NULL;
    }
    ScopedPyObjectPtr new_message(Add(self, NULL, NULL));
    if (new_message == NULL) {
      return NULL;
    }
    CMessage* new_cmessage = reinterpret_cast<CMessage*>(new_message.get());
    if (cmessage::MergeFrom(new_cmessage, next) == NULL) {
      return NULL;
    }
  }
  if (PyErr_Occurred()) {
    return NULL;
  }
  Py_RETURN_NONE;
}

PyObject* MergeFrom(RepeatedCompositeContainer* self, PyObject* other) {
  if (UpdateChildMessages(self) < 0) {
    return NULL;
  }
  return Extend(self, other);
}

PyObject* Subscript(RepeatedCompositeContainer* self, PyObject* slice) {
  if (UpdateChildMessages(self) < 0) {
    return NULL;
  }
  Py_ssize_t from;
  Py_ssize_t to;
  Py_ssize_t step;
  Py_ssize_t length = Length(self);
  Py_ssize_t slicelength;
  if (PySlice_Check(slice)) {
#if PY_MAJOR_VERSION >= 3
    if (PySlice_GetIndicesEx(slice,
#else
    if (PySlice_GetIndicesEx(reinterpret_cast<PySliceObject*>(slice),
#endif
                             length, &from, &to, &step, &slicelength) == -1) {
      return NULL;
    }
    return PyList_GetSlice(self->child_messages, from, to);
  } else if (PyInt_Check(slice) || PyLong_Check(slice)) {
    from = to = PyLong_AsLong(slice);
    if (from < 0) {
      from = to = length + from;
    }
    PyObject* result = PyList_GetItem(self->child_messages, from);
    if (result == NULL) {
      return NULL;
    }
    Py_INCREF(result);
    return result;
  }
  PyErr_SetString(PyExc_TypeError, "index must be an integer or slice");
  return NULL;
}

int AssignSubscript(RepeatedCompositeContainer* self,
                    PyObject* slice,
                    PyObject* value) {
  if (UpdateChildMessages(self) < 0) {
    return -1;
  }
  if (value != NULL) {
    PyErr_SetString(PyExc_TypeError, "does not support assignment");
    return -1;
  }

  // Delete from the underlying Message, if any.
  if (self->message != NULL) {
    if (cmessage::InternalDeleteRepeatedField(self->message,
                                              self->parent_field->descriptor,
                                              slice,
                                              self->child_messages) < 0) {
      return -1;
    }
  } else {
    Py_ssize_t from;
    Py_ssize_t to;
    Py_ssize_t step;
    Py_ssize_t length = Length(self);
    Py_ssize_t slicelength;
    if (PySlice_Check(slice)) {
#if PY_MAJOR_VERSION >= 3
      if (PySlice_GetIndicesEx(slice,
#else
      if (PySlice_GetIndicesEx(reinterpret_cast<PySliceObject*>(slice),
#endif
                               length, &from, &to, &step, &slicelength) == -1) {
        return -1;
      }
      return PySequence_DelSlice(self->child_messages, from, to);
    } else if (PyInt_Check(slice) || PyLong_Check(slice)) {
      from = to = PyLong_AsLong(slice);
      if (from < 0) {
        from = to = length + from;
      }
      return PySequence_DelItem(self->child_messages, from);
    }
  }

  return 0;
}

static PyObject* Remove(RepeatedCompositeContainer* self, PyObject* value) {
  if (UpdateChildMessages(self) < 0) {
    return NULL;
  }
  Py_ssize_t index = PySequence_Index(self->child_messages, value);
  if (index == -1) {
    return NULL;
  }
  ScopedPyObjectPtr py_index(PyLong_FromLong(index));
  if (AssignSubscript(self, py_index, NULL) < 0) {
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* RichCompare(RepeatedCompositeContainer* self,
                             PyObject* other,
                             int opid) {
  if (UpdateChildMessages(self) < 0) {
    return NULL;
  }
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
    ScopedPyObjectPtr list(Subscript(self, full_slice));
    if (list == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr other_list(
        Subscript(
            reinterpret_cast<RepeatedCompositeContainer*>(other), full_slice));
    if (other_list == NULL) {
      return NULL;
    }
    return PyObject_RichCompare(list, other_list, opid);
  } else {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
}

// ---------------------------------------------------------------------
// sort()

static PyObject* SortAttached(RepeatedCompositeContainer* self,
                              PyObject* args,
                              PyObject* kwds) {
  // Sort the underlying Message array.
  PyObject *compare = NULL;
  int reverse = 0;
  PyObject *keyfunc = NULL;
  static char *kwlist[] = {"cmp", "key", "reverse", 0};

  if (args != NULL) {
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOi:sort",
                                     kwlist, &compare, &keyfunc, &reverse))
      return NULL;
  }
  if (compare == Py_None)
    compare = NULL;
  if (keyfunc == Py_None)
    keyfunc = NULL;

  const Py_ssize_t length = Length(self);
  if (InternalQuickSort(self, 0, length, compare, keyfunc) < 0)
    return NULL;

  // Finally reverse the result if requested.
  if (reverse) {
    google::protobuf::Message* message = self->message;
    const google::protobuf::Reflection* reflection = message->GetReflection();
    const google::protobuf::FieldDescriptor* descriptor = self->parent_field->descriptor;

    // Reverse the Message array.
    for (int i = 0; i < length / 2; ++i)
      reflection->SwapElements(message, descriptor, i, length - i - 1);

    // Reverse the Python list.
    ScopedPyObjectPtr res(PyObject_CallMethod(self->child_messages,
                                              "reverse", NULL));
    if (res == NULL)
      return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject* SortReleased(RepeatedCompositeContainer* self,
                              PyObject* args,
                              PyObject* kwds) {
  ScopedPyObjectPtr m(PyObject_GetAttrString(self->child_messages, "sort"));
  if (m == NULL)
    return NULL;
  if (PyObject_Call(m, args, kwds) == NULL)
    return NULL;
  Py_RETURN_NONE;
}

static PyObject* Sort(RepeatedCompositeContainer* self,
                      PyObject* args,
                      PyObject* kwds) {
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

  if (UpdateChildMessages(self) < 0)
    return NULL;
  if (self->message == NULL) {
    return SortReleased(self, args, kwds);
  } else {
    return SortAttached(self, args, kwds);
  }
}

// ---------------------------------------------------------------------

static PyObject* Item(RepeatedCompositeContainer* self, Py_ssize_t index) {
  if (UpdateChildMessages(self) < 0) {
    return NULL;
  }
  Py_ssize_t length = Length(self);
  if (index < 0) {
    index = length + index;
  }
  PyObject* item = PyList_GetItem(self->child_messages, index);
  if (item == NULL) {
    return NULL;
  }
  Py_INCREF(item);
  return item;
}

// The caller takes ownership of the returned Message.
Message* ReleaseLast(const FieldDescriptor* field,
                     const Descriptor* type,
                     Message* message) {
  GOOGLE_CHECK_NOTNULL(field);
  GOOGLE_CHECK_NOTNULL(type);
  GOOGLE_CHECK_NOTNULL(message);

  Message* released_message = message->GetReflection()->ReleaseLast(
      message, field);
  // TODO(tibell): Deal with proto1.

  // ReleaseMessage will return NULL which differs from
  // child_cmessage->message, if the field does not exist.  In this case,
  // the latter points to the default instance via a const_cast<>, so we
  // have to reset it to a new mutable object since we are taking ownership.
  if (released_message == NULL) {
    const Message* prototype = global_message_factory->GetPrototype(type);
    GOOGLE_CHECK_NOTNULL(prototype);
    return prototype->New();
  } else {
    return released_message;
  }
}

// Release field of message and transfer the ownership to cmessage.
void ReleaseLastTo(const FieldDescriptor* field,
                   Message* message,
                   CMessage* cmessage) {
  GOOGLE_CHECK_NOTNULL(field);
  GOOGLE_CHECK_NOTNULL(message);
  GOOGLE_CHECK_NOTNULL(cmessage);

  shared_ptr<Message> released_message(
      ReleaseLast(field, cmessage->message->GetDescriptor(), message));
  cmessage->parent = NULL;
  cmessage->parent_field = NULL;
  cmessage->message = released_message.get();
  cmessage->read_only = false;
  cmessage::SetOwner(cmessage, released_message);
}

// Called to release a container using
// ClearField('container_field_name') on the parent.
int Release(RepeatedCompositeContainer* self) {
  if (UpdateChildMessages(self) < 0) {
    PyErr_WriteUnraisable(PyBytes_FromString("Failed to update released "
                                             "messages"));
    return -1;
  }

  Message* message = self->message;
  const FieldDescriptor* field = self->parent_field->descriptor;

  // The reflection API only lets us release the last message in a
  // repeated field.  Therefore we iterate through the children
  // starting with the last one.
  const Py_ssize_t size = PyList_GET_SIZE(self->child_messages);
  GOOGLE_DCHECK_EQ(size, message->GetReflection()->FieldSize(*message, field));
  for (Py_ssize_t i = size - 1; i >= 0; --i) {
    CMessage* child_cmessage = reinterpret_cast<CMessage*>(
        PyList_GET_ITEM(self->child_messages, i));
    ReleaseLastTo(field, message, child_cmessage);
  }

  // Detach from containing message.
  self->parent = NULL;
  self->parent_field = NULL;
  self->message = NULL;
  self->owner.reset();

  return 0;
}

int SetOwner(RepeatedCompositeContainer* self,
             const shared_ptr<Message>& new_owner) {
  GOOGLE_CHECK_ATTACHED(self);

  self->owner = new_owner;
  const Py_ssize_t n = PyList_GET_SIZE(self->child_messages);
  for (Py_ssize_t i = 0; i < n; ++i) {
    PyObject* msg = PyList_GET_ITEM(self->child_messages, i);
    if (cmessage::SetOwner(reinterpret_cast<CMessage*>(msg), new_owner) == -1) {
      return -1;
    }
  }
  return 0;
}

static int Init(RepeatedCompositeContainer* self,
                PyObject* args,
                PyObject* kwargs) {
  self->message = NULL;
  self->parent = NULL;
  self->parent_field = NULL;
  self->subclass_init = NULL;
  self->child_messages = PyList_New(0);
  return 0;
}

static void Dealloc(RepeatedCompositeContainer* self) {
  Py_CLEAR(self->child_messages);
  // TODO(tibell): Do we need to call delete on these objects to make
  // sure their destructors are called?
  self->owner.reset();
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

static PySequenceMethods SqMethods = {
  (lenfunc)Length,        /* sq_length */
  0, /* sq_concat */
  0, /* sq_repeat */
  (ssizeargfunc)Item /* sq_item */
};

static PyMappingMethods MpMethods = {
  (lenfunc)Length,               /* mp_length */
  (binaryfunc)Subscript,      /* mp_subscript */
  (objobjargproc)AssignSubscript,/* mp_ass_subscript */
};

static PyMethodDef Methods[] = {
  { "add", (PyCFunction) Add, METH_VARARGS | METH_KEYWORDS,
    "Adds an object to the repeated container." },
  { "extend", (PyCFunction) Extend, METH_O,
    "Adds objects to the repeated container." },
  { "remove", (PyCFunction) Remove, METH_O,
    "Removes an object from the repeated container." },
  { "sort", (PyCFunction) Sort, METH_VARARGS | METH_KEYWORDS,
    "Sorts the repeated container." },
  { "MergeFrom", (PyCFunction) MergeFrom, METH_O,
    "Adds objects to the repeated container." },
  { NULL, NULL }
};

}  // namespace repeated_composite_container

PyTypeObject RepeatedCompositeContainer_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "google.protobuf.internal."
  "cpp._message.RepeatedCompositeContainer",  // tp_name
  sizeof(RepeatedCompositeContainer),     // tp_basicsize
  0,                                   //  tp_itemsize
  (destructor)repeated_composite_container::Dealloc,  //  tp_dealloc
  0,                                   //  tp_print
  0,                                   //  tp_getattr
  0,                                   //  tp_setattr
  0,                                   //  tp_compare
  0,                                   //  tp_repr
  0,                                   //  tp_as_number
  &repeated_composite_container::SqMethods,   //  tp_as_sequence
  &repeated_composite_container::MpMethods,   //  tp_as_mapping
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
  (richcmpfunc)repeated_composite_container::RichCompare,  //  tp_richcompare
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
  (initproc)repeated_composite_container::Init,  //  tp_init
};

}  // namespace python
}  // namespace protobuf
}  // namespace google
