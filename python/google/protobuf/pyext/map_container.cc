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

#include <google/protobuf/pyext/map_container.h>

#include <memory>
#ifndef _SHARED_PTR_H
#include <google/protobuf/stubs/shared_ptr.h>
#endif

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/map_field.h>
#include <google/protobuf/map.h>
#include <google/protobuf/message.h>
#include <google/protobuf/pyext/message_factory.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/repeated_composite_container.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

#if PY_MAJOR_VERSION >= 3
  #define PyInt_FromLong PyLong_FromLong
  #define PyInt_FromSize_t PyLong_FromSize_t
#endif

namespace google {
namespace protobuf {
namespace python {

// Functions that need access to map reflection functionality.
// They need to be contained in this class because it is friended.
class MapReflectionFriend {
 public:
  // Methods that are in common between the map types.
  static PyObject* Contains(PyObject* _self, PyObject* key);
  static Py_ssize_t Length(PyObject* _self);
  static PyObject* GetIterator(PyObject *_self);
  static PyObject* IterNext(PyObject* _self);

  // Methods that differ between the map types.
  static PyObject* ScalarMapGetItem(PyObject* _self, PyObject* key);
  static PyObject* MessageMapGetItem(PyObject* _self, PyObject* key);
  static int ScalarMapSetItem(PyObject* _self, PyObject* key, PyObject* v);
  static int MessageMapSetItem(PyObject* _self, PyObject* key, PyObject* v);
};

struct MapIterator {
  PyObject_HEAD;

  google::protobuf::scoped_ptr< ::google::protobuf::MapIterator> iter;

  // A pointer back to the container, so we can notice changes to the version.
  // We own a ref on this.
  MapContainer* container;

  // We need to keep a ref on the Message* too, because
  // MapIterator::~MapIterator() accesses it.  Normally this would be ok because
  // the ref on container (above) would guarantee outlive semantics.  However in
  // the case of ClearField(), InitializeAndCopyToParentContainer() resets the
  // message pointer (and the owner) to a different message, a copy of the
  // original.  But our iterator still points to the original, which could now
  // get deleted before us.
  //
  // To prevent this, we ensure that the Message will always stay alive as long
  // as this iterator does.  This is solely for the benefit of the MapIterator
  // destructor -- we should never actually access the iterator in this state
  // except to delete it.
  shared_ptr<Message> owner;

  // The version of the map when we took the iterator to it.
  //
  // We store this so that if the map is modified during iteration we can throw
  // an error.
  uint64 version;

