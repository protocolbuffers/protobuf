// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: anuraag@google.com (Anuraag Agrawal)
// Author: tibell@google.com (Johan Tibell)

#include "google/protobuf/pyext/extension_dict.h"

#include <cstdint>
#include <memory>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
#include "google/protobuf/pyext/descriptor.h"
#include "google/protobuf/pyext/message.h"
#include "google/protobuf/pyext/message_factory.h"
#include "google/protobuf/pyext/repeated_composite_container.h"
#include "google/protobuf/pyext/repeated_scalar_container.h"
#include "google/protobuf/pyext/scoped_pyobject_ptr.h"

#define PyString_AsStringAndSize(ob, charpp, sizep)              \
  (PyUnicode_Check(ob)                                           \
       ? ((*(charpp) = const_cast<char*>(                        \
               PyUnicode_AsUTF8AndSize(ob, (sizep)))) == nullptr \
              ? -1                                               \
              : 0)                                               \
       : PyBytes_AsStringAndSize(ob, (charpp), (sizep)))

namespace google {
namespace protobuf {
namespace python {

namespace extension_dict {

static Py_ssize_t len(ExtensionDict* self) {
  Py_ssize_t size = 0;
  std::vector<const FieldDescriptor*> fields;
  self->parent->message->GetReflection()->ListFields(*self->parent->message,
                                                     &fields);

  for (size_t i = 0; i < fields.size(); ++i) {
    if (fields[i]->is_extension()) {
      // With C++ descriptors, the field can always be retrieved, but for
      // unknown extensions which have not been imported in Python code, there
      // is no message class and we cannot retrieve the value.
      // ListFields() has the same behavior.
      if (fields[i]->message_type() != nullptr &&
          message_factory::GetMessageClass(
              cmessage::GetFactoryForMessage(self->parent),
              fields[i]->message_type()) == nullptr) {
        PyErr_Clear();
        continue;
      }
      ++size;
    }
  }
  return size;
}

struct ExtensionIterator {
  // clang-format off
  PyObject_HEAD
  Py_ssize_t index;
  // clang-format on
  std::vector<const FieldDescriptor*> fields;

