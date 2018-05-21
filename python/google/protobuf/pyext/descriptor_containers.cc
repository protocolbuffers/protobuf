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

// Mappings and Sequences of descriptors.
// Used by Descriptor.fields_by_name, EnumDescriptor.values...
//
// They avoid the allocation of a full dictionary or a full list: they simply
// store a pointer to the parent descriptor, use the C++ Descriptor methods (see
// google/protobuf/descriptor.h) to retrieve other descriptors, and create
// Python objects on the fly.
//
// The containers fully conform to abc.Mapping and abc.Sequence, and behave just
// like read-only dictionaries and lists.
//
// Because the interface of C++ Descriptors is quite regular, this file actually
// defines only three types, the exact behavior of a container is controlled by
// a DescriptorContainerDef structure, which contains functions that uses the
// public Descriptor API.
//
// Note: This DescriptorContainerDef is similar to the "virtual methods table"
// that a C++ compiler generates for a class. We have to make it explicit
// because the Python API is based on C, and does not play well with C++
// inheritance.

#include <Python.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/pyext/descriptor_containers.h>
#include <google/protobuf/pyext/descriptor_pool.h>
#include <google/protobuf/pyext/descriptor.h>
#include <google/protobuf/pyext/scoped_pyobject_ptr.h>

#if PY_MAJOR_VERSION >= 3
  #define PyString_FromStringAndSize PyUnicode_FromStringAndSize
  #define PyString_FromFormat PyUnicode_FromFormat
  #define PyInt_FromLong PyLong_FromLong
  #if PY_VERSION_HEX < 0x03030000
    #error "Python 3.0 - 3.2 are not supported."
  #endif
  #define PyString_AsStringAndSize(ob, charpp, sizep) \
    (PyUnicode_Check(ob)? \
       ((*(charpp) = PyUnicode_AsUTF8AndSize(ob, (sizep))) == NULL? -1: 0): \
       PyBytes_AsStringAndSize(ob, (charpp), (sizep)))
#endif

namespace google {
namespace protobuf {
namespace python {

struct PyContainer;

typedef int (*CountMethod)(PyContainer* self);
typedef const void* (*GetByIndexMethod)(PyContainer* self, int index);
typedef const void* (*GetByNameMethod)(PyContainer* self, const string& name);
typedef const void* (*GetByCamelcaseNameMethod)(PyContainer* self,
                                                const string& name);
typedef const void* (*GetByNumberMethod)(PyContainer* self, int index);
typedef PyObject* (*NewObjectFromItemMethod)(const void* descriptor);
typedef const string& (*GetItemNameMethod)(const void* descriptor);
typedef const string& (*GetItemCamelcaseNameMethod)(const void* descriptor);
typedef int (*GetItemNumberMethod)(const void* descriptor);
typedef int (*GetItemIndexMethod)(const void* descriptor);

struct DescriptorContainerDef {
  const char* mapping_name;
  // Returns the number of items in the container.
  CountMethod count_fn;
  // Retrieve item by index (usually the order of declaration in the proto file)
  // Used by sequences, but also iterators. 0 <= index < Count().
  GetByIndexMethod get_by_index_fn;
  // Retrieve item by name (usually a call to some 'FindByName' method).
  // Used by "by_name" mappings.
  GetByNameMethod get_by_name_fn;
  // Retrieve item by camelcase name (usually a call to some
  // 'FindByCamelcaseName' method). Used by "by_camelcase_name" mappings.
  GetByCamelcaseNameMethod get_by_camelcase_name_fn;
  // Retrieve item by declared number (field tag, or enum value).
  // Used by "by_number" mappings.
  GetByNumberMethod get_by_number_fn;
  // Converts a item C++ descriptor to a Python object. Returns a new reference.
  NewObjectFromItemMethod new_object_from_item_fn;
  // Retrieve the name of an item. Used by iterators on "by_name" mappings.
  GetItemNameMethod get_item_name_fn;
  // Retrieve the camelcase name of an item. Used by iterators on
  // "by_camelcase_name" mappings.
  GetItemCamelcaseNameMethod get_item_camelcase_name_fn;
  // Retrieve the number of an item. Used by iterators on "by_number" mappings.
  GetItemNumberMethod get_item_number_fn;
  // Retrieve the index of an item for the container type.
  // Used by "__contains__".
  // If not set, "x in sequence" will do a linear search.
  GetItemIndexMethod get_item_index_fn;
};

struct PyContainer {
  PyObject_HEAD

  // The proto2 descriptor this container belongs to the global DescriptorPool.
  const void* descriptor;

  // A pointer to a static structure with function pointers that control the
  // behavior of the container. Very similar to the table of virtual functions
  // of a C++ class.
  const DescriptorContainerDef* container_def;

  // The kind of container: list, or dict by name or value.
  enum ContainerKind {
    KIND_SEQUENCE,
    KIND_BYNAME,
    KIND_BYCAMELCASENAME,
    KIND_BYNUMBER,
  } kind;
};

struct PyContainerIterator {
  PyObject_HEAD

  // The container we are iterating over. Own a reference.
  PyContainer* container;

  // The current index in the iterator.
  int index;

