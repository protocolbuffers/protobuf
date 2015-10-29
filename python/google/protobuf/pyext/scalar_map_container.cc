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

#include <google/protobuf/pyext/scalar_map_container.h>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/message.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

namespace google {
namespace protobuf {
namespace python {

struct ScalarMapIterator {
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
  ScalarMapContainer* container;

  // The version of the map when we took the iterator to it.
  //
  // We store this so that if the map is modified during iteration we can throw
  // an error.
  uint64 version;
};

static ScalarMapIterator* GetIter(PyObject* obj) {
  return reinterpret_cast<ScalarMapIterator*>(obj);
}

namespace scalar_map_container {

static ScalarMapContainer* GetMap(PyObject* obj) {
  return reinterpret_cast<ScalarMapContainer*>(obj);
}

// The private constructor of ScalarMapContainer objects.
PyObject *NewContainer(
    CMessage* parent, const google::protobuf::FieldDescriptor* parent_field_descriptor) {
  if (!CheckFieldBelongsToMessage(parent_field_descriptor, parent->message)) {
    return NULL;
  }

#if PY_MAJOR_VERSION >= 3
  ScopedPyObjectPtr obj(PyType_GenericAlloc(
        reinterpret_cast<PyTypeObject *>(ScalarMapContainer_Type), 0));
#else
  ScopedPyObjectPtr obj(PyType_GenericAlloc(&ScalarMapContainer_Type, 0));
#endif
  if (obj.get() == NULL) {
    return PyErr_Format(PyExc_RuntimeError,
                        "Could not allocate new container.");
  }

  ScalarMapContainer* self = GetMap(obj.get());

  self->message = parent->message;
  self->parent = parent;
  self->parent_field_descriptor = parent_field_descriptor;
  self->owner = parent->owner;
  self->version = 0;

  self->key_field_descriptor =
      parent_field_descriptor->message_type()->FindFieldByName("key");
  self->value_field_descriptor =
      parent_field_descriptor->message_type()->FindFieldByName("value");

  if (self->key_field_descriptor == NULL ||
      self->value_field_descriptor == NULL) {
    return PyErr_Format(PyExc_KeyError,
                        "Map entry descriptor did not have key/value fields");
  }

  return obj.release();
}

// Initializes the underlying Message object of "to" so it becomes a new parent
// repeated scalar, and copies all the values from "from" to it. A child scalar
// container can be released by passing it as both from and to (e.g. making it
// the recipient of the new parent message and copying the values from itself).
static int InitializeAndCopyToParentContainer(
    ScalarMapContainer* from,
    ScalarMapContainer* to) {
  // For now we require from == to, re-evaluate if we want to support deep copy
  // as in repeated_scalar_container.cc.
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

int Release(ScalarMapContainer* self) {
  return InitializeAndCopyToParentContainer(self, self);
}

void SetOwner(ScalarMapContainer* self,
              const shared_ptr<Message>& new_owner) {
  self->owner = new_owner;
}

Py_ssize_t Length(PyObject* _self) {
  ScalarMapContainer* self = GetMap(_self);
  google::protobuf::Message* message = self->message;
  return message->GetReflection()->FieldSize(*message,
                                             self->parent_field_descriptor);
}

int MapKeyMatches(ScalarMapContainer* self, const Message* entry,
                  PyObject* key) {
  // TODO(haberman): do we need more strict type checking?
  ScopedPyObjectPtr entry_key(
      cmessage::InternalGetScalar(entry, self->key_field_descriptor));
  int ret = PyObject_RichCompareBool(key, entry_key.get(), Py_EQ);
  return ret;
}

PyObject* GetItem(PyObject* _self, PyObject* key) {
  ScalarMapContainer* self = GetMap(_self);

  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();

  // Right now the Reflection API doesn't support map lookup, so we implement it
  // via linear search.
  //
  // TODO(haberman): add lookup API to Reflection API.
  size_t size = reflection->FieldSize(*message, self->parent_field_descriptor);
  for (int i = size - 1; i >= 0; i--) {
    const Message& entry = reflection->GetRepeatedMessage(
        *message, self->parent_field_descriptor, i);
    int matches = MapKeyMatches(self, &entry, key);
    if (matches < 0) return NULL;
    if (matches) {
      return cmessage::InternalGetScalar(&entry, self->value_field_descriptor);
    }
  }

  // Need to add a new entry.
  Message* entry =
      reflection->AddMessage(message, self->parent_field_descriptor);
  PyObject* ret = NULL;

  if (cmessage::InternalSetNonOneofScalar(entry, self->key_field_descriptor,
                                          key) >= 0) {
    ret = cmessage::InternalGetScalar(entry, self->value_field_descriptor);
  }

  self->version++;

  // If there was a type error above, it set the Python exception.
  return ret;
}

int SetItem(PyObject *_self, PyObject *key, PyObject *v) {
  ScalarMapContainer* self = GetMap(_self);
  cmessage::AssureWritable(self->parent);

  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();
  size_t size =
      reflection->FieldSize(*message, self->parent_field_descriptor);
  self->version++;

  if (v) {
    // Set item.
    //
    // Right now the Reflection API doesn't support map lookup, so we implement
    // it via linear search.
    //
    // TODO(haberman): add lookup API to Reflection API.
    for (int i = size - 1; i >= 0; i--) {
      Message* entry = reflection->MutableRepeatedMessage(
          message, self->parent_field_descriptor, i);
      int matches = MapKeyMatches(self, entry, key);
      if (matches < 0) return -1;
      if (matches) {
        return cmessage::InternalSetNonOneofScalar(
            entry, self->value_field_descriptor, v);
      }
    }

    // Key is not already present; insert a new entry.
    Message* entry =
        reflection->AddMessage(message, self->parent_field_descriptor);

    if (cmessage::InternalSetNonOneofScalar(entry, self->key_field_descriptor,
                                            key) < 0 ||
        cmessage::InternalSetNonOneofScalar(entry, self->value_field_descriptor,
                                            v) < 0) {
      reflection->RemoveLast(message, self->parent_field_descriptor);
      return -1;
    }

    return 0;
  } else {
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

    if (found) {
      return 0;
    } else {
      PyErr_Format(PyExc_KeyError, "Key not present in map");
      return -1;
    }
  }
}

PyObject* GetIterator(PyObject *_self) {
  ScalarMapContainer* self = GetMap(_self);

  ScopedPyObjectPtr obj(PyType_GenericAlloc(&ScalarMapIterator_Type, 0));
  if (obj == NULL) {
    return PyErr_Format(PyExc_KeyError, "Could not allocate iterator");
  }

  ScalarMapIterator* iter = GetIter(obj.get());

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
  for (size_t i = 0; i < size; i++) {
    Message* entry = reflection->MutableRepeatedMessage(
        message, self->parent_field_descriptor, i);
    ScopedPyObjectPtr key(
        cmessage::InternalGetScalar(entry, self->key_field_descriptor));
    ScopedPyObjectPtr val(
        cmessage::InternalGetScalar(entry, self->value_field_descriptor));
    if (PyDict_SetItem(iter->dict, key.get(), val.get()) < 0) {
      return PyErr_Format(PyExc_RuntimeError,
                          "SetItem failed in iterator construction.");
    }
  }


  iter->iter = PyObject_GetIter(iter->dict);


  return obj.release();
}

PyObject* Clear(PyObject* _self) {
  ScalarMapContainer* self = GetMap(_self);
  cmessage::AssureWritable(self->parent);
  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();

  reflection->ClearField(message, self->parent_field_descriptor);

  Py_RETURN_NONE;
}

PyObject* Contains(PyObject* _self, PyObject* key) {
  ScalarMapContainer* self = GetMap(_self);

  Message* message = self->message;
  const Reflection* reflection = message->GetReflection();

  // Right now the Reflection API doesn't support map lookup, so we implement it
  // via linear search.
  //
  // TODO(haberman): add lookup API to Reflection API.
  size_t size = reflection->FieldSize(*message, self->parent_field_descriptor);
  for (int i = size - 1; i >= 0; i--) {
    const Message& entry = reflection->GetRepeatedMessage(
        *message, self->parent_field_descriptor, i);
    int matches = MapKeyMatches(self, &entry, key);
    if (matches < 0) return NULL;
    if (matches) {
      Py_RETURN_TRUE;
    }
  }

  Py_RETURN_FALSE;
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
  ScalarMapContainer* self = GetMap(_self);
  self->owner.reset();
  Py_TYPE(_self)->tp_free(_self);
}

static PyMethodDef Methods[] = {
  { "__contains__", Contains, METH_O,
    "Tests whether a key is a member of the map." },
  { "clear", (PyCFunction)Clear, METH_NOARGS,
    "Removes all elements from the map." },
  { "get", Get, METH_VARARGS,
    "Gets the value for the given key if present, or otherwise a default" },
  /*
  { "__deepcopy__", (PyCFunction)DeepCopy, METH_VARARGS,
    "Makes a deep copy of the class." },
  { "__reduce__", (PyCFunction)Reduce, METH_NOARGS,
    "Outputs picklable representation of the repeated field." },
  */
  {NULL, NULL},
};

}  // namespace scalar_map_container

namespace scalar_map_iterator {

static void Dealloc(PyObject* _self) {
  ScalarMapIterator* self = GetIter(_self);
  Py_DECREF(self->dict);
  Py_DECREF(self->iter);
  Py_DECREF(self->container);
  Py_TYPE(_self)->tp_free(_self);
}

PyObject* IterNext(PyObject* _self) {
  ScalarMapIterator* self = GetIter(_self);

  // This won't catch mutations to the map performed by MergeFrom(); no easy way
  // to address that.
  if (self->version != self->container->version) {
    return PyErr_Format(PyExc_RuntimeError,
                        "Map modified during iteration.");
  }

  return PyIter_Next(self->iter);
}

}  // namespace scalar_map_iterator

 
#if PY_MAJOR_VERSION >= 3
  static PyType_Slot ScalarMapContainer_Type_slots[] = {
      {Py_tp_dealloc, (void *)scalar_map_container::Dealloc},
      {Py_mp_length, (void *)scalar_map_container::Length},
      {Py_mp_subscript, (void *)scalar_map_container::GetItem},
      {Py_mp_ass_subscript, (void *)scalar_map_container::SetItem},
      {Py_tp_methods, (void *)scalar_map_container::Methods},
      {Py_tp_iter, (void *)scalar_map_container::GetIterator},
      {0, 0},
  };

