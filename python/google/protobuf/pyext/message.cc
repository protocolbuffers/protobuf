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

#include <google/protobuf/pyext/message.h>

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <structmember.h>  // A Python header file.

#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(type, size) PyObject_HEAD_INIT(type) size,
#endif
#ifndef Py_TYPE
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/util/message_differencer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/descriptor_pool.h>
#include <google/protobuf/pyext/extension_dict.h>
#include <google/protobuf/pyext/repeated_composite_container.h>
#include <google/protobuf/pyext/repeated_scalar_container.h>
#include <google/protobuf/pyext/map_container.h>
#include <google/protobuf/pyext/message_factory.h>
#include <google/protobuf/pyext/safe_numerics.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

#if PY_MAJOR_VERSION >= 3
  #define PyInt_AsLong PyLong_AsLong
  #define PyInt_FromLong PyLong_FromLong
  #define PyInt_FromSize_t PyLong_FromSize_t
  #define PyString_Check PyUnicode_Check
  #define PyString_FromString PyUnicode_FromString
  #define PyString_FromStringAndSize PyUnicode_FromStringAndSize
  #if PY_VERSION_HEX < 0x03030000
    #error "Python 3.0 - 3.2 are not supported."
  #else
  #define PyString_AsString(ob) \
    (PyUnicode_Check(ob)? PyUnicode_AsUTF8(ob): PyBytes_AsString(ob))
  #define PyString_AsStringAndSize(ob, charpp, sizep) \
    (PyUnicode_Check(ob)? \
       ((*(charpp) = PyUnicode_AsUTF8AndSize(ob, (sizep))) == NULL? -1: 0): \
       PyBytes_AsStringAndSize(ob, (charpp), (sizep)))
  #endif
#endif

namespace google {
namespace protobuf {
namespace python {

static PyObject* kDESCRIPTOR;
PyObject* EnumTypeWrapper_class;
static PyObject* PythonMessage_class;
static PyObject* kEmptyWeakref;
static PyObject* WKT_classes = NULL;

namespace message_meta {

static int InsertEmptyWeakref(PyTypeObject* base);

namespace {
// Copied oveer from internal 'google/protobuf/stubs/strutil.h'.
inline void UpperString(string * s) {
  string::iterator end = s->end();
  for (string::iterator i = s->begin(); i != end; ++i) {
    // toupper() changes based on locale.  We don't want this!
    if ('a' <= *i && *i <= 'z') *i += 'A' - 'a';
  }
}
}

// Add the number of a field descriptor to the containing message class.
// Equivalent to:
//   _cls.<field>_FIELD_NUMBER = <number>
static bool AddFieldNumberToClass(
    PyObject* cls, const FieldDescriptor* field_descriptor) {
  string constant_name = field_descriptor->name() + "_FIELD_NUMBER";
  UpperString(&constant_name);
  ScopedPyObjectPtr attr_name(PyString_FromStringAndSize(
      constant_name.c_str(), constant_name.size()));
  if (attr_name == NULL) {
    return false;
  }
  ScopedPyObjectPtr number(PyInt_FromLong(field_descriptor->number()));
  if (number == NULL) {
    return false;
  }
  if (PyObject_SetAttr(cls, attr_name.get(), number.get()) == -1) {
    return false;
  }
  return true;
}


// Finalize the creation of the Message class.
static int AddDescriptors(PyObject* cls, const Descriptor* descriptor) {
  // For each field set: cls.<field>_FIELD_NUMBER = <number>
  for (int i = 0; i < descriptor->field_count(); ++i) {
    if (!AddFieldNumberToClass(cls, descriptor->field(i))) {
      return -1;
    }
  }

  // For each enum set cls.<enum name> = EnumTypeWrapper(<enum descriptor>).
  for (int i = 0; i < descriptor->enum_type_count(); ++i) {
    const EnumDescriptor* enum_descriptor = descriptor->enum_type(i);
    ScopedPyObjectPtr enum_type(
        PyEnumDescriptor_FromDescriptor(enum_descriptor));
    if (enum_type == NULL) {
      return -1;
     }
    // Add wrapped enum type to message class.
    ScopedPyObjectPtr wrapped(PyObject_CallFunctionObjArgs(
        EnumTypeWrapper_class, enum_type.get(), NULL));
    if (wrapped == NULL) {
      return -1;
    }
    if (PyObject_SetAttrString(
            cls, enum_descriptor->name().c_str(), wrapped.get()) == -1) {
      return -1;
    }

    // For each enum value add cls.<name> = <number>
    for (int j = 0; j < enum_descriptor->value_count(); ++j) {
      const EnumValueDescriptor* enum_value_descriptor =
          enum_descriptor->value(j);
      ScopedPyObjectPtr value_number(PyInt_FromLong(
          enum_value_descriptor->number()));
      if (value_number == NULL) {
        return -1;
      }
      if (PyObject_SetAttrString(cls, enum_value_descriptor->name().c_str(),
                                 value_number.get()) == -1) {
        return -1;
      }
    }
  }

  // For each extension set cls.<extension name> = <extension descriptor>.
  //
  // Extension descriptors come from
  // <message descriptor>.extensions_by_name[name]
  // which was defined previously.
  for (int i = 0; i < descriptor->extension_count(); ++i) {
    const google::protobuf::FieldDescriptor* field = descriptor->extension(i);
    ScopedPyObjectPtr extension_field(PyFieldDescriptor_FromDescriptor(field));
    if (extension_field == NULL) {
      return -1;
    }

    // Add the extension field to the message class.
    if (PyObject_SetAttrString(
            cls, field->name().c_str(), extension_field.get()) == -1) {
      return -1;
    }

    // For each extension set cls.<extension name>_FIELD_NUMBER = <number>.
    if (!AddFieldNumberToClass(cls, field)) {
      return -1;
    }
  }

  return 0;
}

static PyObject* New(PyTypeObject* type,
                     PyObject* args, PyObject* kwargs) {
  static char *kwlist[] = {"name", "bases", "dict", 0};
  PyObject *bases, *dict;
  const char* name;

  // Check arguments: (name, bases, dict)
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO!O!:type", kwlist,
                                   &name,
                                   &PyTuple_Type, &bases,
                                   &PyDict_Type, &dict)) {
    return NULL;
  }

  // Check bases: only (), or (message.Message,) are allowed
  if (!(PyTuple_GET_SIZE(bases) == 0 ||
        (PyTuple_GET_SIZE(bases) == 1 &&
         PyTuple_GET_ITEM(bases, 0) == PythonMessage_class))) {
    PyErr_SetString(PyExc_TypeError,
                    "A Message class can only inherit from Message");
    return NULL;
  }

  // Check dict['DESCRIPTOR']
  PyObject* py_descriptor = PyDict_GetItem(dict, kDESCRIPTOR);
  if (py_descriptor == NULL) {
    PyErr_SetString(PyExc_TypeError, "Message class has no DESCRIPTOR");
    return NULL;
  }
  if (!PyObject_TypeCheck(py_descriptor, &PyMessageDescriptor_Type)) {
    PyErr_Format(PyExc_TypeError, "Expected a message Descriptor, got %s",
                 py_descriptor->ob_type->tp_name);
    return NULL;
  }

  // Messages have no __dict__
  ScopedPyObjectPtr slots(PyTuple_New(0));
  if (PyDict_SetItemString(dict, "__slots__", slots.get()) < 0) {
    return NULL;
  }

  // Build the arguments to the base metaclass.
  // We change the __bases__ classes.
  ScopedPyObjectPtr new_args;
  const Descriptor* message_descriptor =
      PyMessageDescriptor_AsDescriptor(py_descriptor);
  if (message_descriptor == NULL) {
    return NULL;
  }

  if (WKT_classes == NULL) {
    ScopedPyObjectPtr well_known_types(PyImport_ImportModule(
        "google.protobuf.internal.well_known_types"));
    GOOGLE_DCHECK(well_known_types != NULL);

    WKT_classes = PyObject_GetAttrString(well_known_types.get(), "WKTBASES");
    GOOGLE_DCHECK(WKT_classes != NULL);
  }

  PyObject* well_known_class = PyDict_GetItemString(
      WKT_classes, message_descriptor->full_name().c_str());
  if (well_known_class == NULL) {
    new_args.reset(Py_BuildValue("s(OO)O", name, &CMessage_Type,
                                 PythonMessage_class, dict));
  } else {
    new_args.reset(Py_BuildValue("s(OOO)O", name, &CMessage_Type,
                                 PythonMessage_class, well_known_class, dict));
  }

  if (new_args == NULL) {
    return NULL;
  }
  // Call the base metaclass.
  ScopedPyObjectPtr result(PyType_Type.tp_new(type, new_args.get(), NULL));
  if (result == NULL) {
    return NULL;
  }
  CMessageClass* newtype = reinterpret_cast<CMessageClass*>(result.get());

  // Insert the empty weakref into the base classes.
  if (InsertEmptyWeakref(
          reinterpret_cast<PyTypeObject*>(PythonMessage_class)) < 0 ||
      InsertEmptyWeakref(&CMessage_Type) < 0) {
    return NULL;
  }

  // Cache the descriptor, both as Python object and as C++ pointer.
  const Descriptor* descriptor =
      PyMessageDescriptor_AsDescriptor(py_descriptor);
  if (descriptor == NULL) {
    return NULL;
  }
  Py_INCREF(py_descriptor);
  newtype->py_message_descriptor = py_descriptor;
  newtype->message_descriptor = descriptor;
  // TODO(amauryfa): Don't always use the canonical pool of the descriptor,
  // use the MessageFactory optionally passed in the class dict.
  PyDescriptorPool* py_descriptor_pool =
      GetDescriptorPool_FromPool(descriptor->file()->pool());
  if (py_descriptor_pool == NULL) {
    return NULL;
  }
  newtype->py_message_factory = py_descriptor_pool->py_message_factory;
  Py_INCREF(newtype->py_message_factory);

  // Register the message in the MessageFactory.
  // TODO(amauryfa): Move this call to MessageFactory.GetPrototype() when the
  // MessageFactory is fully implemented in C++.
  if (message_factory::RegisterMessageClass(newtype->py_message_factory,
                                            descriptor, newtype) < 0) {
    return NULL;
  }