  // The kind of container: list, or dict by name or value.
  enum IterKind {
    KIND_ITERKEY,
    KIND_ITERVALUE,
    KIND_ITERITEM,
    KIND_ITERVALUE_REVERSED,  // For sequences
  } kind;
};

namespace descriptor {

// Returns the C++ item descriptor for a given Python key.
// When the descriptor is found, return true and set *item.
// When the descriptor is not found, return true, but set *item to NULL.
// On error, returns false with an exception set.
static bool _GetItemByKey(PyContainer* self, PyObject* key, const void** item) {
  switch (self->kind) {
    case PyContainer::KIND_BYNAME:
      {
        char* name;
        Py_ssize_t name_size;
        if (PyString_AsStringAndSize(key, &name, &name_size) < 0) {
          if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            // Not a string, cannot be in the container.
            PyErr_Clear();
            *item = NULL;
            return true;
          }
          return false;
        }
        *item = self->container_def->get_by_name_fn(
            self, string(name, name_size));
        return true;
      }
    case PyContainer::KIND_BYCAMELCASENAME:
      {
        char* camelcase_name;
        Py_ssize_t name_size;
        if (PyString_AsStringAndSize(key, &camelcase_name, &name_size) < 0) {
          if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            // Not a string, cannot be in the container.
            PyErr_Clear();
            *item = NULL;
            return true;
          }
          return false;
        }
        *item = self->container_def->get_by_camelcase_name_fn(
            self, string(camelcase_name, name_size));
        return true;
      }
    case PyContainer::KIND_BYNUMBER:
      {
        Py_ssize_t number = PyNumber_AsSsize_t(key, NULL);
        if (number == -1 && PyErr_Occurred()) {
          if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            // Not a number, cannot be in the container.
            PyErr_Clear();
            *item = NULL;
            return true;
          }
          return false;
        }
        *item = self->container_def->get_by_number_fn(self, number);
        return true;
      }
    default:
      PyErr_SetNone(PyExc_NotImplementedError);
      return false;
  }
}

// Returns the key of the object at the given index.
// Used when iterating over mappings.
static PyObject* _NewKey_ByIndex(PyContainer* self, Py_ssize_t index) {
  const void* item = self->container_def->get_by_index_fn(self, index);
  switch (self->kind) {
    case PyContainer::KIND_BYNAME:
      {
        const string& name(self->container_def->get_item_name_fn(item));
        return PyString_FromStringAndSize(name.c_str(), name.size());
      }
    case PyContainer::KIND_BYCAMELCASENAME:
      {
        const string& name(
            self->container_def->get_item_camelcase_name_fn(item));
        return PyString_FromStringAndSize(name.c_str(), name.size());
      }
    case PyContainer::KIND_BYNUMBER:
      {
        int value = self->container_def->get_item_number_fn(item);
        return PyInt_FromLong(value);
      }
    default:
      PyErr_SetNone(PyExc_NotImplementedError);
      return NULL;
  }
}

// Returns the object at the given index.
// Also used when iterating over mappings.
static PyObject* _NewObj_ByIndex(PyContainer* self, Py_ssize_t index) {
  return self->container_def->new_object_from_item_fn(
      self->container_def->get_by_index_fn(self, index));
}

static Py_ssize_t Length(PyContainer* self) {
  return self->container_def->count_fn(self);
}

// The DescriptorMapping type.

static PyObject* Subscript(PyContainer* self, PyObject* key) {
  const void* item = NULL;
  if (!_GetItemByKey(self, key, &item)) {
    return NULL;
  }
  if (!item) {
    PyErr_SetObject(PyExc_KeyError, key);
    return NULL;
  }
  return self->container_def->new_object_from_item_fn(item);
}

static int AssSubscript(PyContainer* self, PyObject* key, PyObject* value) {
  if (_CalledFromGeneratedFile(0)) {
    return 0;
  }
  PyErr_Format(PyExc_TypeError,
               "'%.200s' object does not support item assignment",
               Py_TYPE(self)->tp_name);
  return -1;
}

static PyMappingMethods MappingMappingMethods = {
  (lenfunc)Length,               // mp_length
  (binaryfunc)Subscript,         // mp_subscript
  (objobjargproc)AssSubscript,   // mp_ass_subscript
};

static int Contains(PyContainer* self, PyObject* key) {
  const void* item = NULL;
  if (!_GetItemByKey(self, key, &item)) {
    return -1;
  }
  if (item) {
    return 1;
  } else {
    return 0;
  }
}

static PyObject* ContainerRepr(PyContainer* self) {
  const char* kind = "";
  switch (self->kind) {
    case PyContainer::KIND_SEQUENCE:
      kind = "sequence";
      break;
    case PyContainer::KIND_BYNAME:
      kind = "mapping by name";
      break;
    case PyContainer::KIND_BYCAMELCASENAME:
      kind = "mapping by camelCase name";
      break;
    case PyContainer::KIND_BYNUMBER:
      kind = "mapping by number";
      break;
  }
  return PyString_FromFormat(
      "<%s %s>", self->container_def->mapping_name, kind);
}

extern PyTypeObject DescriptorMapping_Type;
extern PyTypeObject DescriptorSequence_Type;

// A sequence container can only be equal to another sequence container, or (for
// backward compatibility) to a list containing the same items.
// Returns 1 if equal, 0 if unequal, -1 on error.
static int DescriptorSequence_Equal(PyContainer* self, PyObject* other) {
  // Check the identity of C++ pointers.
  if (PyObject_TypeCheck(other, &DescriptorSequence_Type)) {
    PyContainer* other_container = reinterpret_cast<PyContainer*>(other);
    if (self->descriptor == other_container->descriptor &&
        self->container_def == other_container->container_def &&
        self->kind == other_container->kind) {
      return 1;
    } else {
      return 0;
    }
  }

  // If other is a list
  if (PyList_Check(other)) {
    // return list(self) == other
    int size = Length(self);
    if (size != PyList_Size(other)) {
      return false;
    }
    for (int index = 0; index < size; index++) {
      ScopedPyObjectPtr value1(_NewObj_ByIndex(self, index));
      if (value1 == NULL) {
        return -1;
      }
      PyObject* value2 = PyList_GetItem(other, index);
      if (value2 == NULL) {
        return -1;
      }
      int cmp = PyObject_RichCompareBool(value1.get(), value2, Py_EQ);
      if (cmp != 1)  // error or not equal
          return cmp;
    }
    // All items were found and equal
    return 1;
  }

  // Any other object is different.
  return 0;
}

// A mapping container can only be equal to another mapping container, or (for
// backward compatibility) to a dict containing the same items.
// Returns 1 if equal, 0 if unequal, -1 on error.
static int DescriptorMapping_Equal(PyContainer* self, PyObject* other) {
  // Check the identity of C++ pointers.
  if (PyObject_TypeCheck(other, &DescriptorMapping_Type)) {
    PyContainer* other_container = reinterpret_cast<PyContainer*>(other);
    if (self->descriptor == other_container->descriptor &&
        self->container_def == other_container->container_def &&
        self->kind == other_container->kind) {
      return 1;
    } else {
      return 0;
    }
  }

  // If other is a dict
  if (PyDict_Check(other)) {
    // equivalent to dict(self.items()) == other
    int size = Length(self);
    if (size != PyDict_Size(other)) {
      return false;
    }
    for (int index = 0; index < size; index++) {
      ScopedPyObjectPtr key(_NewKey_ByIndex(self, index));
      if (key == NULL) {
        return -1;
      }
      ScopedPyObjectPtr value1(_NewObj_ByIndex(self, index));
      if (value1 == NULL) {
        return -1;
      }
      PyObject* value2 = PyDict_GetItem(other, key.get());
      if (value2 == NULL) {
        // Not found in the other dictionary
        return 0;
      }
      int cmp = PyObject_RichCompareBool(value1.get(), value2, Py_EQ);
      if (cmp != 1)  // error or not equal
          return cmp;
    }
    // All items were found and equal
    return 1;
  }

  // Any other object is different.
  return 0;
}

static PyObject* RichCompare(PyContainer* self, PyObject* other, int opid) {
  if (opid != Py_EQ && opid != Py_NE) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  int result;

  if (self->kind == PyContainer::KIND_SEQUENCE) {
    result = DescriptorSequence_Equal(self, other);
  } else {
    result = DescriptorMapping_Equal(self, other);
  }
  if (result < 0) {
    return NULL;
  }
  if (result ^ (opid == Py_NE)) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

static PySequenceMethods MappingSequenceMethods = {
    0,                      // sq_length
    0,                      // sq_concat
    0,                      // sq_repeat
    0,                      // sq_item
    0,                      // sq_slice
    0,                      // sq_ass_item
    0,                      // sq_ass_slice
    (objobjproc)Contains,   // sq_contains
};

static PyObject* Get(PyContainer* self, PyObject* args) {
  PyObject* key;
  PyObject* default_value = Py_None;
  if (!PyArg_UnpackTuple(args, "get", 1, 2, &key, &default_value)) {
    return NULL;
  }

  const void* item;
  if (!_GetItemByKey(self, key, &item)) {
    return NULL;
  }
  if (item == NULL) {
    Py_INCREF(default_value);
    return default_value;
  }
  return self->container_def->new_object_from_item_fn(item);
}

static PyObject* Keys(PyContainer* self, PyObject* args) {
  Py_ssize_t count = Length(self);
  ScopedPyObjectPtr list(PyList_New(count));
  if (list == NULL) {
    return NULL;
  }
  for (Py_ssize_t index = 0; index < count; ++index) {
    PyObject* key = _NewKey_ByIndex(self, index);
    if (key == NULL) {
      return NULL;
    }
    PyList_SET_ITEM(list.get(), index, key);
  }
  return list.release();
}

static PyObject* Values(PyContainer* self, PyObject* args) {
  Py_ssize_t count = Length(self);
  ScopedPyObjectPtr list(PyList_New(count));
  if (list == NULL) {
    return NULL;
  }
  for (Py_ssize_t index = 0; index < count; ++index) {
    PyObject* value = _NewObj_ByIndex(self, index);
    if (value == NULL) {
      return NULL;
    }
    PyList_SET_ITEM(list.get(), index, value);
  }
  return list.release();
}

static PyObject* Items(PyContainer* self, PyObject* args) {
  Py_ssize_t count = Length(self);
  ScopedPyObjectPtr list(PyList_New(count));
  if (list == NULL) {
    return NULL;
  }
  for (Py_ssize_t index = 0; index < count; ++index) {
    ScopedPyObjectPtr obj(PyTuple_New(2));
    if (obj == NULL) {
      return NULL;
    }
    PyObject* key = _NewKey_ByIndex(self, index);
    if (key == NULL) {
      return NULL;
    }
    PyTuple_SET_ITEM(obj.get(), 0, key);
    PyObject* value = _NewObj_ByIndex(self, index);
    if (value == NULL) {
      return NULL;
    }
    PyTuple_SET_ITEM(obj.get(), 1, value);
    PyList_SET_ITEM(list.get(), index, obj.release());
  }
  return list.release();
}

static PyObject* NewContainerIterator(PyContainer* mapping,
                                      PyContainerIterator::IterKind kind);

static PyObject* Iter(PyContainer* self) {
  return NewContainerIterator(self, PyContainerIterator::KIND_ITERKEY);
}
static PyObject* IterKeys(PyContainer* self, PyObject* args) {
  return NewContainerIterator(self, PyContainerIterator::KIND_ITERKEY);
}
static PyObject* IterValues(PyContainer* self, PyObject* args) {
  return NewContainerIterator(self, PyContainerIterator::KIND_ITERVALUE);
}
static PyObject* IterItems(PyContainer* self, PyObject* args) {
  return NewContainerIterator(self, PyContainerIterator::KIND_ITERITEM);
}

static PyMethodDef MappingMethods[] = {
  { "get", (PyCFunction)Get, METH_VARARGS, },
  { "keys", (PyCFunction)Keys, METH_NOARGS, },
  { "values", (PyCFunction)Values, METH_NOARGS, },
  { "items", (PyCFunction)Items, METH_NOARGS, },
  { "iterkeys", (PyCFunction)IterKeys, METH_NOARGS, },
  { "itervalues", (PyCFunction)IterValues, METH_NOARGS, },
  { "iteritems", (PyCFunction)IterItems, METH_NOARGS, },
  {NULL}
};

PyTypeObject DescriptorMapping_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "DescriptorMapping",                  // tp_name
  sizeof(PyContainer),                  // tp_basicsize
  0,                                    // tp_itemsize
  0,                                    // tp_dealloc
  0,                                    // tp_print
  0,                                    // tp_getattr
  0,                                    // tp_setattr
  0,                                    // tp_compare
  (reprfunc)ContainerRepr,              // tp_repr
  0,                                    // tp_as_number
  &MappingSequenceMethods,              // tp_as_sequence
  &MappingMappingMethods,               // tp_as_mapping
  0,                                    // tp_hash
  0,                                    // tp_call
  0,                                    // tp_str
  0,                                    // tp_getattro
  0,                                    // tp_setattro
  0,                                    // tp_as_buffer
  Py_TPFLAGS_DEFAULT,                   // tp_flags
  0,                                    // tp_doc
  0,                                    // tp_traverse
  0,                                    // tp_clear
  (richcmpfunc)RichCompare,             // tp_richcompare
  0,                                    // tp_weaklistoffset
  (getiterfunc)Iter,                    // tp_iter
  0,                                    // tp_iternext
  MappingMethods,                       // tp_methods
  0,                                    // tp_members
  0,                                    // tp_getset
  0,                                    // tp_base
  0,                                    // tp_dict
  0,                                    // tp_descr_get
  0,                                    // tp_descr_set
  0,                                    // tp_dictoffset
  0,                                    // tp_init
  0,                                    // tp_alloc
  0,                                    // tp_new
  0,                                    // tp_free
};

// The DescriptorSequence type.

static PyObject* GetItem(PyContainer* self, Py_ssize_t index) {
  if (index < 0) {
    index += Length(self);
  }
  if (index < 0 || index >= Length(self)) {
    PyErr_SetString(PyExc_IndexError, "index out of range");
    return NULL;
  }
  return _NewObj_ByIndex(self, index);
}

static PyObject *
SeqSubscript(PyContainer* self, PyObject* item) {
  if (PyIndex_Check(item)) {
      Py_ssize_t index;
      index = PyNumber_AsSsize_t(item, PyExc_IndexError);
      if (index == -1 && PyErr_Occurred())
          return NULL;
      return GetItem(self, index);
  }
  // Materialize the list and delegate the operation to it.
  ScopedPyObjectPtr list(PyObject_CallFunctionObjArgs(
      reinterpret_cast<PyObject*>(&PyList_Type), self, NULL));
  if (list == NULL) {
    return NULL;
  }
  return Py_TYPE(list.get())->tp_as_mapping->mp_subscript(list.get(), item);
}

// Returns the position of the item in the sequence, of -1 if not found.
// This function never fails.
int Find(PyContainer* self, PyObject* item) {
  // The item can only be in one position: item.index.
  // Check that self[item.index] == item, it's faster than a linear search.
  //
  // This assumes that sequences are only defined by syntax of the .proto file:
  // a specific item belongs to only one sequence, depending on its position in
  // the .proto file definition.
  const void* descriptor_ptr = PyDescriptor_AsVoidPtr(item);
  if (descriptor_ptr == NULL) {
    // Not a descriptor, it cannot be in the list.
    return -1;
  }
  if (self->container_def->get_item_index_fn) {
    int index = self->container_def->get_item_index_fn(descriptor_ptr);
    if (index < 0 || index >= Length(self)) {
      // This index is not from this collection.
      return -1;
    }
    if (self->container_def->get_by_index_fn(self, index) != descriptor_ptr) {
      // The descriptor at this index is not the same.
      return -1;
    }
    // self[item.index] == item, so return the index.
    return index;
  } else {
    // Fall back to linear search.
    int length = Length(self);
    for (int index=0; index < length; index++) {
      if (self->container_def->get_by_index_fn(self, index) == descriptor_ptr) {
        return index;
      }
    }
    // Not found
    return -1;
  }
}

// Implements list.index(): the position of the item is in the sequence.
static PyObject* Index(PyContainer* self, PyObject* item) {
  int position = Find(self, item);
  if (position < 0) {
    // Not found
    PyErr_SetNone(PyExc_ValueError);
    return NULL;
  } else {
    return PyInt_FromLong(position);
  }
}
// Implements "list.__contains__()": is the object in the sequence.
static int SeqContains(PyContainer* self, PyObject* item) {
  int position = Find(self, item);
  if (position < 0) {
    return 0;
  } else {
    return 1;
  }
}

// Implements list.count(): number of occurrences of the item in the sequence.
// An item can only appear once in a sequence. If it exists, return 1.
static PyObject* Count(PyContainer* self, PyObject* item) {
  int position = Find(self, item);
  if (position < 0) {
    return PyInt_FromLong(0);
  } else {
    return PyInt_FromLong(1);
  }
}

static PyObject* Append(PyContainer* self, PyObject* args) {
  if (_CalledFromGeneratedFile(0)) {
    Py_RETURN_NONE;
  }
  PyErr_Format(PyExc_TypeError,
               "'%.200s' object is not a mutable sequence",
               Py_TYPE(self)->tp_name);
  return NULL;
}

static PyObject* Reversed(PyContainer* self, PyObject* args) {
  return NewContainerIterator(self,
                              PyContainerIterator::KIND_ITERVALUE_REVERSED);
}

static PyMethodDef SeqMethods[] = {
  { "index", (PyCFunction)Index, METH_O, },
  { "count", (PyCFunction)Count, METH_O, },
  { "append", (PyCFunction)Append, METH_O, },
  { "__reversed__", (PyCFunction)Reversed, METH_NOARGS, },
  {NULL}
};

static PySequenceMethods SeqSequenceMethods = {
  (lenfunc)Length,          // sq_length
  0,                        // sq_concat
  0,                        // sq_repeat
  (ssizeargfunc)GetItem,    // sq_item
  0,                        // sq_slice
  0,                        // sq_ass_item
  0,                        // sq_ass_slice
  (objobjproc)SeqContains,  // sq_contains
};

static PyMappingMethods SeqMappingMethods = {
  (lenfunc)Length,           // mp_length
  (binaryfunc)SeqSubscript,  // mp_subscript
  0,                         // mp_ass_subscript
};

PyTypeObject DescriptorSequence_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "DescriptorSequence",                 // tp_name
  sizeof(PyContainer),                  // tp_basicsize
  0,                                    // tp_itemsize
  0,                                    // tp_dealloc
  0,                                    // tp_print
  0,                                    // tp_getattr
  0,                                    // tp_setattr
  0,                                    // tp_compare
  (reprfunc)ContainerRepr,              // tp_repr
  0,                                    // tp_as_number
  &SeqSequenceMethods,                  // tp_as_sequence
  &SeqMappingMethods,                   // tp_as_mapping
  0,                                    // tp_hash
  0,                                    // tp_call
  0,                                    // tp_str
  0,                                    // tp_getattro
  0,                                    // tp_setattro
  0,                                    // tp_as_buffer
  Py_TPFLAGS_DEFAULT,                   // tp_flags
  0,                                    // tp_doc
  0,                                    // tp_traverse
  0,                                    // tp_clear
  (richcmpfunc)RichCompare,             // tp_richcompare
  0,                                    // tp_weaklistoffset
  0,                                    // tp_iter
  0,                                    // tp_iternext
  SeqMethods,                           // tp_methods
  0,                                    // tp_members
  0,                                    // tp_getset
  0,                                    // tp_base
  0,                                    // tp_dict
  0,                                    // tp_descr_get
  0,                                    // tp_descr_set
  0,                                    // tp_dictoffset
  0,                                    // tp_init
  0,                                    // tp_alloc
  0,                                    // tp_new
  0,                                    // tp_free
};