  // Owned reference, to keep the FieldDescriptors alive.
  ExtensionDict* extension_dict;
};

PyObject* GetIter(PyObject* _self) {
  ExtensionDict* self = reinterpret_cast<ExtensionDict*>(_self);

  ScopedPyObjectPtr obj(PyType_GenericAlloc(&ExtensionIterator_Type, 0));
  if (obj == nullptr) {
    return PyErr_Format(PyExc_MemoryError,
                        "Could not allocate extension iterator");
  }

  ExtensionIterator* iter = reinterpret_cast<ExtensionIterator*>(obj.get());

  // Call "placement new" to initialize. So the constructor of
  // std::vector<...> fields will be called.
  new (iter) ExtensionIterator;

  self->parent->message->GetReflection()->ListFields(*self->parent->message,
                                                     &iter->fields);
  iter->index = 0;
  Py_INCREF(self);
  iter->extension_dict = self;

  return obj.release();
}

static void DeallocExtensionIterator(PyObject* _self) {
  ExtensionIterator* self = reinterpret_cast<ExtensionIterator*>(_self);
  self->fields.clear();
  Py_XDECREF(self->extension_dict);
  freefunc tp_free = Py_TYPE(_self)->tp_free;
  self->~ExtensionIterator();
  (*tp_free)(_self);
}

PyObject* subscript(ExtensionDict* self, PyObject* key) {
  const FieldDescriptor* descriptor = cmessage::GetExtensionDescriptor(key);
  if (descriptor == nullptr) {
    return nullptr;
  }
  if (!CheckFieldBelongsToMessage(descriptor, self->parent->message)) {
    return nullptr;
  }

  if (descriptor->label() != FieldDescriptor::LABEL_REPEATED &&
      descriptor->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    return cmessage::InternalGetScalar(self->parent->message, descriptor);
  }

  CMessage::CompositeFieldsMap::iterator iterator =
      self->parent->composite_fields->find(descriptor);
  if (iterator != self->parent->composite_fields->end()) {
    Py_INCREF(iterator->second);
    return iterator->second->AsPyObject();
  }

  if (descriptor->label() != FieldDescriptor::LABEL_REPEATED &&
      descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    // TODO: consider building the class on the fly!
    ContainerBase* sub_message =
        cmessage::InternalGetSubMessage(self->parent, descriptor);
    if (sub_message == nullptr) {
      return nullptr;
    }
    (*self->parent->composite_fields)[descriptor] = sub_message;
    return sub_message->AsPyObject();
  }

  if (descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
    if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      // On the fly message class creation is needed to support the following
      // situation:
      // 1- add FileDescriptor to the pool that contains extensions of a message
      //    defined by another proto file. Do not create any message classes.
      // 2- instantiate an extended message, and access the extension using
      //    the field descriptor.
      // 3- the extension submessage fails to be returned, because no class has
      //    been created.
      // It happens when deserializing text proto format, or when enumerating
      // fields of a deserialized message.
      CMessageClass* message_class = message_factory::GetOrCreateMessageClass(
          cmessage::GetFactoryForMessage(self->parent),
          descriptor->message_type());
      ScopedPyObjectPtr message_class_handler(
          reinterpret_cast<PyObject*>(message_class));
      if (message_class == nullptr) {
        return nullptr;
      }
      ContainerBase* py_container = repeated_composite_container::NewContainer(
          self->parent, descriptor, message_class);
      if (py_container == nullptr) {
        return nullptr;
      }
      (*self->parent->composite_fields)[descriptor] = py_container;
      return py_container->AsPyObject();
    } else {
      ContainerBase* py_container =
          repeated_scalar_container::NewContainer(self->parent, descriptor);
      if (py_container == nullptr) {
        return nullptr;
      }
      (*self->parent->composite_fields)[descriptor] = py_container;
      return py_container->AsPyObject();
    }
  }
  PyErr_SetString(PyExc_ValueError, "control reached unexpected line");
  return nullptr;
}

int ass_subscript(ExtensionDict* self, PyObject* key, PyObject* value) {
  const FieldDescriptor* descriptor = cmessage::GetExtensionDescriptor(key);
  if (descriptor == nullptr) {
    return -1;
  }
  if (!CheckFieldBelongsToMessage(descriptor, self->parent->message)) {
    return -1;
  }

  if (value == nullptr) {
    return cmessage::ClearFieldByDescriptor(self->parent, descriptor);
  }

  if (descriptor->label() != FieldDescriptor::LABEL_OPTIONAL ||
      descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    PyErr_SetString(PyExc_TypeError,
                    "Extension is repeated and/or composite "
                    "type");
    return -1;
  }
  cmessage::AssureWritable(self->parent);
  if (cmessage::InternalSetScalar(self->parent, descriptor, value) < 0) {
    return -1;
  }
  return 0;
}

static const FieldDescriptor* FindMessageSetExtension(
    const Descriptor* message_descriptor) {
  for (int i = 0; i < message_descriptor->extension_count(); i++) {
    const FieldDescriptor* extension = message_descriptor->extension(i);
    if (extension->is_extension() &&
        extension->containing_type()->options().message_set_wire_format() &&
        extension->type() == FieldDescriptor::TYPE_MESSAGE &&
        extension->label() == FieldDescriptor::LABEL_OPTIONAL &&
        extension->message_type() == message_descriptor) {
      return extension;
    }
  }
  return nullptr;
}

PyObject* _FindExtensionByName(ExtensionDict* self, PyObject* arg) {
  char* name;
  Py_ssize_t name_size;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return nullptr;
  }

  PyDescriptorPool* pool = cmessage::GetFactoryForMessage(self->parent)->pool;
  const FieldDescriptor* message_extension =
      pool->pool->FindExtensionByName(absl::string_view(name, name_size));
  if (message_extension == nullptr) {
    // Is is the name of a message set extension?
    const Descriptor* message_descriptor =
        pool->pool->FindMessageTypeByName(absl::string_view(name, name_size));
    if (message_descriptor) {
      message_extension = FindMessageSetExtension(message_descriptor);
    }
  }
  if (message_extension == nullptr) {
    Py_RETURN_NONE;
  }