  // Continue with type initialization: add other descriptors, enum values...
  if (AddDescriptors(result.get(), descriptor) < 0) {
    return NULL;
  }
  return result.release();
}

static void Dealloc(CMessageClass *self) {
  Py_XDECREF(self->py_message_descriptor);
  Py_XDECREF(self->py_message_factory);
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}


// This function inserts and empty weakref at the end of the list of
// subclasses for the main protocol buffer Message class.
//
// This eliminates a O(n^2) behaviour in the internal add_subclass
// routine.
static int InsertEmptyWeakref(PyTypeObject *base_type) {
#if PY_MAJOR_VERSION >= 3
  // Python 3.4 has already included the fix for the issue that this
  // hack addresses. For further background and the fix please see
  // https://bugs.python.org/issue17936.
  return 0;
#else
  PyObject *subclasses = base_type->tp_subclasses;
  if (subclasses && PyList_CheckExact(subclasses)) {
    return PyList_Append(subclasses, kEmptyWeakref);
  }
  return 0;
#endif  // PY_MAJOR_VERSION >= 3
}

// The _extensions_by_name dictionary is built on every access.
// TODO(amauryfa): Migrate all users to pool.FindAllExtensions()
static PyObject* GetExtensionsByName(CMessageClass *self, void *closure) {
  const PyDescriptorPool* pool = self->py_message_factory->pool;

  std::vector<const FieldDescriptor*> extensions;
  pool->pool->FindAllExtensions(self->message_descriptor, &extensions);

  ScopedPyObjectPtr result(PyDict_New());
  for (int i = 0; i < extensions.size(); i++) {
    ScopedPyObjectPtr extension(
        PyFieldDescriptor_FromDescriptor(extensions[i]));
    if (extension == NULL) {
      return NULL;
    }
    if (PyDict_SetItemString(result.get(), extensions[i]->full_name().c_str(),
                             extension.get()) < 0) {
      return NULL;
    }
  }
  return result.release();
}

// The _extensions_by_number dictionary is built on every access.
// TODO(amauryfa): Migrate all users to pool.FindExtensionByNumber()
static PyObject* GetExtensionsByNumber(CMessageClass *self, void *closure) {
  const PyDescriptorPool* pool = self->py_message_factory->pool;

  std::vector<const FieldDescriptor*> extensions;
  pool->pool->FindAllExtensions(self->message_descriptor, &extensions);

  ScopedPyObjectPtr result(PyDict_New());
  for (int i = 0; i < extensions.size(); i++) {
    ScopedPyObjectPtr extension(
        PyFieldDescriptor_FromDescriptor(extensions[i]));
    if (extension == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr number(PyInt_FromLong(extensions[i]->number()));
    if (number == NULL) {
      return NULL;
    }
    if (PyDict_SetItem(result.get(), number.get(), extension.get()) < 0) {
      return NULL;
    }
  }
  return result.release();
}

static PyGetSetDef Getters[] = {
  {"_extensions_by_name", (getter)GetExtensionsByName, NULL},
  {"_extensions_by_number", (getter)GetExtensionsByNumber, NULL},
  {NULL}
};

}  // namespace message_meta

PyTypeObject CMessageClass_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  FULL_MODULE_NAME ".MessageMeta",     // tp_name
  sizeof(CMessageClass),               // tp_basicsize
  0,                                   // tp_itemsize
  (destructor)message_meta::Dealloc,   // tp_dealloc
  0,                                   // tp_print
  0,                                   // tp_getattr
  0,                                   // tp_setattr
  0,                                   // tp_compare
  0,                                   // tp_repr
  0,                                   // tp_as_number
  0,                                   // tp_as_sequence
  0,                                   // tp_as_mapping
  0,                                   // tp_hash
  0,                                   // tp_call
  0,                                   // tp_str
  0,                                   // tp_getattro
  0,                                   // tp_setattro
  0,                                   // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  // tp_flags
  "The metaclass of ProtocolMessages",  // tp_doc
  0,                                   // tp_traverse
  0,                                   // tp_clear
  0,                                   // tp_richcompare
  0,                                   // tp_weaklistoffset
  0,                                   // tp_iter
  0,                                   // tp_iternext
  0,                                   // tp_methods
  0,                                   // tp_members
  message_meta::Getters,               // tp_getset
  0,                                   // tp_base
  0,                                   // tp_dict
  0,                                   // tp_descr_get
  0,                                   // tp_descr_set
  0,                                   // tp_dictoffset
  0,                                   // tp_init
  0,                                   // tp_alloc
  message_meta::New,                   // tp_new
};

static CMessageClass* CheckMessageClass(PyTypeObject* cls) {
  if (!PyObject_TypeCheck(cls, &CMessageClass_Type)) {
    PyErr_Format(PyExc_TypeError, "Class %s is not a Message", cls->tp_name);
    return NULL;
  }
  return reinterpret_cast<CMessageClass*>(cls);
}

static const Descriptor* GetMessageDescriptor(PyTypeObject* cls) {
  CMessageClass* type = CheckMessageClass(cls);
  if (type == NULL) {
    return NULL;
  }
  return type->message_descriptor;
}

// Forward declarations
namespace cmessage {
int InternalReleaseFieldByDescriptor(
    CMessage* self,
    const FieldDescriptor* field_descriptor,
    PyObject* composite_field);
}  // namespace cmessage

// ---------------------------------------------------------------------
// Visiting the composite children of a CMessage

struct ChildVisitor {
  // Returns 0 on success, -1 on failure.
  int VisitRepeatedCompositeContainer(RepeatedCompositeContainer* container) {
    return 0;
  }

  // Returns 0 on success, -1 on failure.
  int VisitRepeatedScalarContainer(RepeatedScalarContainer* container) {
    return 0;
  }

  // Returns 0 on success, -1 on failure.
  int VisitCMessage(CMessage* cmessage,
                    const FieldDescriptor* field_descriptor) {
    return 0;
  }
};

// Apply a function to a composite field.  Does nothing if child is of
// non-composite type.
template<class Visitor>
static int VisitCompositeField(const FieldDescriptor* descriptor,
                               PyObject* child,
                               Visitor visitor) {
  if (descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
    if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (descriptor->is_map()) {
        MapContainer* container = reinterpret_cast<MapContainer*>(child);
        if (visitor.VisitMapContainer(container) == -1) {
          return -1;
        }
      } else {
        RepeatedCompositeContainer* container =
          reinterpret_cast<RepeatedCompositeContainer*>(child);
        if (visitor.VisitRepeatedCompositeContainer(container) == -1)
          return -1;
      }
    } else {
      RepeatedScalarContainer* container =
        reinterpret_cast<RepeatedScalarContainer*>(child);
      if (visitor.VisitRepeatedScalarContainer(container) == -1)
        return -1;
    }
  } else if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    CMessage* cmsg = reinterpret_cast<CMessage*>(child);
    if (visitor.VisitCMessage(cmsg, descriptor) == -1)
      return -1;
  }
  // The ExtensionDict might contain non-composite fields, which we
  // skip here.
  return 0;
}

// Visit each composite field and extension field of this CMessage.
// Returns -1 on error and 0 on success.
template<class Visitor>
int ForEachCompositeField(CMessage* self, Visitor visitor) {
  Py_ssize_t pos = 0;
  PyObject* key;
  PyObject* field;

  // Visit normal fields.
  if (self->composite_fields) {
    // Never use self->message in this function, it may be already freed.
    const Descriptor* message_descriptor =
        GetMessageDescriptor(Py_TYPE(self));
    while (PyDict_Next(self->composite_fields, &pos, &key, &field)) {
      Py_ssize_t key_str_size;
      char *key_str_data;
      if (PyString_AsStringAndSize(key, &key_str_data, &key_str_size) != 0)
        return -1;
      const string key_str(key_str_data, key_str_size);
      const FieldDescriptor* descriptor =
        message_descriptor->FindFieldByName(key_str);
      if (descriptor != NULL) {
        if (VisitCompositeField(descriptor, field, visitor) == -1)
          return -1;
      }
    }
  }

  // Visit extension fields.
  if (self->extensions != NULL) {
    pos = 0;
    while (PyDict_Next(self->extensions->values, &pos, &key, &field)) {
      const FieldDescriptor* descriptor = cmessage::GetExtensionDescriptor(key);
      if (descriptor == NULL)
        return -1;
      if (VisitCompositeField(descriptor, field, visitor) == -1)
        return -1;
    }
  }

  return 0;
}

// ---------------------------------------------------------------------

PyObject* EncodeError_class;
PyObject* DecodeError_class;
PyObject* PickleError_class;

/* Is 64bit */
void FormatTypeError(PyObject* arg, char* expected_types) {
  PyObject* repr = PyObject_Repr(arg);
  if (repr) {
    PyErr_Format(PyExc_TypeError,
                 "%.100s has type %.100s, but expected one of: %s",
                 PyString_AsString(repr),
                 Py_TYPE(arg)->tp_name,
                 expected_types);
    Py_DECREF(repr);
  }
}

void OutOfRangeError(PyObject* arg) {
  PyObject *s = PyObject_Str(arg);
  if (s) {
    PyErr_Format(PyExc_ValueError,
                 "Value out of range: %s",
                 PyString_AsString(s));
    Py_DECREF(s);
  }
}

template<class RangeType, class ValueType>
bool VerifyIntegerCastAndRange(PyObject* arg, ValueType value) {
  if (GOOGLE_PREDICT_FALSE(value == -1 && PyErr_Occurred())) {
    if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
      // Replace it with the same ValueError as pure python protos instead of
      // the default one.
      PyErr_Clear();
      OutOfRangeError(arg);
    }  // Otherwise propagate existing error.
    return false;
    }
    if (GOOGLE_PREDICT_FALSE(!IsValidNumericCast<RangeType>(value))) {
      OutOfRangeError(arg);
      return false;
    }
  return true;
}

template<class T>
bool CheckAndGetInteger(PyObject* arg, T* value) {
  // The fast path.
#if PY_MAJOR_VERSION < 3
  // For the typical case, offer a fast path.
  if (GOOGLE_PREDICT_TRUE(PyInt_Check(arg))) {
    long int_result = PyInt_AsLong(arg);
    if (GOOGLE_PREDICT_TRUE(IsValidNumericCast<T>(int_result))) {
      *value = static_cast<T>(int_result);
      return true;
    } else {
      OutOfRangeError(arg);
      return false;
    }
    }
#endif
  // This effectively defines an integer as "an object that can be cast as
  // an integer and can be used as an ordinal number".
  // This definition includes everything that implements numbers.Integral
  // and shouldn't cast the net too wide.
    if (GOOGLE_PREDICT_FALSE(!PyIndex_Check(arg))) {
      FormatTypeError(arg, "int, long");
      return false;
    }

  // Now we have an integral number so we can safely use PyLong_ functions.
  // We need to treat the signed and unsigned cases differently in case arg is
  // holding a value above the maximum for signed longs.
  if (std::numeric_limits<T>::min() == 0) {
    // Unsigned case.
    unsigned PY_LONG_LONG ulong_result;
    if (PyLong_Check(arg)) {
      ulong_result = PyLong_AsUnsignedLongLong(arg);
    } else {
      // Unlike PyLong_AsLongLong, PyLong_AsUnsignedLongLong is very
      // picky about the exact type.
      PyObject* casted = PyNumber_Long(arg);
      if (GOOGLE_PREDICT_FALSE(casted == nullptr)) {
        // Propagate existing error.
        return false;
        }
      ulong_result = PyLong_AsUnsignedLongLong(casted);
      Py_DECREF(casted);
    }
    if (VerifyIntegerCastAndRange<T, unsigned PY_LONG_LONG>(arg,
                                                            ulong_result)) {
      *value = static_cast<T>(ulong_result);
    } else {
      return false;
    }
  } else {
    // Signed case.
    PY_LONG_LONG long_result;
    PyNumberMethods *nb;
    if ((nb = arg->ob_type->tp_as_number) != NULL && nb->nb_int != NULL) {
      // PyLong_AsLongLong requires it to be a long or to have an __int__()
      // method.
      long_result = PyLong_AsLongLong(arg);
    } else {
      // Valid subclasses of numbers.Integral should have a __long__() method
      // so fall back to that.
      PyObject* casted = PyNumber_Long(arg);
      if (GOOGLE_PREDICT_FALSE(casted == nullptr)) {
        // Propagate existing error.
        return false;
        }
      long_result = PyLong_AsLongLong(casted);
      Py_DECREF(casted);
    }
    if (VerifyIntegerCastAndRange<T, PY_LONG_LONG>(arg, long_result)) {
      *value = static_cast<T>(long_result);
    } else {
      return false;
    }
  }

  return true;
}

// These are referenced by repeated_scalar_container, and must
// be explicitly instantiated.
template bool CheckAndGetInteger<int32>(PyObject*, int32*);
template bool CheckAndGetInteger<int64>(PyObject*, int64*);
template bool CheckAndGetInteger<uint32>(PyObject*, uint32*);
template bool CheckAndGetInteger<uint64>(PyObject*, uint64*);

bool CheckAndGetDouble(PyObject* arg, double* value) {
  *value = PyFloat_AsDouble(arg);
  if (GOOGLE_PREDICT_FALSE(*value == -1 && PyErr_Occurred())) {
    FormatTypeError(arg, "int, long, float");
    return false;
    }
  return true;
}

bool CheckAndGetFloat(PyObject* arg, float* value) {
  double double_value;
  if (!CheckAndGetDouble(arg, &double_value)) {
    return false;
  }
  *value = static_cast<float>(double_value);
  return true;
}

bool CheckAndGetBool(PyObject* arg, bool* value) {
  long long_value = PyInt_AsLong(arg);
  if (long_value == -1 && PyErr_Occurred()) {
    FormatTypeError(arg, "int, long, bool");
    return false;
  }
  *value = static_cast<bool>(long_value);

  return true;
}

// Checks whether the given object (which must be "bytes" or "unicode") contains
// valid UTF-8.
bool IsValidUTF8(PyObject* obj) {
  if (PyBytes_Check(obj)) {
    PyObject* unicode = PyUnicode_FromEncodedObject(obj, "utf-8", NULL);

    // Clear the error indicator; we report our own error when desired.
    PyErr_Clear();

    if (unicode) {
      Py_DECREF(unicode);
      return true;
    } else {
      return false;
    }
  } else {
    // Unicode object, known to be valid UTF-8.
    return true;
  }
}

bool AllowInvalidUTF8(const FieldDescriptor* field) { return false; }

PyObject* CheckString(PyObject* arg, const FieldDescriptor* descriptor) {
  GOOGLE_DCHECK(descriptor->type() == FieldDescriptor::TYPE_STRING ||
         descriptor->type() == FieldDescriptor::TYPE_BYTES);
  if (descriptor->type() == FieldDescriptor::TYPE_STRING) {
    if (!PyBytes_Check(arg) && !PyUnicode_Check(arg)) {
      FormatTypeError(arg, "bytes, unicode");
      return NULL;
    }

    if (!IsValidUTF8(arg) && !AllowInvalidUTF8(descriptor)) {
      PyObject* repr = PyObject_Repr(arg);
      PyErr_Format(PyExc_ValueError,
                   "%s has type str, but isn't valid UTF-8 "
                   "encoding. Non-UTF-8 strings must be converted to "
                   "unicode objects before being added.",
                   PyString_AsString(repr));
      Py_DECREF(repr);
      return NULL;
    }
  } else if (!PyBytes_Check(arg)) {
    FormatTypeError(arg, "bytes");
    return NULL;
  }

  PyObject* encoded_string = NULL;
  if (descriptor->type() == FieldDescriptor::TYPE_STRING) {
    if (PyBytes_Check(arg)) {
      // The bytes were already validated as correctly encoded UTF-8 above.
      encoded_string = arg;  // Already encoded.
      Py_INCREF(encoded_string);
    } else {
      encoded_string = PyUnicode_AsEncodedString(arg, "utf-8", NULL);
    }
  } else {
    // In this case field type is "bytes".
    encoded_string = arg;
    Py_INCREF(encoded_string);
  }

  return encoded_string;
}