static PyObject* NewMappingByName(
    DescriptorContainerDef* container_def, const void* descriptor) {
  PyContainer* self = PyObject_New(PyContainer, &DescriptorMapping_Type);
  if (self == NULL) {
    return NULL;
  }
  self->descriptor = descriptor;
  self->container_def = container_def;
  self->kind = PyContainer::KIND_BYNAME;
  return reinterpret_cast<PyObject*>(self);
}

static PyObject* NewMappingByCamelcaseName(
    DescriptorContainerDef* container_def, const void* descriptor) {
  PyContainer* self = PyObject_New(PyContainer, &DescriptorMapping_Type);
  if (self == NULL) {
    return NULL;
  }
  self->descriptor = descriptor;
  self->container_def = container_def;
  self->kind = PyContainer::KIND_BYCAMELCASENAME;
  return reinterpret_cast<PyObject*>(self);
}

static PyObject* NewMappingByNumber(
    DescriptorContainerDef* container_def, const void* descriptor) {
  if (container_def->get_by_number_fn == NULL ||
      container_def->get_item_number_fn == NULL) {
    PyErr_SetNone(PyExc_NotImplementedError);
    return NULL;
  }
  PyContainer* self = PyObject_New(PyContainer, &DescriptorMapping_Type);
  if (self == NULL) {
    return NULL;
  }
  self->descriptor = descriptor;
  self->container_def = container_def;
  self->kind = PyContainer::KIND_BYNUMBER;
  return reinterpret_cast<PyObject*>(self);
}

