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

#include <unordered_map>
#include <utility>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/pyext/descriptor.h"
#include "google/protobuf/pyext/message.h"
#include "google/protobuf/pyext/message_factory.h"
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

namespace message_factory {

PyMessageFactory* NewMessageFactory(PyTypeObject* type, PyDescriptorPool* pool) {
  PyMessageFactory* factory = reinterpret_cast<PyMessageFactory*>(
      PyType_GenericAlloc(type, 0));
  if (factory == nullptr) {
    return nullptr;
  }

  DynamicMessageFactory* message_factory = new DynamicMessageFactory();
  // This option might be the default some day.
  message_factory->SetDelegateToGeneratedFactory(true);
  factory->message_factory = message_factory;

  factory->pool = pool;
  Py_INCREF(pool);

  factory->classes_by_descriptor = new PyMessageFactory::ClassesByMessageMap();

  return factory;
}

PyObject* New(PyTypeObject* type, PyObject* args, PyObject* kwargs) {
  static const char* kwlist[] = {"pool", nullptr};
  PyObject* pool = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O",
                                   const_cast<char**>(kwlist), &pool)) {
    return nullptr;
  }
  ScopedPyObjectPtr owned_pool;
  if (pool == nullptr || pool == Py_None) {
    owned_pool.reset(PyObject_CallFunction(
        reinterpret_cast<PyObject*>(&PyDescriptorPool_Type), nullptr));
    if (owned_pool == nullptr) {
      return nullptr;
    }
    pool = owned_pool.get();
  } else {
    if (!PyObject_TypeCheck(pool, &PyDescriptorPool_Type)) {
      PyErr_Format(PyExc_TypeError, "Expected a DescriptorPool, got %s",
                   pool->ob_type->tp_name);
      return nullptr;
    }
  }

  return reinterpret_cast<PyObject*>(
      NewMessageFactory(type, reinterpret_cast<PyDescriptorPool*>(pool)));
}

static void Dealloc(PyObject* pself) {
  PyMessageFactory* self = reinterpret_cast<PyMessageFactory*>(pself);

  typedef PyMessageFactory::ClassesByMessageMap::iterator iterator;
  for (iterator it = self->classes_by_descriptor->begin();
       it != self->classes_by_descriptor->end(); ++it) {
    Py_CLEAR(it->second);
  }
  delete self->classes_by_descriptor;
  delete self->message_factory;
  Py_CLEAR(self->pool);
  Py_TYPE(self)->tp_free(pself);
}

static int GcTraverse(PyObject* pself, visitproc visit, void* arg) {
  PyMessageFactory* self = reinterpret_cast<PyMessageFactory*>(pself);
  Py_VISIT(self->pool);
  for (const auto& desc_and_class : *self->classes_by_descriptor) {
    Py_VISIT(desc_and_class.second);
  }
  return 0;
}

static int GcClear(PyObject* pself) {
  PyMessageFactory* self = reinterpret_cast<PyMessageFactory*>(pself);
  // Here it's important to not clear self->pool, so that the C++ DescriptorPool
  // is still alive when self->message_factory is destructed.
  for (auto& desc_and_class : *self->classes_by_descriptor) {
    Py_CLEAR(desc_and_class.second);
  }

  return 0;
}

// Add a message class to our database.
int RegisterMessageClass(PyMessageFactory* self,
                         const Descriptor* message_descriptor,
                         CMessageClass* message_class) {
  Py_INCREF(message_class);
  typedef PyMessageFactory::ClassesByMessageMap::iterator iterator;
  std::pair<iterator, bool> ret = self->classes_by_descriptor->insert(
      std::make_pair(message_descriptor, message_class));
  if (!ret.second) {
    // Update case: DECREF the previous value.
    Py_DECREF(ret.first->second);
    ret.first->second = message_class;
  }
  return 0;
}