  // True if the container is empty.  We signal this separately to avoid calling
  // any of the iteration methods, which are non-const.
  bool empty;
};

Message* MapContainer::GetMutableMessage() {
  cmessage::AssureWritable(parent);
  return const_cast<Message*>(message);
}

// Consumes a reference on the Python string object.
static bool PyStringToSTL(PyObject* py_string, string* stl_string) {
  char *value;
  Py_ssize_t value_len;

  if (!py_string) {
    return false;
  }
  if (PyBytes_AsStringAndSize(py_string, &value, &value_len) < 0) {
    Py_DECREF(py_string);
    return false;
  } else {
    stl_string->assign(value, value_len);
    Py_DECREF(py_string);
    return true;
  }
}

static bool PythonToMapKey(PyObject* obj,
                           const FieldDescriptor* field_descriptor,
                           MapKey* key) {
  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: {
      GOOGLE_CHECK_GET_INT32(obj, value, false);
      key->SetInt32Value(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      GOOGLE_CHECK_GET_INT64(obj, value, false);
      key->SetInt64Value(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT32: {
      GOOGLE_CHECK_GET_UINT32(obj, value, false);
      key->SetUInt32Value(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT64: {
      GOOGLE_CHECK_GET_UINT64(obj, value, false);
      key->SetUInt64Value(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_BOOL: {
      GOOGLE_CHECK_GET_BOOL(obj, value, false);
      key->SetBoolValue(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      string str;
      if (!PyStringToSTL(CheckString(obj, field_descriptor), &str)) {
        return false;
      }
      key->SetStringValue(str);
      break;
    }
    default:
      PyErr_Format(
          PyExc_SystemError, "Type %d cannot be a map key",
          field_descriptor->cpp_type());
      return false;
  }
  return true;
}

static PyObject* MapKeyToPython(const FieldDescriptor* field_descriptor,
                                const MapKey& key) {
  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return PyInt_FromLong(key.GetInt32Value());
    case FieldDescriptor::CPPTYPE_INT64:
      return PyLong_FromLongLong(key.GetInt64Value());
    case FieldDescriptor::CPPTYPE_UINT32:
      return PyInt_FromSize_t(key.GetUInt32Value());
    case FieldDescriptor::CPPTYPE_UINT64:
      return PyLong_FromUnsignedLongLong(key.GetUInt64Value());
    case FieldDescriptor::CPPTYPE_BOOL:
      return PyBool_FromLong(key.GetBoolValue());
    case FieldDescriptor::CPPTYPE_STRING:
      return ToStringObject(field_descriptor, key.GetStringValue());
    default:
      PyErr_Format(
          PyExc_SystemError, "Couldn't convert type %d to value",
          field_descriptor->cpp_type());
      return NULL;
  }
}

// This is only used for ScalarMap, so we don't need to handle the
// CPPTYPE_MESSAGE case.
PyObject* MapValueRefToPython(const FieldDescriptor* field_descriptor,
                              MapValueRef* value) {
  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return PyInt_FromLong(value->GetInt32Value());
    case FieldDescriptor::CPPTYPE_INT64:
      return PyLong_FromLongLong(value->GetInt64Value());
    case FieldDescriptor::CPPTYPE_UINT32:
      return PyInt_FromSize_t(value->GetUInt32Value());
    case FieldDescriptor::CPPTYPE_UINT64:
      return PyLong_FromUnsignedLongLong(value->GetUInt64Value());
    case FieldDescriptor::CPPTYPE_FLOAT:
      return PyFloat_FromDouble(value->GetFloatValue());
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return PyFloat_FromDouble(value->GetDoubleValue());
    case FieldDescriptor::CPPTYPE_BOOL:
      return PyBool_FromLong(value->GetBoolValue());
    case FieldDescriptor::CPPTYPE_STRING:
      return ToStringObject(field_descriptor, value->GetStringValue());
    case FieldDescriptor::CPPTYPE_ENUM:
      return PyInt_FromLong(value->GetEnumValue());
    default:
      PyErr_Format(
          PyExc_SystemError, "Couldn't convert type %d to value",
          field_descriptor->cpp_type());
      return NULL;
  }
}

// This is only used for ScalarMap, so we don't need to handle the
// CPPTYPE_MESSAGE case.
static bool PythonToMapValueRef(PyObject* obj,
                                const FieldDescriptor* field_descriptor,
                                bool allow_unknown_enum_values,
                                MapValueRef* value_ref) {
  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: {
      GOOGLE_CHECK_GET_INT32(obj, value, false);
      value_ref->SetInt32Value(value);
      return true;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      GOOGLE_CHECK_GET_INT64(obj, value, false);
      value_ref->SetInt64Value(value);
      return true;
    }
    case FieldDescriptor::CPPTYPE_UINT32: {
      GOOGLE_CHECK_GET_UINT32(obj, value, false);
      value_ref->SetUInt32Value(value);
      return true;
    }
    case FieldDescriptor::CPPTYPE_UINT64: {
      GOOGLE_CHECK_GET_UINT64(obj, value, false);
      value_ref->SetUInt64Value(value);
      return true;
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      GOOGLE_CHECK_GET_FLOAT(obj, value, false);
      value_ref->SetFloatValue(value);
      return true;
    }
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      GOOGLE_CHECK_GET_DOUBLE(obj, value, false);
      value_ref->SetDoubleValue(value);
      return true;
    }
    case FieldDescriptor::CPPTYPE_BOOL: {
      GOOGLE_CHECK_GET_BOOL(obj, value, false);
      value_ref->SetBoolValue(value);
      return true;;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      string str;
      if (!PyStringToSTL(CheckString(obj, field_descriptor), &str)) {
        return false;
      }
      value_ref->SetStringValue(str);
      return true;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      GOOGLE_CHECK_GET_INT32(obj, value, false);
      if (allow_unknown_enum_values) {
        value_ref->SetEnumValue(value);
        return true;
      } else {
        const EnumDescriptor* enum_descriptor = field_descriptor->enum_type();
        const EnumValueDescriptor* enum_value =
            enum_descriptor->FindValueByNumber(value);
        if (enum_value != NULL) {
          value_ref->SetEnumValue(value);
          return true;
        } else {
          PyErr_Format(PyExc_ValueError, "Unknown enum value: %d", value);
          return false;
        }
      }
      break;
    }
    default:
      PyErr_Format(
          PyExc_SystemError, "Setting value to a field of unknown type %d",
          field_descriptor->cpp_type());
      return false;
  }
}

// Map methods common to ScalarMap and MessageMap //////////////////////////////

static MapContainer* GetMap(PyObject* obj) {
  return reinterpret_cast<MapContainer*>(obj);
}

Py_ssize_t MapReflectionFriend::Length(PyObject* _self) {
  MapContainer* self = GetMap(_self);
  const google::protobuf::Message* message = self->message;
  return message->GetReflection()->MapSize(*message,
                                           self->parent_field_descriptor);
}

PyObject* Clear(PyObject* _self) {
  MapContainer* self = GetMap(_self);
  Message* message = self->GetMutableMessage();
  const Reflection* reflection = message->GetReflection();

  reflection->ClearField(message, self->parent_field_descriptor);

  Py_RETURN_NONE;
}

PyObject* GetEntryClass(PyObject* _self) {
  MapContainer* self = GetMap(_self);
  CMessageClass* message_class = message_factory::GetMessageClass(
      cmessage::GetFactoryForMessage(self->parent),
      self->parent_field_descriptor->message_type());
  Py_XINCREF(message_class);
  return reinterpret_cast<PyObject*>(message_class);
}

PyObject* MapReflectionFriend::Contains(PyObject* _self, PyObject* key) {
  MapContainer* self = GetMap(_self);

  const Message* message = self->message;
  const Reflection* reflection = message->GetReflection();
  MapKey map_key;

  if (!PythonToMapKey(key, self->key_field_descriptor, &map_key)) {
    return NULL;
  }

  if (reflection->ContainsMapKey(*message, self->parent_field_descriptor,
                                 map_key)) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

// Initializes the underlying Message object of "to" so it becomes a new parent
// map container, and copies all the values from "from" to it. A child map
// container can be released by passing it as both from and to (e.g. making it
// the recipient of the new parent message and copying the values from itself).
// In fact, this is the only supported use at the moment.
static int InitializeAndCopyToParentContainer(MapContainer* from,
                                              MapContainer* to) {
  // For now we require from == to, re-evaluate if we want to support deep copy
  // as in repeated_scalar_container.cc.
  GOOGLE_DCHECK(from == to);
  Message* new_message = from->message->New();

  if (MapReflectionFriend::Length(reinterpret_cast<PyObject*>(from)) > 0) {
    // A somewhat roundabout way of copying just one field from old_message to
    // new_message.  This is the best we can do with what Reflection gives us.
    Message* mutable_old = from->GetMutableMessage();
    std::vector<const FieldDescriptor*> fields;
    fields.push_back(from->parent_field_descriptor);

    // Move the map field into the new message.
    mutable_old->GetReflection()->SwapFields(mutable_old, new_message, fields);

    // If/when we support from != to, this will be required also to copy the
    // map field back into the existing message:
    // mutable_old->MergeFrom(*new_message);
  }

  // If from == to this could delete old_message.
  to->owner.reset(new_message);

  to->parent = NULL;
  to->parent_field_descriptor = from->parent_field_descriptor;
  to->message = new_message;

  // Invalidate iterators, since they point to the old copy of the field.
  to->version++;

  return 0;
}

int MapContainer::Release() {
  return InitializeAndCopyToParentContainer(this, this);
}


// ScalarMap ///////////////////////////////////////////////////////////////////

PyObject *NewScalarMapContainer(
    CMessage* parent, const google::protobuf::FieldDescriptor* parent_field_descriptor) {
  if (!CheckFieldBelongsToMessage(parent_field_descriptor, parent->message)) {
    return NULL;
  }

  ScopedPyObjectPtr obj(PyType_GenericAlloc(ScalarMapContainer_Type, 0));
  if (obj.get() == NULL) {
    return PyErr_Format(PyExc_RuntimeError,
                        "Could not allocate new container.");
  }

  MapContainer* self = GetMap(obj.get());

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

PyObject* MapReflectionFriend::ScalarMapGetItem(PyObject* _self,
                                                PyObject* key) {
  MapContainer* self = GetMap(_self);

  Message* message = self->GetMutableMessage();
  const Reflection* reflection = message->GetReflection();
  MapKey map_key;
  MapValueRef value;

  if (!PythonToMapKey(key, self->key_field_descriptor, &map_key)) {
    return NULL;
  }

  if (reflection->InsertOrLookupMapValue(message, self->parent_field_descriptor,
                                         map_key, &value)) {
    self->version++;
  }

  return MapValueRefToPython(self->value_field_descriptor, &value);
}

int MapReflectionFriend::ScalarMapSetItem(PyObject* _self, PyObject* key,
                                          PyObject* v) {
  MapContainer* self = GetMap(_self);

  Message* message = self->GetMutableMessage();
  const Reflection* reflection = message->GetReflection();
  MapKey map_key;
  MapValueRef value;

  if (!PythonToMapKey(key, self->key_field_descriptor, &map_key)) {
    return -1;
  }

  self->version++;

  if (v) {
    // Set item to v.
    reflection->InsertOrLookupMapValue(message, self->parent_field_descriptor,
                                       map_key, &value);

    return PythonToMapValueRef(v, self->value_field_descriptor,
                               reflection->SupportsUnknownEnumValues(), &value)
               ? 0
               : -1;
  } else {
    // Delete key from map.
    if (reflection->DeleteMapValue(message, self->parent_field_descriptor,
                                   map_key)) {
      return 0;
    } else {
      PyErr_Format(PyExc_KeyError, "Key not present in map");
      return -1;
    }
  }
}

static PyObject* ScalarMapGet(PyObject* self, PyObject* args) {
  PyObject* key;
  PyObject* default_value = NULL;
  if (PyArg_ParseTuple(args, "O|O", &key, &default_value) < 0) {
    return NULL;
  }

  ScopedPyObjectPtr is_present(MapReflectionFriend::Contains(self, key));
  if (is_present.get() == NULL) {
    return NULL;
  }

  if (PyObject_IsTrue(is_present.get())) {
    return MapReflectionFriend::ScalarMapGetItem(self, key);
  } else {
    if (default_value != NULL) {
      Py_INCREF(default_value);
      return default_value;
    } else {
      Py_RETURN_NONE;
    }
  }
}

static void ScalarMapDealloc(PyObject* _self) {
  MapContainer* self = GetMap(_self);
  self->owner.reset();
  Py_TYPE(_self)->tp_free(_self);
}

static PyMethodDef ScalarMapMethods[] = {
  { "__contains__", MapReflectionFriend::Contains, METH_O,
    "Tests whether a key is a member of the map." },
  { "clear", (PyCFunction)Clear, METH_NOARGS,
    "Removes all elements from the map." },
  { "get", ScalarMapGet, METH_VARARGS,
    "Gets the value for the given key if present, or otherwise a default" },
  { "GetEntryClass", (PyCFunction)GetEntryClass, METH_NOARGS,
    "Return the class used to build Entries of (key, value) pairs." },
  /*
  { "__deepcopy__", (PyCFunction)DeepCopy, METH_VARARGS,
    "Makes a deep copy of the class." },
  { "__reduce__", (PyCFunction)Reduce, METH_NOARGS,
    "Outputs picklable representation of the repeated field." },
  */
  {NULL, NULL},
};

PyTypeObject *ScalarMapContainer_Type;
#if PY_MAJOR_VERSION >= 3
  static PyType_Slot ScalarMapContainer_Type_slots[] = {
      {Py_tp_dealloc, (void *)ScalarMapDealloc},
      {Py_mp_length, (void *)MapReflectionFriend::Length},
      {Py_mp_subscript, (void *)MapReflectionFriend::ScalarMapGetItem},
      {Py_mp_ass_subscript, (void *)MapReflectionFriend::ScalarMapSetItem},
      {Py_tp_methods, (void *)ScalarMapMethods},
      {Py_tp_iter, (void *)MapReflectionFriend::GetIterator},
      {0, 0},
  };

  PyType_Spec ScalarMapContainer_Type_spec = {
      FULL_MODULE_NAME ".ScalarMapContainer",
      sizeof(MapContainer),
      0,
      Py_TPFLAGS_DEFAULT,
      ScalarMapContainer_Type_slots
  };
#else
  static PyMappingMethods ScalarMapMappingMethods = {
    MapReflectionFriend::Length,             // mp_length
    MapReflectionFriend::ScalarMapGetItem,   // mp_subscript
    MapReflectionFriend::ScalarMapSetItem,   // mp_ass_subscript
  };

  PyTypeObject _ScalarMapContainer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    FULL_MODULE_NAME ".ScalarMapContainer",  //  tp_name
    sizeof(MapContainer),                //  tp_basicsize
    0,                                   //  tp_itemsize
    ScalarMapDealloc,                    //  tp_dealloc
    0,                                   //  tp_print
    0,                                   //  tp_getattr
    0,                                   //  tp_setattr
    0,                                   //  tp_compare
    0,                                   //  tp_repr
    0,                                   //  tp_as_number
    0,                                   //  tp_as_sequence
    &ScalarMapMappingMethods,            //  tp_as_mapping
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
    MapReflectionFriend::GetIterator,    //  tp_iter
    0,                                   //  tp_iternext
    ScalarMapMethods,                    //  tp_methods
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


// MessageMap //////////////////////////////////////////////////////////////////

static MessageMapContainer* GetMessageMap(PyObject* obj) {
  return reinterpret_cast<MessageMapContainer*>(obj);
}

static PyObject* GetCMessage(MessageMapContainer* self, Message* message) {
  // Get or create the CMessage object corresponding to this message.
  ScopedPyObjectPtr key(PyLong_FromVoidPtr(message));
  PyObject* ret = PyDict_GetItem(self->message_dict, key.get());

  if (ret == NULL) {
    CMessage* cmsg = cmessage::NewEmptyMessage(self->message_class);
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

PyObject* NewMessageMapContainer(
    CMessage* parent, const google::protobuf::FieldDescriptor* parent_field_descriptor,
    CMessageClass* message_class) {
  if (!CheckFieldBelongsToMessage(parent_field_descriptor, parent->message)) {
    return NULL;
  }

  PyObject* obj = PyType_GenericAlloc(MessageMapContainer_Type, 0);
  if (obj == NULL) {
    return PyErr_Format(PyExc_RuntimeError,
                        "Could not allocate new container.");
  }

  MessageMapContainer* self = GetMessageMap(obj);

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

  Py_INCREF(message_class);
  self->message_class = message_class;

  if (self->key_field_descriptor == NULL ||
      self->value_field_descriptor == NULL) {
    Py_DECREF(obj);
    return PyErr_Format(PyExc_KeyError,
                        "Map entry descriptor did not have key/value fields");
  }

  return obj;
}

int MapReflectionFriend::MessageMapSetItem(PyObject* _self, PyObject* key,
                                           PyObject* v) {
  if (v) {
    PyErr_Format(PyExc_ValueError,
                 "Direct assignment of submessage not allowed");
    return -1;
  }

  // Now we know that this is a delete, not a set.

  MessageMapContainer* self = GetMessageMap(_self);
  Message* message = self->GetMutableMessage();
  const Reflection* reflection = message->GetReflection();
  MapKey map_key;
  MapValueRef value;

  self->version++;

  if (!PythonToMapKey(key, self->key_field_descriptor, &map_key)) {
    return -1;
  }

  // Delete key from map.
  if (reflection->ContainsMapKey(*message, self->parent_field_descriptor,
                                 map_key)) {
    // Delete key from CMessage dict.
    MapValueRef value;
    reflection->InsertOrLookupMapValue(message, self->parent_field_descriptor,
                                       map_key, &value);
    ScopedPyObjectPtr key(PyLong_FromVoidPtr(value.MutableMessageValue()));

    // PyDict_DelItem will have key error if the key is not in the map. We do
    // not want to call PyErr_Clear() which may clear other errors. Thus
    // PyDict_Contains() check is called before delete.
    int contains = PyDict_Contains(self->message_dict, key.get());
    if (contains < 0) {
      return -1;
    }
    if (contains) {
      if (PyDict_DelItem(self->message_dict, key.get()) < 0) {
        return -1;
      }
    }

    // Delete key from map.
    reflection->DeleteMapValue(message, self->parent_field_descriptor,
                               map_key);
    return 0;
  } else {
    PyErr_Format(PyExc_KeyError, "Key not present in map");
    return -1;
  }
}

PyObject* MapReflectionFriend::MessageMapGetItem(PyObject* _self,
                                                 PyObject* key) {
  MessageMapContainer* self = GetMessageMap(_self);

  Message* message = self->GetMutableMessage();
  const Reflection* reflection = message->GetReflection();
  MapKey map_key;
  MapValueRef value;

  if (!PythonToMapKey(key, self->key_field_descriptor, &map_key)) {
    return NULL;
  }

  if (reflection->InsertOrLookupMapValue(message, self->parent_field_descriptor,
                                         map_key, &value)) {
    self->version++;
  }

  return GetCMessage(self, value.MutableMessageValue());
}

PyObject* MessageMapGet(PyObject* self, PyObject* args) {
  PyObject* key;
  PyObject* default_value = NULL;
  if (PyArg_ParseTuple(args, "O|O", &key, &default_value) < 0) {
    return NULL;
  }

  ScopedPyObjectPtr is_present(MapReflectionFriend::Contains(self, key));
  if (is_present.get() == NULL) {
    return NULL;
  }

  if (PyObject_IsTrue(is_present.get())) {
    return MapReflectionFriend::MessageMapGetItem(self, key);
  } else {
    if (default_value != NULL) {
      Py_INCREF(default_value);
      return default_value;
    } else {
      Py_RETURN_NONE;
    }
  }
}

static void MessageMapDealloc(PyObject* _self) {
  MessageMapContainer* self = GetMessageMap(_self);
  self->owner.reset();
  Py_DECREF(self->message_dict);
  Py_DECREF(self->message_class);
  Py_TYPE(_self)->tp_free(_self);
}

static PyMethodDef MessageMapMethods[] = {
  { "__contains__", (PyCFunction)MapReflectionFriend::Contains, METH_O,
    "Tests whether the map contains this element."},
  { "clear", (PyCFunction)Clear, METH_NOARGS,
    "Removes all elements from the map."},
  { "get", MessageMapGet, METH_VARARGS,
    "Gets the value for the given key if present, or otherwise a default" },
  { "get_or_create", MapReflectionFriend::MessageMapGetItem, METH_O,
    "Alias for getitem, useful to make explicit that the map is mutated." },
  { "GetEntryClass", (PyCFunction)GetEntryClass, METH_NOARGS,
    "Return the class used to build Entries of (key, value) pairs." },
  /*
  { "__deepcopy__", (PyCFunction)DeepCopy, METH_VARARGS,
    "Makes a deep copy of the class." },
  { "__reduce__", (PyCFunction)Reduce, METH_NOARGS,
    "Outputs picklable representation of the repeated field." },
  */
  {NULL, NULL},
};

PyTypeObject *MessageMapContainer_Type;
#if PY_MAJOR_VERSION >= 3
  static PyType_Slot MessageMapContainer_Type_slots[] = {
      {Py_tp_dealloc, (void *)MessageMapDealloc},
      {Py_mp_length, (void *)MapReflectionFriend::Length},
      {Py_mp_subscript, (void *)MapReflectionFriend::MessageMapGetItem},
      {Py_mp_ass_subscript, (void *)MapReflectionFriend::MessageMapSetItem},
      {Py_tp_methods, (void *)MessageMapMethods},
      {Py_tp_iter, (void *)MapReflectionFriend::GetIterator},
      {0, 0}
  };

  PyType_Spec MessageMapContainer_Type_spec = {
      FULL_MODULE_NAME ".MessageMapContainer",
      sizeof(MessageMapContainer),
      0,
      Py_TPFLAGS_DEFAULT,
      MessageMapContainer_Type_slots
  };
#else
  static PyMappingMethods MessageMapMappingMethods = {
    MapReflectionFriend::Length,              // mp_length
    MapReflectionFriend::MessageMapGetItem,   // mp_subscript
    MapReflectionFriend::MessageMapSetItem,   // mp_ass_subscript
  };

  PyTypeObject _MessageMapContainer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    FULL_MODULE_NAME ".MessageMapContainer",  //  tp_name
    sizeof(MessageMapContainer),         //  tp_basicsize
    0,                                   //  tp_itemsize
    MessageMapDealloc,                   //  tp_dealloc
    0,                                   //  tp_print
    0,                                   //  tp_getattr
    0,                                   //  tp_setattr
    0,                                   //  tp_compare
    0,                                   //  tp_repr
    0,                                   //  tp_as_number
    0,                                   //  tp_as_sequence
    &MessageMapMappingMethods,           //  tp_as_mapping
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
    MapReflectionFriend::GetIterator,    //  tp_iter
    0,                                   //  tp_iternext
    MessageMapMethods,                   //  tp_methods
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

// MapIterator /////////////////////////////////////////////////////////////////

static MapIterator* GetIter(PyObject* obj) {
  return reinterpret_cast<MapIterator*>(obj);
}

PyObject* MapReflectionFriend::GetIterator(PyObject *_self) {
  MapContainer* self = GetMap(_self);

  ScopedPyObjectPtr obj(PyType_GenericAlloc(&MapIterator_Type, 0));
  if (obj == NULL) {
    return PyErr_Format(PyExc_KeyError, "Could not allocate iterator");
  }

  MapIterator* iter = GetIter(obj.get());

  Py_INCREF(self);
  iter->container = self;
  iter->version = self->version;
  iter->owner = self->owner;

  if (MapReflectionFriend::Length(_self) > 0) {
    Message* message = self->GetMutableMessage();
    const Reflection* reflection = message->GetReflection();

    iter->iter.reset(new ::google::protobuf::MapIterator(
        reflection->MapBegin(message, self->parent_field_descriptor)));
  }

  return obj.release();
}

PyObject* MapReflectionFriend::IterNext(PyObject* _self) {
  MapIterator* self = GetIter(_self);

  // This won't catch mutations to the map performed by MergeFrom(); no easy way
  // to address that.
  if (self->version != self->container->version) {
    return PyErr_Format(PyExc_RuntimeError,
                        "Map modified during iteration.");
  }

  if (self->iter.get() == NULL) {
    return NULL;
  }

  Message* message = self->container->GetMutableMessage();
  const Reflection* reflection = message->GetReflection();

  if (*self->iter ==
      reflection->MapEnd(message, self->container->parent_field_descriptor)) {
    return NULL;
  }

  PyObject* ret = MapKeyToPython(self->container->key_field_descriptor,
                                 self->iter->GetKey());

  ++(*self->iter);

  return ret;
}

static void DeallocMapIterator(PyObject* _self) {
  MapIterator* self = GetIter(_self);
  self->iter.reset();
  self->owner.reset();
  Py_XDECREF(self->container);
  Py_TYPE(_self)->tp_free(_self);
}

PyTypeObject MapIterator_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  FULL_MODULE_NAME ".MapIterator",     //  tp_name
  sizeof(MapIterator),                 //  tp_basicsize
  0,                                   //  tp_itemsize
  DeallocMapIterator,                  //  tp_dealloc
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
  MapReflectionFriend::IterNext,       //  tp_iternext
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

bool InitMapContainers() {
  // ScalarMapContainer_Type derives from our MutableMapping type.
  ScopedPyObjectPtr containers(PyImport_ImportModule(
      "google.protobuf.internal.containers"));
  if (containers == NULL) {
    return false;
  }

  ScopedPyObjectPtr mutable_mapping(
      PyObject_GetAttrString(containers.get(), "MutableMapping"));
  if (mutable_mapping == NULL) {
    return false;
  }

  if (!PyObject_TypeCheck(mutable_mapping.get(), &PyType_Type)) {
    return false;
  }

  Py_INCREF(mutable_mapping.get());
#if PY_MAJOR_VERSION >= 3
  PyObject* bases = PyTuple_New(1);
  PyTuple_SET_ITEM(bases, 0, mutable_mapping.get());

  ScalarMapContainer_Type = reinterpret_cast<PyTypeObject*>(
      PyType_FromSpecWithBases(&ScalarMapContainer_Type_spec, bases));
#else
  _ScalarMapContainer_Type.tp_base =
      reinterpret_cast<PyTypeObject*>(mutable_mapping.get());

  if (PyType_Ready(&_ScalarMapContainer_Type) < 0) {
    return false;
  }

  ScalarMapContainer_Type = &_ScalarMapContainer_Type;
#endif

  if (PyType_Ready(&MapIterator_Type) < 0) {
    return false;
  }

#if PY_MAJOR_VERSION >= 3
  MessageMapContainer_Type = reinterpret_cast<PyTypeObject*>(
      PyType_FromSpecWithBases(&MessageMapContainer_Type_spec, bases));
#else
  Py_INCREF(mutable_mapping.get());
  _MessageMapContainer_Type.tp_base =
      reinterpret_cast<PyTypeObject*>(mutable_mapping.get());

  if (PyType_Ready(&_MessageMapContainer_Type) < 0) {
    return false;
  }

  MessageMapContainer_Type = &_MessageMapContainer_Type;
#endif
  return true;
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