static PyObject* NewSequence(
    DescriptorContainerDef* container_def, const void* descriptor) {
  PyContainer* self = PyObject_New(PyContainer, &DescriptorSequence_Type);
  if (self == NULL) {
    return NULL;
  }
  self->descriptor = descriptor;
  self->container_def = container_def;
  self->kind = PyContainer::KIND_SEQUENCE;
  return reinterpret_cast<PyObject*>(self);
}

// Implement iterators over PyContainers.

static void Iterator_Dealloc(PyContainerIterator* self) {
  Py_CLEAR(self->container);
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

static PyObject* Iterator_Next(PyContainerIterator* self) {
  int count = self->container->container_def->count_fn(self->container);
  if (self->index >= count) {
    // Return NULL with no exception to indicate the end.
    return NULL;
  }
  int index = self->index;
  self->index += 1;
  switch (self->kind) {
    case PyContainerIterator::KIND_ITERKEY:
      return _NewKey_ByIndex(self->container, index);
    case PyContainerIterator::KIND_ITERVALUE:
      return _NewObj_ByIndex(self->container, index);
    case PyContainerIterator::KIND_ITERVALUE_REVERSED:
      return _NewObj_ByIndex(self->container, count - index - 1);
    case PyContainerIterator::KIND_ITERITEM:
      {
        PyObject* obj = PyTuple_New(2);
        if (obj == NULL) {
          return NULL;
        }
        PyObject* key = _NewKey_ByIndex(self->container, index);
        if (key == NULL) {
          Py_DECREF(obj);
          return NULL;
        }
        PyTuple_SET_ITEM(obj, 0, key);
        PyObject* value = _NewObj_ByIndex(self->container, index);
        if (value == NULL) {
          Py_DECREF(obj);
          return NULL;
        }
        PyTuple_SET_ITEM(obj, 1, value);
        return obj;
      }
    default:
      PyErr_SetNone(PyExc_NotImplementedError);
      return NULL;
  }
}

static PyTypeObject ContainerIterator_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "DescriptorContainerIterator",        // tp_name
  sizeof(PyContainerIterator),          // tp_basicsize
  0,                                    // tp_itemsize
  (destructor)Iterator_Dealloc,         // tp_dealloc
  0,                                    // tp_print
  0,                                    // tp_getattr
  0,                                    // tp_setattr
  0,                                    // tp_compare
  0,                                    // tp_repr
  0,                                    // tp_as_number
  0,                                    // tp_as_sequence
  0,                                    // tp_as_mapping
  0,                                    // tp_hash
  0,                                    // tp_call
  0,                                    // tp_str
  0,                                    // tp_getattro
  0,                                    // tp_setattro
  0,                                    // tp_as_buffer
  Py_TPFLAGS_DEFAULT,                   // tp_flags
  0,                                    // tp_doc
  0,                                    // tp_traverse
  0,                                    // tp_clear
  0,                                    // tp_richcompare
  0,                                    // tp_weaklistoffset
  PyObject_SelfIter,                    // tp_iter
  (iternextfunc)Iterator_Next,          // tp_iternext
  0,                                    // tp_methods
  0,                                    // tp_members
  0,                                    // tp_getset
  0,                                    // tp_base
  0,                                    // tp_dict
  0,                                    // tp_descr_get
  0,                                    // tp_descr_set
  0,                                    // tp_dictoffset
  0,                                    // tp_init
  0,                                    // tp_alloc
  0,                                    // tp_new
  0,                                    // tp_free
};

