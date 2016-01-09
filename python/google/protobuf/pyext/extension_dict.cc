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

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/descriptor_pool.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/repeated_composite_container.h>
#include <google/protobuf/pyext/repeated_scalar_container.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>
#include <google/protobuf/stubs/shared_ptr.h>

namespace google {
namespace protobuf {
namespace python {

namespace extension_dict {

PyObject* len(ExtensionDict* self) {
#if PY_MAJOR_VERSION >= 3
  return PyLong_FromLong(PyDict_Size(self->values));
#else
  return PyInt_FromLong(PyDict_Size(self->values));
#endif
}

// TODO(tibell): Use VisitCompositeField.
int ReleaseExtension(ExtensionDict* self,
                     PyObject* extension,
                     const FieldDescriptor* descriptor) {
  if (descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
    if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (repeated_composite_container::Release(
              reinterpret_cast<RepeatedCompositeContainer*>(
                  extension)) < 0) {
        return -1;
      }
    } else {
      if (repeated_scalar_container::Release(
              reinterpret_cast<RepeatedScalarContainer*>(
                  extension)) < 0) {
        return -1;
      }
    }
  } else if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    if (cmessage::ReleaseSubMessage(
            self->parent, descriptor,
            reinterpret_cast<CMessage*>(extension)) < 0) {
      return -1;
    }
  }

  return 0;
}

PyObject* subscript(ExtensionDict* self, PyObject* key) {
  const FieldDescriptor* descriptor = cmessage::GetExtensionDescriptor(key);
  if (descriptor == NULL) {
    return NULL;
  }
  if (!CheckFieldBelongsToMessage(descriptor, self->message)) {
    return NULL;
  }

  if (descriptor->label() != FieldDescriptor::LABEL_REPEATED &&
      descriptor->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    return cmessage::InternalGetScalar(self->message, descriptor);
  }

  PyObject* value = PyDict_GetItem(self->values, key);
  if (value != NULL) {
    Py_INCREF(value);
    return value;
  }

  if (self->parent == NULL) {
    // We are in "detached" state. Don't allow further modifications.
    // TODO(amauryfa): Support adding non-scalars to a detached extension dict.
    // This probably requires to store the type of the main message.
    PyErr_SetObject(PyExc_KeyError, key);
    return NULL;
  }

  if (descriptor->label() != FieldDescriptor::LABEL_REPEATED &&
      descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    PyObject* sub_message = cmessage::InternalGetSubMessage(
        self->parent, descriptor);
    if (sub_message == NULL) {
      return NULL;
    }
    PyDict_SetItem(self->values, key, sub_message);
    return sub_message;
  }

  if (descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
    if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      PyObject *message_class = cdescriptor_pool::GetMessageClass(
          cmessage::GetDescriptorPoolForMessage(self->parent),
          descriptor->message_type());
      if (message_class == NULL) {
        return NULL;
      }
      PyObject* py_container = repeated_composite_container::NewContainer(
          self->parent, descriptor, message_class);
      if (py_container == NULL) {
        return NULL;
      }
      PyDict_SetItem(self->values, key, py_container);
      return py_container;
    } else {
      PyObject* py_container = repeated_scalar_container::NewContainer(
          self->parent, descriptor);
      if (py_container == NULL) {
        return NULL;
      }
      PyDict_SetItem(self->values, key, py_container);
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
  if (!CheckFieldBelongsToMessage(descriptor, self->message)) {
    return -1;
  }

  if (descriptor->label() != FieldDescriptor::LABEL_OPTIONAL ||
      descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    PyErr_SetString(PyExc_TypeError, "Extension is repeated and/or composite "
                    "type");
    return -1;
  }
  if (self->parent) {
    cmessage::AssureWritable(self->parent);
    if (cmessage::InternalSetScalar(self->parent, descriptor, value) < 0) {
      return -1;
    }
  }
  // TODO(tibell): We shouldn't write scalars to the cache.
  PyDict_SetItem(self->values, key, value);
  return 0;
}

PyObject* ClearExtension(ExtensionDict* self, PyObject* extension) {
  const FieldDescriptor* descriptor =
      cmessage::GetExtensionDescriptor(extension);
  if (descriptor == NULL) {
    return NULL;
  }
  PyObject* value = PyDict_GetItem(self->values, extension);
  if (self->parent) {
    if (value != NULL) {
      if (ReleaseExtension(self, value, descriptor) < 0) {
        return NULL;
      }
    }
    if (ScopedPyObjectPtr(cmessage::ClearFieldByDescriptor(
            self->parent, descriptor)) == NULL) {
      return NULL;
    }
  }
  if (PyDict_DelItem(self->values, extension) < 0) {
    PyErr_Clear();
  }
  Py_RETURN_NONE;
}

PyObject* HasExtension(ExtensionDict* self, PyObject* extension) {
  const FieldDescriptor* descriptor =
      cmessage::GetExtensionDescriptor(extension);
  if (descriptor == NULL) {
    return NULL;
  }
  if (self->parent) {
    return cmessage::HasFieldByDescriptor(self->parent, descriptor);
  } else {
    int exists = PyDict_Contains(self->values, extension);
    if (exists < 0) {
      return NULL;
    }
    return PyBool_FromLong(exists);
  }
}

PyObject* _FindExtensionByName(ExtensionDict* self, PyObject* name) {
  ScopedPyObjectPtr extensions_by_name(PyObject_GetAttrString(
      reinterpret_cast<PyObject*>(self->parent), "_extensions_by_name"));
  if (extensions_by_name == NULL) {
    return NULL;
  }
  PyObject* result = PyDict_GetItem(extensions_by_name.get(), name);
  if (result == NULL) {
    Py_RETURN_NONE;
  } else {
    Py_INCREF(result);
    return result;
  }
}

ExtensionDict* NewExtensionDict(CMessage *parent) {
  ExtensionDict* self = reinterpret_cast<ExtensionDict*>(
      PyType_GenericAlloc(&ExtensionDict_Type, 0));
  if (self == NULL) {
    return NULL;
  }

  self->parent = parent;  // Store a borrowed reference.
  self->message = parent->message;
  self->owner = parent->owner;
  self->values = PyDict_New();
  return self;
}

void dealloc(ExtensionDict* self) {
  Py_CLEAR(self->values);
  self->owner.reset();
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

static PyMappingMethods MpMethods = {
  (lenfunc)len,               /* mp_length */
  (binaryfunc)subscript,      /* mp_subscript */
  (objobjargproc)ass_subscript,/* mp_ass_subscript */
};

#define EDMETHOD(name, args, doc) { #name, (PyCFunction)name, args, doc }
static PyMethodDef Methods[] = {
  EDMETHOD(ClearExtension, METH_O, "Clears an extension from the object."),
  EDMETHOD(HasExtension, METH_O, "Checks if the object has an extension."),
  EDMETHOD(_FindExtensionByName, METH_O,
           "Finds an extension by name."),
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
  0,                                   //  tp_richcompare
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
