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

// Author: haberman@google.com (Josh Haberman)

#include <google/protobuf/pyext/message_map_container.h>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/message.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

namespace google {
namespace protobuf {
namespace python {

struct MessageMapIterator {
  PyObject_HEAD;

  // This dict contains the full contents of what we want to iterate over.
  // There's no way to avoid building this, because the list representation
  // (which is canonical) can contain duplicate keys.  So at the very least we
  // need a set that lets us skip duplicate keys.  And at the point that we're
  // doing that, we might as well just build the actual dict we're iterating
  // over and use dict's built-in iterator.
  PyObject* dict;

  // An iterator on dict.
  PyObject* iter;

  // A pointer back to the container, so we can notice changes to the version.
  MessageMapContainer* container;

  // The version of the map when we took the iterator to it.
  //
  // We store this so that if the map is modified during iteration we can throw
  // an error.
  uint64 version;
};

static MessageMapIterator* GetIter(PyObject* obj) {
  return reinterpret_cast<MessageMapIterator*>(obj);
}

namespace message_map_container {

static MessageMapContainer* GetMap(PyObject* obj) {
  return reinterpret_cast<MessageMapContainer*>(obj);
}

// The private constructor of MessageMapContainer objects.
PyObject* NewContainer(CMessage* parent,
                       const google::protobuf::FieldDescriptor* parent_field_descriptor,
                       PyObject* concrete_class) {
  if (!CheckFieldBelongsToMessage(parent_field_descriptor, parent->message)) {
    return NULL;
  }

#if PY_MAJOR_VERSION >= 3
  PyObject* obj = PyType_GenericAlloc(
        reinterpret_cast<PyTypeObject *>(MessageMapContainer_Type), 0);
#else
  PyObject* obj = PyType_GenericAlloc(&MessageMapContainer_Type, 0);
#endif
  if (obj == NULL) {
    return PyErr_Format(PyExc_RuntimeError,
                        "Could not allocate new container.");
  }

  MessageMapContainer* self = GetMap(obj);

  self->message = parent->message;
  self->parent = parent;
  self->parent_field_descriptor = parent_field_descriptor;
  self->owner = parent->owner;
  self->version = 0;

  self->key_field_descriptor =
      parent_field_descriptor->message_type()->FindFieldByName("key");
  self->value_field_descriptor =
      parent_field_descriptor->message_type()->FindFieldByName("value");

  self->message_dict = PyDict_New();
  if (self->message_dict == NULL) {
    return PyErr_Format(PyExc_RuntimeError,
                        "Could not allocate message dict.");
  }

  Py_INCREF(concrete_class);
  self->subclass_init = concrete_class;

  if (self->key_field_descriptor == NULL ||
      self->value_field_descriptor == NULL) {
    Py_DECREF(obj);
    return PyErr_Format(PyExc_KeyError,
                        "Map entry descriptor did not have key/value fields");
  }

  return obj;
}

// Initializes the underlying Message object of "to" so it becomes a new parent
// repeated scalar, and copies all the values from "from" to it. A child scalar
// container can be released by passing it as both from and to (e.g. making it
// the recipient of the new parent message and copying the values from itself).
static int InitializeAndCopyToParentContainer(
    MessageMapContainer* from,
    MessageMapContainer* to) {
  // For now we require from == to, re-evaluate if we want to support deep copy
  // as in repeated_composite_container.cc.
  GOOGLE_DCHECK(from == to);
  Message* old_message = from->message;
  Message* new_message = old_message->New();
  to->parent = NULL;
  to->parent_field_descriptor = from->parent_field_descriptor;
  to->message = new_message;
  to->owner.reset(new_message);

  vector<const FieldDescriptor*> fields;
  fields.push_back(from->parent_field_descriptor);
  old_message->GetReflection()->SwapFields(old_message, new_message, fields);
  return 0;
}

static PyObject* GetCMessage(MessageMapContainer* self, Message* entry) {
  // Get or create the CMessage object corresponding to this message.
  Message* message = entry->GetReflection()->MutableMessage(
      entry, self->value_field_descriptor);
  ScopedPyObjectPtr key(PyLong_FromVoidPtr(message));
  PyObject* ret = PyDict_GetItem(self->message_dict, key.get());

  if (ret == NULL) {
    CMessage* cmsg = cmessage::NewEmptyMessage(self->subclass_init,
                                               message->GetDescriptor());
    ret = reinterpret_cast<PyObject*>(cmsg);

    if (cmsg == NULL) {
      return NULL;
    }
    cmsg->owner = self->owner;
    cmsg->message = message;
    cmsg->parent = self->parent;

    if (PyDict_SetItem(self->message_dict, key.get(), ret) < 0) {
      Py_DECREF(ret);
      return NULL;
    }
  } else {
    Py_INCREF(ret);
  }

  return ret;
}

int Release(MessageMapContainer* self) {
  InitializeAndCopyToParentContainer(self, self);
  return 0;
}

void SetOwner(MessageMapContainer* self,
              const shared_ptr<Message>& new_owner) {
  self->owner = new_owner;
}

Py_ssize_t Length(PyObject* _self) {
  MessageMapContainer* self = GetMap(_self);
  google::protobuf::Message* message = self->message;
  return message->GetReflection()->FieldSize(*message,
                                             self->parent_field_descriptor);
}

int MapKeyMatches(MessageMapContainer* self, const Message* entry,
                  PyObject* key) {
  // TODO(haberman): do we need more strict type checking?
  ScopedPyObjectPtr entry_key(
      cmessage::InternalGetScalar(entry, self->key_field_descriptor));
  int ret = PyObject_RichCompareBool(key, entry_key.get(), Py_EQ);
  return ret;
}

int SetItem(PyObject *_self, PyObject *key, PyObject *v) {
  if (v) {
    PyErr_Format(PyExc_ValueError,
                 "Direct assignment of submessage not allowed");
    return -1;
  }

  // Now we know that this is a delete, not a set.

  MessageMapContainer* self = GetMap(_self);
  cmessage::AssureWritable(self->parent);

  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();
  size_t size =
      reflection->FieldSize(*message, self->parent_field_descriptor);

  // Right now the Reflection API doesn't support map lookup, so we implement it
  // via linear search.  We need to search from the end because the underlying
  // representation can have duplicates if a user calls MergeFrom(); the last
  // one needs to win.
  //
  // TODO(haberman): add lookup API to Reflection API.
  bool found = false;
  for (int i = size - 1; i >= 0; i--) {
    Message* entry = reflection->MutableRepeatedMessage(
        message, self->parent_field_descriptor, i);
    int matches = MapKeyMatches(self, entry, key);
    if (matches < 0) return -1;
    if (matches) {
      found = true;
      if (i != (int)size - 1) {
        reflection->SwapElements(message, self->parent_field_descriptor, i,
                                 size - 1);
      }
      reflection->RemoveLast(message, self->parent_field_descriptor);

      // Can't exit now, the repeated field representation of maps allows
      // duplicate keys, and we have to be sure to remove all of them.
    }
  }

  if (!found) {
    PyErr_Format(PyExc_KeyError, "Key not present in map");
    return -1;
  }

  self->version++;

  return 0;
}

PyObject* GetIterator(PyObject *_self) {
  MessageMapContainer* self = GetMap(_self);

  ScopedPyObjectPtr obj(PyType_GenericAlloc(&MessageMapIterator_Type, 0));
  if (obj == NULL) {
    return PyErr_Format(PyExc_KeyError, "Could not allocate iterator");
  }

  MessageMapIterator* iter = GetIter(obj.get());

  Py_INCREF(self);
  iter->container = self;
  iter->version = self->version;
  iter->dict = PyDict_New();
  if (iter->dict == NULL) {
    return PyErr_Format(PyExc_RuntimeError,
                        "Could not allocate dict for iterator.");
  }

  // Build the entire map into a dict right now.  Start from the beginning so
  // that later entries win in the case of duplicates.
  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();

  // Right now the Reflection API doesn't support map lookup, so we implement it
  // via linear search.  We need to search from the end because the underlying
  // representation can have duplicates if a user calls MergeFrom(); the last
  // one needs to win.
  //
  // TODO(haberman): add lookup API to Reflection API.
  size_t size =
      reflection->FieldSize(*message, self->parent_field_descriptor);
  for (int i = size - 1; i >= 0; i--) {
    Message* entry = reflection->MutableRepeatedMessage(
        message, self->parent_field_descriptor, i);
    ScopedPyObjectPtr key(
        cmessage::InternalGetScalar(entry, self->key_field_descriptor));
    if (PyDict_SetItem(iter->dict, key.get(), GetCMessage(self, entry)) < 0) {
      return PyErr_Format(PyExc_RuntimeError,
                          "SetItem failed in iterator construction.");
    }
  }

  iter->iter = PyObject_GetIter(iter->dict);

  return obj.release();
}

PyObject* GetItem(PyObject* _self, PyObject* key) {
  MessageMapContainer* self = GetMap(_self);
  cmessage::AssureWritable(self->parent);
  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();

  // Right now the Reflection API doesn't support map lookup, so we implement it
  // via linear search.  We need to search from the end because the underlying
  // representation can have duplicates if a user calls MergeFrom(); the last
  // one needs to win.
  //
  // TODO(haberman): add lookup API to Reflection API.
  size_t size =
      reflection->FieldSize(*message, self->parent_field_descriptor);
  for (int i = size - 1; i >= 0; i--) {
    Message* entry = reflection->MutableRepeatedMessage(
        message, self->parent_field_descriptor, i);
    int matches = MapKeyMatches(self, entry, key);
    if (matches < 0) return NULL;
    if (matches) {
      return GetCMessage(self, entry);
    }
  }

  // Key is not already present; insert a new entry.
  Message* entry =
      reflection->AddMessage(message, self->parent_field_descriptor);

  self->version++;

  if (cmessage::InternalSetNonOneofScalar(entry, self->key_field_descriptor,
                                          key) < 0) {
    reflection->RemoveLast(message, self->parent_field_descriptor);
    return NULL;
  }

  return GetCMessage(self, entry);
}

PyObject* Contains(PyObject* _self, PyObject* key) {
  MessageMapContainer* self = GetMap(_self);
  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();

  // Right now the Reflection API doesn't support map lookup, so we implement it
  // via linear search.
  //
  // TODO(haberman): add lookup API to Reflection API.
  int size =
      reflection->FieldSize(*message, self->parent_field_descriptor);
  for (int i = 0; i < size; i++) {
    Message* entry = reflection->MutableRepeatedMessage(
        message, self->parent_field_descriptor, i);
    int matches = MapKeyMatches(self, entry, key);
    if (matches < 0) return NULL;
    if (matches) {
      Py_RETURN_TRUE;
    }
  }

  Py_RETURN_FALSE;
}

PyObject* Clear(PyObject* _self) {
  MessageMapContainer* self = GetMap(_self);
  cmessage::AssureWritable(self->parent);
  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();

  self->version++;
  reflection->ClearField(message, self->parent_field_descriptor);

  Py_RETURN_NONE;
}

PyObject* Get(PyObject* self, PyObject* args) {
  PyObject* key;
  PyObject* default_value = NULL;
  if (PyArg_ParseTuple(args, "O|O", &key, &default_value) < 0) {
    return NULL;
  }

  ScopedPyObjectPtr is_present(Contains(self, key));
  if (is_present.get() == NULL) {
    return NULL;
  }

  if (PyObject_IsTrue(is_present.get())) {
    return GetItem(self, key);
  } else {
    if (default_value != NULL) {
      Py_INCREF(default_value);
      return default_value;
    } else {
      Py_RETURN_NONE;
    }
  }
}

static void Dealloc(PyObject* _self) {
  MessageMapContainer* self = GetMap(_self);
  self->owner.reset();
  Py_DECREF(self->message_dict);
  Py_TYPE(_self)->tp_free(_self);
}

static PyMethodDef Methods[] = {
  { "__contains__", (PyCFunction)Contains, METH_O,
    "Tests whether the map contains this element."},
  { "clear", (PyCFunction)Clear, METH_NOARGS,
    "Removes all elements from the map."},
  { "get", Get, METH_VARARGS,
    "Gets the value for the given key if present, or otherwise a default" },
  { "get_or_create", GetItem, METH_O,
    "Alias for getitem, useful to make explicit that the map is mutated." },
  /*
  { "__deepcopy__", (PyCFunction)DeepCopy, METH_VARARGS,
    "Makes a deep copy of the class." },
  { "__reduce__", (PyCFunction)Reduce, METH_NOARGS,
    "Outputs picklable representation of the repeated field." },
  */
  {NULL, NULL},
};

}  // namespace message_map_container

namespace message_map_iterator {

static void Dealloc(PyObject* _self) {
  MessageMapIterator* self = GetIter(_self);
  Py_DECREF(self->dict);
  Py_DECREF(self->iter);
  Py_DECREF(self->container);
  Py_TYPE(_self)->tp_free(_self);
}

PyObject* IterNext(PyObject* _self) {
  MessageMapIterator* self = GetIter(_self);

  // This won't catch mutations to the map performed by MergeFrom(); no easy way
  // to address that.
  if (self->version != self->container->version) {
    return PyErr_Format(PyExc_RuntimeError,
                        "Map modified during iteration.");
  }

  return PyIter_Next(self->iter);
}

}  // namespace message_map_iterator

#if PY_MAJOR_VERSION >= 3
  static PyType_Slot MessageMapContainer_Type_slots[] = {
      {Py_tp_dealloc, (void *)message_map_container::Dealloc},
      {Py_mp_length, (void *)message_map_container::Length},
      {Py_mp_subscript, (void *)message_map_container::GetItem},
      {Py_mp_ass_subscript, (void *)message_map_container::SetItem},
      {Py_tp_methods, (void *)message_map_container::Methods},
      {Py_tp_iter, (void *)message_map_container::GetIterator},
      {0, 0}
  };

