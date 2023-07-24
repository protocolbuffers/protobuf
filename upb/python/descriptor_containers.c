/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "python/descriptor_containers.h"

#include "python/descriptor.h"
#include "python/protobuf.h"
#include "upb/reflection/def.h"

// Implements __repr__ as str(dict(self)).
static PyObject* PyUpb_DescriptorMap_Repr(PyObject* _self) {
  PyObject* dict = PyDict_New();
  PyObject* ret = NULL;
  if (!dict) goto err;
  if (PyDict_Merge(dict, _self, 1) != 0) goto err;
  ret = PyObject_Str(dict);

err:
  Py_XDECREF(dict);
  return ret;
}

// -----------------------------------------------------------------------------
// ByNameIterator
// -----------------------------------------------------------------------------

typedef struct {
  PyObject_HEAD;
  const PyUpb_ByNameMap_Funcs* funcs;
  const void* parent;    // upb_MessageDef*, upb_DefPool*, etc.
  PyObject* parent_obj;  // Python object that keeps parent alive, we own a ref.
  int index;             // Current iterator index.
} PyUpb_ByNameIterator;

static PyUpb_ByNameIterator* PyUpb_ByNameIterator_Self(PyObject* obj) {
  assert(Py_TYPE(obj) == PyUpb_ModuleState_Get()->by_name_iterator_type);
  return (PyUpb_ByNameIterator*)obj;
}

static void PyUpb_ByNameIterator_Dealloc(PyObject* _self) {
  PyUpb_ByNameIterator* self = PyUpb_ByNameIterator_Self(_self);
  Py_DECREF(self->parent_obj);
  PyUpb_Dealloc(self);
}

static PyObject* PyUpb_ByNameIterator_New(const PyUpb_ByNameMap_Funcs* funcs,
                                          const void* parent,
                                          PyObject* parent_obj) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_Get();
  PyUpb_ByNameIterator* iter =
      (void*)PyType_GenericAlloc(s->by_name_iterator_type, 0);
  iter->funcs = funcs;
  iter->parent = parent;
  iter->parent_obj = parent_obj;
  iter->index = 0;
  Py_INCREF(iter->parent_obj);
  return &iter->ob_base;
}

static PyObject* PyUpb_ByNameIterator_IterNext(PyObject* _self) {
  PyUpb_ByNameIterator* self = PyUpb_ByNameIterator_Self(_self);
  int size = self->funcs->base.get_elem_count(self->parent);
  if (self->index >= size) return NULL;
  const void* elem = self->funcs->base.index(self->parent, self->index);
  self->index++;
  return PyUnicode_FromString(self->funcs->get_elem_name(elem));
}

static PyType_Slot PyUpb_ByNameIterator_Slots[] = {
    {Py_tp_dealloc, PyUpb_ByNameIterator_Dealloc},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, PyUpb_ByNameIterator_IterNext},
    {0, NULL}};

static PyType_Spec PyUpb_ByNameIterator_Spec = {
    PYUPB_MODULE_NAME "._ByNameIterator",  // tp_name
    sizeof(PyUpb_ByNameIterator),          // tp_basicsize
    0,                                     // tp_itemsize
    Py_TPFLAGS_DEFAULT,                    // tp_flags
    PyUpb_ByNameIterator_Slots,
};

// -----------------------------------------------------------------------------
// ByNumberIterator
// -----------------------------------------------------------------------------

typedef struct {
  PyObject_HEAD;
  const PyUpb_ByNumberMap_Funcs* funcs;
  const void* parent;    // upb_MessageDef*, upb_DefPool*, etc.
  PyObject* parent_obj;  // Python object that keeps parent alive, we own a ref.
  int index;             // Current iterator index.
} PyUpb_ByNumberIterator;

static PyUpb_ByNumberIterator* PyUpb_ByNumberIterator_Self(PyObject* obj) {
  assert(Py_TYPE(obj) == PyUpb_ModuleState_Get()->by_number_iterator_type);
  return (PyUpb_ByNumberIterator*)obj;
}

static void PyUpb_ByNumberIterator_Dealloc(PyObject* _self) {
  PyUpb_ByNumberIterator* self = PyUpb_ByNumberIterator_Self(_self);
  Py_DECREF(self->parent_obj);
  PyUpb_Dealloc(self);
}