bool CheckAndSetString(
    PyObject* arg, Message* message,
    const FieldDescriptor* descriptor,
    const Reflection* reflection,
    bool append,
    int index) {
  ScopedPyObjectPtr encoded_string(CheckString(arg, descriptor));

  if (encoded_string.get() == NULL) {
    return false;
  }

  char* value;
  Py_ssize_t value_len;
  if (PyBytes_AsStringAndSize(encoded_string.get(), &value, &value_len) < 0) {
    return false;
  }

  string value_string(value, value_len);
  if (append) {
    reflection->AddString(message, descriptor, value_string);
  } else if (index < 0) {
    reflection->SetString(message, descriptor, value_string);
  } else {
    reflection->SetRepeatedString(message, descriptor, index, value_string);
  }
  return true;
}

PyObject* ToStringObject(const FieldDescriptor* descriptor,
                         const string& value) {
  if (descriptor->type() != FieldDescriptor::TYPE_STRING) {
    return PyBytes_FromStringAndSize(value.c_str(), value.length());
  }

  PyObject* result = PyUnicode_DecodeUTF8(value.c_str(), value.length(), NULL);
  // If the string can't be decoded in UTF-8, just return a string object that
  // contains the raw bytes. This can't happen if the value was assigned using
  // the members of the Python message object, but can happen if the values were
  // parsed from the wire (binary).
  if (result == NULL) {
    PyErr_Clear();
    result = PyBytes_FromStringAndSize(value.c_str(), value.length());
  }
  return result;
}

bool CheckFieldBelongsToMessage(const FieldDescriptor* field_descriptor,
                                const Message* message) {
  if (message->GetDescriptor() == field_descriptor->containing_type()) {
    return true;
  }
  PyErr_Format(PyExc_KeyError, "Field '%s' does not belong to message '%s'",
               field_descriptor->full_name().c_str(),
               message->GetDescriptor()->full_name().c_str());
  return false;
}

namespace cmessage {

PyMessageFactory* GetFactoryForMessage(CMessage* message) {
  GOOGLE_DCHECK(PyObject_TypeCheck(message, &CMessage_Type));
  return reinterpret_cast<CMessageClass*>(Py_TYPE(message))->py_message_factory;
}

static int MaybeReleaseOverlappingOneofField(
    CMessage* cmessage,
    const FieldDescriptor* field) {
#ifdef GOOGLE_PROTOBUF_HAS_ONEOF
  Message* message = cmessage->message;
  const Reflection* reflection = message->GetReflection();
  if (!field->containing_oneof() ||
      !reflection->HasOneof(*message, field->containing_oneof()) ||
      reflection->HasField(*message, field)) {
    // No other field in this oneof, no need to release.
    return 0;
  }

  const OneofDescriptor* oneof = field->containing_oneof();
  const FieldDescriptor* existing_field =
      reflection->GetOneofFieldDescriptor(*message, oneof);
  if (existing_field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    // Non-message fields don't need to be released.
    return 0;
  }
  const char* field_name = existing_field->name().c_str();
  PyObject* child_message = cmessage->composite_fields ?
      PyDict_GetItemString(cmessage->composite_fields, field_name) : NULL;
  if (child_message == NULL) {
    // No python reference to this field so no need to release.
    return 0;
  }

  if (InternalReleaseFieldByDescriptor(
          cmessage, existing_field, child_message) < 0) {
    return -1;
  }
  return PyDict_DelItemString(cmessage->composite_fields, field_name);
#else
  return 0;
#endif
}

// ---------------------------------------------------------------------
// Making a message writable

static Message* GetMutableMessage(
    CMessage* parent,
    const FieldDescriptor* parent_field) {
  Message* parent_message = parent->message;
  const Reflection* reflection = parent_message->GetReflection();
  if (MaybeReleaseOverlappingOneofField(parent, parent_field) < 0) {
    return NULL;
  }
  return reflection->MutableMessage(
      parent_message, parent_field,
      GetFactoryForMessage(parent)->message_factory);
}

struct FixupMessageReference : public ChildVisitor {
  // message must outlive this object.
  explicit FixupMessageReference(Message* message) :
      message_(message) {}

  int VisitRepeatedCompositeContainer(RepeatedCompositeContainer* container) {
    container->message = message_;
    return 0;
  }

  int VisitRepeatedScalarContainer(RepeatedScalarContainer* container) {
    container->message = message_;
    return 0;
  }

  int VisitMapContainer(MapContainer* container) {
    container->message = message_;
    return 0;
  }

 private:
  Message* message_;
};

int AssureWritable(CMessage* self) {
  if (self == NULL || !self->read_only) {
    return 0;
  }

  if (self->parent == NULL) {
    // If parent is NULL but we are trying to modify a read-only message, this
    // is a reference to a constant default instance that needs to be replaced
    // with a mutable top-level message.
    self->message = self->message->New();
    self->owner.reset(self->message);
    // Cascade the new owner to eventual children: even if this message is
    // empty, some submessages or repeated containers might exist already.
    SetOwner(self, self->owner);
  } else {
    // Otherwise, we need a mutable child message.
    if (AssureWritable(self->parent) == -1)
      return -1;

    // Make self->message writable.
    Message* mutable_message = GetMutableMessage(
        self->parent,
        self->parent_field_descriptor);
    if (mutable_message == NULL) {
      return -1;
    }
    self->message = mutable_message;
  }
  self->read_only = false;

  // When a CMessage is made writable its Message pointer is updated
  // to point to a new mutable Message.  When that happens we need to
  // update any references to the old, read-only CMessage.  There are
  // four places such references occur: RepeatedScalarContainer,
  // RepeatedCompositeContainer, MapContainer, and ExtensionDict.
  if (self->extensions != NULL)
    self->extensions->message = self->message;
  if (ForEachCompositeField(self, FixupMessageReference(self->message)) == -1)
    return -1;

  return 0;
}

// --- Globals:

// Retrieve a C++ FieldDescriptor for a message attribute.
// The C++ message must be valid.
// TODO(amauryfa): This function should stay internal, because exception
// handling is not consistent.
static const FieldDescriptor* GetFieldDescriptor(
    CMessage* self, PyObject* name) {
  const Descriptor *message_descriptor = self->message->GetDescriptor();
  char* field_name;
  Py_ssize_t size;
  if (PyString_AsStringAndSize(name, &field_name, &size) < 0) {
    return NULL;
  }
  const FieldDescriptor *field_descriptor =
      message_descriptor->FindFieldByName(string(field_name, size));
  if (field_descriptor == NULL) {
    // Note: No exception is set!
    return NULL;
  }
  return field_descriptor;
}

// Retrieve a C++ FieldDescriptor for an extension handle.
const FieldDescriptor* GetExtensionDescriptor(PyObject* extension) {
  ScopedPyObjectPtr cdescriptor;
  if (!PyObject_TypeCheck(extension, &PyFieldDescriptor_Type)) {
    // Most callers consider extensions as a plain dictionary.  We should
    // allow input which is not a field descriptor, and simply pretend it does
    // not exist.
    PyErr_SetObject(PyExc_KeyError, extension);
    return NULL;
  }
  return PyFieldDescriptor_AsDescriptor(extension);
}

// If value is a string, convert it into an enum value based on the labels in
// descriptor, otherwise simply return value.  Always returns a new reference.
static PyObject* GetIntegerEnumValue(const FieldDescriptor& descriptor,
                                     PyObject* value) {
  if (PyString_Check(value) || PyUnicode_Check(value)) {
    const EnumDescriptor* enum_descriptor = descriptor.enum_type();
    if (enum_descriptor == NULL) {
      PyErr_SetString(PyExc_TypeError, "not an enum field");
      return NULL;
    }
    char* enum_label;
    Py_ssize_t size;
    if (PyString_AsStringAndSize(value, &enum_label, &size) < 0) {
      return NULL;
    }
    const EnumValueDescriptor* enum_value_descriptor =
        enum_descriptor->FindValueByName(string(enum_label, size));
    if (enum_value_descriptor == NULL) {
      PyErr_SetString(PyExc_ValueError, "unknown enum label");
      return NULL;
    }
    return PyInt_FromLong(enum_value_descriptor->number());
  }
  Py_INCREF(value);
  return value;
}

// If cmessage_list is not NULL, this function releases values into the
// container CMessages instead of just removing. Repeated composite container
// needs to do this to make sure CMessages stay alive if they're still
// referenced after deletion. Repeated scalar container doesn't need to worry.
int InternalDeleteRepeatedField(
    CMessage* self,
    const FieldDescriptor* field_descriptor,
    PyObject* slice,
    PyObject* cmessage_list) {
  Message* message = self->message;
  Py_ssize_t length, from, to, step, slice_length;
  const Reflection* reflection = message->GetReflection();
  int min, max;
  length = reflection->FieldSize(*message, field_descriptor);

  if (PySlice_Check(slice)) {
    from = to = step = slice_length = 0;
#if PY_MAJOR_VERSION < 3
    PySlice_GetIndicesEx(
        reinterpret_cast<PySliceObject*>(slice),
        length, &from, &to, &step, &slice_length);
#else
    PySlice_GetIndicesEx(
        slice,
        length, &from, &to, &step, &slice_length);
#endif
    if (from < to) {
      min = from;
      max = to - 1;
    } else {
      min = to + 1;
      max = from;
    }
  } else {
    from = to = PyLong_AsLong(slice);
    if (from == -1 && PyErr_Occurred()) {
      PyErr_SetString(PyExc_TypeError, "list indices must be integers");
      return -1;
    }

    if (from < 0) {
      from = to = length + from;
    }
    step = 1;
    min = max = from;

    // Range check.
    if (from < 0 || from >= length) {
      PyErr_Format(PyExc_IndexError, "list assignment index out of range");
      return -1;
    }
  }

  Py_ssize_t i = from;
  std::vector<bool> to_delete(length, false);
  while (i >= min && i <= max) {
    to_delete[i] = true;
    i += step;
  }

  to = 0;
  for (i = 0; i < length; ++i) {
    if (!to_delete[i]) {
      if (i != to) {
        reflection->SwapElements(message, field_descriptor, i, to);
        if (cmessage_list != NULL) {
          // If a list of cmessages is passed in (i.e. from a repeated
          // composite container), swap those as well to correspond to the
          // swaps in the underlying message so they're in the right order
          // when we start releasing.
          PyObject* tmp = PyList_GET_ITEM(cmessage_list, i);
          PyList_SET_ITEM(cmessage_list, i,
                          PyList_GET_ITEM(cmessage_list, to));
          PyList_SET_ITEM(cmessage_list, to, tmp);
        }
      }
      ++to;
    }
  }

  while (i > to) {
    if (cmessage_list == NULL) {
      reflection->RemoveLast(message, field_descriptor);
    } else {
      CMessage* last_cmessage = reinterpret_cast<CMessage*>(
          PyList_GET_ITEM(cmessage_list, PyList_GET_SIZE(cmessage_list) - 1));
      repeated_composite_container::ReleaseLastTo(
          self, field_descriptor, last_cmessage);
      if (PySequence_DelItem(cmessage_list, -1) < 0) {
        return -1;
      }
    }
    --i;
  }

  return 0;
}

// Initializes fields of a message. Used in constructors.
int InitAttributes(CMessage* self, PyObject* args, PyObject* kwargs) {
  if (args != NULL && PyTuple_Size(args) != 0) {
    PyErr_SetString(PyExc_TypeError, "No positional arguments allowed");
    return -1;
  }

  if (kwargs == NULL) {
    return 0;
  }

  Py_ssize_t pos = 0;
  PyObject* name;
  PyObject* value;
  while (PyDict_Next(kwargs, &pos, &name, &value)) {
    if (!PyString_Check(name)) {
      PyErr_SetString(PyExc_ValueError, "Field name must be a string");
      return -1;
    }
    const FieldDescriptor* descriptor = GetFieldDescriptor(self, name);
    if (descriptor == NULL) {
      PyErr_Format(PyExc_ValueError, "Protocol message %s has no \"%s\" field.",
                   self->message->GetDescriptor()->name().c_str(),
                   PyString_AsString(name));
      return -1;
    }
    if (value == Py_None) {
      // field=None is the same as no field at all.
      continue;
    }
    if (descriptor->is_map()) {
      ScopedPyObjectPtr map(GetAttr(reinterpret_cast<PyObject*>(self), name));
      const FieldDescriptor* value_descriptor =
          descriptor->message_type()->FindFieldByName("value");
      if (value_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        Py_ssize_t map_pos = 0;
        PyObject* map_key;
        PyObject* map_value;
        while (PyDict_Next(value, &map_pos, &map_key, &map_value)) {
          ScopedPyObjectPtr function_return;
          function_return.reset(PyObject_GetItem(map.get(), map_key));
          if (function_return.get() == NULL) {
            return -1;
          }
          ScopedPyObjectPtr ok(PyObject_CallMethod(
              function_return.get(), "MergeFrom", "O", map_value));
          if (ok.get() == NULL) {
            return -1;
          }
        }
      } else {
        ScopedPyObjectPtr function_return;
        function_return.reset(
            PyObject_CallMethod(map.get(), "update", "O", value));
        if (function_return.get() == NULL) {
          return -1;
        }
      }
    } else if (descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
      ScopedPyObjectPtr container(
          GetAttr(reinterpret_cast<PyObject*>(self), name));
      if (container == NULL) {
        return -1;
      }
      if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        RepeatedCompositeContainer* rc_container =
            reinterpret_cast<RepeatedCompositeContainer*>(container.get());
        ScopedPyObjectPtr iter(PyObject_GetIter(value));
        if (iter == NULL) {
          PyErr_SetString(PyExc_TypeError, "Value must be iterable");
          return -1;
        }
        ScopedPyObjectPtr next;
        while ((next.reset(PyIter_Next(iter.get()))) != NULL) {
          PyObject* kwargs = (PyDict_Check(next.get()) ? next.get() : NULL);
          ScopedPyObjectPtr new_msg(
              repeated_composite_container::Add(rc_container, NULL, kwargs));
          if (new_msg == NULL) {
            return -1;
          }
          if (kwargs == NULL) {
            // next was not a dict, it's a message we need to merge
            ScopedPyObjectPtr merged(MergeFrom(
                reinterpret_cast<CMessage*>(new_msg.get()), next.get()));
            if (merged.get() == NULL) {
              return -1;
            }
          }
        }
        if (PyErr_Occurred()) {
          // Check to see how PyIter_Next() exited.
          return -1;
        }
      } else if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
        RepeatedScalarContainer* rs_container =
            reinterpret_cast<RepeatedScalarContainer*>(container.get());
        ScopedPyObjectPtr iter(PyObject_GetIter(value));
        if (iter == NULL) {
          PyErr_SetString(PyExc_TypeError, "Value must be iterable");
          return -1;
        }
        ScopedPyObjectPtr next;
        while ((next.reset(PyIter_Next(iter.get()))) != NULL) {
          ScopedPyObjectPtr enum_value(
              GetIntegerEnumValue(*descriptor, next.get()));
          if (enum_value == NULL) {
            return -1;
          }
          ScopedPyObjectPtr new_msg(repeated_scalar_container::Append(
              rs_container, enum_value.get()));
          if (new_msg == NULL) {
            return -1;
          }
        }
        if (PyErr_Occurred()) {
          // Check to see how PyIter_Next() exited.
          return -1;
        }
      } else {
        if (ScopedPyObjectPtr(repeated_scalar_container::Extend(
                reinterpret_cast<RepeatedScalarContainer*>(container.get()),
                value)) ==
            NULL) {
          return -1;
        }
      }
    } else if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      ScopedPyObjectPtr message(
          GetAttr(reinterpret_cast<PyObject*>(self), name));
      if (message == NULL) {
        return -1;
      }
      CMessage* cmessage = reinterpret_cast<CMessage*>(message.get());
      if (PyDict_Check(value)) {
        // Make the message exist even if the dict is empty.
        AssureWritable(cmessage);
        if (InitAttributes(cmessage, NULL, value) < 0) {
          return -1;
        }
      } else {
        ScopedPyObjectPtr merged(MergeFrom(cmessage, value));
        if (merged == NULL) {
          return -1;
        }
      }
    } else {
      ScopedPyObjectPtr new_val;
      if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
        new_val.reset(GetIntegerEnumValue(*descriptor, value));
        if (new_val == NULL) {
          return -1;
        }
      }
      if (SetAttr(reinterpret_cast<PyObject*>(self), name,
                  (new_val.get() == NULL) ? value : new_val.get()) < 0) {
        return -1;
      }
    }
  }
  return 0;
}