  return PyFieldDescriptor_FromDescriptor(message_extension);
}

PyObject* _FindExtensionByNumber(ExtensionDict* self, PyObject* arg) {
  int64_t number = PyLong_AsLong(arg);
  if (number == -1 && PyErr_Occurred()) {
    return nullptr;
  }

  PyDescriptorPool* pool = cmessage::GetFactoryForMessage(self->parent)->pool;
  const FieldDescriptor* message_extension = pool->pool->FindExtensionByNumber(
      self->parent->message->GetDescriptor(), number);
  if (message_extension == nullptr) {
    Py_RETURN_NONE;
  }

  return PyFieldDescriptor_FromDescriptor(message_extension);
}

static int Contains(PyObject* _self, PyObject* key) {
  ExtensionDict* self = reinterpret_cast<ExtensionDict*>(_self);
  const FieldDescriptor* field_descriptor =
      cmessage::GetExtensionDescriptor(key);
  if (field_descriptor == nullptr) {
    return -1;
  }

  if (!field_descriptor->is_extension()) {
    PyErr_Format(PyExc_KeyError, "%s is not an extension",
                 std::string(field_descriptor->full_name()).c_str());
    return -1;
  }

  const Message* message = self->parent->message;
  const Reflection* reflection = message->GetReflection();
  if (field_descriptor->is_repeated()) {
    if (reflection->FieldSize(*message, field_descriptor) > 0) {
      return 1;
    }
  } else {
    if (reflection->HasField(*message, field_descriptor)) {
      return 1;
    }
  }

  return 0;
}

ExtensionDict* NewExtensionDict(CMessage* parent) {
  ExtensionDict* self = reinterpret_cast<ExtensionDict*>(
      PyType_GenericAlloc(&ExtensionDict_Type, 0));
  if (self == nullptr) {
    return nullptr;
  }

  Py_INCREF(parent);
  self->parent = parent;
  return self;
}

void dealloc(PyObject* pself) {
  ExtensionDict* self = reinterpret_cast<ExtensionDict*>(pself);
  Py_CLEAR(self->parent);
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

static PyObject* RichCompare(ExtensionDict* self, PyObject* other, int opid) {
  // Only equality comparisons are implemented.
  if (opid != Py_EQ && opid != Py_NE) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
  bool equals = false;
  if (PyObject_TypeCheck(other, &ExtensionDict_Type)) {
    equals = self->parent == reinterpret_cast<ExtensionDict*>(other)->parent;
  }
  if (equals ^ (opid == Py_EQ)) {
    Py_RETURN_FALSE;
  } else {
    Py_RETURN_TRUE;
  }
}
static PySequenceMethods SeqMethods = {
    (lenfunc)len,          // sq_length
    nullptr,               // sq_concat
    nullptr,               // sq_repeat
    nullptr,               // sq_item
    nullptr,               // sq_slice
    nullptr,               // sq_ass_item
    nullptr,               // sq_ass_slice
    (objobjproc)Contains,  // sq_contains
};

static PyMappingMethods MpMethods = {
    (lenfunc)len,                 /* mp_length */
    (binaryfunc)subscript,        /* mp_subscript */
    (objobjargproc)ass_subscript, /* mp_ass_subscript */
};

#define EDMETHOD(name, args, doc) {#name, (PyCFunction)name, args, doc}
static PyMethodDef Methods[] = {
    EDMETHOD(_FindExtensionByName, METH_O, "Finds an extension by name."),
    EDMETHOD(_FindExtensionByNumber, METH_O,
             "Finds an extension by field number."),
    {nullptr, nullptr},
};

}  // namespace extension_dict

PyTypeObject ExtensionDict_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  //
    FULL_MODULE_NAME ".ExtensionDict",      // tp_name
    sizeof(ExtensionDict),                  // tp_basicsize
    0,                                      //  tp_itemsize
    (destructor)extension_dict::dealloc,    //  tp_dealloc
#if PY_VERSION_HEX < 0x03080000
    nullptr,  // tp_print
#else
    0,  // tp_vectorcall_offset
#endif
    nullptr,                                   //  tp_getattr
    nullptr,                                   //  tp_setattr
    nullptr,                                   //  tp_compare
    nullptr,                                   //  tp_repr
    nullptr,                                   //  tp_as_number
    &extension_dict::SeqMethods,               //  tp_as_sequence
    &extension_dict::MpMethods,                //  tp_as_mapping
    PyObject_HashNotImplemented,               //  tp_hash
    nullptr,                                   //  tp_call
    nullptr,                                   //  tp_str
    nullptr,                                   //  tp_getattro
    nullptr,                                   //  tp_setattro
    nullptr,                                   //  tp_as_buffer
    Py_TPFLAGS_DEFAULT,                        //  tp_flags
    "An extension dict",                       //  tp_doc
    nullptr,                                   //  tp_traverse
    nullptr,                                   //  tp_clear
    (richcmpfunc)extension_dict::RichCompare,  //  tp_richcompare
    0,                                         //  tp_weaklistoffset
    extension_dict::GetIter,                   //  tp_iter
    nullptr,                                   //  tp_iternext
    extension_dict::Methods,                   //  tp_methods
    nullptr,                                   //  tp_members
    nullptr,                                   //  tp_getset
    nullptr,                                   //  tp_base
    nullptr,                                   //  tp_dict
    nullptr,                                   //  tp_descr_get
    nullptr,                                   //  tp_descr_set
    0,                                         //  tp_dictoffset
    nullptr,                                   //  tp_init
};

PyObject* IterNext(PyObject* _self) {
  extension_dict::ExtensionIterator* self =
      reinterpret_cast<extension_dict::ExtensionIterator*>(_self);
  Py_ssize_t total_size = self->fields.size();
  Py_ssize_t index = self->index;
  while (self->index < total_size) {
    index = self->index;
    ++self->index;
    if (self->fields[index]->is_extension()) {
      // With C++ descriptors, the field can always be retrieved, but for
      // unknown extensions which have not been imported in Python code, there
      // is no message class and we cannot retrieve the value.
      // ListFields() has the same behavior.
      if (self->fields[index]->message_type() != nullptr &&
          message_factory::GetMessageClass(
              cmessage::GetFactoryForMessage(self->extension_dict->parent),
              self->fields[index]->message_type()) == nullptr) {
        PyErr_Clear();
        continue;
      }

      return PyFieldDescriptor_FromDescriptor(self->fields[index]);
    }
  }

  return nullptr;
}

PyTypeObject ExtensionIterator_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)      //
    FULL_MODULE_NAME ".ExtensionIterator",      //  tp_name
    sizeof(extension_dict::ExtensionIterator),  //  tp_basicsize
    0,                                          //  tp_itemsize
    extension_dict::DeallocExtensionIterator,   //  tp_dealloc