static PyObject* PyUpb_ByNumberIterator_New(
    const PyUpb_ByNumberMap_Funcs* funcs, const void* parent,
    PyObject* parent_obj) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_Get();
  PyUpb_ByNumberIterator* iter =
      (void*)PyType_GenericAlloc(s->by_number_iterator_type, 0);
  iter->funcs = funcs;
  iter->parent = parent;
  iter->parent_obj = parent_obj;
  iter->index = 0;
  Py_INCREF(iter->parent_obj);
  return &iter->ob_base;
}

static PyObject* PyUpb_ByNumberIterator_IterNext(PyObject* _self) {
  PyUpb_ByNumberIterator* self = PyUpb_ByNumberIterator_Self(_self);
  int size = self->funcs->base.get_elem_count(self->parent);
  if (self->index >= size) return NULL;
  const void* elem = self->funcs->base.index(self->parent, self->index);
  self->index++;
  return PyLong_FromLong(self->funcs->get_elem_num(elem));
}

static PyType_Slot PyUpb_ByNumberIterator_Slots[] = {
    {Py_tp_dealloc, PyUpb_ByNumberIterator_Dealloc},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, PyUpb_ByNumberIterator_IterNext},
    {0, NULL}};

static PyType_Spec PyUpb_ByNumberIterator_Spec = {
    PYUPB_MODULE_NAME "._ByNumberIterator",  // tp_name
    sizeof(PyUpb_ByNumberIterator),          // tp_basicsize
    0,                                       // tp_itemsize
    Py_TPFLAGS_DEFAULT,                      // tp_flags
    PyUpb_ByNumberIterator_Slots,
};

// -----------------------------------------------------------------------------
// GenericSequence
// -----------------------------------------------------------------------------

typedef struct {
  PyObject_HEAD;
  const PyUpb_GenericSequence_Funcs* funcs;
  const void* parent;    // upb_MessageDef*, upb_DefPool*, etc.
  PyObject* parent_obj;  // Python object that keeps parent alive, we own a ref.
} PyUpb_GenericSequence;

PyUpb_GenericSequence* PyUpb_GenericSequence_Self(PyObject* obj) {
  assert(Py_TYPE(obj) == PyUpb_ModuleState_Get()->generic_sequence_type);
  return (PyUpb_GenericSequence*)obj;
}

static void PyUpb_GenericSequence_Dealloc(PyObject* _self) {
  PyUpb_GenericSequence* self = PyUpb_GenericSequence_Self(_self);
  Py_CLEAR(self->parent_obj);
  PyUpb_Dealloc(self);
}

PyObject* PyUpb_GenericSequence_New(const PyUpb_GenericSequence_Funcs* funcs,
                                    const void* parent, PyObject* parent_obj) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_Get();
  PyUpb_GenericSequence* seq =
      (PyUpb_GenericSequence*)PyType_GenericAlloc(s->generic_sequence_type, 0);
  seq->funcs = funcs;
  seq->parent = parent;
  seq->parent_obj = parent_obj;
  Py_INCREF(parent_obj);
  return &seq->ob_base;
}

static Py_ssize_t PyUpb_GenericSequence_Length(PyObject* _self) {
  PyUpb_GenericSequence* self = PyUpb_GenericSequence_Self(_self);
  return self->funcs->get_elem_count(self->parent);
}

static PyObject* PyUpb_GenericSequence_GetItem(PyObject* _self,
                                               Py_ssize_t index) {
  PyUpb_GenericSequence* self = PyUpb_GenericSequence_Self(_self);
  Py_ssize_t size = self->funcs->get_elem_count(self->parent);
  if (index < 0 || index >= size) {
    PyErr_Format(PyExc_IndexError, "list index (%zd) out of range", index);
    return NULL;
  }
  const void* elem = self->funcs->index(self->parent, index);
  return self->funcs->get_elem_wrapper(elem);
}