// Allocates an incomplete Python Message: the caller must fill self->message,
// self->owner and eventually self->parent.
CMessage* NewEmptyMessage(CMessageClass* type) {
  CMessage* self = reinterpret_cast<CMessage*>(
      PyType_GenericAlloc(&type->super.ht_type, 0));
  if (self == NULL) {
    return NULL;
  }

  // Use "placement new" syntax to initialize the C++ object.
  new (&self->owner) CMessage::OwnerRef(NULL);
  self->message = NULL;
  self->parent = NULL;
  self->parent_field_descriptor = NULL;
  self->read_only = false;
  self->extensions = NULL;

  self->composite_fields = NULL;

  return self;
}

// The __new__ method of Message classes.
// Creates a new C++ message and takes ownership.
static PyObject* New(PyTypeObject* cls,
                     PyObject* unused_args, PyObject* unused_kwargs) {
  CMessageClass* type = CheckMessageClass(cls);
  if (type == NULL) {
    return NULL;
  }
  // Retrieve the message descriptor and the default instance (=prototype).
  const Descriptor* message_descriptor = type->message_descriptor;
  if (message_descriptor == NULL) {
    return NULL;
  }
  const Message* default_message = type->py_message_factory->message_factory
                                   ->GetPrototype(message_descriptor);
  if (default_message == NULL) {
    PyErr_SetString(PyExc_TypeError, message_descriptor->full_name().c_str());
    return NULL;
  }

  CMessage* self = NewEmptyMessage(type);
  if (self == NULL) {
    return NULL;
  }
  self->message = default_message->New();
  self->owner.reset(self->message);
  return reinterpret_cast<PyObject*>(self);
}

// The __init__ method of Message classes.
// It initializes fields from keywords passed to the constructor.
static int Init(CMessage* self, PyObject* args, PyObject* kwargs) {
  return InitAttributes(self, args, kwargs);
}

// ---------------------------------------------------------------------
// Deallocating a CMessage
//
// Deallocating a CMessage requires that we clear any weak references
// from children to the message being deallocated.

// Clear the weak reference from the child to the parent.
struct ClearWeakReferences : public ChildVisitor {
  int VisitRepeatedCompositeContainer(RepeatedCompositeContainer* container) {
    container->parent = NULL;
    // The elements in the container have the same parent as the
    // container itself, so NULL out that pointer as well.
    const Py_ssize_t n = PyList_GET_SIZE(container->child_messages);
    for (Py_ssize_t i = 0; i < n; ++i) {
      CMessage* child_cmessage = reinterpret_cast<CMessage*>(
          PyList_GET_ITEM(container->child_messages, i));
      child_cmessage->parent = NULL;
    }
    return 0;
  }

  int VisitRepeatedScalarContainer(RepeatedScalarContainer* container) {
    container->parent = NULL;
    return 0;
  }

  int VisitMapContainer(MapContainer* container) {
    container->parent = NULL;
    return 0;
  }

  int VisitCMessage(CMessage* cmessage,
                    const FieldDescriptor* field_descriptor) {
    cmessage->parent = NULL;
    return 0;
  }
};