#if PY_VERSION_HEX < 0x03080000
    nullptr,  // tp_print
#else
    0,  // tp_vectorcall_offset
#endif
    nullptr,                  //  tp_getattr
    nullptr,                  //  tp_setattr
    nullptr,                  //  tp_compare
    nullptr,                  //  tp_repr
    nullptr,                  //  tp_as_number
    nullptr,                  //  tp_as_sequence
    nullptr,                  //  tp_as_mapping
    nullptr,                  //  tp_hash
    nullptr,                  //  tp_call
    nullptr,                  //  tp_str
    nullptr,                  //  tp_getattro
    nullptr,                  //  tp_setattro
    nullptr,                  //  tp_as_buffer
    Py_TPFLAGS_DEFAULT,       //  tp_flags
    "A scalar map iterator",  //  tp_doc
    nullptr,                  //  tp_traverse
    nullptr,                  //  tp_clear
    nullptr,                  //  tp_richcompare
    0,                        //  tp_weaklistoffset
    PyObject_SelfIter,        //  tp_iter
    IterNext,                 //  tp_iternext
    nullptr,                  //  tp_methods
    nullptr,                  //  tp_members
    nullptr,                  //  tp_getset
    nullptr,                  //  tp_base
    nullptr,                  //  tp_dict
    nullptr,                  //  tp_descr_get
    nullptr,                  //  tp_descr_set
    0,                        //  tp_dictoffset
    nullptr,                  //  tp_init
};
}  // namespace python
}  // namespace protobuf
}  // namespace google