// A sequence container can only be equal to another sequence container, or (for
// backward compatibility) to a list containing the same items.
// Returns 1 if equal, 0 if unequal, -1 on error.
static int PyUpb_GenericSequence_IsEqual(PyUpb_GenericSequence* self,
                                         PyObject* other) {
  // Check the identity of C++ pointers.
  if (PyObject_TypeCheck(other, Py_TYPE(self))) {
    PyUpb_GenericSequence* other_seq = (void*)other;
    return self->parent == other_seq->parent && self->funcs == other_seq->funcs;
  }

  if (!PyList_Check(other)) return 0;

  // return list(self) == other
  // We can clamp `i` to int because GenericSequence uses int for size (this
  // is useful when we do int iteration below).
  int n = PyUpb_GenericSequence_Length((PyObject*)self);
  if ((Py_ssize_t)n != PyList_Size(other)) {
    return false;
  }

  PyObject* item1;
  for (int i = 0; i < n; i++) {
    item1 = PyUpb_GenericSequence_GetItem((PyObject*)self, i);
    PyObject* item2 = PyList_GetItem(other, i);
    if (!item1 || !item2) goto error;
    int cmp = PyObject_RichCompareBool(item1, item2, Py_EQ);
    Py_DECREF(item1);
    if (cmp != 1) return cmp;
  }
  // All items were found and equal
  return 1;

error:
  Py_XDECREF(item1);
  return -1;
}

static PyObject* PyUpb_GenericSequence_RichCompare(PyObject* _self,
                                                   PyObject* other, int opid) {
  PyUpb_GenericSequence* self = PyUpb_GenericSequence_Self(_self);
  if (opid != Py_EQ && opid != Py_NE) {
    Py_RETURN_NOTIMPLEMENTED;
  }
  bool ret = PyUpb_GenericSequence_IsEqual(self, other);
  if (opid == Py_NE) ret = !ret;
  return PyBool_FromLong(ret);
}

// Linear search.  Could optimize this in some cases (defs that have index),
// but not all (FileDescriptor.dependencies).
static int PyUpb_GenericSequence_Find(PyObject* _self, PyObject* item) {
  PyUpb_GenericSequence* self = PyUpb_GenericSequence_Self(_self);
  const void* item_ptr = PyUpb_AnyDescriptor_GetDef(item);
  int count = self->funcs->get_elem_count(self->parent);
  for (int i = 0; i < count; i++) {
    if (self->funcs->index(self->parent, i) == item_ptr) {
      return i;
    }
  }
  return -1;
}

static PyObject* PyUpb_GenericSequence_Index(PyObject* self, PyObject* item) {
  int position = PyUpb_GenericSequence_Find(self, item);
  if (position < 0) {
    PyErr_SetNone(PyExc_ValueError);
    return NULL;
  } else {
    return PyLong_FromLong(position);
  }
}

static PyObject* PyUpb_GenericSequence_Count(PyObject* _self, PyObject* item) {
  PyUpb_GenericSequence* self = PyUpb_GenericSequence_Self(_self);
  const void* item_ptr = PyUpb_AnyDescriptor_GetDef(item);
  int n = self->funcs->get_elem_count(self->parent);
  int count = 0;
  for (int i = 0; i < n; i++) {
    if (self->funcs->index(self->parent, i) == item_ptr) {
      count++;
    }
  }
  return PyLong_FromLong(count);
}

static PyObject* PyUpb_GenericSequence_Append(PyObject* self, PyObject* args) {
  PyErr_Format(PyExc_TypeError, "'%R' is not a mutable sequence", self);
  return NULL;
}

static PyMethodDef PyUpb_GenericSequence_Methods[] = {
    {"index", PyUpb_GenericSequence_Index, METH_O},
    {"count", PyUpb_GenericSequence_Count, METH_O},
    {"append", PyUpb_GenericSequence_Append, METH_O},
    // This was implemented for Python/C++ but so far has not been required.
    //{ "__reversed__", (PyCFunction)Reversed, METH_NOARGS, },
    {NULL}};

