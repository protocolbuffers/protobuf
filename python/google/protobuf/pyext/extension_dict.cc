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

#include <google/protobuf/pyext/extension_dict.h>
#include <memory>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/message_factory.h>
#include <google/protobuf/pyext/repeated_composite_container.h>
#include <google/protobuf/pyext/repeated_scalar_container.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

#if PY_MAJOR_VERSION >= 3
  #if PY_VERSION_HEX < 0x03030000
    #error "Python 3.0 - 3.2 are not supported."
  #endif
#define PyString_AsStringAndSize(ob, charpp, sizep)                           \
  (PyUnicode_Check(ob) ? ((*(charpp) = const_cast<char*>(                     \
                               PyUnicode_AsUTF8AndSize(ob, (sizep)))) == NULL \
                              ? -1                                            \
                              : 0)                                            \
                       : PyBytes_AsStringAndSize(ob, (charpp), (sizep)))
#endif

namespace google {
namespace protobuf {
namespace python {

namespace extension_dict {

PyObject* subscript(ExtensionDict* self, PyObject* key) {
  const FieldDescriptor* descriptor = cmessage::GetExtensionDescriptor(key);
  if (descriptor == NULL) {
    return NULL;
  }
  if (!CheckFieldBelongsToMessage(descriptor, self->parent->message)) {
    return NULL;
  }

  if (descriptor->label() != FieldDescriptor::LABEL_REPEATED &&
      descriptor->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    return cmessage::InternalGetScalar(self->parent->message, descriptor);
  }

  CMessage::CompositeFieldsMap::iterator iterator =
      self->parent->composite_fields->find(descriptor);
  if (iterator != self->parent->composite_fields->end()) {
    Py_INCREF(iterator->second);
    return iterator->second;
  }

  if (descriptor->label() != FieldDescriptor::LABEL_REPEATED &&
      descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    // TODO(plabatut): consider building the class on the fly!
    PyObject* sub_message = cmessage::InternalGetSubMessage(
        self->parent, descriptor);
    if (sub_message == NULL) {
      return NULL;
    }
    Py_INCREF(sub_message);
    (*self->parent->composite_fields)[descriptor] = sub_message;
    return sub_message;
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
      if (message_class == NULL) {
        return NULL;
      }
      PyObject* py_container = repeated_composite_container::NewContainer(
          self->parent, descriptor, message_class);
      if (py_container == NULL) {
        return NULL;
      }
      Py_INCREF(py_container);
      (*self->parent->composite_fields)[descriptor] = py_container;
      return py_container;
    } else {
      PyObject* py_container = repeated_scalar_container::NewContainer(
          self->parent, descriptor);
      if (py_container == NULL) {
        return NULL;
      }
      Py_INCREF(py_container);
      (*self->parent->composite_fields)[descriptor] = py_container;
      return py_container;
    }
  }
  PyErr_SetString(PyExc_ValueError, "control reached unexpected line");
  return NULL;
}

int ass_subscript(ExtensionDict* self, PyObject* key, PyObject* value) {
  const FieldDescriptor* descriptor = cmessage::GetExtensionDescriptor(key);
  if (descriptor == NULL) {
    return -1;
  }
  if (!CheckFieldBelongsToMessage(descriptor, self->parent->message)) {
    return -1;
  }

  if (descriptor->label() != FieldDescriptor::LABEL_OPTIONAL ||
      descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    PyErr_SetString(PyExc_TypeError, "Extension is repeated and/or composite "
                    "type");
    return -1;
  }
  cmessage::AssureWritable(self->parent);
  if (cmessage::InternalSetScalar(self->parent, descriptor, value) < 0) {
    return -1;
  }
  return 0;
}

PyObject* _FindExtensionByName(ExtensionDict* self, PyObject* arg) {
  char* name;
  Py_ssize_t name_size;
  if (PyString_AsStringAndSize(arg, &name, &name_size) < 0) {
    return NULL;
  }

  PyDescriptorPool* pool = cmessage::GetFactoryForMessage(self->parent)->pool;
  const FieldDescriptor* message_extension =
      pool->pool->FindExtensionByName(string(name, name_size));
  if (message_extension == NULL) {
    // Is is the name of a message set extension?
    const Descriptor* message_descriptor = pool->pool->FindMessageTypeByName(
        string(name, name_size));
    if (message_descriptor && message_descriptor->extension_count() > 0) {
      const FieldDescriptor* extension = message_descriptor->extension(0);
      if (extension->is_extension() &&
          extension->containing_type()->options().message_set_wire_format() &&
          extension->type() == FieldDescriptor::TYPE_MESSAGE &&
          extension->label() == FieldDescriptor::LABEL_OPTIONAL) {
        message_extension = extension;
      }
    }
  }
  if (message_extension == NULL) {
    Py_RETURN_NONE;
  }

  return PyFieldDescriptor_FromDescriptor(message_extension);
}

PyObject* _FindExtensionByNumber(ExtensionDict* self, PyObject* arg) {
  int64 number = PyLong_AsLong(arg);
  if (number == -1 && PyErr_Occurred()) {
    return NULL;
  }

  PyDescriptorPool* pool = cmessage::GetFactoryForMessage(self->parent)->pool;
  const FieldDescriptor* message_extension = pool->pool->FindExtensionByNumber(
      self->parent->message->GetDescriptor(), number);
  if (message_extension == NULL) {
    Py_RETURN_NONE;
  }

  return PyFieldDescriptor_FromDescriptor(message_extension);
}

ExtensionDict* NewExtensionDict(CMessage *parent) {
  ExtensionDict* self = reinterpret_cast<ExtensionDict*>(
      PyType_GenericAlloc(&ExtensionDict_Type, 0));
  if (self == NULL) {
    return NULL;
  }

  Py_INCREF(parent);
  self->parent = parent;
  return self;
}

void dealloc(ExtensionDict* self) {
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
    equals = self->parent == reinterpret_cast<ExtensionDict*>(other)->parent;;
  }
  if (equals ^ (opid == Py_EQ)) {
    Py_RETURN_FALSE;
  } else {
    Py_RETURN_TRUE;
  }
}

static PyMappingMethods MpMethods = {
  (lenfunc)NULL,               /* mp_length */
  (binaryfunc)subscript,       /* mp_subscript */
  (objobjargproc)ass_subscript,/* mp_ass_subscript */
};

#define EDMETHOD(name, args, doc) { #name, (PyCFunction)name, args, doc }
static PyMethodDef Methods[] = {
  EDMETHOD(_FindExtensionByName, METH_O,
           "Finds an extension by name."),
  EDMETHOD(_FindExtensionByNumber, METH_O,
           "Finds an extension by field number."),
  { NULL, NULL }
};

}  // namespace extension_dict

PyTypeObject ExtensionDict_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  FULL_MODULE_NAME ".ExtensionDict",   // tp_name
  sizeof(ExtensionDict),               // tp_basicsize
  0,                                   //  tp_itemsize
  (destructor)extension_dict::dealloc,  //  tp_dealloc
  0,                                   //  tp_print
  0,                                   //  tp_getattr
  0,                                   //  tp_setattr
  0,                                   //  tp_compare
  0,                                   //  tp_repr
  0,                                   //  tp_as_number
  0,                                   //  tp_as_sequence
  &extension_dict::MpMethods,          //  tp_as_mapping
  PyObject_HashNotImplemented,         //  tp_hash
  0,                                   //  tp_call
  0,                                   //  tp_str
  0,                                   //  tp_getattro
  0,                                   //  tp_setattro
  0,                                   //  tp_as_buffer
  Py_TPFLAGS_DEFAULT,                  //  tp_flags
  "An extension dict",                 //  tp_doc
  0,                                   //  tp_traverse
  0,                                   //  tp_clear
  (richcmpfunc)extension_dict::RichCompare,  //  tp_richcompare
  0,                                   //  tp_weaklistoffset
  0,                                   //  tp_iter
  0,                                   //  tp_iternext
  extension_dict::Methods,             //  tp_methods
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