  PyType_Spec ScalarMapContainer_Type_spec = {
      FULL_MODULE_NAME ".ScalarMapContainer",
      sizeof(ScalarMapContainer),
      0,
      Py_TPFLAGS_DEFAULT,
      ScalarMapContainer_Type_slots
  };
  PyObject *ScalarMapContainer_Type;
#else
  static PyMappingMethods MpMethods = {
    scalar_map_container::Length,    // mp_length
    scalar_map_container::GetItem,   // mp_subscript
    scalar_map_container::SetItem,   // mp_ass_subscript
  };

  PyTypeObject ScalarMapContainer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    FULL_MODULE_NAME ".ScalarMapContainer",  //  tp_name
    sizeof(ScalarMapContainer),          //  tp_basicsize
    0,                                   //  tp_itemsize
    scalar_map_container::Dealloc,       //  tp_dealloc
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
    "A scalar map container",            //  tp_doc
    0,                                   //  tp_traverse
    0,                                   //  tp_clear
    0,                                   //  tp_richcompare
    0,                                   //  tp_weaklistoffset
    scalar_map_container::GetIterator,   //  tp_iter
    0,                                   //  tp_iternext
    scalar_map_container::Methods,       //  tp_methods
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

PyTypeObject ScalarMapIterator_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  FULL_MODULE_NAME ".ScalarMapIterator",  //  tp_name
  sizeof(ScalarMapIterator),           //  tp_basicsize
  0,                                   //  tp_itemsize
  scalar_map_iterator::Dealloc,        //  tp_dealloc
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
  scalar_map_iterator::IterNext,       //  tp_iternext
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