static PyType_Slot PyUpb_GenericSequence_Slots[] = {
    {Py_tp_dealloc, &PyUpb_GenericSequence_Dealloc},
    {Py_tp_methods, &PyUpb_GenericSequence_Methods},
    {Py_sq_length, PyUpb_GenericSequence_Length},
    {Py_sq_item, PyUpb_GenericSequence_GetItem},
    {Py_tp_richcompare, &PyUpb_GenericSequence_RichCompare},
    // These were implemented for Python/C++ but so far have not been required.
    // {Py_tp_repr, &PyUpb_GenericSequence_Repr},
    // {Py_sq_contains, PyUpb_GenericSequence_Contains},
    // {Py_mp_subscript, PyUpb_GenericSequence_Subscript},
    // {Py_mp_ass_subscript, PyUpb_GenericSequence_AssignSubscript},
    {0, NULL},
};

static PyType_Spec PyUpb_GenericSequence_Spec = {
    PYUPB_MODULE_NAME "._GenericSequence",  // tp_name
    sizeof(PyUpb_GenericSequence),          // tp_basicsize
    0,                                      // tp_itemsize
    Py_TPFLAGS_DEFAULT,                     // tp_flags
    PyUpb_GenericSequence_Slots,
};

// -----------------------------------------------------------------------------
// ByNameMap
// -----------------------------------------------------------------------------

typedef struct {
  PyObject_HEAD;
  const PyUpb_ByNameMap_Funcs* funcs;
  const void* parent;    // upb_MessageDef*, upb_DefPool*, etc.
  PyObject* parent_obj;  // Python object that keeps parent alive, we own a ref.
} PyUpb_ByNameMap;

PyUpb_ByNameMap* PyUpb_ByNameMap_Self(PyObject* obj) {
  assert(Py_TYPE(obj) == PyUpb_ModuleState_Get()->by_name_map_type);
  return (PyUpb_ByNameMap*)obj;
}

static void PyUpb_ByNameMap_Dealloc(PyObject* _self) {
  PyUpb_ByNameMap* self = PyUpb_ByNameMap_Self(_self);
  Py_DECREF(self->parent_obj);
  PyUpb_Dealloc(self);
}

PyObject* PyUpb_ByNameMap_New(const PyUpb_ByNameMap_Funcs* funcs,
                              const void* parent, PyObject* parent_obj) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_Get();
  PyUpb_ByNameMap* map = (void*)PyType_GenericAlloc(s->by_name_map_type, 0);
  map->funcs = funcs;
  map->parent = parent;
  map->parent_obj = parent_obj;
  Py_INCREF(parent_obj);
  return &map->ob_base;
}

static Py_ssize_t PyUpb_ByNameMap_Length(PyObject* _self) {
  PyUpb_ByNameMap* self = PyUpb_ByNameMap_Self(_self);
  return self->funcs->base.get_elem_count(self->parent);
}

static PyObject* PyUpb_ByNameMap_Subscript(PyObject* _self, PyObject* key) {
  PyUpb_ByNameMap* self = PyUpb_ByNameMap_Self(_self);
  const char* name = PyUpb_GetStrData(key);
  const void* elem = name ? self->funcs->lookup(self->parent, name) : NULL;

  if (!name && PyObject_Hash(key) == -1) return NULL;

  if (elem) {
    return self->funcs->base.get_elem_wrapper(elem);
  } else {
    PyErr_SetObject(PyExc_KeyError, key);
    return NULL;
  }
}

static int PyUpb_ByNameMap_AssignSubscript(PyObject* self, PyObject* key,
                                           PyObject* value) {
  PyErr_Format(PyExc_TypeError, PYUPB_MODULE_NAME
               ".ByNameMap' object does not support item assignment");
  return -1;
}

static int PyUpb_ByNameMap_Contains(PyObject* _self, PyObject* key) {
  PyUpb_ByNameMap* self = PyUpb_ByNameMap_Self(_self);
  const char* name = PyUpb_GetStrData(key);
  const void* elem = name ? self->funcs->lookup(self->parent, name) : NULL;
  if (!name && PyObject_Hash(key) == -1) return -1;
  return elem ? 1 : 0;
}

static PyObject* PyUpb_ByNameMap_Get(PyObject* _self, PyObject* args) {
  PyUpb_ByNameMap* self = PyUpb_ByNameMap_Self(_self);
  PyObject* key;
  PyObject* default_value = Py_None;
  if (!PyArg_UnpackTuple(args, "get", 1, 2, &key, &default_value)) {
    return NULL;
  }

  const char* name = PyUpb_GetStrData(key);
  const void* elem = name ? self->funcs->lookup(self->parent, name) : NULL;

  if (!name && PyObject_Hash(key) == -1) return NULL;

  if (elem) {
    return self->funcs->base.get_elem_wrapper(elem);
  } else {
    Py_INCREF(default_value);
    return default_value;
  }
}