CMessageClass* GetOrCreateMessageClass(PyMessageFactory* self,
                                       const Descriptor* descriptor) {
  // This is the same implementation as MessageFactory.GetPrototype().

  // Do not create a MessageClass that already exists.
  std::unordered_map<const Descriptor*, CMessageClass*>::iterator it =
      self->classes_by_descriptor->find(descriptor);
  if (it != self->classes_by_descriptor->end()) {
    Py_INCREF(it->second);
    return it->second;
  }
  ScopedPyObjectPtr py_descriptor(
      PyMessageDescriptor_FromDescriptor(descriptor));
  if (py_descriptor == nullptr) {
    return nullptr;
  }
  // Create a new message class.
  ScopedPyObjectPtr args(Py_BuildValue(
      "s(){sOsOsO}", descriptor->name().c_str(),
      "DESCRIPTOR", py_descriptor.get(),
      "__module__", Py_None,
      "message_factory", self));
  if (args == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr message_class(PyObject_CallObject(
      reinterpret_cast<PyObject*>(CMessageClass_Type), args.get()));
  if (message_class == nullptr) {
    return nullptr;
  }
  // Create messages class for the messages used by the fields, and registers
  // all extensions for these messages during the recursion.
  for (int field_idx = 0; field_idx < descriptor->field_count(); field_idx++) {
    const Descriptor* sub_descriptor =
        descriptor->field(field_idx)->message_type();
    // It is null if the field type is not a message.
    if (sub_descriptor != nullptr) {
      CMessageClass* result = GetOrCreateMessageClass(self, sub_descriptor);
      if (result == nullptr) {
        return nullptr;
      }
      Py_DECREF(result);
    }
  }

  // Register extensions defined in this message.
  for (int ext_idx = 0 ; ext_idx < descriptor->extension_count() ; ext_idx++) {
    const FieldDescriptor* extension = descriptor->extension(ext_idx);
    ScopedPyObjectPtr py_extended_class(
        GetOrCreateMessageClass(self, extension->containing_type())
            ->AsPyObject());
    if (py_extended_class == nullptr) {
      return nullptr;
    }
    ScopedPyObjectPtr py_extension(PyFieldDescriptor_FromDescriptor(extension));
    if (py_extension == nullptr) {
      return nullptr;
    }
    ScopedPyObjectPtr result(cmessage::RegisterExtension(
        py_extended_class.get(), py_extension.get()));
    if (result == nullptr) {
      return nullptr;
    }
  }
  return reinterpret_cast<CMessageClass*>(message_class.release());
}

// Retrieve the message class added to our database.
CMessageClass* GetMessageClass(PyMessageFactory* self,
                               const Descriptor* message_descriptor) {
  typedef PyMessageFactory::ClassesByMessageMap::iterator iterator;
  iterator ret = self->classes_by_descriptor->find(message_descriptor);
  if (ret == self->classes_by_descriptor->end()) {
    PyErr_Format(PyExc_TypeError, "No message class registered for '%s'",
                 message_descriptor->full_name().c_str());
    return nullptr;
  } else {
    return ret->second;
  }
}

static PyMethodDef Methods[] = {
    {nullptr},
};

static PyObject* GetPool(PyMessageFactory* self, void* closure) {
  Py_INCREF(self->pool);
  return reinterpret_cast<PyObject*>(self->pool);
}

static PyGetSetDef Getters[] = {
    {"pool", (getter)GetPool, nullptr, "DescriptorPool"},
    {nullptr},
};

}  // namespace message_factory

PyTypeObject PyMessageFactory_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) FULL_MODULE_NAME
    ".MessageFactory",         // tp_name
    sizeof(PyMessageFactory),  // tp_basicsize
    0,                         // tp_itemsize
    message_factory::Dealloc,  // tp_dealloc
#if PY_VERSION_HEX < 0x03080000
    nullptr,  // tp_print
#else
    0,  // tp_vectorcall_offset
#endif
    nullptr,  // tp_getattr
    nullptr,  // tp_setattr
    nullptr,  // tp_compare
    nullptr,  // tp_repr
    nullptr,  // tp_as_number
    nullptr,  // tp_as_sequence
    nullptr,  // tp_as_mapping
    nullptr,  // tp_hash
    nullptr,  // tp_call
    nullptr,  // tp_str
    nullptr,  // tp_getattro
    nullptr,  // tp_setattro
    nullptr,  // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,  // tp_flags
    "A static Message Factory",                                     // tp_doc
    message_factory::GcTraverse,  // tp_traverse
    message_factory::GcClear,     // tp_clear
    nullptr,                      // tp_richcompare
    0,                            // tp_weaklistoffset
    nullptr,                      // tp_iter
    nullptr,                      // tp_iternext
    message_factory::Methods,     // tp_methods
    nullptr,                      // tp_members
    message_factory::Getters,     // tp_getset
    nullptr,                      // tp_base
    nullptr,                      // tp_dict
    nullptr,                      // tp_descr_get
    nullptr,                      // tp_descr_set
    0,                            // tp_dictoffset
    nullptr,                      // tp_init
    nullptr,                      // tp_alloc
    message_factory::New,         // tp_new
    PyObject_GC_Del,              // tp_free
};

bool InitMessageFactory() {
  if (PyType_Ready(&PyMessageFactory_Type) < 0) {
    return false;
  }

  return true;
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
