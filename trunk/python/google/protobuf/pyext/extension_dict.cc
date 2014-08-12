// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/message.h>
#include <google/protobuf/pyext/repeated_composite_container.h>
#include <google/protobuf/pyext/repeated_scalar_container.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>
#include <google/protobuf/stubs/shared_ptr.h>

namespace google {
namespace protobuf {
namespace python {

extern google::protobuf::DynamicMessageFactory* global_message_factory;

namespace extension_dict {

// TODO(tibell): Always use self->message for clarity, just like in
// RepeatedCompositeContainer.
static google::protobuf::Message* GetMessage(ExtensionDict* self) {
  if (self->parent != NULL) {
    return self->parent->message;
  } else {
    return self->message;
  }
}

CFieldDescriptor* InternalGetCDescriptorFromExtension(PyObject* extension) {
  PyObject* cdescriptor = PyObject_GetAttrString(extension, "_cdescriptor");
  if (cdescriptor == NULL) {
    PyErr_SetString(PyExc_KeyError, "Unregistered extension.");
    return NULL;
  }
  if (!PyObject_TypeCheck(cdescriptor, &CFieldDescriptor_Type)) {
    PyErr_SetString(PyExc_TypeError, "Not a CFieldDescriptor");
    Py_DECREF(cdescriptor);
    return NULL;
  }
  CFieldDescriptor* descriptor =
      reinterpret_cast<CFieldDescriptor*>(cdescriptor);
  return descriptor;
}

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
                     const google::protobuf::FieldDescriptor* descriptor) {
  if (descriptor->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
    if (descriptor->cpp_type() ==
        google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
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
  } else if (descriptor->cpp_type() ==
             google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    if (cmessage::ReleaseSubMessage(
            GetMessage(self), descriptor,
            reinterpret_cast<CMessage*>(extension)) < 0) {
      return -1;
    }
  }

  return 0;
}

PyObject* subscript(ExtensionDict* self, PyObject* key) {
  CFieldDescriptor* cdescriptor = InternalGetCDescriptorFromExtension(
      key);
  if (cdescriptor == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr py_cdescriptor(reinterpret_cast<PyObject*>(cdescriptor));
  const google::protobuf::FieldDescriptor* descriptor = cdescriptor->descriptor;
  if (descriptor == NULL) {
    return NULL;
  }
  if (descriptor->label() != FieldDescriptor::LABEL_REPEATED &&
      descriptor->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    return cmessage::InternalGetScalar(self->parent, descriptor);
  }

  PyObject* value = PyDict_GetItem(self->values, key);
  if (value != NULL) {
    Py_INCREF(value);
    return value;
  }

  if (descriptor->label() != FieldDescriptor::LABEL_REPEATED &&
      descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    PyObject* sub_message = cmessage::InternalGetSubMessage(
        self->parent, cdescriptor);
    if (sub_message == NULL) {
      return NULL;
    }
    PyDict_SetItem(self->values, key, sub_message);
    return sub_message;
  }

  if (descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
    if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      // COPIED
      PyObject* py_container = PyObject_CallObject(
          reinterpret_cast<PyObject*>(&RepeatedCompositeContainer_Type),
          NULL);
      if (py_container == NULL) {
        return NULL;
      }
      RepeatedCompositeContainer* container =
          reinterpret_cast<RepeatedCompositeContainer*>(py_container);
      PyObject* field = cdescriptor->descriptor_field;
      PyObject* message_type = PyObject_GetAttrString(field, "message_type");
      PyObject* concrete_class = PyObject_GetAttrString(message_type,
                                                        "_concrete_class");
      container->owner = self->owner;
      container->parent = self->parent;
      container->message = self->parent->message;
      container->parent_field = cdescriptor;
      container->subclass_init = concrete_class;
      Py_DECREF(message_type);
      PyDict_SetItem(self->values, key, py_container);
      return py_container;
    } else {
      // COPIED
      ScopedPyObjectPtr init_args(PyTuple_Pack(2, self->parent, cdescriptor));
      PyObject* py_container = PyObject_CallObject(
          reinterpret_cast<PyObject*>(&RepeatedScalarContainer_Type),
          init_args);
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
  CFieldDescriptor* cdescriptor = InternalGetCDescriptorFromExtension(
      key);
  if (cdescriptor == NULL) {
    return -1;
  }
  ScopedPyObjectPtr py_cdescriptor(reinterpret_cast<PyObject*>(cdescriptor));
  const google::protobuf::FieldDescriptor* descriptor = cdescriptor->descriptor;
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
  // TODO(tibell): We shouldn't write scalars to the cache.
  PyDict_SetItem(self->values, key, value);
  return 0;
}

PyObject* ClearExtension(ExtensionDict* self, PyObject* extension) {
  CFieldDescriptor* cdescriptor = InternalGetCDescriptorFromExtension(
      extension);
  if (cdescriptor == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr py_cdescriptor(reinterpret_cast<PyObject*>(cdescriptor));
  PyObject* value = PyDict_GetItem(self->values, extension);
  if (value != NULL) {
    if (ReleaseExtension(self, value, cdescriptor->descriptor) < 0) {
      return NULL;
    }
  }
  if (cmessage::ClearFieldByDescriptor(self->parent,
                                       cdescriptor->descriptor) == NULL) {
    return NULL;
  }
  if (PyDict_DelItem(self->values, extension) < 0) {
    PyErr_Clear();
  }
  Py_RETURN_NONE;
}

PyObject* HasExtension(ExtensionDict* self, PyObject* extension) {
  CFieldDescriptor* cdescriptor = InternalGetCDescriptorFromExtension(
      extension);
  if (cdescriptor == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr py_cdescriptor(reinterpret_cast<PyObject*>(cdescriptor));
  PyObject* result = cmessage::HasFieldByDescriptor(
      self->parent, cdescriptor->descriptor);
  return result;
}

PyObject* _FindExtensionByName(ExtensionDict* self, PyObject* name) {
  ScopedPyObjectPtr extensions_by_name(PyObject_GetAttrString(
      reinterpret_cast<PyObject*>(self->parent), "_extensions_by_name"));
  if (extensions_by_name == NULL) {
    return NULL;
  }
  PyObject* result = PyDict_GetItem(extensions_by_name, name);
  if (result == NULL) {
    Py_RETURN_NONE;
  } else {
    Py_INCREF(result);
    return result;
  }
}

int init(ExtensionDict* self, PyObject* args, PyObject* kwargs) {
  self->parent = NULL;
  self->message = NULL;
  self->values = PyDict_New();
  return 0;
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
  "google.protobuf.internal."
  "cpp._message.ExtensionDict",        // tp_name
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
  0,                                   //  tp_hash
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
  (initproc)extension_dict::init,      //  tp_init
};

}  // namespace python
}  // namespace protobuf
}  // namespace google