static PyObject* PyUpb_ByNameMap_GetIter(PyObject* _self) {
  PyUpb_ByNameMap* self = PyUpb_ByNameMap_Self(_self);
  return PyUpb_ByNameIterator_New(self->funcs, self->parent, self->parent_obj);
}

static PyObject* PyUpb_ByNameMap_Keys(PyObject* _self, PyObject* args) {
  PyUpb_ByNameMap* self = PyUpb_ByNameMap_Self(_self);
  int n = self->funcs->base.get_elem_count(self->parent);
  PyObject* ret = PyList_New(n);
  if (!ret) return NULL;
  for (int i = 0; i < n; i++) {
    const void* elem = self->funcs->base.index(self->parent, i);
    PyObject* key = PyUnicode_FromString(self->funcs->get_elem_name(elem));
    if (!key) goto error;
    PyList_SetItem(ret, i, key);
  }
  return ret;

error:
  Py_XDECREF(ret);
  return NULL;
}

static PyObject* PyUpb_ByNameMap_Values(PyObject* _self, PyObject* args) {
  PyUpb_ByNameMap* self = PyUpb_ByNameMap_Self(_self);
  int n = self->funcs->base.get_elem_count(self->parent);
  PyObject* ret = PyList_New(n);
  if (!ret) return NULL;
  for (int i = 0; i < n; i++) {
    const void* elem = self->funcs->base.index(self->parent, i);
    PyObject* py_elem = self->funcs->base.get_elem_wrapper(elem);
    if (!py_elem) goto error;
    PyList_SetItem(ret, i, py_elem);
  }
  return ret;

error:
  Py_XDECREF(ret);
  return NULL;
}

static PyObject* PyUpb_ByNameMap_Items(PyObject* _self, PyObject* args) {
  PyUpb_ByNameMap* self = (PyUpb_ByNameMap*)_self;
  int n = self->funcs->base.get_elem_count(self->parent);
  PyObject* ret = PyList_New(n);
  PyObject* item;
  PyObject* py_elem;
  if (!ret) return NULL;
  for (int i = 0; i < n; i++) {
    const void* elem = self->funcs->base.index(self->parent, i);
    item = PyTuple_New(2);
    py_elem = self->funcs->base.get_elem_wrapper(elem);
    if (!item || !py_elem) goto error;
    PyTuple_SetItem(item, 0,
                    PyUnicode_FromString(self->funcs->get_elem_name(elem)));
    PyTuple_SetItem(item, 1, py_elem);
    PyList_SetItem(ret, i, item);
  }
  return ret;

error:
  Py_XDECREF(py_elem);
  Py_XDECREF(item);
  Py_XDECREF(ret);
  return NULL;
}

// A mapping container can only be equal to another mapping container, or (for
// backward compatibility) to a dict containing the same items.
// Returns 1 if equal, 0 if unequal, -1 on error.
static int PyUpb_ByNameMap_IsEqual(PyUpb_ByNameMap* self, PyObject* other) {
  // Check the identity of C++ pointers.
  if (PyObject_TypeCheck(other, Py_TYPE(self))) {
    PyUpb_ByNameMap* other_map = (void*)other;
    return self->parent == other_map->parent && self->funcs == other_map->funcs;
  }

  if (!PyDict_Check(other)) return 0;

  PyObject* self_dict = PyDict_New();
  PyDict_Merge(self_dict, (PyObject*)self, 0);
  int eq = PyObject_RichCompareBool(self_dict, other, Py_EQ);
  Py_DECREF(self_dict);
  return eq;
}

static PyObject* PyUpb_ByNameMap_RichCompare(PyObject* _self, PyObject* other,
                                             int opid) {
  PyUpb_ByNameMap* self = PyUpb_ByNameMap_Self(_self);
  if (opid != Py_EQ && opid != Py_NE) {
    Py_RETURN_NOTIMPLEMENTED;
  }
  bool ret = PyUpb_ByNameMap_IsEqual(self, other);
  if (opid == Py_NE) ret = !ret;
  return PyBool_FromLong(ret);
}