  PyType_Spec MessageMapContainer_Type_spec = {
      FULL_MODULE_NAME ".MessageMapContainer",
      sizeof(MessageMapContainer),
      0,
      Py_TPFLAGS_DEFAULT,
      MessageMapContainer_Type_slots
  };

  PyObject *MessageMapContainer_Type;

#else
  static PyMappingMethods MpMethods = {
    message_map_container::Length,    // mp_length
    message_map_container::GetItem,   // mp_subscript
    message_map_container::SetItem,   // mp_ass_subscript
  };

  PyTypeObject MessageMapContainer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    FULL_MODULE_NAME ".MessageMapContainer",  //  tp_name
    sizeof(MessageMapContainer),         //  tp_basicsize
    0,                                   //  tp_itemsize
    message_map_container::Dealloc,      //  tp_dealloc
    0,                                   //  tp_print
    0,                                   //  tp_getattr
    0,                                   //  tp_setattr
    0,                                   //  tp_compare
    0,                                   //  tp_repr
    0,                                   //  tp_as_number
    0,                                   //  tp_as_sequence
    &MpMethods,                          //  tp_as_mapping
    0,                                   //  tp_hash
    0,                                   //  tp_call
    0,                                   //  tp_str
    0,                                   //  tp_getattro
    0,                                   //  tp_setattro
    0,                                   //  tp_as_buffer
    Py_TPFLAGS_DEFAULT,                  //  tp_flags
    "A map container for message",       //  tp_doc
    0,                                   //  tp_traverse
    0,                                   //  tp_clear
    0,                                   //  tp_richcompare
    0,                                   //  tp_weaklistoffset
    message_map_container::GetIterator,  //  tp_iter
    0,                                   //  tp_iternext
    message_map_container::Methods,      //  tp_methods
    0,                                   //  tp_members
    0,                                   //  tp_getset
    0,                                   //  tp_base
    0,                                   //  tp_dict
    0,                                   //  tp_descr_get
    0,                                   //  tp_descr_set
    0,                                   //  tp_dictoffset
    0,                                   //  tp_init
  };
#endif

PyTypeObject MessageMapIterator_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  FULL_MODULE_NAME ".MessageMapIterator",  //  tp_name
  sizeof(MessageMapIterator),          //  tp_basicsize
  0,                                   //  tp_itemsize
  message_map_iterator::Dealloc,       //  tp_dealloc
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
  0,                                   //  tp_str
  0,                                   //  tp_getattro
  0,                                   //  tp_setattro
  0,                                   //  tp_as_buffer
  Py_TPFLAGS_DEFAULT,                  //  tp_flags
  "A scalar map iterator",             //  tp_doc
  0,                                   //  tp_traverse
  0,                                   //  tp_clear
  0,                                   //  tp_richcompare
  0,                                   //  tp_weaklistoffset
  PyObject_SelfIter,                   //  tp_iter
  message_map_iterator::IterNext,       //  tp_iternext
  0,                                   //  tp_methods
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