static PyObject* NewContainerIterator(PyContainer* container,
                                      PyContainerIterator::IterKind kind) {
  PyContainerIterator* self = PyObject_New(PyContainerIterator,
                                           &ContainerIterator_Type);
  if (self == NULL) {
    return NULL;
  }
  Py_INCREF(container);
  self->container = container;
  self->kind = kind;
  self->index = 0;

  return reinterpret_cast<PyObject*>(self);
}

}  // namespace descriptor

// Now define the real collections!

namespace message_descriptor {

typedef const Descriptor* ParentDescriptor;

static ParentDescriptor GetDescriptor(PyContainer* self) {
  return reinterpret_cast<ParentDescriptor>(self->descriptor);
}

namespace fields {

typedef const FieldDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->field_count();
}

static const void* GetByName(PyContainer* self, const string& name) {
  return GetDescriptor(self)->FindFieldByName(name);
}

static const void* GetByCamelcaseName(PyContainer* self,
                                         const string& name) {
  return GetDescriptor(self)->FindFieldByCamelcaseName(name);
}

static const void* GetByNumber(PyContainer* self, int number) {
  return GetDescriptor(self)->FindFieldByNumber(number);
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->field(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyFieldDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static const string& GetItemName(const void* item) {
  return static_cast<ItemDescriptor>(item)->name();
}

static const string& GetItemCamelcaseName(const void* item) {
  return static_cast<ItemDescriptor>(item)->camelcase_name();
}

static int GetItemNumber(const void* item) {
  return static_cast<ItemDescriptor>(item)->number();
}

static int GetItemIndex(const void* item) {
  return static_cast<ItemDescriptor>(item)->index();
}

static DescriptorContainerDef ContainerDef = {
  "MessageFields",
  Count,
  GetByIndex,
  GetByName,
  GetByCamelcaseName,
  GetByNumber,
  NewObjectFromItem,
  GetItemName,
  GetItemCamelcaseName,
  GetItemNumber,
  GetItemIndex,
};

}  // namespace fields

PyObject* NewMessageFieldsByName(ParentDescriptor descriptor) {
  return descriptor::NewMappingByName(&fields::ContainerDef, descriptor);
}

PyObject* NewMessageFieldsByCamelcaseName(ParentDescriptor descriptor) {
  return descriptor::NewMappingByCamelcaseName(&fields::ContainerDef,
                                               descriptor);
}

PyObject* NewMessageFieldsByNumber(ParentDescriptor descriptor) {
  return descriptor::NewMappingByNumber(&fields::ContainerDef, descriptor);
}

PyObject* NewMessageFieldsSeq(ParentDescriptor descriptor) {
  return descriptor::NewSequence(&fields::ContainerDef, descriptor);
}

namespace nested_types {

typedef const Descriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->nested_type_count();
}

static const void* GetByName(PyContainer* self, const string& name) {
  return GetDescriptor(self)->FindNestedTypeByName(name);
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->nested_type(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyMessageDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static const string& GetItemName(const void* item) {
  return static_cast<ItemDescriptor>(item)->name();
}

static int GetItemIndex(const void* item) {
  return static_cast<ItemDescriptor>(item)->index();
}

static DescriptorContainerDef ContainerDef = {
  "MessageNestedTypes",
  Count,
  GetByIndex,
  GetByName,
  NULL,
  NULL,
  NewObjectFromItem,
  GetItemName,
  NULL,
  NULL,
  GetItemIndex,
};

}  // namespace nested_types

PyObject* NewMessageNestedTypesSeq(ParentDescriptor descriptor) {
  return descriptor::NewSequence(&nested_types::ContainerDef, descriptor);
}

PyObject* NewMessageNestedTypesByName(ParentDescriptor descriptor) {
  return descriptor::NewMappingByName(&nested_types::ContainerDef, descriptor);
}

namespace enums {

typedef const EnumDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->enum_type_count();
}

static const void* GetByName(PyContainer* self, const string& name) {
  return GetDescriptor(self)->FindEnumTypeByName(name);
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->enum_type(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyEnumDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static const string& GetItemName(const void* item) {
  return static_cast<ItemDescriptor>(item)->name();
}

static int GetItemIndex(const void* item) {
  return static_cast<ItemDescriptor>(item)->index();
}

static DescriptorContainerDef ContainerDef = {
  "MessageNestedEnums",
  Count,
  GetByIndex,
  GetByName,
  NULL,
  NULL,
  NewObjectFromItem,
  GetItemName,
  NULL,
  NULL,
  GetItemIndex,
};

}  // namespace enums

PyObject* NewMessageEnumsByName(ParentDescriptor descriptor) {
  return descriptor::NewMappingByName(&enums::ContainerDef, descriptor);
}

PyObject* NewMessageEnumsSeq(ParentDescriptor descriptor) {
  return descriptor::NewSequence(&enums::ContainerDef, descriptor);
}

namespace enumvalues {

// This is the "enum_values_by_name" mapping, which collects values from all
// enum types in a message.
//
// Note that the behavior of the C++ descriptor is different: it will search and
// return the first value that matches the name, whereas the Python
// implementation retrieves the last one.

typedef const EnumValueDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  int count = 0;
  for (int i = 0; i < GetDescriptor(self)->enum_type_count(); ++i) {
    count += GetDescriptor(self)->enum_type(i)->value_count();
  }
  return count;
}

static const void* GetByName(PyContainer* self, const string& name) {
  return GetDescriptor(self)->FindEnumValueByName(name);
}

static const void* GetByIndex(PyContainer* self, int index) {
  // This is not optimal, but the number of enums *types* in a given message
  // is small.  This function is only used when iterating over the mapping.
  const EnumDescriptor* enum_type = NULL;
  int enum_type_count = GetDescriptor(self)->enum_type_count();
  for (int i = 0; i < enum_type_count; ++i) {
    enum_type = GetDescriptor(self)->enum_type(i);
    int enum_value_count = enum_type->value_count();
    if (index < enum_value_count) {
      // Found it!
      break;
    }
    index -= enum_value_count;
  }
  // The next statement cannot overflow, because this function is only called by
  // internal iterators which ensure that 0 <= index < Count().
  return enum_type->value(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyEnumValueDescriptor_FromDescriptor(
      static_cast<ItemDescriptor>(item));
}

static const string& GetItemName(const void* item) {
  return static_cast<ItemDescriptor>(item)->name();
}

static DescriptorContainerDef ContainerDef = {
  "MessageEnumValues",
  Count,
  GetByIndex,
  GetByName,
  NULL,
  NULL,
  NewObjectFromItem,
  GetItemName,
  NULL,
  NULL,
  NULL,
};

}  // namespace enumvalues

PyObject* NewMessageEnumValuesByName(ParentDescriptor descriptor) {
  return descriptor::NewMappingByName(&enumvalues::ContainerDef, descriptor);
}

namespace extensions {

typedef const FieldDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->extension_count();
}

static const void* GetByName(PyContainer* self, const string& name) {
  return GetDescriptor(self)->FindExtensionByName(name);
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->extension(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyFieldDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static const string& GetItemName(const void* item) {
  return static_cast<ItemDescriptor>(item)->name();
}

static int GetItemIndex(const void* item) {
  return static_cast<ItemDescriptor>(item)->index();
}

static DescriptorContainerDef ContainerDef = {
  "MessageExtensions",
  Count,
  GetByIndex,
  GetByName,
  NULL,
  NULL,
  NewObjectFromItem,
  GetItemName,
  NULL,
  NULL,
  GetItemIndex,
};

}  // namespace extensions

PyObject* NewMessageExtensionsByName(ParentDescriptor descriptor) {
  return descriptor::NewMappingByName(&extensions::ContainerDef, descriptor);
}

PyObject* NewMessageExtensionsSeq(ParentDescriptor descriptor) {
  return descriptor::NewSequence(&extensions::ContainerDef, descriptor);
}

namespace oneofs {

typedef const OneofDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->oneof_decl_count();
}

static const void* GetByName(PyContainer* self, const string& name) {
  return GetDescriptor(self)->FindOneofByName(name);
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->oneof_decl(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyOneofDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static const string& GetItemName(const void* item) {
  return static_cast<ItemDescriptor>(item)->name();
}

static int GetItemIndex(const void* item) {
  return static_cast<ItemDescriptor>(item)->index();
}

static DescriptorContainerDef ContainerDef = {
  "MessageOneofs",
  Count,
  GetByIndex,
  GetByName,
  NULL,
  NULL,
  NewObjectFromItem,
  GetItemName,
  NULL,
  NULL,
  GetItemIndex,
};

}  // namespace oneofs

PyObject* NewMessageOneofsByName(ParentDescriptor descriptor) {
  return descriptor::NewMappingByName(&oneofs::ContainerDef, descriptor);
}

PyObject* NewMessageOneofsSeq(ParentDescriptor descriptor) {
  return descriptor::NewSequence(&oneofs::ContainerDef, descriptor);
}

}  // namespace message_descriptor

namespace enum_descriptor {

typedef const EnumDescriptor* ParentDescriptor;

static ParentDescriptor GetDescriptor(PyContainer* self) {
  return reinterpret_cast<ParentDescriptor>(self->descriptor);
}

namespace enumvalues {

typedef const EnumValueDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->value_count();
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->value(index);
}

static const void* GetByName(PyContainer* self, const string& name) {
  return GetDescriptor(self)->FindValueByName(name);
}

static const void* GetByNumber(PyContainer* self, int number) {
  return GetDescriptor(self)->FindValueByNumber(number);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyEnumValueDescriptor_FromDescriptor(
      static_cast<ItemDescriptor>(item));
}

static const string& GetItemName(const void* item) {
  return static_cast<ItemDescriptor>(item)->name();
}

static int GetItemNumber(const void* item) {
  return static_cast<ItemDescriptor>(item)->number();
}

static int GetItemIndex(const void* item) {
  return static_cast<ItemDescriptor>(item)->index();
}

static DescriptorContainerDef ContainerDef = {
  "EnumValues",
  Count,
  GetByIndex,
  GetByName,
  NULL,
  GetByNumber,
  NewObjectFromItem,
  GetItemName,
  NULL,
  GetItemNumber,
  GetItemIndex,
};

}  // namespace enumvalues

PyObject* NewEnumValuesByName(ParentDescriptor descriptor) {
  return descriptor::NewMappingByName(&enumvalues::ContainerDef, descriptor);
}

PyObject* NewEnumValuesByNumber(ParentDescriptor descriptor) {
  return descriptor::NewMappingByNumber(&enumvalues::ContainerDef, descriptor);
}

PyObject* NewEnumValuesSeq(ParentDescriptor descriptor) {
  return descriptor::NewSequence(&enumvalues::ContainerDef, descriptor);
}

}  // namespace enum_descriptor

namespace oneof_descriptor {

typedef const OneofDescriptor* ParentDescriptor;

static ParentDescriptor GetDescriptor(PyContainer* self) {
  return reinterpret_cast<ParentDescriptor>(self->descriptor);
}

namespace fields {

typedef const FieldDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->field_count();
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->field(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyFieldDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static int GetItemIndex(const void* item) {
  return static_cast<ItemDescriptor>(item)->index_in_oneof();
}

static DescriptorContainerDef ContainerDef = {
  "OneofFields",
  Count,
  GetByIndex,
  NULL,
  NULL,
  NULL,
  NewObjectFromItem,
  NULL,
  NULL,
  NULL,
  GetItemIndex,
};

}  // namespace fields

PyObject* NewOneofFieldsSeq(ParentDescriptor descriptor) {
  return descriptor::NewSequence(&fields::ContainerDef, descriptor);
}

}  // namespace oneof_descriptor

namespace service_descriptor {

typedef const ServiceDescriptor* ParentDescriptor;

static ParentDescriptor GetDescriptor(PyContainer* self) {
  return reinterpret_cast<ParentDescriptor>(self->descriptor);
}

namespace methods {

typedef const MethodDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->method_count();
}

static const void* GetByName(PyContainer* self, const string& name) {
  return GetDescriptor(self)->FindMethodByName(name);
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->method(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyMethodDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static const string& GetItemName(const void* item) {
  return static_cast<ItemDescriptor>(item)->name();
}

static int GetItemIndex(const void* item) {
  return static_cast<ItemDescriptor>(item)->index();
}

static DescriptorContainerDef ContainerDef = {
  "ServiceMethods",
  Count,
  GetByIndex,
  GetByName,
  NULL,
  NULL,
  NewObjectFromItem,
  GetItemName,
  NULL,
  NULL,
  GetItemIndex,
};

}  // namespace methods

PyObject* NewServiceMethodsSeq(ParentDescriptor descriptor) {
  return descriptor::NewSequence(&methods::ContainerDef, descriptor);
}

PyObject* NewServiceMethodsByName(ParentDescriptor descriptor) {
  return descriptor::NewMappingByName(&methods::ContainerDef, descriptor);
}

}  // namespace service_descriptor

namespace file_descriptor {

typedef const FileDescriptor* ParentDescriptor;

static ParentDescriptor GetDescriptor(PyContainer* self) {
  return reinterpret_cast<ParentDescriptor>(self->descriptor);
}

namespace messages {

typedef const Descriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->message_type_count();
}

static const void* GetByName(PyContainer* self, const string& name) {
  return GetDescriptor(self)->FindMessageTypeByName(name);
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->message_type(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyMessageDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static const string& GetItemName(const void* item) {
  return static_cast<ItemDescriptor>(item)->name();
}

static int GetItemIndex(const void* item) {
  return static_cast<ItemDescriptor>(item)->index();
}

static DescriptorContainerDef ContainerDef = {
  "FileMessages",
  Count,
  GetByIndex,
  GetByName,
  NULL,
  NULL,
  NewObjectFromItem,
  GetItemName,
  NULL,
  NULL,
  GetItemIndex,
};

}  // namespace messages

PyObject* NewFileMessageTypesByName(ParentDescriptor descriptor) {
  return descriptor::NewMappingByName(&messages::ContainerDef, descriptor);
}

namespace enums {

typedef const EnumDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->enum_type_count();
}

static const void* GetByName(PyContainer* self, const string& name) {
  return GetDescriptor(self)->FindEnumTypeByName(name);
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->enum_type(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyEnumDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static const string& GetItemName(const void* item) {
  return static_cast<ItemDescriptor>(item)->name();
}

static int GetItemIndex(const void* item) {
  return static_cast<ItemDescriptor>(item)->index();
}

static DescriptorContainerDef ContainerDef = {
  "FileEnums",
  Count,
  GetByIndex,
  GetByName,
  NULL,
  NULL,
  NewObjectFromItem,
  GetItemName,
  NULL,
  NULL,
  GetItemIndex,
};

}  // namespace enums

PyObject* NewFileEnumTypesByName(ParentDescriptor descriptor) {
  return descriptor::NewMappingByName(&enums::ContainerDef, descriptor);
}

namespace extensions {

typedef const FieldDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->extension_count();
}

static const void* GetByName(PyContainer* self, const string& name) {
  return GetDescriptor(self)->FindExtensionByName(name);
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->extension(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyFieldDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static const string& GetItemName(const void* item) {
  return static_cast<ItemDescriptor>(item)->name();
}

static int GetItemIndex(const void* item) {
  return static_cast<ItemDescriptor>(item)->index();
}

static DescriptorContainerDef ContainerDef = {
  "FileExtensions",
  Count,
  GetByIndex,
  GetByName,
  NULL,
  NULL,
  NewObjectFromItem,
  GetItemName,
  NULL,
  NULL,
  GetItemIndex,
};

}  // namespace extensions

PyObject* NewFileExtensionsByName(ParentDescriptor descriptor) {
  return descriptor::NewMappingByName(&extensions::ContainerDef, descriptor);
}

namespace services {

typedef const ServiceDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->service_count();
}

static const void* GetByName(PyContainer* self, const string& name) {
  return GetDescriptor(self)->FindServiceByName(name);
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->service(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyServiceDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static const string& GetItemName(const void* item) {
  return static_cast<ItemDescriptor>(item)->name();
}

static int GetItemIndex(const void* item) {
  return static_cast<ItemDescriptor>(item)->index();
}

static DescriptorContainerDef ContainerDef = {
  "FileServices",
  Count,
  GetByIndex,
  GetByName,
  NULL,
  NULL,
  NewObjectFromItem,
  GetItemName,
  NULL,
  NULL,
  GetItemIndex,
};

}  // namespace services

PyObject* NewFileServicesByName(const FileDescriptor* descriptor) {
  return descriptor::NewMappingByName(&services::ContainerDef, descriptor);
}

namespace dependencies {

typedef const FileDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->dependency_count();
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->dependency(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyFileDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static DescriptorContainerDef ContainerDef = {
  "FileDependencies",
  Count,
  GetByIndex,
  NULL,
  NULL,
  NULL,
  NewObjectFromItem,
  NULL,
  NULL,
  NULL,
  NULL,
};

}  // namespace dependencies

PyObject* NewFileDependencies(const FileDescriptor* descriptor) {
  return descriptor::NewSequence(&dependencies::ContainerDef, descriptor);
}

namespace public_dependencies {

typedef const FileDescriptor* ItemDescriptor;

static int Count(PyContainer* self) {
  return GetDescriptor(self)->public_dependency_count();
}

static const void* GetByIndex(PyContainer* self, int index) {
  return GetDescriptor(self)->public_dependency(index);
}

static PyObject* NewObjectFromItem(const void* item) {
  return PyFileDescriptor_FromDescriptor(static_cast<ItemDescriptor>(item));
}

static DescriptorContainerDef ContainerDef = {
  "FilePublicDependencies",
  Count,
  GetByIndex,
  NULL,
  NULL,
  NULL,
  NewObjectFromItem,
  NULL,
  NULL,
  NULL,
  NULL,
};

}  // namespace public_dependencies

PyObject* NewFilePublicDependencies(const FileDescriptor* descriptor) {
  return descriptor::NewSequence(&public_dependencies::ContainerDef,
                                 descriptor);
}

}  // namespace file_descriptor


// Register all implementations

bool InitDescriptorMappingTypes() {
  if (PyType_Ready(&descriptor::DescriptorMapping_Type) < 0)
    return false;
  if (PyType_Ready(&descriptor::DescriptorSequence_Type) < 0)
    return false;
  if (PyType_Ready(&descriptor::ContainerIterator_Type) < 0)
    return false;
  return true;
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