static PyMethodDef PyUpb_ByNameMap_Methods[] = {
    {"get", (PyCFunction)&PyUpb_ByNameMap_Get, METH_VARARGS},
    {"keys", PyUpb_ByNameMap_Keys, METH_NOARGS},
    {"values", PyUpb_ByNameMap_Values, METH_NOARGS},
    {"items", PyUpb_ByNameMap_Items, METH_NOARGS},
    {NULL}};

static PyType_Slot PyUpb_ByNameMap_Slots[] = {
    {Py_mp_ass_subscript, PyUpb_ByNameMap_AssignSubscript},
    {Py_mp_length, PyUpb_ByNameMap_Length},
    {Py_mp_subscript, PyUpb_ByNameMap_Subscript},
    {Py_sq_contains, &PyUpb_ByNameMap_Contains},
    {Py_tp_dealloc, &PyUpb_ByNameMap_Dealloc},
    {Py_tp_iter, PyUpb_ByNameMap_GetIter},
    {Py_tp_methods, &PyUpb_ByNameMap_Methods},
    {Py_tp_repr, &PyUpb_DescriptorMap_Repr},
    {Py_tp_richcompare, &PyUpb_ByNameMap_RichCompare},
    {0, NULL},
};

static PyType_Spec PyUpb_ByNameMap_Spec = {
    PYUPB_MODULE_NAME "._ByNameMap",  // tp_name
    sizeof(PyUpb_ByNameMap),          // tp_basicsize
    0,                                // tp_itemsize
    Py_TPFLAGS_DEFAULT,               // tp_flags
    PyUpb_ByNameMap_Slots,
};

// -----------------------------------------------------------------------------
// ByNumberMap
// -----------------------------------------------------------------------------

typedef struct {
  PyObject_HEAD;
  const PyUpb_ByNumberMap_Funcs* funcs;
  const void* parent;    // upb_MessageDef*, upb_DefPool*, etc.
  PyObject* parent_obj;  // Python object that keeps parent alive, we own a ref.
} PyUpb_ByNumberMap;

PyUpb_ByNumberMap* PyUpb_ByNumberMap_Self(PyObject* obj) {
  assert(Py_TYPE(obj) == PyUpb_ModuleState_Get()->by_number_map_type);
  return (PyUpb_ByNumberMap*)obj;
}

PyObject* PyUpb_ByNumberMap_New(const PyUpb_ByNumberMap_Funcs* funcs,
                                const void* parent, PyObject* parent_obj) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_Get();
  PyUpb_ByNumberMap* map = (void*)PyType_GenericAlloc(s->by_number_map_type, 0);
  map->funcs = funcs;
  map->parent = parent;
  map->parent_obj = parent_obj;
  Py_INCREF(parent_obj);
  return &map->ob_base;
}

static void PyUpb_ByNumberMap_Dealloc(PyObject* _self) {
  PyUpb_ByNumberMap* self = PyUpb_ByNumberMap_Self(_self);
  Py_DECREF(self->parent_obj);
  PyUpb_Dealloc(self);
}

static Py_ssize_t PyUpb_ByNumberMap_Length(PyObject* _self) {
  PyUpb_ByNumberMap* self = PyUpb_ByNumberMap_Self(_self);
  return self->funcs->base.get_elem_count(self->parent);
}

static const void* PyUpb_ByNumberMap_LookupHelper(PyUpb_ByNumberMap* self,
                                                  PyObject* key) {
  long num = PyLong_AsLong(key);
  if (num == -1 && PyErr_Occurred()) {
    PyErr_Clear();
    // Ensure that the key is hashable (this will raise an error if not).
    PyObject_Hash(key);
    return NULL;
  } else {
    return self->funcs->lookup(self->parent, num);
  }
}