static void Dealloc(CMessage* self) {
  if (self->weakreflist) {
    PyObject_ClearWeakRefs(reinterpret_cast<PyObject*>(self));
  }
  // Null out all weak references from children to this message.
  GOOGLE_CHECK_EQ(0, ForEachCompositeField(self, ClearWeakReferences()));
  if (self->extensions) {
    self->extensions->parent = NULL;
  }

  Py_CLEAR(self->extensions);
  Py_CLEAR(self->composite_fields);
  self->owner.~ThreadUnsafeSharedPtr<Message>();
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

// ---------------------------------------------------------------------


PyObject* IsInitialized(CMessage* self, PyObject* args) {
  PyObject* errors = NULL;
  if (PyArg_ParseTuple(args, "|O", &errors) < 0) {
    return NULL;
  }
  if (self->message->IsInitialized()) {
    Py_RETURN_TRUE;
  }
  if (errors != NULL) {
    ScopedPyObjectPtr initialization_errors(
        FindInitializationErrors(self));
    if (initialization_errors == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr extend_name(PyString_FromString("extend"));
    if (extend_name == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr result(PyObject_CallMethodObjArgs(
        errors,
        extend_name.get(),
        initialization_errors.get(),
        NULL));
    if (result == NULL) {
      return NULL;
    }
  }
  Py_RETURN_FALSE;
}

PyObject* HasFieldByDescriptor(
    CMessage* self, const FieldDescriptor* field_descriptor) {
  Message* message = self->message;
  if (!CheckFieldBelongsToMessage(field_descriptor, message)) {
    return NULL;
  }
  if (field_descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
    PyErr_SetString(PyExc_KeyError,
                    "Field is repeated. A singular method is required.");
    return NULL;
  }
  bool has_field =
      message->GetReflection()->HasField(*message, field_descriptor);
  return PyBool_FromLong(has_field ? 1 : 0);
}

const FieldDescriptor* FindFieldWithOneofs(
    const Message* message, const string& field_name, bool* in_oneof) {
  *in_oneof = false;
  const Descriptor* descriptor = message->GetDescriptor();
  const FieldDescriptor* field_descriptor =
      descriptor->FindFieldByName(field_name);
  if (field_descriptor != NULL) {
    return field_descriptor;
  }
  const OneofDescriptor* oneof_desc =
      descriptor->FindOneofByName(field_name);
  if (oneof_desc != NULL) {
    *in_oneof = true;
    return message->GetReflection()->GetOneofFieldDescriptor(*message,
                                                             oneof_desc);
  }
  return NULL;
}

bool CheckHasPresence(const FieldDescriptor* field_descriptor, bool in_oneof) {
  if (field_descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
    PyErr_Format(PyExc_ValueError,
                 "Protocol message has no singular \"%s\" field.",
                 field_descriptor->name().c_str());
    return false;
  }

  if (field_descriptor->file()->syntax() == FileDescriptor::SYNTAX_PROTO3) {
    // HasField() for a oneof *itself* isn't supported.
    if (in_oneof) {
      PyErr_Format(PyExc_ValueError,
                   "Can't test oneof field \"%s\" for presence in proto3, use "
                   "WhichOneof instead.",
                   field_descriptor->containing_oneof()->name().c_str());
      return false;
    }

    // ...but HasField() for fields *in* a oneof is supported.
    if (field_descriptor->containing_oneof() != NULL) {
      return true;
    }

    if (field_descriptor->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
      PyErr_Format(
          PyExc_ValueError,
          "Can't test non-submessage field \"%s\" for presence in proto3.",
          field_descriptor->name().c_str());
      return false;
    }
  }

  return true;
}

PyObject* HasField(CMessage* self, PyObject* arg) {
  char* field_name;
  Py_ssize_t size;
#if PY_MAJOR_VERSION < 3
  if (PyString_AsStringAndSize(arg, &field_name, &size) < 0) {
    return NULL;
  }
#else
  field_name = PyUnicode_AsUTF8AndSize(arg, &size);
  if (!field_name) {
    return NULL;
  }
#endif

  Message* message = self->message;
  bool is_in_oneof;
  const FieldDescriptor* field_descriptor =
      FindFieldWithOneofs(message, string(field_name, size), &is_in_oneof);
  if (field_descriptor == NULL) {
    if (!is_in_oneof) {
      PyErr_Format(PyExc_ValueError, "Unknown field %s.", field_name);
      return NULL;
    } else {
      Py_RETURN_FALSE;
    }
  }

  if (!CheckHasPresence(field_descriptor, is_in_oneof)) {
    return NULL;
  }

  if (message->GetReflection()->HasField(*message, field_descriptor)) {
    Py_RETURN_TRUE;
  }

  Py_RETURN_FALSE;
}

PyObject* ClearExtension(CMessage* self, PyObject* extension) {
  const FieldDescriptor* descriptor = GetExtensionDescriptor(extension);
  if (descriptor == NULL) {
    return NULL;
  }
  if (self->extensions != NULL) {
    PyObject* value = PyDict_GetItem(self->extensions->values, extension);
    if (value != NULL) {
      if (InternalReleaseFieldByDescriptor(self, descriptor, value) < 0) {
        return NULL;
      }
      PyDict_DelItem(self->extensions->values, extension);
    }
  }
  return ClearFieldByDescriptor(self, descriptor);
}

PyObject* HasExtension(CMessage* self, PyObject* extension) {
  const FieldDescriptor* descriptor = GetExtensionDescriptor(extension);
  if (descriptor == NULL) {
    return NULL;
  }
  return HasFieldByDescriptor(self, descriptor);
}

// ---------------------------------------------------------------------
// Releasing messages
//
// The Python API's ClearField() and Clear() methods behave
// differently than their C++ counterparts.  While the C++ versions
// clears the children the Python versions detaches the children,
// without touching their content.  This impedance mismatch causes
// some complexity in the implementation, which is captured in this
// section.
//
// When a CMessage field is cleared we need to:
//
// * Release the Message used as the backing store for the CMessage
//   from its parent.
//
// * Change the owner field of the released CMessage and all of its
//   children to point to the newly released Message.
//
// * Clear the weak references from the released CMessage to the
//   parent.
//
// When a RepeatedCompositeContainer field is cleared we need to:
//
// * Release all the Message used as the backing store for the
//   CMessages stored in the container.
//
// * Change the owner field of all the released CMessage and all of
//   their children to point to the newly released Messages.
//
// * Clear the weak references from the released container to the
//   parent.

class SetOwnerVisitor : public ChildVisitor {
 public:
  // new_owner must outlive this object.
  explicit SetOwnerVisitor(const CMessage::OwnerRef& new_owner)
      : new_owner_(new_owner) {}

  int VisitRepeatedCompositeContainer(RepeatedCompositeContainer* container) {
    repeated_composite_container::SetOwner(container, new_owner_);
    return 0;
  }

  int VisitRepeatedScalarContainer(RepeatedScalarContainer* container) {
    repeated_scalar_container::SetOwner(container, new_owner_);
    return 0;
  }

  int VisitMapContainer(MapContainer* container) {
    container->SetOwner(new_owner_);
    return 0;
  }

  int VisitCMessage(CMessage* cmessage,
                    const FieldDescriptor* field_descriptor) {
    return SetOwner(cmessage, new_owner_);
  }

 private:
  const CMessage::OwnerRef& new_owner_;
};

// Change the owner of this CMessage and all its children, recursively.
int SetOwner(CMessage* self, const CMessage::OwnerRef& new_owner) {
  self->owner = new_owner;
  if (ForEachCompositeField(self, SetOwnerVisitor(new_owner)) == -1)
    return -1;
  return 0;
}

// Releases the message specified by 'field' and returns the
// pointer. If the field does not exist a new message is created using
// 'descriptor'. The caller takes ownership of the returned pointer.
Message* ReleaseMessage(CMessage* self,
                        const Descriptor* descriptor,
                        const FieldDescriptor* field_descriptor) {
  MessageFactory* message_factory = GetFactoryForMessage(self)->message_factory;
  Message* released_message = self->message->GetReflection()->ReleaseMessage(
      self->message, field_descriptor, message_factory);
  // ReleaseMessage will return NULL which differs from
  // child_cmessage->message, if the field does not exist.  In this case,
  // the latter points to the default instance via a const_cast<>, so we
  // have to reset it to a new mutable object since we are taking ownership.
  if (released_message == NULL) {
    const Message* prototype = message_factory->GetPrototype(descriptor);
    GOOGLE_DCHECK(prototype != NULL);
    released_message = prototype->New();
  }

  return released_message;
}

int ReleaseSubMessage(CMessage* self,
                      const FieldDescriptor* field_descriptor,
                      CMessage* child_cmessage) {
  // Release the Message
  CMessage::OwnerRef released_message(ReleaseMessage(
      self, child_cmessage->message->GetDescriptor(), field_descriptor));
  child_cmessage->message = released_message.get();
  child_cmessage->owner.swap(released_message);
  child_cmessage->parent = NULL;
  child_cmessage->parent_field_descriptor = NULL;
  child_cmessage->read_only = false;
  return ForEachCompositeField(child_cmessage,
                               SetOwnerVisitor(child_cmessage->owner));
}

struct ReleaseChild : public ChildVisitor {
  // message must outlive this object.
  explicit ReleaseChild(CMessage* parent) :
      parent_(parent) {}

  int VisitRepeatedCompositeContainer(RepeatedCompositeContainer* container) {
    return repeated_composite_container::Release(container);
  }

  int VisitRepeatedScalarContainer(RepeatedScalarContainer* container) {
    return repeated_scalar_container::Release(container);
  }

  int VisitMapContainer(MapContainer* container) {
    return container->Release();
  }

  int VisitCMessage(CMessage* cmessage,
                    const FieldDescriptor* field_descriptor) {
    return ReleaseSubMessage(parent_, field_descriptor, cmessage);
  }

  CMessage* parent_;
};

int InternalReleaseFieldByDescriptor(
    CMessage* self,
    const FieldDescriptor* field_descriptor,
    PyObject* composite_field) {
  return VisitCompositeField(
      field_descriptor,
      composite_field,
      ReleaseChild(self));
}

PyObject* ClearFieldByDescriptor(
    CMessage* self,
    const FieldDescriptor* field_descriptor) {
  if (!CheckFieldBelongsToMessage(field_descriptor, self->message)) {
    return NULL;
  }
  AssureWritable(self);
  Message* message = self->message;
  message->GetReflection()->ClearField(message, field_descriptor);
  Py_RETURN_NONE;
}

PyObject* ClearField(CMessage* self, PyObject* arg) {
  if (!PyString_Check(arg)) {
    PyErr_SetString(PyExc_TypeError, "field name must be a string");
    return NULL;
  }
#if PY_MAJOR_VERSION < 3
  const char* field_name = PyString_AS_STRING(arg);
  Py_ssize_t size = PyString_GET_SIZE(arg);
#else
  Py_ssize_t size;
  const char* field_name = PyUnicode_AsUTF8AndSize(arg, &size);
#endif
  AssureWritable(self);
  Message* message = self->message;
  ScopedPyObjectPtr arg_in_oneof;
  bool is_in_oneof;
  const FieldDescriptor* field_descriptor =
      FindFieldWithOneofs(message, string(field_name, size), &is_in_oneof);
  if (field_descriptor == NULL) {
    if (!is_in_oneof) {
      PyErr_Format(PyExc_ValueError,
                   "Protocol message has no \"%s\" field.", field_name);
      return NULL;
    } else {
      Py_RETURN_NONE;
    }
  } else if (is_in_oneof) {
    const string& name = field_descriptor->name();
    arg_in_oneof.reset(PyString_FromStringAndSize(name.c_str(), name.size()));
    arg = arg_in_oneof.get();
  }

  // Release the field if it exists in the dict of composite fields.
  if (self->composite_fields) {
    PyObject* value = PyDict_GetItem(self->composite_fields, arg);
    if (value != NULL) {
      if (InternalReleaseFieldByDescriptor(self, field_descriptor, value) < 0) {
        return NULL;
      }
      PyDict_DelItem(self->composite_fields, arg);
    }
  }
  return ClearFieldByDescriptor(self, field_descriptor);
}

PyObject* Clear(CMessage* self) {
  AssureWritable(self);
  if (ForEachCompositeField(self, ReleaseChild(self)) == -1)
    return NULL;
  Py_CLEAR(self->extensions);
  if (self->composite_fields) {
    PyDict_Clear(self->composite_fields);
  }
  self->message->Clear();
  Py_RETURN_NONE;
}

// ---------------------------------------------------------------------

static string GetMessageName(CMessage* self) {
  if (self->parent_field_descriptor != NULL) {
    return self->parent_field_descriptor->full_name();
  } else {
    return self->message->GetDescriptor()->full_name();
  }
}

static PyObject* InternalSerializeToString(
    CMessage* self, PyObject* args, PyObject* kwargs,
    bool require_initialized) {
  // Parse the "deterministic" kwarg; defaults to False.
  static char* kwlist[] = { "deterministic", 0 };
  PyObject* deterministic_obj = Py_None;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist,
                                   &deterministic_obj)) {
    return NULL;
  }
  // Preemptively convert to a bool first, so we don't need to back out of
  // allocating memory if this raises an exception.
  // NOTE: This is unused later if deterministic == Py_None, but that's fine.
  int deterministic = PyObject_IsTrue(deterministic_obj);
  if (deterministic < 0) {
    return NULL;
  }

  if (require_initialized && !self->message->IsInitialized()) {
    ScopedPyObjectPtr errors(FindInitializationErrors(self));
    if (errors == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr comma(PyString_FromString(","));
    if (comma == NULL) {
      return NULL;
    }
    ScopedPyObjectPtr joined(
        PyObject_CallMethod(comma.get(), "join", "O", errors.get()));
    if (joined == NULL) {
      return NULL;
    }

    // TODO(haberman): this is a (hopefully temporary) hack.  The unit testing
    // infrastructure reloads all pure-Python modules for every test, but not
    // C++ modules (because that's generally impossible:
    // http://bugs.python.org/issue1144263).  But if we cache EncodeError, we'll
    // return the EncodeError from a previous load of the module, which won't
    // match a user's attempt to catch EncodeError.  So we have to look it up
    // again every time.
    ScopedPyObjectPtr message_module(PyImport_ImportModule(
        "google.protobuf.message"));
    if (message_module.get() == NULL) {
      return NULL;
    }

    ScopedPyObjectPtr encode_error(
        PyObject_GetAttrString(message_module.get(), "EncodeError"));
    if (encode_error.get() == NULL) {
      return NULL;
    }
    PyErr_Format(encode_error.get(),
                 "Message %s is missing required fields: %s",
                 GetMessageName(self).c_str(), PyString_AsString(joined.get()));
    return NULL;
  }

  // Ok, arguments parsed and errors checked, now encode to a string
  const size_t size = self->message->ByteSizeLong();
  if (size == 0) {
    return PyBytes_FromString("");
  }
  PyObject* result = PyBytes_FromStringAndSize(NULL, size);
  if (result == NULL) {
    return NULL;
  }
  io::ArrayOutputStream out(PyBytes_AS_STRING(result), size);
  io::CodedOutputStream coded_out(&out);
  if (deterministic_obj != Py_None) {
    coded_out.SetSerializationDeterministic(deterministic);
  }
  self->message->SerializeWithCachedSizes(&coded_out);
  GOOGLE_CHECK(!coded_out.HadError());
  return result;
}

static PyObject* SerializeToString(
    CMessage* self, PyObject* args, PyObject* kwargs) {
  return InternalSerializeToString(self, args, kwargs,
                                   /*require_initialized=*/true);
}

static PyObject* SerializePartialToString(
    CMessage* self, PyObject* args, PyObject* kwargs) {
  return InternalSerializeToString(self, args, kwargs,
                                   /*require_initialized=*/false);
}

// Formats proto fields for ascii dumps using python formatting functions where
// appropriate.
class PythonFieldValuePrinter : public TextFormat::FieldValuePrinter {
 public:
  // Python has some differences from C++ when printing floating point numbers.
  //
  // 1) Trailing .0 is always printed.
  // 2) (Python2) Output is rounded to 12 digits.
  // 3) (Python3) The full precision of the double is preserved (and Python uses
  //    David M. Gay's dtoa(), when the C++ code uses SimpleDtoa. There are some
  //    differences, but they rarely happen)
  //
  // We override floating point printing with the C-API function for printing
  // Python floats to ensure consistency.
  string PrintFloat(float value) const { return PrintDouble(value); }
  string PrintDouble(double value) const {
    // This implementation is not highly optimized (it allocates two temporary
    // Python objects) but it is simple and portable.  If this is shown to be a
    // performance bottleneck, we can optimize it, but the results will likely
    // be more complicated to accommodate the differing behavior of double
    // formatting between Python 2 and Python 3.
    //
    // (Though a valid question is: do we really want to make out output
    // dependent on the Python version?)
    ScopedPyObjectPtr py_value(PyFloat_FromDouble(value));
    if (!py_value.get()) {
      return string();
    }

    ScopedPyObjectPtr py_str(PyObject_Str(py_value.get()));
    if (!py_str.get()) {
      return string();
    }

    return string(PyString_AsString(py_str.get()));
  }
};

static PyObject* ToStr(CMessage* self) {
  TextFormat::Printer printer;
  // Passes ownership
  printer.SetDefaultFieldValuePrinter(new PythonFieldValuePrinter());
  printer.SetHideUnknownFields(true);
  string output;
  if (!printer.PrintToString(*self->message, &output)) {
    PyErr_SetString(PyExc_ValueError, "Unable to convert message to str");
    return NULL;
  }
  return PyString_FromString(output.c_str());
}

PyObject* MergeFrom(CMessage* self, PyObject* arg) {
  CMessage* other_message;
  if (!PyObject_TypeCheck(arg, &CMessage_Type)) {
    PyErr_Format(PyExc_TypeError,
                 "Parameter to MergeFrom() must be instance of same class: "
                 "expected %s got %s.",
                 self->message->GetDescriptor()->full_name().c_str(),
                 Py_TYPE(arg)->tp_name);
    return NULL;
  }

  other_message = reinterpret_cast<CMessage*>(arg);
  if (other_message->message->GetDescriptor() !=
      self->message->GetDescriptor()) {
    PyErr_Format(PyExc_TypeError,
                 "Parameter to MergeFrom() must be instance of same class: "
                 "expected %s got %s.",
                 self->message->GetDescriptor()->full_name().c_str(),
                 other_message->message->GetDescriptor()->full_name().c_str());
    return NULL;
  }
  AssureWritable(self);

  // TODO(tibell): Message::MergeFrom might turn some child Messages
  // into mutable messages, invalidating the message field in the
  // corresponding CMessages.  We should run a FixupMessageReferences
  // pass here.

  self->message->MergeFrom(*other_message->message);
  Py_RETURN_NONE;
}

static PyObject* CopyFrom(CMessage* self, PyObject* arg) {
  CMessage* other_message;
  if (!PyObject_TypeCheck(arg, &CMessage_Type)) {
    PyErr_Format(PyExc_TypeError,
                 "Parameter to CopyFrom() must be instance of same class: "
                 "expected %s got %s.",
                 self->message->GetDescriptor()->full_name().c_str(),
                 Py_TYPE(arg)->tp_name);
    return NULL;
  }

  other_message = reinterpret_cast<CMessage*>(arg);

  if (self == other_message) {
    Py_RETURN_NONE;
  }

  if (other_message->message->GetDescriptor() !=
      self->message->GetDescriptor()) {
    PyErr_Format(PyExc_TypeError,
                 "Parameter to CopyFrom() must be instance of same class: "
                 "expected %s got %s.",
                 self->message->GetDescriptor()->full_name().c_str(),
                 other_message->message->GetDescriptor()->full_name().c_str());
    return NULL;
  }

  AssureWritable(self);

  // CopyFrom on the message will not clean up self->composite_fields,
  // which can leave us in an inconsistent state, so clear it out here.
  (void)ScopedPyObjectPtr(Clear(self));

  self->message->CopyFrom(*other_message->message);

  Py_RETURN_NONE;
}

// Protobuf has a 64MB limit built in, this variable will override this. Please
// do not enable this unless you fully understand the implications: protobufs
// must all be kept in memory at the same time, so if they grow too big you may
// get OOM errors. The protobuf APIs do not provide any tools for processing
// protobufs in chunks.  If you have protos this big you should break them up if
// it is at all convenient to do so.
#ifdef PROTOBUF_PYTHON_ALLOW_OVERSIZE_PROTOS
static bool allow_oversize_protos = true;
#else
static bool allow_oversize_protos = false;
#endif

// Provide a method in the module to set allow_oversize_protos to a boolean
// value. This method returns the newly value of allow_oversize_protos.
PyObject* SetAllowOversizeProtos(PyObject* m, PyObject* arg) {
  if (!arg || !PyBool_Check(arg)) {
    PyErr_SetString(PyExc_TypeError,
                    "Argument to SetAllowOversizeProtos must be boolean");
    return NULL;
  }
  allow_oversize_protos = PyObject_IsTrue(arg);
  if (allow_oversize_protos) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

static PyObject* MergeFromString(CMessage* self, PyObject* arg) {
  const void* data;
  Py_ssize_t data_length;
  if (PyObject_AsReadBuffer(arg, &data, &data_length) < 0) {
    return NULL;
  }

  AssureWritable(self);
  io::CodedInputStream input(
      reinterpret_cast<const uint8*>(data), data_length);
  if (allow_oversize_protos) {
    input.SetTotalBytesLimit(INT_MAX, INT_MAX);
  }
  PyMessageFactory* factory = GetFactoryForMessage(self);
  input.SetExtensionRegistry(factory->pool->pool, factory->message_factory);
  bool success = self->message->MergePartialFromCodedStream(&input);
  if (success) {
    if (!input.ConsumedEntireMessage()) {
      // TODO(jieluo): Raise error and return NULL instead.
      // b/27494216
      PyErr_Warn(NULL, "Unexpected end-group tag: Not all data was converted");
    }
    return PyInt_FromLong(input.CurrentPosition());
  } else {
    PyErr_Format(DecodeError_class, "Error parsing message");
    return NULL;
  }
}

static PyObject* ParseFromString(CMessage* self, PyObject* arg) {
  if (ScopedPyObjectPtr(Clear(self)) == NULL) {
    return NULL;
  }
  return MergeFromString(self, arg);
}

static PyObject* ByteSize(CMessage* self, PyObject* args) {
  return PyLong_FromLong(self->message->ByteSize());
}

PyObject* RegisterExtension(PyObject* cls, PyObject* extension_handle) {
  const FieldDescriptor* descriptor =
      GetExtensionDescriptor(extension_handle);
  if (descriptor == NULL) {
    return NULL;
  }
  if (!PyObject_TypeCheck(cls, &CMessageClass_Type)) {
    PyErr_Format(PyExc_TypeError, "Expected a message class, got %s",
                 cls->ob_type->tp_name);
    return NULL;
  }
  CMessageClass *message_class = reinterpret_cast<CMessageClass*>(cls);
  if (message_class == NULL) {
    return NULL;
  }
  // If the extension was already registered, check that it is the same.
  const FieldDescriptor* existing_extension =
      message_class->py_message_factory->pool->pool->FindExtensionByNumber(
          descriptor->containing_type(), descriptor->number());
  if (existing_extension != NULL && existing_extension != descriptor) {
    PyErr_SetString(PyExc_ValueError, "Double registration of Extensions");
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* SetInParent(CMessage* self, PyObject* args) {
  AssureWritable(self);
  Py_RETURN_NONE;
}

static PyObject* WhichOneof(CMessage* self, PyObject* arg) {
  Py_ssize_t name_size;
  char *name_data;
  if (PyString_AsStringAndSize(arg, &name_data, &name_size) < 0)
    return NULL;
  string oneof_name = string(name_data, name_size);
  const OneofDescriptor* oneof_desc =
      self->message->GetDescriptor()->FindOneofByName(oneof_name);
  if (oneof_desc == NULL) {
    PyErr_Format(PyExc_ValueError,
                 "Protocol message has no oneof \"%s\" field.",
                 oneof_name.c_str());
    return NULL;
  }
  const FieldDescriptor* field_in_oneof =
      self->message->GetReflection()->GetOneofFieldDescriptor(
          *self->message, oneof_desc);
  if (field_in_oneof == NULL) {
    Py_RETURN_NONE;
  } else {
    const string& name = field_in_oneof->name();
    return PyString_FromStringAndSize(name.c_str(), name.size());
  }
}

static PyObject* GetExtensionDict(CMessage* self, void *closure);

static PyObject* ListFields(CMessage* self) {
  std::vector<const FieldDescriptor*> fields;
  self->message->GetReflection()->ListFields(*self->message, &fields);

  // Normally, the list will be exactly the size of the fields.
  ScopedPyObjectPtr all_fields(PyList_New(fields.size()));
  if (all_fields == NULL) {
    return NULL;
  }

  // When there are unknown extensions, the py list will *not* contain
  // the field information.  Thus the actual size of the py list will be
  // smaller than the size of fields.  Set the actual size at the end.
  Py_ssize_t actual_size = 0;
  for (size_t i = 0; i < fields.size(); ++i) {
    ScopedPyObjectPtr t(PyTuple_New(2));
    if (t == NULL) {
      return NULL;
    }

    if (fields[i]->is_extension()) {
      ScopedPyObjectPtr extension_field(
          PyFieldDescriptor_FromDescriptor(fields[i]));
      if (extension_field == NULL) {
        return NULL;
      }
      // With C++ descriptors, the field can always be retrieved, but for
      // unknown extensions which have not been imported in Python code, there
      // is no message class and we cannot retrieve the value.
      // TODO(amauryfa): consider building the class on the fly!
      if (fields[i]->message_type() != NULL &&
          message_factory::GetMessageClass(
              GetFactoryForMessage(self),
              fields[i]->message_type()) == NULL) {
        PyErr_Clear();
        continue;
      }
      ScopedPyObjectPtr extensions(GetExtensionDict(self, NULL));
      if (extensions == NULL) {
        return NULL;
      }
      // 'extension' reference later stolen by PyTuple_SET_ITEM.
      PyObject* extension = PyObject_GetItem(
          extensions.get(), extension_field.get());
      if (extension == NULL) {
        return NULL;
      }
      PyTuple_SET_ITEM(t.get(), 0, extension_field.release());
      // Steals reference to 'extension'
      PyTuple_SET_ITEM(t.get(), 1, extension);
    } else {
      // Normal field
      const string& field_name = fields[i]->name();
      ScopedPyObjectPtr py_field_name(PyString_FromStringAndSize(
          field_name.c_str(), field_name.length()));
      if (py_field_name == NULL) {
        PyErr_SetString(PyExc_ValueError, "bad string");
        return NULL;
      }
      ScopedPyObjectPtr field_descriptor(
          PyFieldDescriptor_FromDescriptor(fields[i]));
      if (field_descriptor == NULL) {
        return NULL;
      }

      PyObject* field_value =
          GetAttr(reinterpret_cast<PyObject*>(self), py_field_name.get());
      if (field_value == NULL) {
        PyErr_SetObject(PyExc_ValueError, py_field_name.get());
        return NULL;
      }
      PyTuple_SET_ITEM(t.get(), 0, field_descriptor.release());
      PyTuple_SET_ITEM(t.get(), 1, field_value);
    }
    PyList_SET_ITEM(all_fields.get(), actual_size, t.release());
    ++actual_size;
  }
  if (static_cast<size_t>(actual_size) != fields.size() &&
      (PyList_SetSlice(all_fields.get(), actual_size, fields.size(), NULL) <
       0)) {
    return NULL;
  }
  return all_fields.release();
}

static PyObject* DiscardUnknownFields(CMessage* self) {
  AssureWritable(self);
  self->message->DiscardUnknownFields();
  Py_RETURN_NONE;
}

PyObject* FindInitializationErrors(CMessage* self) {
  Message* message = self->message;
  std::vector<string> errors;
  message->FindInitializationErrors(&errors);

  PyObject* error_list = PyList_New(errors.size());
  if (error_list == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < errors.size(); ++i) {
    const string& error = errors[i];
    PyObject* error_string = PyString_FromStringAndSize(
        error.c_str(), error.length());
    if (error_string == NULL) {
      Py_DECREF(error_list);
      return NULL;
    }
    PyList_SET_ITEM(error_list, i, error_string);
  }
  return error_list;
}

static PyObject* RichCompare(CMessage* self, PyObject* other, int opid) {
  // Only equality comparisons are implemented.
  if (opid != Py_EQ && opid != Py_NE) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
  bool equals = true;
  // If other is not a message, it cannot be equal.
  if (!PyObject_TypeCheck(other, &CMessage_Type)) {
    equals = false;
  }
  const google::protobuf::Message* other_message =
      reinterpret_cast<CMessage*>(other)->message;
  // If messages don't have the same descriptors, they are not equal.
  if (equals &&
      self->message->GetDescriptor() != other_message->GetDescriptor()) {
    equals = false;
  }
  // Check the message contents.
  if (equals && !google::protobuf::util::MessageDifferencer::Equals(
          *self->message,
          *reinterpret_cast<CMessage*>(other)->message)) {
    equals = false;
  }
  if (equals ^ (opid == Py_EQ)) {
    Py_RETURN_FALSE;
  } else {
    Py_RETURN_TRUE;
  }
}

PyObject* InternalGetScalar(const Message* message,
                            const FieldDescriptor* field_descriptor) {
  const Reflection* reflection = message->GetReflection();

  if (!CheckFieldBelongsToMessage(field_descriptor, message)) {
    return NULL;
  }

  PyObject* result = NULL;
  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: {
      int32 value = reflection->GetInt32(*message, field_descriptor);
      result = PyInt_FromLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      int64 value = reflection->GetInt64(*message, field_descriptor);
      result = PyLong_FromLongLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT32: {
      uint32 value = reflection->GetUInt32(*message, field_descriptor);
      result = PyInt_FromSize_t(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT64: {
      uint64 value = reflection->GetUInt64(*message, field_descriptor);
      result = PyLong_FromUnsignedLongLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      float value = reflection->GetFloat(*message, field_descriptor);
      result = PyFloat_FromDouble(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      double value = reflection->GetDouble(*message, field_descriptor);
      result = PyFloat_FromDouble(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_BOOL: {
      bool value = reflection->GetBool(*message, field_descriptor);
      result = PyBool_FromLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      string scratch;
      const string& value =
          reflection->GetStringReference(*message, field_descriptor, &scratch);
      result = ToStringObject(field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      const EnumValueDescriptor* enum_value =
          message->GetReflection()->GetEnum(*message, field_descriptor);
      result = PyInt_FromLong(enum_value->number());
      break;
    }
    default:
      PyErr_Format(
          PyExc_SystemError, "Getting a value from a field of unknown type %d",
          field_descriptor->cpp_type());
  }

  return result;
}

PyObject* InternalGetSubMessage(
    CMessage* self, const FieldDescriptor* field_descriptor) {
  const Reflection* reflection = self->message->GetReflection();
  PyMessageFactory* factory = GetFactoryForMessage(self);
  const Message& sub_message = reflection->GetMessage(
      *self->message, field_descriptor, factory->message_factory);

  CMessageClass* message_class = message_factory::GetOrCreateMessageClass(
      factory, field_descriptor->message_type());
  ScopedPyObjectPtr message_class_handler(
      reinterpret_cast<PyObject*>(message_class));
  if (message_class == NULL) {
    return NULL;
  }

  CMessage* cmsg = cmessage::NewEmptyMessage(message_class);
  if (cmsg == NULL) {
    return NULL;
  }

  cmsg->owner = self->owner;
  cmsg->parent = self;
  cmsg->parent_field_descriptor = field_descriptor;
  cmsg->read_only = !reflection->HasField(*self->message, field_descriptor);
  cmsg->message = const_cast<Message*>(&sub_message);

  return reinterpret_cast<PyObject*>(cmsg);
}

int InternalSetNonOneofScalar(
    Message* message,
    const FieldDescriptor* field_descriptor,
    PyObject* arg) {
  const Reflection* reflection = message->GetReflection();

  if (!CheckFieldBelongsToMessage(field_descriptor, message)) {
    return -1;
  }

  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: {
      GOOGLE_CHECK_GET_INT32(arg, value, -1);
      reflection->SetInt32(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      GOOGLE_CHECK_GET_INT64(arg, value, -1);
      reflection->SetInt64(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT32: {
      GOOGLE_CHECK_GET_UINT32(arg, value, -1);
      reflection->SetUInt32(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT64: {
      GOOGLE_CHECK_GET_UINT64(arg, value, -1);
      reflection->SetUInt64(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      GOOGLE_CHECK_GET_FLOAT(arg, value, -1);
      reflection->SetFloat(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      GOOGLE_CHECK_GET_DOUBLE(arg, value, -1);
      reflection->SetDouble(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_BOOL: {
      GOOGLE_CHECK_GET_BOOL(arg, value, -1);
      reflection->SetBool(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      if (!CheckAndSetString(
          arg, message, field_descriptor, reflection, false, -1)) {
        return -1;
      }
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      GOOGLE_CHECK_GET_INT32(arg, value, -1);
      if (reflection->SupportsUnknownEnumValues()) {
        reflection->SetEnumValue(message, field_descriptor, value);
      } else {
        const EnumDescriptor* enum_descriptor = field_descriptor->enum_type();
        const EnumValueDescriptor* enum_value =
            enum_descriptor->FindValueByNumber(value);
        if (enum_value != NULL) {
          reflection->SetEnum(message, field_descriptor, enum_value);
        } else {
          PyErr_Format(PyExc_ValueError, "Unknown enum value: %d", value);
          return -1;
        }
      }
      break;
    }
    default:
      PyErr_Format(
          PyExc_SystemError, "Setting value to a field of unknown type %d",
          field_descriptor->cpp_type());
      return -1;
  }

  return 0;
}

int InternalSetScalar(
    CMessage* self,
    const FieldDescriptor* field_descriptor,
    PyObject* arg) {
  if (!CheckFieldBelongsToMessage(field_descriptor, self->message)) {
    return -1;
  }

  if (MaybeReleaseOverlappingOneofField(self, field_descriptor) < 0) {
    return -1;
  }

  return InternalSetNonOneofScalar(self->message, field_descriptor, arg);
}

PyObject* FromString(PyTypeObject* cls, PyObject* serialized) {
  PyObject* py_cmsg = PyObject_CallObject(
      reinterpret_cast<PyObject*>(cls), NULL);
  if (py_cmsg == NULL) {
    return NULL;
  }
  CMessage* cmsg = reinterpret_cast<CMessage*>(py_cmsg);

  ScopedPyObjectPtr py_length(MergeFromString(cmsg, serialized));
  if (py_length == NULL) {
    Py_DECREF(py_cmsg);
    return NULL;
  }

  return py_cmsg;
}

PyObject* DeepCopy(CMessage* self, PyObject* arg) {
  PyObject* clone = PyObject_CallObject(
      reinterpret_cast<PyObject*>(Py_TYPE(self)), NULL);
  if (clone == NULL) {
    return NULL;
  }
  if (!PyObject_TypeCheck(clone, &CMessage_Type)) {
    Py_DECREF(clone);
    return NULL;
  }
  if (ScopedPyObjectPtr(MergeFrom(
          reinterpret_cast<CMessage*>(clone),
          reinterpret_cast<PyObject*>(self))) == NULL) {
    Py_DECREF(clone);
    return NULL;
  }
  return clone;
}

PyObject* ToUnicode(CMessage* self) {
  // Lazy import to prevent circular dependencies
  ScopedPyObjectPtr text_format(
      PyImport_ImportModule("google.protobuf.text_format"));
  if (text_format == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr method_name(PyString_FromString("MessageToString"));
  if (method_name == NULL) {
    return NULL;
  }
  Py_INCREF(Py_True);
  ScopedPyObjectPtr encoded(PyObject_CallMethodObjArgs(
      text_format.get(), method_name.get(), self, Py_True, NULL));
  Py_DECREF(Py_True);
  if (encoded == NULL) {
    return NULL;
  }
#if PY_MAJOR_VERSION < 3
  PyObject* decoded = PyString_AsDecodedObject(encoded.get(), "utf-8", NULL);
#else
  PyObject* decoded = PyUnicode_FromEncodedObject(encoded.get(), "utf-8", NULL);
#endif
  if (decoded == NULL) {
    return NULL;
  }
  return decoded;
}

PyObject* Reduce(CMessage* self) {
  ScopedPyObjectPtr constructor(reinterpret_cast<PyObject*>(Py_TYPE(self)));
  constructor.inc();
  ScopedPyObjectPtr args(PyTuple_New(0));
  if (args == NULL) {
    return NULL;
  }
  ScopedPyObjectPtr state(PyDict_New());
  if (state == NULL) {
    return  NULL;
  }
  string contents;
  self->message->SerializePartialToString(&contents);
  ScopedPyObjectPtr serialized(
      PyBytes_FromStringAndSize(contents.c_str(), contents.size()));
  if (serialized == NULL) {
    return NULL;
  }
  if (PyDict_SetItemString(state.get(), "serialized", serialized.get()) < 0) {
    return NULL;
  }
  return Py_BuildValue("OOO", constructor.get(), args.get(), state.get());
}

PyObject* SetState(CMessage* self, PyObject* state) {
  if (!PyDict_Check(state)) {
    PyErr_SetString(PyExc_TypeError, "state not a dict");
    return NULL;
  }
  PyObject* serialized = PyDict_GetItemString(state, "serialized");
  if (serialized == NULL) {
    return NULL;
  }
  if (ScopedPyObjectPtr(ParseFromString(self, serialized)) == NULL) {
    return NULL;
  }
  Py_RETURN_NONE;
}

// CMessage static methods:
PyObject* _CheckCalledFromGeneratedFile(PyObject* unused,
                                        PyObject* unused_arg) {
  if (!_CalledFromGeneratedFile(1)) {
    PyErr_SetString(PyExc_TypeError,
                    "Descriptors should not be created directly, "
                    "but only retrieved from their parent.");
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* GetExtensionDict(CMessage* self, void *closure) {
  if (self->extensions)  {
    Py_INCREF(self->extensions);
    return reinterpret_cast<PyObject*>(self->extensions);
  }

  // If there are extension_ranges, the message is "extendable". Allocate a
  // dictionary to store the extension fields.
  const Descriptor* descriptor = GetMessageDescriptor(Py_TYPE(self));
  if (descriptor->extension_range_count() > 0) {
    ExtensionDict* extension_dict = extension_dict::NewExtensionDict(self);
    if (extension_dict == NULL) {
      return NULL;
    }
    self->extensions = extension_dict;
    Py_INCREF(self->extensions);
    return reinterpret_cast<PyObject*>(self->extensions);
  }

  PyErr_SetNone(PyExc_AttributeError);
  return NULL;
}

static PyObject* GetExtensionsByName(CMessage *self, void *closure) {
  return message_meta::GetExtensionsByName(
      reinterpret_cast<CMessageClass*>(Py_TYPE(self)), closure);
}

static PyObject* GetExtensionsByNumber(CMessage *self, void *closure) {
  return message_meta::GetExtensionsByNumber(
      reinterpret_cast<CMessageClass*>(Py_TYPE(self)), closure);
}

static PyGetSetDef Getters[] = {
  {"Extensions", (getter)GetExtensionDict, NULL, "Extension dict"},
  {"_extensions_by_name", (getter)GetExtensionsByName, NULL},
  {"_extensions_by_number", (getter)GetExtensionsByNumber, NULL},
  {NULL}
};


static PyMethodDef Methods[] = {
  { "__deepcopy__", (PyCFunction)DeepCopy, METH_VARARGS,
    "Makes a deep copy of the class." },
  { "__reduce__", (PyCFunction)Reduce, METH_NOARGS,
    "Outputs picklable representation of the message." },
  { "__setstate__", (PyCFunction)SetState, METH_O,
    "Inputs picklable representation of the message." },
  { "__unicode__", (PyCFunction)ToUnicode, METH_NOARGS,
    "Outputs a unicode representation of the message." },
  { "ByteSize", (PyCFunction)ByteSize, METH_NOARGS,
    "Returns the size of the message in bytes." },
  { "Clear", (PyCFunction)Clear, METH_NOARGS,
    "Clears the message." },
  { "ClearExtension", (PyCFunction)ClearExtension, METH_O,
    "Clears a message field." },
  { "ClearField", (PyCFunction)ClearField, METH_O,
    "Clears a message field." },
  { "CopyFrom", (PyCFunction)CopyFrom, METH_O,
    "Copies a protocol message into the current message." },
  { "DiscardUnknownFields", (PyCFunction)DiscardUnknownFields, METH_NOARGS,
    "Discards the unknown fields." },
  { "FindInitializationErrors", (PyCFunction)FindInitializationErrors,
    METH_NOARGS,
    "Finds unset required fields." },
  { "FromString", (PyCFunction)FromString, METH_O | METH_CLASS,
    "Creates new method instance from given serialized data." },
  { "HasExtension", (PyCFunction)HasExtension, METH_O,
    "Checks if a message field is set." },
  { "HasField", (PyCFunction)HasField, METH_O,
    "Checks if a message field is set." },
  { "IsInitialized", (PyCFunction)IsInitialized, METH_VARARGS,
    "Checks if all required fields of a protocol message are set." },
  { "ListFields", (PyCFunction)ListFields, METH_NOARGS,
    "Lists all set fields of a message." },
  { "MergeFrom", (PyCFunction)MergeFrom, METH_O,
    "Merges a protocol message into the current message." },
  { "MergeFromString", (PyCFunction)MergeFromString, METH_O,
    "Merges a serialized message into the current message." },
  { "ParseFromString", (PyCFunction)ParseFromString, METH_O,
    "Parses a serialized message into the current message." },
  { "RegisterExtension", (PyCFunction)RegisterExtension, METH_O | METH_CLASS,
    "Registers an extension with the current message." },
  { "SerializePartialToString", (PyCFunction)SerializePartialToString,
    METH_VARARGS | METH_KEYWORDS,
    "Serializes the message to a string, even if it isn't initialized." },
  { "SerializeToString", (PyCFunction)SerializeToString,
    METH_VARARGS | METH_KEYWORDS,
    "Serializes the message to a string, only for initialized messages." },
  { "SetInParent", (PyCFunction)SetInParent, METH_NOARGS,
    "Sets the has bit of the given field in its parent message." },
  { "WhichOneof", (PyCFunction)WhichOneof, METH_O,
    "Returns the name of the field set inside a oneof, "
    "or None if no field is set." },

  // Static Methods.
  { "_CheckCalledFromGeneratedFile", (PyCFunction)_CheckCalledFromGeneratedFile,
    METH_NOARGS | METH_STATIC,
    "Raises TypeError if the caller is not in a _pb2.py file."},
  { NULL, NULL}
};

static bool SetCompositeField(
    CMessage* self, PyObject* name, PyObject* value) {
  if (self->composite_fields == NULL) {
    self->composite_fields = PyDict_New();
    if (self->composite_fields == NULL) {
      return false;
    }
  }
  return PyDict_SetItem(self->composite_fields, name, value) == 0;
}

PyObject* GetAttr(PyObject* pself, PyObject* name) {
  CMessage* self = reinterpret_cast<CMessage*>(pself);
  PyObject* value = self->composite_fields ?
      PyDict_GetItem(self->composite_fields, name) : NULL;
  if (value != NULL) {
    Py_INCREF(value);
    return value;
  }

  const FieldDescriptor* field_descriptor = GetFieldDescriptor(self, name);
  if (field_descriptor == NULL) {
    return CMessage_Type.tp_base->tp_getattro(
        reinterpret_cast<PyObject*>(self), name);
  }

  if (field_descriptor->is_map()) {
    PyObject* py_container = NULL;
    const Descriptor* entry_type = field_descriptor->message_type();
    const FieldDescriptor* value_type = entry_type->FindFieldByName("value");
    if (value_type->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      CMessageClass* value_class = message_factory::GetMessageClass(
          GetFactoryForMessage(self), value_type->message_type());
      if (value_class == NULL) {
        return NULL;
      }
      py_container =
          NewMessageMapContainer(self, field_descriptor, value_class);
    } else {
      py_container = NewScalarMapContainer(self, field_descriptor);
    }
    if (py_container == NULL) {
      return NULL;
    }
    if (!SetCompositeField(self, name, py_container)) {
      Py_DECREF(py_container);
      return NULL;
    }
    return py_container;
  }

  if (field_descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
    PyObject* py_container = NULL;
    if (field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      CMessageClass* message_class = message_factory::GetMessageClass(
          GetFactoryForMessage(self), field_descriptor->message_type());
      if (message_class == NULL) {
        return NULL;
      }
      py_container = repeated_composite_container::NewContainer(
          self, field_descriptor, message_class);
    } else {
      py_container = repeated_scalar_container::NewContainer(
          self, field_descriptor);
    }
    if (py_container == NULL) {
      return NULL;
    }
    if (!SetCompositeField(self, name, py_container)) {
      Py_DECREF(py_container);
      return NULL;
    }
    return py_container;
  }

  if (field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    PyObject* sub_message = InternalGetSubMessage(self, field_descriptor);
    if (sub_message == NULL) {
      return NULL;
    }
    if (!SetCompositeField(self, name, sub_message)) {
      Py_DECREF(sub_message);
      return NULL;
    }
    return sub_message;
  }

  return InternalGetScalar(self->message, field_descriptor);
}

int SetAttr(PyObject* pself, PyObject* name, PyObject* value) {
  CMessage* self = reinterpret_cast<CMessage*>(pself);
  if (self->composite_fields && PyDict_Contains(self->composite_fields, name)) {
    PyErr_SetString(PyExc_TypeError, "Can't set composite field");
    return -1;
  }

  const FieldDescriptor* field_descriptor = GetFieldDescriptor(self, name);
  if (field_descriptor != NULL) {
    AssureWritable(self);
    if (field_descriptor->label() == FieldDescriptor::LABEL_REPEATED) {
      PyErr_Format(PyExc_AttributeError, "Assignment not allowed to repeated "
                   "field \"%s\" in protocol message object.",
                   field_descriptor->name().c_str());
      return -1;
    } else {
      if (field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        PyErr_Format(PyExc_AttributeError, "Assignment not allowed to "
                     "field \"%s\" in protocol message object.",
                     field_descriptor->name().c_str());
        return -1;
      } else {
        return InternalSetScalar(self, field_descriptor, value);
      }
    }
  }

  PyErr_Format(PyExc_AttributeError,
               "Assignment not allowed "
               "(no field \"%s\" in protocol message object).",
               PyString_AsString(name));
  return -1;
}

}  // namespace cmessage

PyTypeObject CMessage_Type = {
  PyVarObject_HEAD_INIT(&CMessageClass_Type, 0)
  FULL_MODULE_NAME ".CMessage",        // tp_name
  sizeof(CMessage),                    // tp_basicsize
  0,                                   //  tp_itemsize
  (destructor)cmessage::Dealloc,       //  tp_dealloc
  0,                                   //  tp_print
  0,                                   //  tp_getattr
  0,                                   //  tp_setattr
  0,                                   //  tp_compare
  (reprfunc)cmessage::ToStr,           //  tp_repr
  0,                                   //  tp_as_number
  0,                                   //  tp_as_sequence
  0,                                   //  tp_as_mapping
  PyObject_HashNotImplemented,         //  tp_hash
  0,                                   //  tp_call
  (reprfunc)cmessage::ToStr,           //  tp_str
  cmessage::GetAttr,                   //  tp_getattro
  cmessage::SetAttr,                   //  tp_setattro
  0,                                   //  tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  //  tp_flags
  "A ProtocolMessage",                 //  tp_doc
  0,                                   //  tp_traverse
  0,                                   //  tp_clear
  (richcmpfunc)cmessage::RichCompare,  //  tp_richcompare
  offsetof(CMessage, weakreflist),     //  tp_weaklistoffset
  0,                                   //  tp_iter
  0,                                   //  tp_iternext
  cmessage::Methods,                   //  tp_methods
  0,                                   //  tp_members
  cmessage::Getters,                   //  tp_getset
  0,                                   //  tp_base
  0,                                   //  tp_dict
  0,                                   //  tp_descr_get
  0,                                   //  tp_descr_set
  0,                                   //  tp_dictoffset
  (initproc)cmessage::Init,            //  tp_init
  0,                                   //  tp_alloc
  cmessage::New,                       //  tp_new
};

// --- Exposing the C proto living inside Python proto to C code:

const Message* (*GetCProtoInsidePyProtoPtr)(PyObject* msg);
Message* (*MutableCProtoInsidePyProtoPtr)(PyObject* msg);

static const Message* GetCProtoInsidePyProtoImpl(PyObject* msg) {
  if (!PyObject_TypeCheck(msg, &CMessage_Type)) {
    return NULL;
  }
  CMessage* cmsg = reinterpret_cast<CMessage*>(msg);
  return cmsg->message;
}

static Message* MutableCProtoInsidePyProtoImpl(PyObject* msg) {
  if (!PyObject_TypeCheck(msg, &CMessage_Type)) {
    return NULL;
  }
  CMessage* cmsg = reinterpret_cast<CMessage*>(msg);
  if ((cmsg->composite_fields && PyDict_Size(cmsg->composite_fields) != 0) ||
      (cmsg->extensions != NULL &&
       PyDict_Size(cmsg->extensions->values) != 0)) {
    // There is currently no way of accurately syncing arbitrary changes to
    // the underlying C++ message back to the CMessage (e.g. removed repeated
    // composite containers). We only allow direct mutation of the underlying
    // C++ message if there is no child data in the CMessage.
    return NULL;
  }
  cmessage::AssureWritable(cmsg);
  return cmsg->message;
}

void InitGlobals() {
  // TODO(gps): Check all return values in this function for NULL and propagate
  // the error (MemoryError) on up to result in an import failure.  These should
  // also be freed and reset to NULL during finalization.
  kDESCRIPTOR = PyString_FromString("DESCRIPTOR");

  PyObject *dummy_obj = PySet_New(NULL);
  kEmptyWeakref = PyWeakref_NewRef(dummy_obj, NULL);
  Py_DECREF(dummy_obj);
}

bool InitProto2MessageModule(PyObject *m) {
  // Initialize types and globals in descriptor.cc
  if (!InitDescriptor()) {
    return false;
  }

  // Initialize types and globals in descriptor_pool.cc
  if (!InitDescriptorPool()) {
    return false;
  }

  // Initialize types and globals in message_factory.cc
  if (!InitMessageFactory()) {
    return false;
  }

  // Initialize constants defined in this file.
  InitGlobals();

  CMessageClass_Type.tp_base = &PyType_Type;
  if (PyType_Ready(&CMessageClass_Type) < 0) {
    return false;
  }
  PyModule_AddObject(m, "MessageMeta",
                     reinterpret_cast<PyObject*>(&CMessageClass_Type));

  if (PyType_Ready(&CMessage_Type) < 0) {
    return false;
  }

  // DESCRIPTOR is set on each protocol buffer message class elsewhere, but set
  // it here as well to document that subclasses need to set it.
  PyDict_SetItem(CMessage_Type.tp_dict, kDESCRIPTOR, Py_None);

  PyModule_AddObject(m, "Message", reinterpret_cast<PyObject*>(&CMessage_Type));

  // Initialize Repeated container types.
  {
    if (PyType_Ready(&RepeatedScalarContainer_Type) < 0) {
      return false;
    }

    PyModule_AddObject(m, "RepeatedScalarContainer",
                       reinterpret_cast<PyObject*>(
                           &RepeatedScalarContainer_Type));

    if (PyType_Ready(&RepeatedCompositeContainer_Type) < 0) {
      return false;
    }

    PyModule_AddObject(
        m, "RepeatedCompositeContainer",
        reinterpret_cast<PyObject*>(
            &RepeatedCompositeContainer_Type));

    // Register them as collections.Sequence
    ScopedPyObjectPtr collections(PyImport_ImportModule("collections"));
    if (collections == NULL) {
      return false;
    }
    ScopedPyObjectPtr mutable_sequence(
        PyObject_GetAttrString(collections.get(), "MutableSequence"));
    if (mutable_sequence == NULL) {
      return false;
    }
    if (ScopedPyObjectPtr(
            PyObject_CallMethod(mutable_sequence.get(), "register", "O",
                                &RepeatedScalarContainer_Type)) == NULL) {
      return false;
    }
    if (ScopedPyObjectPtr(
            PyObject_CallMethod(mutable_sequence.get(), "register", "O",
                                &RepeatedCompositeContainer_Type)) == NULL) {
      return false;
    }
  }

  // Initialize Map container types.
  if (!InitMapContainers()) {
    return false;
  }
  PyModule_AddObject(m, "ScalarMapContainer",
                     reinterpret_cast<PyObject*>(ScalarMapContainer_Type));
  PyModule_AddObject(m, "MessageMapContainer",
                     reinterpret_cast<PyObject*>(MessageMapContainer_Type));
  PyModule_AddObject(m, "MapIterator",
                     reinterpret_cast<PyObject*>(&MapIterator_Type));

  if (PyType_Ready(&ExtensionDict_Type) < 0) {
    return false;
  }
  PyModule_AddObject(
      m, "ExtensionDict",
      reinterpret_cast<PyObject*>(&ExtensionDict_Type));

  // Expose the DescriptorPool used to hold all descriptors added from generated
  // pb2.py files.
  // PyModule_AddObject steals a reference.
  Py_INCREF(GetDefaultDescriptorPool());
  PyModule_AddObject(m, "default_pool",
                     reinterpret_cast<PyObject*>(GetDefaultDescriptorPool()));

  PyModule_AddObject(m, "DescriptorPool", reinterpret_cast<PyObject*>(
      &PyDescriptorPool_Type));

  // This implementation provides full Descriptor types, we advertise it so that
  // descriptor.py can use them in replacement of the Python classes.
  PyModule_AddIntConstant(m, "_USE_C_DESCRIPTORS", 1);

  PyModule_AddObject(m, "Descriptor", reinterpret_cast<PyObject*>(
      &PyMessageDescriptor_Type));
  PyModule_AddObject(m, "FieldDescriptor", reinterpret_cast<PyObject*>(
      &PyFieldDescriptor_Type));
  PyModule_AddObject(m, "EnumDescriptor", reinterpret_cast<PyObject*>(
      &PyEnumDescriptor_Type));
  PyModule_AddObject(m, "EnumValueDescriptor", reinterpret_cast<PyObject*>(
      &PyEnumValueDescriptor_Type));
  PyModule_AddObject(m, "FileDescriptor", reinterpret_cast<PyObject*>(
      &PyFileDescriptor_Type));
  PyModule_AddObject(m, "OneofDescriptor", reinterpret_cast<PyObject*>(
      &PyOneofDescriptor_Type));
  PyModule_AddObject(m, "ServiceDescriptor", reinterpret_cast<PyObject*>(
      &PyServiceDescriptor_Type));
  PyModule_AddObject(m, "MethodDescriptor", reinterpret_cast<PyObject*>(
      &PyMethodDescriptor_Type));

  PyObject* enum_type_wrapper = PyImport_ImportModule(
      "google.protobuf.internal.enum_type_wrapper");
  if (enum_type_wrapper == NULL) {
    return false;
  }
  EnumTypeWrapper_class =
      PyObject_GetAttrString(enum_type_wrapper, "EnumTypeWrapper");
  Py_DECREF(enum_type_wrapper);

  PyObject* message_module = PyImport_ImportModule(
      "google.protobuf.message");
  if (message_module == NULL) {
    return false;
  }
  EncodeError_class = PyObject_GetAttrString(message_module, "EncodeError");
  DecodeError_class = PyObject_GetAttrString(message_module, "DecodeError");
  PythonMessage_class = PyObject_GetAttrString(message_module, "Message");
  Py_DECREF(message_module);

  PyObject* pickle_module = PyImport_ImportModule("pickle");
  if (pickle_module == NULL) {
    return false;
  }
  PickleError_class = PyObject_GetAttrString(pickle_module, "PickleError");
  Py_DECREF(pickle_module);

  // Override {Get,Mutable}CProtoInsidePyProto.
  GetCProtoInsidePyProtoPtr = GetCProtoInsidePyProtoImpl;
  MutableCProtoInsidePyProtoPtr = MutableCProtoInsidePyProtoImpl;

  return true;
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
