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

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/descriptor_pool.h>
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

namespace repeated_composite_container {

// TODO(tibell): We might also want to check:
//   GOOGLE_CHECK_NOTNULL((self)->owner.get());
#define GOOGLE_CHECK_ATTACHED(self)             \
  do {                                   \
    GOOGLE_CHECK_NOTNULL((self)->message);      \
    GOOGLE_CHECK_NOTNULL((self)->parent_field_descriptor); \
  } while (0);

#define GOOGLE_CHECK_RELEASED(self)             \
  do {                                   \
    GOOGLE_CHECK((self)->owner.get() == NULL);  \
    GOOGLE_CHECK((self)->message == NULL);      \
    GOOGLE_CHECK((self)->parent_field_descriptor == NULL); \
    GOOGLE_CHECK((self)->parent == NULL);       \
  } while (0);

// ---------------------------------------------------------------------
// len()

static Py_ssize_t Length(RepeatedCompositeContainer* self) {
  Message* message = self->message;
  if (message != NULL) {
    return message->GetReflection()->FieldSize(*message,
                                               self->parent_field_descriptor);
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
  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();
  for (Py_ssize_t i = child_length; i < message_length; ++i) {
    const Message& sub_message = reflection->GetRepeatedMessage(
        *(self->message), self->parent_field_descriptor, i);
    CMessage* cmsg = cmessage::NewEmptyMessage(self->child_message_class);
    ScopedPyObjectPtr py_cmsg(reinterpret_cast<PyObject*>(cmsg));
    if (cmsg == NULL) {
      return -1;
    }
    cmsg->owner = self->owner;
    cmsg->message = const_cast<Message*>(&sub_message);
    cmsg->parent = self->parent;
    if (PyList_Append(self->child_messages, py_cmsg.get()) < 0) {
      return -1;
    }
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
  Message* message = self->message;
  Message* sub_message =
      message->GetReflection()->AddMessage(message,
                                           self->parent_field_descriptor);
  CMessage* cmsg = cmessage::NewEmptyMessage(self->child_message_class);
  if (cmsg == NULL)
    return NULL;

  cmsg->owner = self->owner;
  cmsg->message = sub_message;
  cmsg->parent = self->parent;
  if (cmessage::InitAttributes(cmsg, args, kwargs) < 0) {
    Py_DECREF(cmsg);
    return NULL;
  }

  PyObject* py_cmsg = reinterpret_cast<PyObject*>(cmsg);
  if (PyList_Append(self->child_messages, py_cmsg) < 0) {
    Py_DECREF(py_cmsg);
    return NULL;
  }
  return py_cmsg;
}

static PyObject* AddToReleased(RepeatedCompositeContainer* self,
                               PyObject* args,
                               PyObject* kwargs) {
  GOOGLE_CHECK_RELEASED(self);

  // Create a new Message detached from the rest.
  PyObject* py_cmsg = PyEval_CallObjectWithKeywords(
      self->child_message_class->AsPyObject(), args, kwargs);
  if (py_cmsg == NULL)
    return NULL;

  if (PyList_Append(self->child_messages, py_cmsg) < 0) {
    Py_DECREF(py_cmsg);
    return NULL;
  }
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
  while ((next.reset(PyIter_Next(iter.get()))) != NULL) {
    if (!PyObject_TypeCheck(next.get(), &CMessage_Type)) {
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
  // Just forward the call to the subscript-handling function of the
  // list containing the child messages.
  return PyObject_GetItem(self->child_messages, slice);
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
  if (self->parent != NULL) {
    if (cmessage::InternalDeleteRepeatedField(self->parent,
                                              self->parent_field_descriptor,
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
  if (AssignSubscript(self, py_index.get(), NULL) < 0) {
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

// ---------------------------------------------------------------------
// sort()

static void ReorderAttached(RepeatedCompositeContainer* self) {
  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();
  const FieldDescriptor* descriptor = self->parent_field_descriptor;
  const Py_ssize_t length = Length(self);

  // Since Python protobuf objects are never arena-allocated, adding and
  // removing message pointers to the underlying array is just updating
  // pointers.
  for (Py_ssize_t i = 0; i < length; ++i)
    reflection->ReleaseLast(message, descriptor);

  for (Py_ssize_t i = 0; i < length; ++i) {
    CMessage* py_cmsg = reinterpret_cast<CMessage*>(
        PyList_GET_ITEM(self->child_messages, i));
    reflection->AddAllocatedMessage(message, descriptor, py_cmsg->message);
  }
}

// Returns 0 if successful; returns -1 and sets an exception if
// unsuccessful.
static int SortPythonMessages(RepeatedCompositeContainer* self,
                               PyObject* args,
                               PyObject* kwds) {
  ScopedPyObjectPtr m(PyObject_GetAttrString(self->child_messages, "sort"));
  if (m == NULL)
    return -1;
  if (PyObject_Call(m.get(), args, kwds) == NULL)
    return -1;
  if (self->message != NULL) {
    ReorderAttached(self);
  }
  return 0;
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

  if (UpdateChildMessages(self) < 0) {
    return NULL;
  }
  if (SortPythonMessages(self, args, kwds) < 0) {
    return NULL;
  }
  Py_RETURN_NONE;
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

static PyObject* Pop(RepeatedCompositeContainer* self,
                     PyObject* args) {
  Py_ssize_t index = -1;
  if (!PyArg_ParseTuple(args, "|n", &index)) {
    return NULL;
  }
  PyObject* item = Item(self, index);
  if (item == NULL) {
    PyErr_Format(PyExc_IndexError,
                 "list index (%zd) out of range",
                 index);
    return NULL;
  }
  ScopedPyObjectPtr py_index(PyLong_FromSsize_t(index));
  if (AssignSubscript(self, py_index.get(), NULL) < 0) {
    return NULL;
  }
  return item;
}

// Release field of parent message and transfer the ownership to target.
void ReleaseLastTo(CMessage* parent,
                   const FieldDescriptor* field,
                   CMessage* target) {
  GOOGLE_CHECK_NOTNULL(parent);
  GOOGLE_CHECK_NOTNULL(field);
  GOOGLE_CHECK_NOTNULL(target);

  shared_ptr<Message> released_message(
      parent->message->GetReflection()->ReleaseLast(parent->message, field));
  // TODO(tibell): Deal with proto1.

  target->parent = NULL;
  target->parent_field_descriptor = NULL;
  target->message = released_message.get();
  target->read_only = false;
  cmessage::SetOwner(target, released_message);
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
  const FieldDescriptor* field = self->parent_field_descriptor;

  // The reflection API only lets us release the last message in a
  // repeated field.  Therefore we iterate through the children
  // starting with the last one.
  const Py_ssize_t size = PyList_GET_SIZE(self->child_messages);
  GOOGLE_DCHECK_EQ(size, message->GetReflection()->FieldSize(*message, field));
  for (Py_ssize_t i = size - 1; i >= 0; --i) {
    CMessage* child_cmessage = reinterpret_cast<CMessage*>(
        PyList_GET_ITEM(self->child_messages, i));
    ReleaseLastTo(self->parent, field, child_cmessage);
  }

  // Detach from containing message.
  self->parent = NULL;
  self->parent_field_descriptor = NULL;
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

// The private constructor of RepeatedCompositeContainer objects.
PyObject *NewContainer(
    CMessage* parent,
    const FieldDescriptor* parent_field_descriptor,
    CMessageClass* concrete_class) {
  if (!CheckFieldBelongsToMessage(parent_field_descriptor, parent->message)) {
    return NULL;
  }

  RepeatedCompositeContainer* self =
      reinterpret_cast<RepeatedCompositeContainer*>(
          PyType_GenericAlloc(&RepeatedCompositeContainer_Type, 0));
  if (self == NULL) {
    return NULL;
  }

  self->message = parent->message;
  self->parent = parent;
  self->parent_field_descriptor = parent_field_descriptor;
  self->owner = parent->owner;
  Py_INCREF(concrete_class);
  self->child_message_class = concrete_class;
  self->child_messages = PyList_New(0);

  return reinterpret_cast<PyObject*>(self);
}

static void Dealloc(RepeatedCompositeContainer* self) {
  Py_CLEAR(self->child_messages);
  Py_CLEAR(self->child_message_class);
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
  { "pop", (PyCFunction)Pop, METH_VARARGS,
    "Removes an object from the repeated container and returns it." },
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
  FULL_MODULE_NAME ".RepeatedCompositeContainer",  // tp_name
  sizeof(RepeatedCompositeContainer),  // tp_basicsize
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
  0,                                   //  tp_init
};

}  // namespace python
}  // namespace protobuf
}  // namespace google