static PyObject* PyUpb_ByNumberMap_Subscript(PyObject* _self, PyObject* key) {
  PyUpb_ByNumberMap* self = PyUpb_ByNumberMap_Self(_self);
  const void* elem = PyUpb_ByNumberMap_LookupHelper(self, key);
  if (elem) {
    return self->funcs->base.get_elem_wrapper(elem);
  } else {
    if (!PyErr_Occurred()) {
      PyErr_SetObject(PyExc_KeyError, key);
    }
    return NULL;
  }
}

static int PyUpb_ByNumberMap_AssignSubscript(PyObject* self, PyObject* key,
                                             PyObject* value) {
  PyErr_Format(PyExc_TypeError, PYUPB_MODULE_NAME
               ".ByNumberMap' object does not support item assignment");
  return -1;
}

static PyObject* PyUpb_ByNumberMap_Get(PyObject* _self, PyObject* args) {
  PyUpb_ByNumberMap* self = PyUpb_ByNumberMap_Self(_self);
  PyObject* key;
  PyObject* default_value = Py_None;
  if (!PyArg_UnpackTuple(args, "get", 1, 2, &key, &default_value)) {
    return NULL;
  }

  const void* elem = PyUpb_ByNumberMap_LookupHelper(self, key);
  if (elem) {
    return self->funcs->base.get_elem_wrapper(elem);
  } else if (PyErr_Occurred()) {
    return NULL;
  } else {
    return PyUpb_NewRef(default_value);
  }
}

static PyObject* PyUpb_ByNumberMap_GetIter(PyObject* _self) {
  PyUpb_ByNumberMap* self = PyUpb_ByNumberMap_Self(_self);
  return PyUpb_ByNumberIterator_New(self->funcs, self->parent,
                                    self->parent_obj);
}

static PyObject* PyUpb_ByNumberMap_Keys(PyObject* _self, PyObject* args) {
  PyUpb_ByNumberMap* self = PyUpb_ByNumberMap_Self(_self);
  int n = self->funcs->base.get_elem_count(self->parent);
  PyObject* ret = PyList_New(n);
  if (!ret) return NULL;
  for (int i = 0; i < n; i++) {
    const void* elem = self->funcs->base.index(self->parent, i);
    PyObject* key = PyLong_FromLong(self->funcs->get_elem_num(elem));
    if (!key) goto error;
    PyList_SetItem(ret, i, key);
  }
  return ret;

error:
  Py_XDECREF(ret);
  return NULL;
}

static PyObject* PyUpb_ByNumberMap_Values(PyObject* _self, PyObject* args) {
  PyUpb_ByNumberMap* self = PyUpb_ByNumberMap_Self(_self);
  int n = self->funcs->base.get_elem_count(self->parent);
  PyObject* ret = PyList_New(n);
  if (!ret) return NULL;
  for (int i = 0; i < n; i++) {
    const void* elem = self->funcs->base.index(self->parent, i);
    PyObject* py_elem = self->funcs->base.get_elem_wrapper(elem);
    if (!py_elem) goto error;
    PyList_SetItem(ret, i, py_elem);
  }
  return ret;

error:
  Py_XDECREF(ret);
  return NULL;
}

static PyObject* PyUpb_ByNumberMap_Items(PyObject* _self, PyObject* args) {
  PyUpb_ByNumberMap* self = PyUpb_ByNumberMap_Self(_self);
  int n = self->funcs->base.get_elem_count(self->parent);
  PyObject* ret = PyList_New(n);
  PyObject* item;
  PyObject* py_elem;
  if (!ret) return NULL;
  for (int i = 0; i < n; i++) {
    const void* elem = self->funcs->base.index(self->parent, i);
    int number = self->funcs->get_elem_num(elem);
    item = PyTuple_New(2);
    py_elem = self->funcs->base.get_elem_wrapper(elem);
    if (!item || !py_elem) goto error;
    PyTuple_SetItem(item, 0, PyLong_FromLong(number));
    PyTuple_SetItem(item, 1, py_elem);
    PyList_SetItem(ret, i, item);
  }
  return ret;

error:
  Py_XDECREF(py_elem);
  Py_XDECREF(item);
  Py_XDECREF(ret);
  return NULL;
}

static int PyUpb_ByNumberMap_Contains(PyObject* _self, PyObject* key) {
  PyUpb_ByNumberMap* self = PyUpb_ByNumberMap_Self(_self);
  const void* elem = PyUpb_ByNumberMap_LookupHelper(self, key);
  if (elem) return 1;
  if (PyErr_Occurred()) return -1;
  return 0;
}

// A mapping container can only be equal to another mapping container, or (for
// backward compatibility) to a dict containing the same items.
// Returns 1 if equal, 0 if unequal, -1 on error.
static int PyUpb_ByNumberMap_IsEqual(PyUpb_ByNumberMap* self, PyObject* other) {
  // Check the identity of C++ pointers.
  if (PyObject_TypeCheck(other, Py_TYPE(self))) {
    PyUpb_ByNumberMap* other_map = (void*)other;
    return self->parent == other_map->parent && self->funcs == other_map->funcs;
  }

  if (!PyDict_Check(other)) return 0;

  PyObject* self_dict = PyDict_New();
  PyDict_Merge(self_dict, (PyObject*)self, 0);
  int eq = PyObject_RichCompareBool(self_dict, other, Py_EQ);
  Py_DECREF(self_dict);
  return eq;
}

static PyObject* PyUpb_ByNumberMap_RichCompare(PyObject* _self, PyObject* other,
                                               int opid) {
  PyUpb_ByNumberMap* self = PyUpb_ByNumberMap_Self(_self);
  if (opid != Py_EQ && opid != Py_NE) {
    Py_RETURN_NOTIMPLEMENTED;
  }
  bool ret = PyUpb_ByNumberMap_IsEqual(self, other);
  if (opid == Py_NE) ret = !ret;
  return PyBool_FromLong(ret);
}

static PyMethodDef PyUpb_ByNumberMap_Methods[] = {
    {"get", (PyCFunction)&PyUpb_ByNumberMap_Get, METH_VARARGS},
    {"keys", PyUpb_ByNumberMap_Keys, METH_NOARGS},
    {"values", PyUpb_ByNumberMap_Values, METH_NOARGS},
    {"items", PyUpb_ByNumberMap_Items, METH_NOARGS},
    {NULL}};

static PyType_Slot PyUpb_ByNumberMap_Slots[] = {
    {Py_mp_ass_subscript, PyUpb_ByNumberMap_AssignSubscript},
    {Py_mp_length, PyUpb_ByNumberMap_Length},
    {Py_mp_subscript, PyUpb_ByNumberMap_Subscript},
    {Py_sq_contains, &PyUpb_ByNumberMap_Contains},
    {Py_tp_dealloc, &PyUpb_ByNumberMap_Dealloc},
    {Py_tp_iter, PyUpb_ByNumberMap_GetIter},
    {Py_tp_methods, &PyUpb_ByNumberMap_Methods},
    {Py_tp_repr, &PyUpb_DescriptorMap_Repr},
    {Py_tp_richcompare, &PyUpb_ByNumberMap_RichCompare},
    {0, NULL},
};

static PyType_Spec PyUpb_ByNumberMap_Spec = {
    PYUPB_MODULE_NAME "._ByNumberMap",  // tp_name
    sizeof(PyUpb_ByNumberMap),          // tp_basicsize
    0,                                  // tp_itemsize
    Py_TPFLAGS_DEFAULT,                 // tp_flags
    PyUpb_ByNumberMap_Slots,
};

// -----------------------------------------------------------------------------
// Top Level
// -----------------------------------------------------------------------------

bool PyUpb_InitDescriptorContainers(PyObject* m) {
  PyUpb_ModuleState* s = PyUpb_ModuleState_GetFromModule(m);

  s->by_name_map_type = PyUpb_AddClass(m, &PyUpb_ByNameMap_Spec);
  s->by_number_map_type = PyUpb_AddClass(m, &PyUpb_ByNumberMap_Spec);
  s->by_name_iterator_type = PyUpb_AddClass(m, &PyUpb_ByNameIterator_Spec);
  s->by_number_iterator_type = PyUpb_AddClass(m, &PyUpb_ByNumberIterator_Spec);
  s->generic_sequence_type = PyUpb_AddClass(m, &PyUpb_GenericSequence_Spec);

  return s->by_name_map_type && s->by_number_map_type &&
         s->by_name_iterator_type && s->by_number_iterator_type &&
         s->generic_sequence_type;
}
